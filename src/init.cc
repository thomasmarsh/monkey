#include "cards.h"
#include "util.h"

int gTraceLevel = 0;

void Initialize() {
    LOG("initialize");
    LoadCards();
    BuildDoubleIndices();
}
