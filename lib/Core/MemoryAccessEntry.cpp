#include "MemoryAccessEntry.h"

#include "TimingSolver.h"

namespace klee {

MemoryAccessEntry::MemoryAccessEntry(Thread::thread_id_t _thread, const ref<VectorClock> _vc,
                                     const ref<Expr> _address, unsigned _length,
                                     const std::string _varName, const InstructionInfo *_location,
                                     bool _isWrite, std::vector<Thread::thread_id_t>::size_type _scheduleIndex)
  : thread(_thread), vc(_vc), address(_address), length(_length),
    // Build end expression: address + length, not in body because operator= for ref is overloaded
    end(AddExpr::create(address, ConstantExpr::create(length, address->getWidth()))),
    varName(_varName), location(_location), isWrite(_isWrite), scheduleIndex(_scheduleIndex) {
}

bool MemoryAccessEntry::isRace(const ExecutionState &state, TimingSolver &solver, const MemoryAccessEntry &other) const {
  if (thread == other.thread)
    return false;

  if (isWrite == false && other.isWrite == false)
    return false;

  if (varName != other.varName)
    return false;

  if (vc->happensBefore(*other.vc) || other.vc->happensBefore(*vc))
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

void MemoryAccessEntry::print(llvm::raw_ostream &os) const {
  os << (isWrite ? "store" : "load");
  os << " at address ";
  if (ConstantExpr *addrExpr = dyn_cast<ConstantExpr>(address))
     os << addrExpr;
  else
     os << "???";
  os << " of length " << length << "\n"
     << "    by thread " << thread << "\n"
     << "    from ";
  if (location)
    os << location->file << ":" << location->line;
  else
    os << "???";
  os << "\n"
     << "    clock " << vc;
}
}
