#ifndef MEMORYACCESSENTRY_H
#define MEMORYACCESSENTRY_H

#include "Thread.h"
#include "VectorClock.h"

#include "klee/Internal/Module/InstructionInfoTable.h"

namespace klee {

class MemoryAccessEntry {
  friend class ExecutionState;
  friend class RaceReport;

public:
  MemoryAccessEntry(Thread::thread_id_t _thread, VectorClock<Thread::thread_id_t> _vc,
                    uint64_t start, uint64_t end, std::string _varName, 
                    const InstructionInfo *_location, bool _isWrite,
                    std::vector<Thread::thread_id_t>::size_type _scheduleIndex);
  
  bool operator<(const MemoryAccessEntry &other) const;
  bool operator>(const MemoryAccessEntry &other) const {
    return other < *this;
  };
  
  bool isRace(const MemoryAccessEntry &other) const;
  
  std::string toString() const;
  std::string toString(const std::vector<Thread::thread_id_t> schedulingHistory) const;

private:
  Thread::thread_id_t thread;
  VectorClock<Thread::thread_id_t> vc;
  uint64_t start;
  uint64_t end;
  std::string varName;
  const InstructionInfo *location;
  bool isWrite;
  std::vector<Thread::thread_id_t>::size_type scheduleIndex;
};
}

#endif // MEMORYACCESSENTRY_H
