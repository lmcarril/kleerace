#ifndef RACEREPORT_H
#define RACEREPORT_H

#include "MemoryAccessEntry.h"

#include "llvm/Support/raw_ostream.h"

#include <set>
#include <vector>

namespace klee {

class RaceReport {
private:
  const ref<MemoryAccessEntry> previous;
  const ref<MemoryAccessEntry> current;
  const std::vector<Thread::thread_id_t> schedulingHistory;

  void printSchedule(llvm::raw_ostream &os,
                     std::vector<Thread::thread_id_t>::size_type scheduleIndex,
                     const std::vector<Thread::thread_id_t> schedulingHistory) const;

public:
  static std::set<RaceReport> emittedReports;

  RaceReport(const ref<MemoryAccessEntry> &_previous, const ref<MemoryAccessEntry> &_current,
             const std::vector<Thread::thread_id_t> &_schedulingHistory) :
             previous(_previous), current(_current), schedulingHistory(_schedulingHistory) {}

  bool operator<(const RaceReport &rr) const;

  void print(llvm::raw_ostream &os) const;

};

inline llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const RaceReport &rr) {
  rr.print(os);
  return os;
}
}

#endif // RACEREPORT_H
