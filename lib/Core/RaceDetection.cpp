#include "RaceDetection.h"

using namespace klee;
using namespace llvm;

cl::opt<RaceAlg>
klee::RaceDetectionAlgorithm("race-detection",
                       cl::desc("Race detection algorithm (default=hb)"),
                       cl::values(
                         clEnumValN(None, "off", "Disabled race detection"),
                         clEnumValN(HappensBeforeAlg, "hb", "Happens before"),
                         clEnumValN(WeakHappensBeforeAlg, "whb", "Weak happens before (no mutex hb-edges)"),
                         clEnumValN(LocksetAlg, "ls", "Lockset"),
                         clEnumValN(HybridAlg, "hyb", "Weak happens before with lockset"),
                         clEnumValEnd),
                       cl::init(HappensBeforeAlg));
