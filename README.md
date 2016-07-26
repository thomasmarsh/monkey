Card game test bed for AI implementation. The game rules are a work in
progress (a family collaborative effort), and this implementation
corresponds to an old version of the game. It is a non-deterministic
game of hidden information for 2-4 players.

The principal AI uses the Information Set Monte Carlo Tree Search (ISMCTS)
approach to finding the best move. More information on the algorithm
can be found here: http://eprints.whiterose.ac.uk/75048/1/CowlingPowleyWhitehouse2012.pdf. Based on empirical results, and as is consistent with other
MCTS implementations, a number of domain-specific adjustments improved the performance of the AI.

For comparison, a flat Monte Carlo search AI is also provided. It performs
surprisingly well. The performance is on par with untuned, naive ISMCTS,
and due to the stochastic nature of the game will occasionally win against
the tuned ISMCTS imlementation.

There is also a heuristic based player which performs a simple strategy
based on maximizing score. This player can get lucky, but lacks strategic
depth to win consistently. It primarily serves to help benchmark the flat
MC and ISMCTS AIs.

There is a terminal based input mechanism for human players. It is quite
unweildy. The UI should be reimplemented to allow direct human input.
