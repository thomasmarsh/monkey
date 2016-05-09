#include "cards.h"
#include "util.h"
#include "rand.h"

ARC4RNG Arc4RNG;
int gTraceLevel = 0;

void Initialize() {
    LOG("initialize");
    LoadCards();
    BuildDoubleIndices();
}
