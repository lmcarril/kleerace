//===-- PTree.cpp ---------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "PTree.h"

#include <klee/ExecutionState.h>
#include <klee/Expr.h>
#include <klee/util/ExprPPrinter.h>

#include <vector>

using namespace klee;

  /* *** */

PTree::PTree(const data_type &_root) : root(new Node(0,_root)) {
}

PTree::~PTree() {}

std::pair<PTreeNode*, PTreeNode*>
PTree::split(Node *n, 
             const data_type &leftData, 
             const data_type &rightData,
             ForkTag forkTag) {
  assert(n && !n->left && !n->right);
  n->left = new Node(n, leftData);
  n->right = new Node(n, rightData);
  n->forkTag = forkTag;
  return std::make_pair(n->left, n->right);
}

void PTree::remove(Node *n) {
  assert(!n->left && !n->right);
  do {
    Node *p = n->parent;
    delete n;
    if (p) {
      if (n == p->left) {
        p->left = 0;
      } else {
        assert(n == p->right);
        p->right = 0;
      }
    }
    n = p;
  } while (n && !n->left && !n->right);
}

void PTree::dump(llvm::raw_ostream &os) {
  ExprPPrinter *pp = ExprPPrinter::create(os);
  pp->setNewline("\\l");
  os << "digraph G {\n";
  os << "\tsize=\"10,7.5\";\n";
  os << "\tratio=fill;\n";
  os << "\trotate=0;\n";
  os << "\tcenter = \"true\";\n";
  os << "\tnode [style=\"filled\",width=.1,height=.1,fontname=\"Terminus\"]\n";
  os << "\tedge [arrowsize=.3]\n";
  std::vector<PTree::Node*> stack;
  stack.push_back(root);
  while (!stack.empty()) {
    PTree::Node *n = stack.back();
    stack.pop_back();
    os << "\tn" << n << " [";
    os << "label=\"";
    os << "n" << n << "\n";
    os << n->forkTag.forkType << "\n";

    if (n->forkTag.forkType == KLEE_FORK_SCHEDULE) {
      os << "\\[";
      for (std::set<Thread::thread_id_t>::iterator it = n->enabled.begin(), ite = n->enabled.end();
           it != ite; ) {
        os << *it;
        if (n->done.count(*it))
          os << "*";
        if (++it != ite)
          os << ",";
      }
      os << "\\]";
    }

    if (n->condition.isNull()) {
      os << "\"";
    } else {
      os << " ";
      pp->print(n->condition);
      os << "\",shape=diamond";
    }

    if (n->forkTag.forkType == KLEE_FORK_MULTI)
      os << "style=dotted";

    if (n->data && n->forkTag.forkType == KLEE_FORK_SCHEDULE)
      os << ",fillcolor=purple";
    else if (n->data)
      os << ",fillcolor=green";
    os << "];\n";

    if (n->left) {
      os << "\tn" << n << " -> n" << n->left;
      if ((n->forkTag.forkType == KLEE_FORK_SCHEDULE
           || n->forkTag.forkType == KLEE_FORK_MULTI)) {
        os << "[";
        if (n->left->forkTag.forkType != KLEE_FORK_MULTI)
          os << "label=" << n->left->tid;
        else
          os << "style=dotted";
        os << "];\n";
      }
      stack.push_back(n->left);
    }
    if (n->right) {
      os << "\tn" << n << " -> n" << n->right;
      if ((n->forkTag.forkType == KLEE_FORK_SCHEDULE
           || n->forkTag.forkType == KLEE_FORK_MULTI)) {
        os << "[";
        if (n->right->forkTag.forkType != KLEE_FORK_MULTI)
          os << "label=" << n->right->tid;
        else
          os << "style=dotted";
        os << "];\n";
      }
      stack.push_back(n->right);
    }
  }
  os << "}\n";
  delete pp;
}

PTreeNode::PTreeNode(PTreeNode *_parent, 
                     ExecutionState *_data) 
  : parent(_parent),
    left(0),
    right(0),
    data(_data),
    condition(0),
    forkTag(KLEE_FORK_DEFAULT) {
  if (data)
    tid = data->crtThread().getTid();
}

PTreeNode::~PTreeNode() {
}

