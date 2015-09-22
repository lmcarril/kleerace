#include "Lockset.h"

using namespace klee;

ref<Lockset> Lockset::alloc(const std::set<uint64_t> locks) {
  ref<Lockset> r(new Lockset(locks));
  return r;
}

int Lockset::compare(const Lockset &other) const {
  if (locks.size() < other.locks.size())
    return -1;
  if (locks == other.locks)
    return 0;
  return 1;
}

ref<Lockset> Lockset::erase(uint64_t val) const {
  std::set<uint64_t> upd(locks);
  upd.erase(val);
  return Lockset::alloc(upd);
}

ref<Lockset> Lockset::insert(uint64_t val) const {
  std::set<uint64_t> upd(locks);
  upd.insert(val);
  return Lockset::alloc(upd);
}

ref<Lockset> Lockset::intersect(const Lockset &other) const {
  std::set<uint64_t> inter;
  for (locks_iterator_t it = locks.begin();
       it != locks.end();++it) {
    if (other.locks.find(*it) != other.locks.end())
      inter.insert(*it);
  }

  return Lockset::alloc(inter);
}

void Lockset::print(llvm::raw_ostream &os) const {
  os << "(";
  for (locks_iterator_t it = locks.begin();
       it != locks.end();) {
    os.write_hex(*it);
    if (++it!=locks.end())
      os << ",";
  }
  os << ")";
}
