#include "MemoryAccessEntry.h"

#include "RaceDetection.h"
#include "TimingSolver.h"

#include "llvm/Support/CommandLine.h"

using namespace llvm;
namespace {
  cl::opt<bool>
  PrintLocksets("print-locksets",
               cl::desc("Print the locksets in memory accesses (default=off)"),
               cl::init(false));
}

using namespace klee;

ref<MemoryAccessEntry> MemoryAccessEntry::create(thread_id_t _thread, const ref<VectorClock> _vc,
                                                 const ref<Lockset> _lockset, const ref<Expr> _address, unsigned _length,
                                                 const InstructionInfo *_location,
                                                 bool _isWrite, bool _isAtomic,
                                                 std::vector<thread_id_t>::size_type _scheduleIndex) {

  ref<Expr> end(AddExpr::create(_address, ConstantExpr::create(_length, _address->getWidth())));
  return MemoryAccessEntry::alloc(_thread, _vc, _lockset, _address, _length, end, _location, _isWrite, _isAtomic, _scheduleIndex);
}

ref<MemoryAccessEntry> MemoryAccessEntry::alloc(thread_id_t _thread, const ref<VectorClock> _vc,
                                                const ref<Lockset> _lockset, const ref<Expr> _address, unsigned _length, const ref<Expr> _end,
                                                const InstructionInfo *_location,
                                                bool _isWrite, bool _isAtomic,
                                                std::vector<thread_id_t>::size_type _scheduleIndex) {
  ref<MemoryAccessEntry> r(new MemoryAccessEntry(_thread, _vc, _lockset, _address, _length, _end, _location, _isWrite, _isAtomic, _scheduleIndex));
  return r;
}

bool MemoryAccessEntry::overlap(const ExecutionState &state, TimingSolver &solver, const MemoryAccessEntry &other) const {
  // Check if true: address+length >= other.address AND address <= other.address+other.length
  ref<Expr> overlapExpr = AndExpr::create(UgeExpr::create(end, other.address), UleExpr::create(address, other.end));
  bool result = false;
  return (solver.mustBeTrue(state, overlapExpr, result) && result);
}

bool MemoryAccessEntry::isRace(const ExecutionState &state, TimingSolver &solver, const MemoryAccessEntry &other) const {
  if (thread == other.thread)
    return false;

  if (!isWrite && !other.isWrite)
    return false;

  // TODO atomic read/write vs normal read/write, any case is not a race?
  if (isAtomic && other.isAtomic)
    return false;

  switch(RaceDetectionAlgorithm) {
    case None:
      return false;
    case HappensBeforeAlg:
    case WeakHappensBeforeAlg:
      if (vc->isOrdered(*other.vc))
        return false;
      break;
    case LocksetAlg:
      if (!lockset->disjoint(*other.lockset))
        return false;
      break;
    case HybridAlg:
      if (vc->isOrdered(*other.vc))
        return false;
      if (!lockset->disjoint(*other.lockset))
        return false;
      break;
    default: klee_error("invalid -race-detection");
  }

  return overlap(state, solver, other);
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

  if (isWrite && !other.isWrite)
    return -1;
  else if (!isWrite && other.isWrite)
    return 1;

  if (isAtomic && !other.isAtomic)
    return -1;
  else if (!isAtomic && other.isAtomic)
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
  if (isAtomic)
    os << "atomic ";
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
  if (PrintLocksets) {
    os << "\n"
       << "    lockset "
       << lockset;
  }
}
