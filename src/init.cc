#include "cards.h"
#include "util.h"
#include "rand.h"

ARC4RNG Arc4RNG;
LogContext gLogContext;

void Initialize() {
    LOG("initialize");
    LoadCards();
}
