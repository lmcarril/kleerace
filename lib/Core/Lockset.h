#ifndef LOCKSET_H
#define LOCKSET_H

#include "Common.h"

#include "klee/util/Ref.h"

#include "llvm/Support/raw_ostream.h"

#include <set>

namespace klee {

class Lockset {
private:
  typedef std::set<uint64_t>::const_iterator
  locks_iterator_t;
  const std::set<uint64_t> locks;

  Lockset(const std::set<uint64_t> _locks)
      : locks(_locks), refCount(0) {}

public:
  unsigned refCount;

  static ref<Lockset> create() {
    std::set<uint64_t> empty;
    return Lockset::alloc(empty);
  };
  static ref<Lockset> create(const std::set<uint64_t> locks) {
    return Lockset::alloc(locks);
  };
  static ref<Lockset> alloc(const std::set<uint64_t> locks);

  int compare(const Lockset &other) const;

  ref<Lockset> erase(uint64_t val) const;
  ref<Lockset> insert(uint64_t val) const;

  ref<Lockset> intersect(const Lockset &other) const;

  bool empty() const { return locks.empty(); };

  bool disjoint(const Lockset &other) const {
    return intersect(other)->empty();
  };

  void print(llvm::raw_ostream &os) const;
};

inline llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const Lockset &vc) {
  vc.print(os);
  return os;
}
}
#endif
