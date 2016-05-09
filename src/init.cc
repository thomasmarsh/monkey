#include "cards.h"
#include "util.h"

void Initialize() {
    LOG("initialize");
    LoadCards();
    BuildDoubleIndices();
}
