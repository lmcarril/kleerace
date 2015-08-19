#ifndef RACEREPORT_H
#define RACEREPORT_H

#include "MemoryAccessEntry.h"

namespace klee {

class RaceReport {
public:
  RaceReport(const MemoryAccessEntry &_previous, const MemoryAccessEntry &_current, 
             const std::vector<Thread::thread_id_t> &_schedulingHistory);

  std::string toString() const;

  bool operator<(const RaceReport &rr) const;

  static std::set<RaceReport> emittedReports;

private:
  const MemoryAccessEntry previous;
  const MemoryAccessEntry current;
  const std::vector<Thread::thread_id_t> schedulingHistory;
  
  std::string printMemoryAccess(const MemoryAccessEntry &access) const;
};
}

#endif // RACEREPORT_H
