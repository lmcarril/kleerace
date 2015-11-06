#include "ForkTag.h"

#include "klee/Internal/Module/InstructionInfoTable.h"

using namespace klee;

void ForkTag::print(llvm::raw_ostream &os) const {
  os << "Fork " << forkType;
  if (instruction) {
    os << "at" << instruction->file << ":" << instruction->line;
  }
}
