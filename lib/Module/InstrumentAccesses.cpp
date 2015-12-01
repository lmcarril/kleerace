#include "Passes.h"
#include "klee/Config/Version.h"

#include "llvm/Transforms/Instrumentation.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Analysis/CaptureTracking.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"

using namespace llvm;
using namespace klee;

static cl::opt<bool>
ClInstrumentMemoryAccesses("instrument-memory-accesses",
                           cl::desc("Instrument memory accesses"),
                           cl::init(false));
static cl::opt<bool>
ClInstrumentAtomics("instrument-atomics",
                    cl::desc("Instrument atomics"),
                    cl::init(false));
static cl::opt<bool>
ClInstrumentMemIntrinsics("instrument-memory-function",
                          cl::desc("Instrument memory functions (memset/memcpy/memmove)"),
                          cl::init(false));
static cl::opt<bool>
ClInstrumentAll("instrument-all",
                cl::desc("Enable the options: instrument-atomics, instrument-memory-function and instrument-memory-accesses"),
                cl::init(false));

char InstrumentAccesses::ID = 0;

static Function *checkInterfaceFunction(Constant *FuncOrBitcast) {
  if (Function *F = dyn_cast<Function>(FuncOrBitcast))
    return F;
  FuncOrBitcast->dump();
  report_fatal_error("InstrumentMemoryAccessesPass interface function redefined");
}

bool InstrumentAccesses::doInitialization(Module &M) {
  IRBuilder<> IRB(M.getContext());
  IntptrTy = IRB.getIntPtrTy(&DL);

  // void klee_mem_access(void * address, size_t size, bool isWrite, bool isAtomic, bool isRaceCandidate)
  Access = checkInterfaceFunction(M.getOrInsertFunction("klee_mem_access",
                                                        IRB.getVoidTy(), IRB.getInt8PtrTy(),
                                                        IntptrTy, IRB.getInt8Ty(),
                                                        IRB.getInt8Ty(), IRB.getInt8Ty(), NULL));

  if (ClInstrumentAll) {
    ClInstrumentMemoryAccesses = true;
    ClInstrumentAtomics = true;
    ClInstrumentMemIntrinsics = true;
  }
  return true;
}

static bool isVtableAccess(Instruction *I) {
  if (MDNode *Tag = I->getMetadata(LLVMContext::MD_tbaa))
    return Tag->isTBAAVtableAccess();
  return false;
}

bool InstrumentAccesses::addrPointsToConstantData(Value *Addr) {
  // If this is a GEP, just analyze its pointer operand.
  if (GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(Addr))
    Addr = GEP->getPointerOperand();

  if (GlobalVariable *GV = dyn_cast<GlobalVariable>(Addr)) {
    if (GV->isConstant())
      return true;
  } else if (LoadInst *L = dyn_cast<LoadInst>(Addr)) {
    if (isVtableAccess(L))
      return true;
  }
  return false;
}

// Instrumenting some of the accesses may be proven redundant.
// Currently handled:
//  - read-before-write (within same BB, no calls between)
//  - not captured variables
//
// We do not handle some of the patterns that should not survive
// after the classic compiler optimizations.
// E.g. two reads from the same temp should be eliminated by CSE,
// two writes should be eliminated by DSE, etc.
//
// 'Local' is a vector of insns within the same BB (no calls between).
// 'All' is a vector of insns that will be instrumented.
void InstrumentAccesses::chooseInstructionsToInstrument(
    SmallVectorImpl<Instruction *> &Local, SmallVectorImpl<Instruction *> &All,
    const DataLayout &DL) {
  SmallSet<Value*, 8> WriteTargets;
  // Iterate from the end.
  for (SmallVectorImpl<Instruction*>::reverse_iterator It = Local.rbegin(),
       E = Local.rend(); It != E; ++It) {
    Instruction *I = *It;
    if (StoreInst *Store = dyn_cast<StoreInst>(I)) {
      WriteTargets.insert(Store->getPointerOperand());
    } else {
      LoadInst *Load = cast<LoadInst>(I);
      Value *Addr = Load->getPointerOperand();
      if (WriteTargets.count(Addr))
        // We will write to this temp, so no reason to analyze the read.
        continue;
      if (addrPointsToConstantData(Addr))
        // Addr points to some constant data -- it can not race with any writes.
        continue;
    }
    Value *Addr = isa<StoreInst>(*I)
        ? cast<StoreInst>(I)->getPointerOperand()
        : cast<LoadInst>(I)->getPointerOperand();
    if (isa<AllocaInst>(GetUnderlyingObject(Addr, &DL)) &&
        !PointerMayBeCaptured(Addr, true, true))
      // The variable is addressable but not captured, so it cannot be
      // referenced from a different thread and participate in a data race
      // (see llvm/Analysis/CaptureTracking.h for details).
      continue;

    All.push_back(I);
  }
  Local.clear();
}

