#ifndef RACEREPORT_H
#define RACEREPORT_H

#include "MemoryAccessEntry.h"

#include "llvm/Support/raw_ostream.h"

#include <set>
#include <vector>

namespace klee {

class RaceReport {
private:
  const std::string allocInfo;
  const ref<MemoryAccessEntry> current;
  const ref<MemoryAccessEntry> previous;
  const std::vector<MemoryAccessEntry::thread_id_t> schedulingHistory;

  void printSchedule(llvm::raw_ostream &os,
                     std::vector<MemoryAccessEntry::thread_id_t>::size_type scheduleIndex,
                     const std::vector<MemoryAccessEntry::thread_id_t> schedulingHistory) const;

public:
  static std::set<RaceReport> emittedReports;

  RaceReport(const std::string _allocInfo,
             const ref<MemoryAccessEntry> &_current, const ref<MemoryAccessEntry> &_previous,
             const std::vector<MemoryAccessEntry::thread_id_t> &_schedulingHistory) :
             allocInfo(_allocInfo), current(_current), previous(_previous),
             schedulingHistory(_schedulingHistory) {}

  bool operator<(const RaceReport &rr) const;

  void print(llvm::raw_ostream &os) const;

};

inline llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const RaceReport &rr) {
  rr.print(os);
  return os;
}
}

#endif // RACEREPORT_H
