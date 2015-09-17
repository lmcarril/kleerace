#ifndef VECTORCLOCK_H
#define VECTORCLOCK_H

#include "Common.h"

#include "klee/util/Ref.h"

#include "llvm/Support/raw_ostream.h"

#include <vector>

namespace klee {

class VectorClock {
private:
  typedef uint32_t clock_counter_t;
  typedef std::vector<clock_counter_t>::const_iterator
  clock_iterator_t;
  const std::vector<clock_counter_t> clocks;

  VectorClock(const std::vector<clock_counter_t> _clocks)
      : clocks(_clocks), refCount(0) {}

public:
  unsigned refCount;

  static ref<VectorClock> create(const uint32_t *buf, const uint32_t nelements);
  static ref<VectorClock> alloc(const std::vector<clock_counter_t> clocks);

  int compare(const VectorClock &other) const;

  int happensBefore(const VectorClock &other) const;

  void print(llvm::raw_ostream &os) const;

  void print(llvm::raw_ostream &os, std::vector<clock_counter_t>::size_type index) const;
};

inline llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const VectorClock &vc) {
  vc.print(os);
  return os;
}
}
#endif
