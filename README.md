# EvoSim — Genetic Algorithm Creature Evolver

**Computer Programming (1st Semester UG) — Semester Project**
Bahria University, H-11 Campus, Islamabad

A console-based **C++** application in which simple creatures evolve survival
strategies inside a 2D grid world using a **genetic algorithm (GA)**. Creatures
that collect more food and hit fewer hazards earn higher fitness, are selected
more often, and pass their genes to the next generation through crossover and
mutation. Over successive generations, random wanderers become efficient
foragers.

## Team

| Name | Registration # | GitHub |
|------|----------------|--------|
| Asad Jehan Zeb | _to be filled_ | [@zanni098](https://github.com/zanni098) |
| _Member 2_ | _to be filled_ | _to be filled_ |
| _Member 3_ | _to be filled_ | _to be filled_ |

## How it works

Each creature carries a **genome** — an array of 8 genes encoding a movement
rule table. The environment is reduced to a 2-bit *situation code* computed
every tick from sensor readings:

| Situation code | Meaning              | Genes used        |
|----------------|----------------------|-------------------|
| 0              | no food, no hazard   | `genes[0..1]`     |
| 1              | food near            | `genes[2..3]`     |
| 2              | hazard near          | `genes[4..5]`     |
| 3              | food AND hazard near | `genes[6..7]`     |

A *memory tick* alternates between the two genes of the active pair, so even
simple genomes produce search patterns (circles, zigzags) instead of getting
stuck vibrating in one cell. Gene values are directions: `0=up, 1=right,
2=down, 3=left`.

The GA loop:

1. **Initialize** a population of 60 creatures with random genomes.
2. **Evaluate fitness** — each creature lives 200 ticks on an identical fixed
   map: `fitness = foodEaten*10 − hazardHits*5 + survivalBonus*2`.
3. **Select** parents via tournament selection (size 3).
4. **Crossover** — uniform per-gene mix of two parents.
5. **Mutate** — each gene flips with 2% probability.
6. **Elitism** — the best 2 creatures are copied unchanged; repeat for 100
   generations.

The same pattern optimizes delivery routes, antenna shapes, and neural-network
weights in industry — the algorithm never sees the "correct" strategy, it only
measures outcomes.

## Requirements

Any C++17 compiler. Developed and tested with MSVC (Visual Studio 2022) and
`cl`; also builds with `g++`.

## Build & run

**Windows (MSVC):**

```bat
cl /EHsc /O2 evosim.cpp
```

**Windows / Linux (g++):**

```bash
g++ -O2 -o evosim evosim.cpp
```

**Run:**

```bash
./evosim          # default deterministic seed 42
./evosim 7        # custom RNG seed
```

The program prints per-generation statistics (best / average / worst fitness),
an ASCII fitness-history graph, the champion genome, saves the evolved
population to `population.dat`, and finishes with a live 200-tick demo of the
champion on a fresh map.

On the next run it automatically loads `population.dat`, so you can continue
evolving a saved population or demo a champion weeks later.

## Parameters

| Parameter        | Value | Why                                              |
|------------------|-------|--------------------------------------------------|
| Population size  | 60    | diversity vs. speed                              |
| Generations      | 100   | fitness curve visibly plateaus by then           |
| Genome length    | 8     | rule table; keeps crossover meaningful           |
| Ticks per life   | 200   | enough time to exhibit strategy                  |
| Mutation rate    | 2%    | balanced exploration vs. stability               |
| Elitism          | 2     | best fitness never decreases                     |
| Tournament size  | 3     | moderate selection pressure                      |
| Map size         | 15×31 | fits console, renders quickly                    |
| World contents   | 40 food, 12 hazards | scarcity creates selection pressure   |

## C++ concepts used

| Concept              | Where                                                        |
|----------------------|--------------------------------------------------------------|
| Structures           | `Creature`, `World`                                           |
| Arrays               | `genes[8]`, `grid[15][31]`, `historyBest/Avg[100]`, `population[60]` |
| Pointers             | `Creature*`, `World*` passed to simulate/selection functions; pointer arithmetic in `tournamentSelect` |
| Loops                | generation / creature / tick / gene loops                    |
| Conditionals         | situation sensing, gene-driven movement, collision & reward rules |
| Functions            | `initPopulation`, `buildWorld`, `senseSituation`, `simulate`, `evaluateFitness`, `tournamentSelect`, `crossover`, `mutate`, `renderWorld`, `printStats`, `savePopulation`, `loadPopulation` |
| File handling        | binary `ofstream`/`ifstream` save & load of the population   |

## Project structure

```
evosim-ga/
├── evosim.cpp      # complete implementation (single file)
├── population.dat  # saved evolved population (generated at runtime)
└── README.md
```

## Viva notes

- **Why not random search?** Random search keeps no memory; the GA concentrates
  search around what already works (selection + heredity) while mutation keeps
  exploring.
- **Why arrays?** Population, genome, and world grid are fixed-size collections;
  loops process every element uniformly, and the genome must be indexable by
  the situation code.
- **Why pointers?** `Creature`/`World` structs are large; pointers avoid
  copying whole structs and enable pointer-arithmetic traversal of the
  population.
- **Premature convergence?** Raise mutation slightly, increase population
  diversity, or reduce elitism — demonstrate live by changing `MUT_PCT`.
- **Is this AI?** It is a classic evolutionary-computation / metaheuristic
  technique — the same family used for route optimization and design search.

## License

Course project — all rights reserved by the team.
