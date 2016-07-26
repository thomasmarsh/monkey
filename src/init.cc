#include "cards.h"
#include "util.h"
#include "rand.h"
#include "moves.h"
#include "mcts.h"

ARC4RNG Arc4RNG;
LogContext gLogContext;

size_t Moves::call_count {0};
size_t Moves::moves_count {0};
size_t MCTSAgent::move_count {0};

void Initialize() {
    LOG("initialize");
    LoadCards();
}
