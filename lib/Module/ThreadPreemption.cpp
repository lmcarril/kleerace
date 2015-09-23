//===-- ThreadPreemption.cpp - Introduce thread preemption points ---------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "Passes.h"
#include "klee/Config/Version.h"
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 3)
#include "llvm/IR/LLVMContext.h"
#else
#include "llvm/LLVMContext.h"
#endif
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

using namespace llvm;
using namespace klee;

static cl::opt<bool>
ClPreemptBefore("preempt-before-pthread",
        cl::desc("Add preemption points before pthread calls (default=off)"),
        cl::init(false));

static cl::opt<bool>
ClPreemptAfterIfSuccess("preempt-after-pthread-success",
        cl::desc("Add preemption points after pthread calls, if the call is succesful (default=on)"),
        cl::init(true));

char ThreadPreemptionPass::ID = 0;

static Function *checkInterfaceFunction(Constant *FuncOrBitcast) {
  if (Function *F = dyn_cast<Function>(FuncOrBitcast))
    return F;
  FuncOrBitcast->dump();
  report_fatal_error("Instrument Memory Access Pass interface function redefined");
}

static const int nPthreadNames = 14;
static const char * pthreadFuncNames[nPthreadNames] = {
  "pthread_create",
  "pthread_mutex_lock",
  "pthread_mutex_trylock",
  "pthread_mutex_unlock",
  "pthread_cond_wait",
  "pthread_cond_signal",
  "pthread_rwlock_rdlock",
  "pthread_rwlock_tryrdlock",
  "pthread_rwlock_wrlock",
  "pthread_rwlock_trywrlock",
  "pthread_rwlock_unlock",
  "sem_wait",
  "sem_trywait",
  "sem_post"
};

bool ThreadPreemptionPass::doInitialization(Module &M) {
  IRBuilder<> IRB(M.getContext());
  // void klee_thread_preempt(int yield)
  preemptFunction = checkInterfaceFunction(M.getOrInsertFunction("klee_thread_preempt",
                                                             IRB.getVoidTy(), IRB.getInt32Ty(), NULL));

  ConstantInt *zero = ConstantInt::get(IRB.getInt32Ty(), 0);
  for (int i = 0; i < nPthreadNames; i++)
    if (Function* f = M.getFunction(pthreadFuncNames[i]))
      pthreadFunctions.push_back(std::make_pair(f, zero));

  if (Function* f = M.getFunction("pthread_barrier_wait"))
    pthreadFunctions.push_back(std::make_pair(f, ConstantInt::get(IRB.getInt32Ty(),
                                                                  PTHREAD_BARRIER_SERIAL_THREAD)));
  return true;
}

bool ThreadPreemptionPass::runOnModule(Module &M) {
  bool changed = false;

  // Collect function call instructions
  SmallVector<std::pair<CallInst *, ConstantInt *>, 32> pthreadCalls;
  for (SmallVectorImpl<std::pair<Function*, ConstantInt*> >::iterator itF = pthreadFunctions.begin(),
       itFe = pthreadFunctions.end(); itF != itFe; ++itF) {
    Function *F = itF->first;
    for (Value::use_iterator itU = F->use_begin(),
         itUe = F->use_end(); itU != itUe; ++itU) {
      if (CallInst* call = dyn_cast<CallInst>(*itU))
        pthreadCalls.push_back(std::make_pair(call,itF->second));
    }
  }

  if (ClPreemptBefore) {
    for (SmallVectorImpl<std::pair<CallInst*,ConstantInt *> >::iterator it = pthreadCalls.begin(),
         ite = pthreadCalls.end(); it != ite; ++it)
      changed |= addPreemptionBefore(it->first);
  }

  if (ClPreemptAfterIfSuccess) {
    for (SmallVectorImpl<std::pair<CallInst*,ConstantInt *> >::iterator it = pthreadCalls.begin(),
         ite = pthreadCalls.end(); it != ite; ++it) {
      // Check to avoid a double preeemption
      if (CallInst *nCall = dyn_cast<CallInst>(it->first->getNextNode()))
        if (nCall->getCalledFunction() == preemptFunction)
          continue;
      changed |= addPreemptionAfterIfSuccess(it->first, it->second);
    }
  }

  return changed;
}

bool ThreadPreemptionPass::addPreemptionBefore(Instruction *I) {
  IRBuilder<> IRB(I);
  IRB.CreateCall(preemptFunction, ConstantInt::get(IRB.getInt32Ty(), 0), "");
  return true;
}

bool ThreadPreemptionPass::addPreemptionAfter(Instruction *I) {
  IRBuilder<> IRB(I->getNextNode());
  IRB.CreateCall(preemptFunction, ConstantInt::get(IRB.getInt32Ty(), 0), "");
  return true;
}

bool ThreadPreemptionPass::addPreemptionAfterIfSuccess(CallInst *inst, ConstantInt *ret) {
  IRBuilder<> IRB(inst->getNextNode());
  if (!(inst->getCalledFunction()->getFunctionType()->isValidReturnType(ret->getType())))
    return false;

  Value *cmp = IRB.CreateICmpEQ(inst, ret, "_preemptcmp");
  TerminatorInst *term = SplitBlockAndInsertIfThen(cast<Instruction>(cmp), false, NULL);
  addPreemptionBefore(term);
  return true;
}