static bool isAtomic(Instruction *I) {
  if (LoadInst *LI = dyn_cast<LoadInst>(I))
    return LI->isAtomic() && LI->getSynchScope() == CrossThread;
  if (StoreInst *SI = dyn_cast<StoreInst>(I))
    return SI->isAtomic() && SI->getSynchScope() == CrossThread;
  if (isa<AtomicRMWInst>(I))
    return true;
  if (isa<AtomicCmpXchgInst>(I))
    return true;
  if (isa<FenceInst>(I))
    return true;
  return false;
}

bool InstrumentAccesses::runOnFunction(Function &F) {
  SmallVector<Instruction*, 8> RetVec;
  SmallVector<Instruction*, 8> AllLoadsAndStores;
  SmallVector<Instruction*, 8> LocalLoadsAndStores;
  SmallVector<Instruction*, 8> AtomicAccesses;
  SmallVector<Instruction*, 8> MemIntrinCalls;
  bool Res = false;

  // Traverse all instructions, collect loads/stores/returns, check for calls.
  for (Function::iterator FI = F.begin(), FE = F.end();
       FI != FE; ++FI) {
    BasicBlock &BB = *FI;
    for (BasicBlock::iterator BI = BB.begin(), BE = BB.end();
         BI != BE; ++BI) {
      Instruction &Inst = *BI;
      if (isAtomic(&Inst))
        AtomicAccesses.push_back(&Inst);
      else if (isa<LoadInst>(Inst) || isa<StoreInst>(Inst))
        LocalLoadsAndStores.push_back(&Inst);
      else if (isa<ReturnInst>(Inst))
        RetVec.push_back(&Inst);
      else if (isa<CallInst>(Inst) || isa<InvokeInst>(Inst)) {
        if (isa<MemIntrinsic>(Inst))
          MemIntrinCalls.push_back(&Inst);
        chooseInstructionsToInstrument(LocalLoadsAndStores, AllLoadsAndStores,
                                       DL);
      }
    }
    chooseInstructionsToInstrument(LocalLoadsAndStores, AllLoadsAndStores, DL);
  }

  // We have collected all loads and stores.
  // FIXME: many of these accesses do not need to be checked for races
  // (e.g. variables that do not escape, etc).

  // Instrument memory accesses only if we want to report bugs in the function.
  if (ClInstrumentMemoryAccesses)
    for (size_t i = 0, n = AllLoadsAndStores.size(); i < n; ++i)
      Res |= instrumentLoadOrStore(AllLoadsAndStores[i], DL);

  // Instrument atomic memory accesses in any case (they can be used to
  // implement synchronization).
  if (ClInstrumentAtomics)
    for (size_t i = 0, n = AtomicAccesses.size(); i < n; ++i)
      Res |= instrumentAtomic(AtomicAccesses[i], DL);

  if (ClInstrumentMemIntrinsics)
    for (size_t i = 0, n = MemIntrinCalls.size(); i < n; ++i)
      Res |= instrumentMemIntrinsic(MemIntrinCalls[i]);

  return Res;
}

bool InstrumentAccesses::instrumentLoadOrStore(Instruction *I,
                                               const DataLayout &DL) {
  IRBuilder<> IRB(I);
  bool IsWrite = isa<StoreInst>(*I);
  Value *Addr = IsWrite
      ? cast<StoreInst>(I)->getPointerOperand()
      : cast<LoadInst>(I)->getPointerOperand();
  int Idx = getMemoryAccessWidth(Addr, DL);
  if (Idx < 0)
    return false;

  if (isVtableAccess(I))
    llvm::errs() << "Unsupported virtual tables instrumentation\n";

  IRB.CreateCall5(Access, IRB.CreatePointerCast(Addr, IRB.getInt8PtrTy()),
                  ConstantInt::get(IntptrTy, Idx),//size
                  ConstantInt::get(IRB.getInt8Ty(), IsWrite?1:0),//is write?
                  ConstantInt::get(IRB.getInt8Ty(), 0), //not atomic
                  ConstantInt::get(IRB.getInt8Ty(), 1), //is race candidate
                 "");
  return true;
}

