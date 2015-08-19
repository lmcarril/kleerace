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

std::string RaceReport::toString() const {
  std::stringstream ss;
  ss << "========\n";
  ss << "Race found on variable: " << current.varName << "\n";
  ss << current.toString(schedulingHistory);
  ss << "Conflicts with previous operation: " << "\n";
  ss << previous.toString(schedulingHistory);
  ss << "========\n";
  return ss.str();
}
}
