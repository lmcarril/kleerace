#ifndef VECTORCLOCK_H
#define VECTORCLOCK_H

#include "Common.h"

#include <inttypes.h>
#include <map>
#include <string>

namespace klee {

template <typename index_t> class VectorClock {
private:
  typedef uint32_t clock_counter_t;
  std::map<index_t, clock_counter_t> clockMap; //TODO move to a array based approach

public:
  typedef uint64_t vc_id_t;
  typedef typename std::map<index_t, clock_counter_t>::const_iterator
  clock_iterator_t;

  void merge(const VectorClock<index_t> &other) {
    for (clock_iterator_t it = other.clockMap.begin();
         it != other.clockMap.end(); ++it)
      if (clockMap[it->first] < it->second)
        clockMap[it->first] = it->second;
  }

  void clear() { clockMap.clear(); }
  void tock(index_t index) { clockMap[index]++; }

  bool happensBefore(const VectorClock<index_t> &other) const {
    bool greaterExists = false;
    bool allLessOrEqual = true;

    clock_iterator_t iterA = clockMap.begin();
    clock_iterator_t iterB = other.clockMap.begin();
    while (iterA != clockMap.end() || iterB != other.clockMap.end()) {
      if (iterA == clockMap.end()) {
        greaterExists = true;
        iterB++;
      } else if (iterB == other.clockMap.end()) {
        allLessOrEqual = false;
        iterA++;
      } else {
        if (iterA->first == iterB->first) {
          // value included in both vcs
          if (iterA->second < iterB->second)
            greaterExists = true;

          if (iterA->second > iterB->second)
            allLessOrEqual = false;

          iterA++;
          iterB++;
        } else if (iterA->first > iterB->first) {
          // vc B got an entry more
          greaterExists = true;
          iterB++;
        } else {
          allLessOrEqual = false;
          iterA++;
        }
      }
    }
    return (greaterExists && allLessOrEqual);
  }
/*
  bool operator<(const VectorClock<index_t> &other) const {
    clock_iterator_t iterA = clockMap.begin();
    clock_iterator_t iterB = other.clockMap.begin();
    while (iterA != clockMap.end() || iterB != other.clockMap.end()) {
      if (iterA == clockMap.end())
        return true;
      else if (iterB == other.clockMap.end())
        return false;
      else {
        if (iterA->first == iterB->first && iterA->second == iterB->second) {
          iterA++;
          iterB++;
        } else if (iterA->first < iterB->first)
          return true;
        else if (iterA->first == iterB->first && iterA->second < iterB->second)
          return true;
        else
          return false;
      }
    }
    return false;
  }
*/
  std::string toString() const {
    std::stringstream ss;
    ss << "(";
    for (clock_iterator_t iter = clockMap.begin();
         iter != clockMap.end(); ++iter)
      ss << iter->first << ":" << iter->second << ",";
    if (clockMap.size() > 0)
      ss.seekp(((long)ss.tellp())-1);
    ss << ")";
    return ss.str();
  }
};
}
#endif
