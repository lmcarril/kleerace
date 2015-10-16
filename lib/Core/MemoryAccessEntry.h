#ifndef MEMORYACCESSENTRY_H
#define MEMORYACCESSENTRY_H

#include "Lockset.h"
#include "VectorClock.h"

#include "klee/Expr.h"
#include "klee/Internal/Module/InstructionInfoTable.h"

#include "llvm/Support/raw_ostream.h"

#include <vector>

namespace klee {
class ExecutionState;
class TimingSolver;

class MemoryAccessEntry {
  friend class RaceReport;

private:
  // TODO should be the same as Thread::thread_id_t
  // But including Thread produces a dependecy cycle between MemoryAccessEntry, Thread and ThreadSegment
  // forward declaration does not help => incomplete type madness
  typedef uint64_t thread_id_t;
  thread_id_t thread;
  ref<VectorClock> vc;
  ref<Lockset> lockset;
  ref<Expr> address;
  unsigned length;
  ref<Expr> end;
  const InstructionInfo *location;
  bool isWrite;
  bool isAtomic;
  std::vector<thread_id_t>::size_type scheduleIndex;

  MemoryAccessEntry(thread_id_t _thread, const ref<VectorClock> _vc,
                    const ref<Lockset> _lockset, const ref<Expr> _address,
                    unsigned _length, const ref<Expr> _end,
                    const InstructionInfo *_location,
                    bool _isWrite, bool _isAtomic,
                    std::vector<thread_id_t>::size_type _scheduleIndex) :
                    thread(_thread), vc(_vc), lockset(_lockset),
                    address(_address), length(_length), end(_end),
                    location(_location), isWrite(_isWrite), isAtomic(_isAtomic),
                    scheduleIndex(_scheduleIndex), refCount(0) {};

public:
  unsigned refCount;
  static ref<MemoryAccessEntry> create(thread_id_t _thread, const ref<VectorClock> _vc,
                                       const ref<Lockset> _lockset, const ref<Expr> _address, unsigned _length,
                                       const InstructionInfo *_location,
                                       bool _isWrite, bool _isAtomic,
                                       std::vector<thread_id_t>::size_type _scheduleIndex);

  static ref<MemoryAccessEntry> alloc(thread_id_t _thread, const ref<VectorClock> _vc,
                                      const ref<Lockset> _lockset, const ref<Expr> _address,
                                      unsigned _length, const ref<Expr> _end,
                                      const InstructionInfo *_location,
                                      bool _isWrite, bool _isAtomic,
                                      std::vector<thread_id_t>::size_type _scheduleIndex);

  int compare(const MemoryAccessEntry &other) const;

  bool isRace(const ExecutionState &state, TimingSolver &solver, const MemoryAccessEntry &other) const;

  void print(llvm::raw_ostream &os) const;
};

inline llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const MemoryAccessEntry &ma) {
  ma.print(os);
  return os;
}
}

#endif // MEMORYACCESSENTRY_H
