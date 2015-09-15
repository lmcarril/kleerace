#include "MemoryAccessEntry.h"

#include "TimingSolver.h"

#include <iomanip>

namespace klee {

MemoryAccessEntry::MemoryAccessEntry(Thread::thread_id_t _thread,
                                     VectorClock<Thread::thread_id_t> _vc,
                                     ref<Expr> _address, unsigned _length,
                                     std::string _varName, const InstructionInfo *_location,
                                     bool _isWrite, std::vector<Thread::thread_id_t>::size_type _scheduleIndex)
  : thread(_thread), vc(_vc), address(_address), length(_length), varName(_varName),
    location(_location), isWrite(_isWrite), scheduleIndex(_scheduleIndex) {
  // Build end expression: address + length
  end = AddExpr::create(address, ConstantExpr::create(length, address->getWidth()));
}

bool MemoryAccessEntry::isRace(const ExecutionState &state, TimingSolver &solver, const MemoryAccessEntry &other) const {
  if (thread == other.thread)
    return false;

  if (isWrite == false && other.isWrite == false)
    return false;

  if (varName != other.varName)
    return false;

  if (vc.happensBefore(other.vc) || other.vc.happensBefore(vc))
    return false;

  // Check if true: address+length >= other.address AND address <= other.address+other.length
  ref<Expr> overlapExpr = AndExpr::create(UgeExpr::create(end, other.address), UleExpr::create(address, other.end));
  bool result = false;
  if (!(solver.mustBeTrue(state, overlapExpr, result) && result))
    return false;

  return true;
}

bool MemoryAccessEntry::operator<(const MemoryAccessEntry &other) const {
  if (thread < other.thread)
    return true;
  else if (thread > other.thread)
    return false;

  if (address < other.address)
    return true;
  else if (other.address < address)
    return false;

  if (length < other.length)
    return true;
  else if (length > other.length)
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
  ss << (isWrite ? "store" : "load");
  if (ConstantExpr *addrExpr = dyn_cast<ConstantExpr>(address))
     ss << " at address " << addrExpr;
  else
     ss << " at address ???";
  ss << " of length " << length << "\n"
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
