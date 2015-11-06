/*
 * Cloud9 Parallel Symbolic Execution Engine
 *
 * Copyright (c) 2011, Dependable Systems Laboratory, EPFL
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Dependable Systems Laboratory, EPFL nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE DEPENDABLE SYSTEMS LABORATORY, EPFL BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * All contributors are listed in CLOUD9-AUTHORS file.
 *
*/

#ifndef KLEE_FORKTAG_H
#define KLEE_FORKTAG_H

#include "llvm/Support/raw_ostream.h"

namespace llvm {
class Function;
}

namespace klee {
struct InstructionInfo;

enum ForkType {
  KLEE_FORK_DEFAULT  = 0,
  KLEE_FORK_INTERNAL = 1,
  KLEE_FORK_SCHEDULE = 2,
  KLEE_FORK_MULTI    = 3 
};

inline llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const ForkType reason) {
  switch(reason) {
    case KLEE_FORK_DEFAULT: os << "DEFAULT"; break;
    case KLEE_FORK_INTERNAL: os << "INTERNAL"; break;
    case KLEE_FORK_SCHEDULE: os << "SCHEDULE"; break;
    case KLEE_FORK_MULTI: os << "MULTI"; break;
    default: os << "UNKNOWN"; break;
  }
  return os;
}

struct ForkTag {
  ForkType forkType;

  // The location in the code where the fork was decided (it can be NULL)
  const llvm::Function * function;
  const InstructionInfo * instruction;

  ForkTag(ForkType _ftype) :
    forkType(_ftype), function(0), instruction(0) {}

  void print(llvm::raw_ostream &os) const;
};

inline llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const ForkTag &tag) {
  tag.print(os);
  return os;
}

}

#endif /* KLEE_FORKTAG_H */
