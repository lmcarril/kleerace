#include "VectorClock.h"

using namespace klee;

ref<VectorClock> VectorClock::create(const uint32_t *buf, const uint32_t nelements) {
  std::vector<clock_counter_t> clocks(buf, buf+nelements);
  return VectorClock::alloc(clocks);
}

ref<VectorClock> VectorClock::alloc(const std::vector<clock_counter_t> clocks) {
  ref<VectorClock> r(new VectorClock(clocks));
  return r;
}

int VectorClock::compare(const VectorClock &other) const {
  bool allLess = true;
  bool allEqual = true;
  clock_iterator_t itA = clocks.begin();
  clock_iterator_t itB = other.clocks.begin();
  for (;itA != clocks.end() && itB != other.clocks.end(); ++itA, ++itB) {
    allLess &= (*itA < *itB);
    allEqual &= (*itA == *itB);
  }

  if ((allLess) && (clocks.size() <= other.clocks.size()))
    return -1;
  if ((allEqual) && (clocks.size() == other.clocks.size()))
    return 0;
  return 1;
}

int VectorClock::happensBefore(const VectorClock &other) const {
  bool strictSmallerExists = false;
  bool allLessOrEqual = true;

  if (clocks.size() != other.clocks.size())
    return false;

  clock_iterator_t itA = clocks.begin();
  clock_iterator_t itB = other.clocks.begin();
  for (;itA != clocks.end() && itB != other.clocks.end(); ++itA, ++itB) {
    strictSmallerExists |= (*itA < *itB);
    allLessOrEqual &= (*itA <= *itB);
  }
  return (allLessOrEqual && strictSmallerExists);
}

#define ANSI_UNDERLINED_PRE  "\033[4m"
#define ANSI_UNDERLINED_POST "\033[0m"
void VectorClock::print(llvm::raw_ostream &os, std::vector<clock_counter_t>::size_type index) const {
  os << "(";
  unsigned int i = 0;
  for (clock_iterator_t it = clocks.begin();
       it != clocks.end(); ++i) {
    if (i==index)
      os << ANSI_UNDERLINED_PRE << *it << ANSI_UNDERLINED_POST;
    else
      os << *it;
    if (++it!=clocks.end())
      os << ",";
  }
  os << ")";
}

void VectorClock::print(llvm::raw_ostream &os) const {
  os << "(";
  unsigned int i = 0;
  for (clock_iterator_t it = clocks.begin();
       it != clocks.end(); ++i) {
    os << i << ":" << *it;
    if (++it!=clocks.end())
      os << ",";
  }
  os << ")";
}
