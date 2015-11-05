#ifndef RACEDETECTION_H
#define RACEDETECTION_H

#include "llvm/Support/CommandLine.h"

namespace klee {
enum RaceAlg {
  None,
  HappensBeforeAlg,
  WeakHappensBeforeAlg,
  LocksetAlg,
  HybridAlg
};

extern llvm::cl::opt<klee::RaceAlg> RaceDetectionAlgorithm;
}
#endif // RACEDETECTION_H
