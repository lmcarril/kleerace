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

  MemoryAccessEntry(Thread::thread_id_t _thread, const ref<VectorClock> _vc,
                    const ref<Expr> _address, unsigned _length,
                    const ref<Expr> _end,
                    const std::string _varName, const InstructionInfo *_location,
                    bool _isWrite, std::vector<Thread::thread_id_t>::size_type _scheduleIndex) :
                    thread(_thread), vc(_vc), address(_address), length(_length), end(_end),
                    varName(_varName), location(_location), isWrite(_isWrite), scheduleIndex(_scheduleIndex),
                    refCount(0) {};

public:
  unsigned refCount;
  static ref<MemoryAccessEntry> create(Thread::thread_id_t _thread, const ref<VectorClock> _vc,
                                       const ref<Expr> _address, unsigned _length,
                                       const std::string _varName, const InstructionInfo *_location,
                                       bool _isWrite, std::vector<Thread::thread_id_t>::size_type _scheduleIndex);

  static ref<MemoryAccessEntry> alloc(Thread::thread_id_t _thread, const ref<VectorClock> _vc,
                                      const ref<Expr> _address, unsigned _length, const ref<Expr> _end,
                                      const std::string _varName, const InstructionInfo *_location,
                                      bool _isWrite, std::vector<Thread::thread_id_t>::size_type _scheduleIndex);

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
