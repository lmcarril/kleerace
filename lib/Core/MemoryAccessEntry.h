#ifndef MEMORYACCESSENTRY_H
#define MEMORYACCESSENTRY_H

#include "Thread.h"
#include "VectorClock.h"

#include "klee/Internal/Module/InstructionInfoTable.h"

#include "llvm/Support/raw_ostream.h"

#include <vector>

namespace klee {

class MemoryAccessEntry {
  friend class RaceReport;

public:
  MemoryAccessEntry(Thread::thread_id_t _thread, const ref<VectorClock> _vc,
                    const ref<Expr> _address, unsigned _length,
                    const std::string _varName, const InstructionInfo *_location,
                    bool _isWrite, std::vector<Thread::thread_id_t>::size_type _scheduleIndex);
  
  bool operator<(const MemoryAccessEntry &other) const;
  bool operator>(const MemoryAccessEntry &other) const {
    return other < *this;
  };
  
  bool isRace(const ExecutionState &state, TimingSolver &solver, const MemoryAccessEntry &other) const;

  void print(llvm::raw_ostream &os) const;

private:
  Thread::thread_id_t thread;
  ref<VectorClock> vc;
  ref<Expr> address;
  unsigned length;
  ref<Expr> end;
  std::string varName;
  const InstructionInfo *location;
  bool isWrite;
  std::vector<Thread::thread_id_t>::size_type scheduleIndex;
};

inline llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const MemoryAccessEntry &ma) {
  ma.print(os);
  return os;
}
}

#endif // MEMORYACCESSENTRY_H
