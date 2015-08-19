#include "MemoryAccessEntry.h"

#include <iomanip>

namespace klee {

MemoryAccessEntry::MemoryAccessEntry(Thread::thread_id_t _thread,
                                     VectorClock<Thread::thread_id_t> _vc,
                                     uint64_t _start, uint64_t _end,
                                     std::string _varName, const InstructionInfo *_location,
                                     bool _isWrite, std::vector<Thread::thread_id_t>::size_type _scheduleIndex)
  : thread(_thread), vc(_vc), start(_start), end(_end), varName(_varName), 
    location(_location), isWrite(_isWrite), scheduleIndex(_scheduleIndex) {}

bool MemoryAccessEntry::isRace(const MemoryAccessEntry &other) const {
  if (thread == other.thread)
    return false;
  if (isWrite == false && other.isWrite == false)
    return false;
  if (varName != other.varName)
    return false;
  if (!(end >= other.start && start <= other.end))
    return false;
  if (vc.happensBefore(other.vc) || other.vc.happensBefore(vc))
    return false;
  return true;
}

bool MemoryAccessEntry::operator<(const MemoryAccessEntry &other) const {
  if (thread < other.thread)
    return true;
  else if (thread > other.thread)
    return false;

  if (start < other.start)
    return true;
  else if (start > other.start)
    return false;

  if (end < other.end)
    return true;
  else if (end > other.end)
    return false;
  
  if (location && other.location) {
    if (location->file == other.location->file) {
      if (location->line < other.location->line)
        return true;
      else if (location->line > other.location->line)
        return false;
    } else {
      if (location < other.location)
        return true;
      else if (location > other.location)
        return false;
    }
  }
  return false;
}

std::string MemoryAccessEntry::toString() const {
  std::stringstream ss;
  ss << (isWrite ? "store" : "load")
     << " at adress 0x" << std::setbase(16) << start
     << " of length " << end-start+1 << "\n"
     << "    by thread " << thread << "\n"
     << "    from ";
  if (location)
    ss << location->file << ":" << location->line;
  else
    ss << "???";
  ss << "\n"
     << "    clock " << vc.toString() << "\n";
  return ss.str();
}

std::string MemoryAccessEntry::toString(const std::vector<Thread::thread_id_t> schedulingHistory) const {
  std::stringstream ss;
  ss << toString()
     << "    schedule ";
  for (std::vector<Thread::thread_id_t>::size_type i = 0;
       i < scheduleIndex; i++)
    ss << schedulingHistory.at(i) << ",";
  ss.seekp(((long)ss.tellp())-1);
  ss << "\n";
  return ss.str();
}
}
