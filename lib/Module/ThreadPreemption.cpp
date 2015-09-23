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

using namespace llvm;
using namespace klee;

static cl::opt<bool>
ClPreemptBefore("preempt-before-pthread",
        cl::desc("Add preemption points before the relevant pthread calls"),
        cl::init(false));
static cl::opt<bool>
ClPreemptAfter("preempt-after-pthread",
        cl::desc("Add preemption points after the relevant pthread calls, if the call is succesful"),
        cl::init(false));

char ThreadPreemptionPass::ID = 0;

static Function *checkInterfaceFunction(Constant *FuncOrBitcast) {
  if (Function *F = dyn_cast<Function>(FuncOrBitcast))
    return F;
  FuncOrBitcast->dump();
  report_fatal_error("Instrument Memory Access Pass interface function redefined");
}

bool ThreadPreemptionPass::doInitialization(Module &M) {
  IRBuilder<> IRB(M.getContext());
  // void klee_thread_preempt(int yield)
  preemptFunction = checkInterfaceFunction(M.getOrInsertFunction("klee_thread_preempt",
                                                             IRB.getVoidTy(), IRB.getInt32Ty(), NULL));

  if (Function* f = M.getFunction("pthread_create")) pthreadFunctions.push_back(f);
  if (Function* f = M.getFunction("pthread_mutex_lock")) pthreadFunctions.push_back(f);
  if (Function* f = M.getFunction("pthread_mutex_trylock")) pthreadFunctions.push_back(f);
  if (Function* f = M.getFunction("pthread_mutex_unlock")) pthreadFunctions.push_back(f);
  if (Function* f = M.getFunction("pthread_cond_wait")) pthreadFunctions.push_back(f);
  if (Function* f = M.getFunction("pthread_cond_signal")) pthreadFunctions.push_back(f);
  if (Function* f = M.getFunction("pthread_barrier_wait")) pthreadFunctions.push_back(f);
  if (Function* f = M.getFunction("pthread_rwlock_rdlock")) pthreadFunctions.push_back(f);
  if (Function* f = M.getFunction("pthread_rwlock_tryrdlock")) pthreadFunctions.push_back(f);
  if (Function* f = M.getFunction("pthread_rwlock_wrlock")) pthreadFunctions.push_back(f);
  if (Function* f = M.getFunction("pthread_rwlock_trywrlock")) pthreadFunctions.push_back(f);
  if (Function* f = M.getFunction("pthread_rwlock_unlock")) pthreadFunctions.push_back(f);
  if (Function* f = M.getFunction("sem_wait")) pthreadFunctions.push_back(f);
  if (Function* f = M.getFunction("sem_trywait")) pthreadFunctions.push_back(f);
  if (Function* f = M.getFunction("sem_post")) pthreadFunctions.push_back(f);
  return true;
}

bool ThreadPreemptionPass::runOnModule(Module &M) {
  bool changed = false;

  // Collect function call instructions
  SmallVector<CallInst *, 32> pthreadCalls;
  for (SmallVectorImpl<Function*>::iterator itF = pthreadFunctions.begin(),
       itFe = pthreadFunctions.end(); itF != itFe; ++itF) {
    Function *F = *itF;
    for (Value::use_iterator itU = F->use_begin(),
         itUe = F->use_end(); itU != itUe; ++itU) {
      if (CallInst* call = dyn_cast<CallInst>(*itU))
        pthreadCalls.push_back(call);
    }
  }

  llvm::errs() << "Found "<< pthreadCalls.size()<<"\n";

  if (ClPreemptBefore) {
    for (SmallVectorImpl<CallInst*>::iterator it = pthreadCalls.begin(),
         ite = pthreadCalls.end(); it != ite; ++it)
      changed |= addPreemptionBefore(*it);
  }

  if (ClPreemptAfter) {
    for (SmallVectorImpl<CallInst*>::iterator it = pthreadCalls.begin(),
         ite = pthreadCalls.end(); it != ite; ++it)
      //changed |= addPreemptionAfterIfSuccess(*it);
      changed |= addPreemptionAfter(*it);
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

bool ThreadPreemptionPass::addPreemptionAfterIfSuccess(CallInst *inst) {
  // TODO
  return false;
}