bool InstrumentAccesses::instrumentMemIntrinsic(Instruction *I) {
  IRBuilder<> IRB(I);
  if (MemSetInst *M = dyn_cast<MemSetInst>(I)) {
    IRB.CreateCall5(Access, IRB.CreatePointerCast(M->getArgOperand(0), IRB.getInt8PtrTy()),
                   M->getOperand(2),//size
                   ConstantInt::get(IRB.getInt8Ty(), 1),//write
                   ConstantInt::get(IRB.getInt8Ty(), 0),//not atomic
                   ConstantInt::get(IRB.getInt8Ty(), 1),//is race candidate
                   "");
  } else if (MemTransferInst *M = dyn_cast<MemTransferInst>(I)) {
    // Read on src
    IRB.CreateCall5(Access, IRB.CreatePointerCast(M->getArgOperand(1), IRB.getInt8PtrTy()),
                   M->getOperand(2),//size
                   ConstantInt::get(IRB.getInt8Ty(), 0),//read
                   ConstantInt::get(IRB.getInt8Ty(), 0),//not atomic
                   ConstantInt::get(IRB.getInt8Ty(), 1),//is race candidate
                   "");
    // Write on dst
    IRB.CreateCall5(Access, IRB.CreatePointerCast(M->getArgOperand(0), IRB.getInt8PtrTy()),
                   M->getOperand(2),//size
                   ConstantInt::get(IRB.getInt8Ty(), 1),//write
                   ConstantInt::get(IRB.getInt8Ty(), 0),//not atomic
                   ConstantInt::get(IRB.getInt8Ty(), 1),//is race candidate
                   "");
  }
  return true;
}

bool InstrumentAccesses::instrumentAtomic(Instruction *I, const DataLayout &DL) {
  IRBuilder<> IRB(I);
  if (LoadInst *LI = dyn_cast<LoadInst>(I)) {
    Value *Addr = LI->getPointerOperand();
    int Idx = getMemoryAccessWidth(Addr, DL);
    if (Idx < 0)
      return false;

    IRB.CreateCall5(Access, IRB.CreatePointerCast(Addr, IRB.getInt8PtrTy()),
                    ConstantInt::get(IntptrTy, Idx),//size
                    ConstantInt::get(IRB.getInt8Ty(), 0),//read
                    ConstantInt::get(IRB.getInt8Ty(), 1),//atomic
                    ConstantInt::get(IRB.getInt8Ty(), 1),//is race candidate
                    "");
  } else if (StoreInst *SI = dyn_cast<StoreInst>(I)) {
    Value *Addr = SI->getPointerOperand();
    int Idx = getMemoryAccessWidth(Addr, DL);
    if (Idx < 0)
      return false;

    IRB.CreateCall5(Access, IRB.CreatePointerCast(Addr, IRB.getInt8PtrTy()),
                    ConstantInt::get(IntptrTy, Idx),//size
                    ConstantInt::get(IRB.getInt8Ty(), 1),//write
                    ConstantInt::get(IRB.getInt8Ty(), 1),//atomic
                    ConstantInt::get(IRB.getInt8Ty(), 1),//is race candidate
                    "");
  } else if (AtomicRMWInst *RMWI = dyn_cast<AtomicRMWInst>(I)) {
    Value *Addr = RMWI->getPointerOperand();
    int Idx = getMemoryAccessWidth(Addr, DL);
    if (Idx < 0)
      return false;

    IRB.CreateCall5(Access, IRB.CreatePointerCast(Addr, IRB.getInt8PtrTy()),
                    ConstantInt::get(IntptrTy, Idx),//size
                    ConstantInt::get(IRB.getInt8Ty(), 1),//write
                    ConstantInt::get(IRB.getInt8Ty(), 1),//atomic
                    ConstantInt::get(IRB.getInt8Ty(), 1),//is race candidate
                    "");
  } else if (AtomicCmpXchgInst *CASI = dyn_cast<AtomicCmpXchgInst>(I)) {
    Value *Addr = CASI->getPointerOperand();
    int Idx = getMemoryAccessWidth(Addr, DL);
    if (Idx < 0)
      return false;

    IRB.CreateCall5(Access, IRB.CreatePointerCast(Addr, IRB.getInt8PtrTy()),
                    ConstantInt::get(IntptrTy, Idx),//size
                    ConstantInt::get(IRB.getInt8Ty(), 1),//write? XXX really depends on success or not...
                    ConstantInt::get(IRB.getInt8Ty(), 1),//atomic
                    ConstantInt::get(IRB.getInt8Ty(), 1),//is race candidate
                    "");
  }
  return true;
}

int InstrumentAccesses::getMemoryAccessWidth(Value *Addr,
                                             const DataLayout &DL) {
  Type *OrigPtrTy = Addr->getType();
  Type *OrigTy = cast<PointerType>(OrigPtrTy)->getElementType();
  assert(OrigTy->isSized());
  uint32_t TypeSize = DL.getTypeStoreSizeInBits(OrigTy);
  return countTrailingZeros(TypeSize / 8);
}
