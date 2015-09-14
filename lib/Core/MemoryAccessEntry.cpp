#include "MemoryAccessEntry.h"

#include <iomanip>

namespace klee {

MemoryAccessEntry::MemoryAccessEntry(Thread::thread_id_t _thread,
                                     VectorClock<Thread::thread_id_t> _vc,
                                     ref<Expr> _address, unsigned _length,
                                     std::string _varName, const InstructionInfo *_location,
                                     bool _isWrite, std::vector<Thread::thread_id_t>::size_type _scheduleIndex)
  : thread(_thread), vc(_vc), address(_address), length(_length), varName(_varName),
    location(_location), isWrite(_isWrite), scheduleIndex(_scheduleIndex) {}

bool MemoryAccessEntry::isRace(const MemoryAccessEntry &other) const {
  if (thread == other.thread)
    return false;
  if (isWrite == false && other.isWrite == false)
    return false;
  if (varName != other.varName)
    return false;
  if (!overlaps(address, length, other.address, other.length))
    return false;
  if (vc.happensBefore(other.vc) || other.vc.happensBefore(vc))
    return false;
  return true;
}

bool MemoryAccessEntry::overlaps(ref<Expr> a, unsigned a_len, ref<Expr> b, unsigned b_len) const {
  if (isa<ConstantExpr>(a) && isa<ConstantExpr>(b)) {
    uint64_t a_base = cast<ConstantExpr>(a)->getZExtValue();
    uint64_t b_base = cast<ConstantExpr>(b)->getZExtValue();
    if (((a_base+a_len) >= b_base) && (a_base <= (b_base+b_len)))
      return true;
    return false;
  } else {
    // TODO manage case when both are not constant
    return true;
  }
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
     ss << " at address 0x" << std::setbase(16) << addrExpr->getZExtValue();
  else
     ss << " at variable " << varName;
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
