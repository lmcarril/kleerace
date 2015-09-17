#include "MemoryAccessEntry.h"

#include "TimingSolver.h"

using namespace klee;

ref<MemoryAccessEntry> MemoryAccessEntry::create(Thread::thread_id_t _thread, const ref<VectorClock> _vc,
                                                const ref<Expr> _address, unsigned _length,
                                                const std::string _varName, const InstructionInfo *_location,
                                                bool _isWrite, std::vector<Thread::thread_id_t>::size_type _scheduleIndex) {

  ref<Expr> end(AddExpr::create(_address, ConstantExpr::create(_length, _address->getWidth())));
  return MemoryAccessEntry::alloc(_thread, _vc, _address, _length, end, _varName, _location, _isWrite, _scheduleIndex);
}

ref<MemoryAccessEntry> MemoryAccessEntry::alloc(Thread::thread_id_t _thread, const ref<VectorClock> _vc,
                                                const ref<Expr> _address, unsigned _length, const ref<Expr> _end,
                                                const std::string _varName, const InstructionInfo *_location,
                                                bool _isWrite, std::vector<Thread::thread_id_t>::size_type _scheduleIndex) {
  ref<MemoryAccessEntry> r(new MemoryAccessEntry(_thread, _vc, _address, _length, _end, _varName, _location, _isWrite, _scheduleIndex));
  return r;
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

int MemoryAccessEntry::compare(const MemoryAccessEntry &other) const {
  if (thread < other.thread)
    return -1;
  else if (thread > other.thread)
    return 1;

  if (address < other.address)
    return -1;
  else if (other.address < address)
    return 1;

  if (length < other.length)
    return -1;
  else if (length > other.length)
    return 1;
  
  if (location && other.location) {
    if (location->file == other.location->file) {
      if (location->line < other.location->line)
        return -1;
      else if (location->line > other.location->line)
        return 1;
    } else {
      if (location < other.location)
        return -1;
      else if (location > other.location)
        return 1;
    }
  }
  return 0;
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
     << "    clock ";
  vc->print(os, thread);
}
