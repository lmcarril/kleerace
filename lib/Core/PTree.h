//===-- PTree.h -------------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef __UTIL_PTREE_H__
#define __UTIL_PTREE_H__

#include <klee/Expr.h>

#include "ForkTag.h"
#include "Thread.h"

#include <set>

namespace klee {
  class ExecutionState;

  class PTree { 
    typedef ExecutionState* data_type;

  public:
    typedef class PTreeNode Node;
    Node *root;

    PTree(const data_type &_root);
    ~PTree();
    
    std::pair<Node*,Node*> split(Node *n,
                                 const data_type &leftData,
                                 const data_type &rightData,
                                 ForkTag forkTag);
    void remove(Node *n);

    void dump(llvm::raw_ostream &os);
  };

  class PTreeNode {
    friend class PTree;
  public:
    PTreeNode *parent, *left, *right;
    ExecutionState *data;
    ref<Expr> condition;

    ForkTag forkTag;

    // Thread at PNode instantiation step
    Thread::thread_id_t tid;
    std::set<Thread::thread_id_t>::size_type schedulingIndex;

    // Thread enabled at end of PNode
    std::set<Thread::thread_id_t> enabled;
    std::set<Thread::thread_id_t> done;
  private:
    PTreeNode(PTreeNode *_parent, ExecutionState *_data);
    ~PTreeNode();
  };
}

#endif
