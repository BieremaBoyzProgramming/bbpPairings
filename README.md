This is a fork of [BieremaBoyzProgramming/bbpPairings](https://github.com/BieremaBoyzProgramming/bbpPairings) that adds
a new swiss system, --fast, that is an alternative to --dutch/--burstein.

## Fast Swiss

### Motivation

The goal of the Fast Swiss system is to create a swiss system that can pair tens of thousands of players in under a
second. This will be used by [Lichess](https://github.com/ornicar/lila) instead of Dutch/Burstein for swiss tournaments
with many players. 

While Dutch and Burstein operate as graph optimization problems (and are correspondingly slow), the Fast Swiss algorithm
operates linearly, finding the best pairing for each player from top to bottom. This means that pairings will not be
ideal for all players, but they should be good enough to provide a fair tournament experience. 

### Performance

`n` is the number of players, `r` the number of previous rounds

Time complexity comparison:
- Dutch: O(n<sup>3</sup> * s * (d + s) * log n)
- Burstein: O(n<sup>3</sup>)
- Fast: O(n log n + nr + r<sup>3</sup>)

Assuming `r ~ log n`:
- Dutch: O(n<sup>3</sup> * (log n)<sup>3</sup>)
- Burstein: O(n<sup>3</sup>)
- Fast: O(n log n)

Real world performance:
- TODO: Add some tables with actual measurements

### Algorithm

Note: The Fast Swiss algorithm is based on Burstein, and where not specified, operates the same (e.g. for byes,
acceleration).

1. For each player `i`, from `1` to `n`:
    1. If player `i` is already paired, continue
    2. Find the optimal pairing `j` for player `i`:
        1. In several passes:
            1. Players in the same score group with an optimal color match
            2. Players in the same score group with an allowable color match
            3. Players outside the score group with an optimal color match
            4. Players outside the score group with an allowable color match
            5. Players inside the score group ignoring colors
            6. Players outside the score group ignoring colors
        3. For each pass, look for a player `j` that player `i` has not already played
        4. If `10` possible pairings have been discarded due only to color mismatches, advance to the next pass
        5. If a pairing is found, advance to the next player 
        6. If no such pairing is found, advance to step 2
2. While `k > 0` players are unpaired:
    1. Backtrack and unpair the last `k` players paired, so there are now `k' = 2k` players unpaired
    2. Attempt to do an optimal pairing for the remaining `k'` players as in Burstein (but without absolute color constraints)

- Pre-processing (sorting players) takes O(n log n) time.
- Step 1 takes O(nr) time.
- Step 2 takes O(r<sup>3</sup>) time.
    - Proof outline:
        - The initial value of `k` is at most `r` since we only fail to find a pairing when player `i` has already played every unpaired player
        - If `k >= 2r`, we are guaranteed to find a pairing
            - The graph of possible pairings has minimum degree `k - r >= k / 2`
            - That [implies](https://math.stackexchange.com/questions/421496/hamilton-path-and-minimum-degree) a Hamiltonian path exists
            - We can select alternating edges from the Hamiltonian path to find a perfect matching, i.e. a valid set of pairings
        - The total time of all iterations O(k<sup>3</sup> + (k/2)<sup>3</sup> + (k/4)<sup>3</sup> ...) = O(k<sup>3</sup>) = O(r<sup>3</sup>)