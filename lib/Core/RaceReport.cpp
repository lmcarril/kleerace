#include "RaceReport.h"

namespace klee {
std::set<RaceReport> RaceReport::emittedReports;
 
RaceReport::RaceReport(const MemoryAccessEntry &_previous,
                       const MemoryAccessEntry &_current,
                       const std::vector<Thread::thread_id_t> &_schedulingHistory)
    : previous(_previous), current(_current), schedulingHistory(_schedulingHistory) {}

bool RaceReport::operator<(const RaceReport &rr) const {
  if (current < rr.current)
    return true;
  else if (current > rr.current)
    return false;
  else if (previous < rr.previous)
    return true;
  else if (previous > rr.previous)
    return false;

  return false;
}

void RaceReport::print(llvm::raw_ostream &os) const {
  os << "========\n";
  os << "Race found on variable: " << current.varName << "\n";
  os << current << "\n";
  os << "    schedule ";
  printSchedule(os, current.scheduleIndex, schedulingHistory);
  os << "\n";
  os << "Conflicts with previous operation:\n";
  os << previous << "\n";
  os << "    schedule ";
  printSchedule(os, previous.scheduleIndex, schedulingHistory);
  os << "\n";
  os << "========";
}

void RaceReport::printSchedule(llvm::raw_ostream &os,
                               std::vector<Thread::thread_id_t>::size_type scheduleIndex,
                               const std::vector<Thread::thread_id_t> schedulingHistory) const {
  for (std::vector<Thread::thread_id_t>::size_type i = 0; i < scheduleIndex;) {
    os << schedulingHistory.at(i);
    if (++i < scheduleIndex)
      os << ",";
  }
}

}
