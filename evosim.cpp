// ============================================================
// EvoSim - Genetic Algorithm Creature Evolver
// Computer Programming (1st Semester UG) - Semester Project
//
// A console-based C++ application in which simple creatures
// evolve survival strategies inside a 2D grid world using a
// genetic algorithm (GA).
//
// Concepts exercised: structures, arrays, pointers, loops,
// conditional statements, user-defined functions, file handling.
// ============================================================

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <algorithm>

// -------------------- Parameters --------------------
const int POP_SIZE   = 60;    // population size
const int GENS       = 100;   // generations
const int TICKS      = 200;   // ticks per life
const int GENES_LEN  = 8;     // genome length (rule table)
const int ROWS       = 15;    // world grid rows (incl. border)
const int COLS       = 31;    // world grid cols (incl. border)
const int FOOD_COUNT = 40;
const int HAZ_COUNT  = 12;
const int TOURN_SIZE = 3;     // tournament selection size
const int ELITE      = 2;     // number of elites copied unchanged
const int MUT_PCT    = 2;     // mutation rate, percent per gene
const char* POP_FILE = "population.dat";

// Direction codes stored in genes
const int DIR_UP    = 0;
const int DIR_RIGHT = 1;
const int DIR_DOWN  = 2;
const int DIR_LEFT  = 3;

// -------------------- Structures --------------------
struct Creature {
    int genes[GENES_LEN];   // genome: 8 movement rules
    int fitness;
    int foodEaten;
    int hazardHits;
    int survivalBonus;      // ticks survived (used in fitness)
    int x, y;               // position
    int memTick;            // alternates 0/1 to pick gene pair member
    int id;
};

struct World {
    char grid[ROWS][COLS];  // '.' empty, 'f' food, '^' hazard, '#' wall
    int foodLeft;
};

// -------------------- Globals --------------------
Creature population[POP_SIZE];
Creature nextPop[POP_SIZE];
float historyBest[GENS];
float historyAvg[GENS];

static const int DX[4] = { -1, 0, 1, 0 };   // up, right, down, left
static const int DY[4] = { 0, 1, 0, -1 };

// -------------------- Functions --------------------

void initRng(unsigned seed) {
    srand(seed);
}

int randInt(int n) {                 // 0..n-1
    return rand() % n;
}

// Build a fresh world: border walls + scattered food/hazards.
void buildWorld(World* w) {
    memset(w->grid, '.', sizeof(w->grid));
    w->foodLeft = 0;
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++)
            if (r == 0 || c == 0 || r == ROWS - 1 || c == COLS - 1)
                w->grid[r][c] = '#';

    int placed = 0;
    while (placed < FOOD_COUNT) {   // food
        int r = 1 + randInt(ROWS - 2), c = 1 + randInt(COLS - 2);
        if (w->grid[r][c] == '.') { w->grid[r][c] = 'f'; w->foodLeft++; placed++; }
    }
    placed = 0;
    while (placed < HAZ_COUNT) {    // hazards
        int r = 1 + randInt(ROWS - 2), c = 1 + randInt(COLS - 2);
        if (w->grid[r][c] == '.') { w->grid[r][c] = '^'; placed++; }
    }
}

// Random starting position inside the walls.
void randomSpawn(Creature* c) {
    do {
        c->x = 1 + randInt(ROWS - 2);
        c->y = 1 + randInt(COLS - 2);
    } while (false);   // walls are only on the border; any inner cell is free
}

// Fill a creature with random genes and reset its stats.
void initCreature(Creature* c, int id) {
    for (int g = 0; g < GENES_LEN; g++)
        c->genes[g] = randInt(4);
    c->fitness = 0;
    c->foodEaten = 0;
    c->hazardHits = 0;
    c->survivalBonus = 0;
    c->memTick = 0;
    c->id = id;
    randomSpawn(c);
}

void initPopulation(Creature* pop) {
    for (int i = 0; i < POP_SIZE; i++)
        initCreature(&pop[i], i);
}

// Sense the local environment and return the 2-bit situation code:
//   bit0 = food near, bit1 = hazard near  (0..3)
// The genes literally ARE this conditional logic.
int senseSituation(Creature* c, World* w) {
    int foodNear = 0, hazardNear = 0;
    for (int r = c->x - 1; r <= c->x + 1 && !(foodNear && hazardNear); r++) {
        for (int col = c->y - 1; col <= c->y + 1; col++) {
            if (r < 0 || r >= ROWS || col < 0 || col >= COLS) continue;
            if (w->grid[r][col] == 'f') foodNear = 1;
            if (w->grid[r][col] == '^') hazardNear = 1;
        }
    }
    return (foodNear ? 1 : 0) | (hazardNear ? 2 : 0);
}

// Move one tick: pick the gene for the active situation pair and
// the current memory tick, then apply collision / reward rules.
void simulateTick(Creature* c, World* w) {
    int sit = senseSituation(c, w);
    int move = c->genes[sit * 2 + (c->memTick & 1)];   // genes[0..7]

    int nx = c->x + DX[move];
    int ny = c->y + DY[move];

    // clamp into playable area (never read outside the grid)
    if (nx < 1) nx = 1; if (nx > ROWS - 2) nx = ROWS - 2;
    if (ny < 1) ny = 1; if (ny > COLS - 2) ny = COLS - 2;

    char cell = w->grid[nx][ny];
    if (cell == '^') {
        c->hazardHits++;          // damage
    } else {
        c->x = nx; c->y = ny;     // move
        if (cell == 'f') {
            c->foodEaten++;       // reward
            w->grid[nx][ny] = '.';
            w->foodLeft--;
            // respawn food somewhere empty to keep selection pressure
            int tries = 0;
            while (tries++ < 200) {
                int r = 1 + randInt(ROWS - 2), col = 1 + randInt(COLS - 2);
                if (w->grid[r][col] == '.') {
                    w->grid[r][col] = 'f'; w->foodLeft++; break;
                }
            }
        }
    }
    c->memTick = 1 - (c->memTick & 1);   // toggle memory tick
}

// Run one creature's whole life on its own copy of the world.
void simulate(Creature* c, World* w) {
    World local = *w;               // identical evaluation for everyone
    c->foodEaten = 0;
    c->hazardHits = 0;
    c->survivalBonus = 0;
    randomSpawn(c);
    for (int t = 0; t < TICKS; t++) {
        simulateTick(c, &local);
        c->survivalBonus = t + 1;
    }
}

// fitness = (foodEaten * 10) - (hazardHits * 5) + (survivalBonus * 2)
void evaluateFitness(Creature* c, World* w) {
    simulate(c, w);
    c->fitness = c->foodEaten * 10 - c->hazardHits * 5 + c->survivalBonus * 2;
}

// Tournament selection: pick TOURN_SIZE at random, keep the fittest.
Creature* tournamentSelect(Creature* pop) {
    Creature* best = nullptr;
    for (int i = 0; i < TOURN_SIZE; i++) {
        Creature* pick = pop + randInt(POP_SIZE);   // pointer arithmetic
        if (!best || pick->fitness > best->fitness)
            best = pick;
    }
    return best;
}

// Uniform crossover: each gene comes from parent A or parent B.
void crossover(const Creature* a, const Creature* b, Creature* child) {
    child->fitness = 0;
    child->foodEaten = child->hazardHits = child->survivalBonus = 0;
    child->memTick = 0;
    for (int g = 0; g < GENES_LEN; g++)
        child->genes[g] = (rand() & 1) ? a->genes[g] : b->genes[g];
    randomSpawn(child);
}

// Each gene mutates with MUT_PCT percent chance into 0..3.
void mutate(Creature* c) {
    for (int g = 0; g < GENES_LEN; g++)
        if (randInt(100) < MUT_PCT)
            c->genes[g] = randInt(4);
}

// Render the world and one creature on top of it.
void renderWorld(const World* w, const Creature* c) {
    for (int r = 0; r < ROWS; r++) {
        for (int col = 0; col < COLS; col++) {
            if (c && r == c->x && col == c->y) {
                std::cout << '@';
            } else {
                std::cout << w->grid[r][col];
            }
        }
        std::cout << '\n';
    }
}

// One-line generational statistics.
void printStats(int gen, const Creature* pop) {
    int best = pop[0].fitness, worst = pop[0].fitness, sum = 0;
    const Creature* champ = &pop[0];
    for (int i = 0; i < POP_SIZE; i++) {
        sum += pop[i].fitness;
        if (pop[i].fitness > best)  { best = pop[i].fitness; champ = &pop[i]; }
        if (pop[i].fitness < worst) worst = pop[i].fitness;
    }
    historyBest[gen] = (float)best;
    historyAvg[gen]  = (float)sum / POP_SIZE;
    std::cout << "Gen " << gen
              << " | best " << best
              << " | avg " << (int)(sum / POP_SIZE)
              << " | worst " << worst
              << " | champ#" << champ->id
              << " food=" << champ->foodEaten
              << " hits=" << champ->hazardHits
              << "\n";
}

// ASCII fitness history graph (prints best vs avg per generation).
void printFitnessGraph() {
    std::cout << "\n=== Fitness History (best vs average) ===\n";
    int maxV = 1;
    for (int g = 0; g < GENS; g++)
        maxV = std::max(maxV, std::max((int)historyBest[g], (int)historyAvg[g]));
    for (int g = 0; g < GENS; g += 10) {
        int b = (int)((historyBest[g] * 50.0f) / maxV);
        int a = (int)((historyAvg[g]  * 50.0f) / maxV);
        std::cout << "Gen " << g << " |";
        for (int i = 0; i < 50; i++) {
            if (i < a && i < b)      std::cout << '#';   // both
            else if (i < b)          std::cout << 'B';   // best only
            else if (i < a)          std::cout << 'a';   // avg only
            else                     std::cout << ' ';
        }
        std::cout << "| " << (int)historyBest[g] << "\n";
    }
}

// -------------------- File handling (extra credit) --------------------

bool savePopulation(const Creature* pop, const char* path) {
    std::ofstream out(path, std::ios::binary);
    if (!out) return false;
    out.write((char*)pop, sizeof(Creature) * POP_SIZE);
    return out.good();
}

bool loadPopulation(Creature* pop, const char* path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    in.read((char*)pop, sizeof(Creature) * POP_SIZE);
    return in.good() && in.gcount() == (std::streamsize)(sizeof(Creature) * POP_SIZE);
}

// -------------------- Main --------------------

int main(int argc, char** argv) {
    // Optional args: evosim [seed]   (deterministic demo seed = 42)
    unsigned seed = 42;
    if (argc > 1) seed = (unsigned)atoi(argv[1]);
    initRng(seed);

    std::cout << "============================================\n"
              << " EvoSim - Genetic Algorithm Creature Evolver\n"
              << " pop=" << POP_SIZE << " gens=" << GENS
              << " ticks=" << TICKS << " genome=" << GENES_LEN
              << " mutation=" << MUT_PCT << "%\n"
              << " seed=" << seed << "\n"
              << "============================================\n\n";

    // Load saved population if present, else initialise randomly.
    if (!loadPopulation(population, POP_FILE)) {
        std::cout << "No saved population - starting random.\n\n";
        initPopulation(population);
    } else {
        std::cout << "Loaded evolved population from " << POP_FILE << ".\n\n";
    }

    // ---------- Evolution loop ----------
    for (int gen = 0; gen < GENS; gen++) {
        World w;
        buildWorld(&w);                 // fixed map for this generation

        // Evaluate every creature on an identical world (fairness).
        for (int i = 0; i < POP_SIZE; i++) {
            population[i].id = i;
            evaluateFitness(&population[i], &w);
        }
        printStats(gen, population);

        // Elitism: best ELITE survive unchanged.
        Creature* sorted = population;   // find elites by fitness
        for (int e = 0; e < ELITE; e++) {
            int bestIdx = 0;
            for (int i = 1; i < POP_SIZE; i++)
                if (sorted[i].fitness > sorted[bestIdx].fitness)
                    bestIdx = i;
            nextPop[e] = sorted[bestIdx];
            sorted[bestIdx].fitness = -1000000;   // mark as taken
            // restore real fitness afterwards is unnecessary; pop is
            // rebuilt from nextPop below, but elites must keep real
            // fitness for selection - re-evaluate cheaply instead.
        }

        // Re-evaluate population fitness is costly; instead keep a copy.
        // (Elites were mutated above only in the working array.)

        // Fill the rest by selection + crossover + mutation.
        for (int i = ELITE; i < POP_SIZE; i += 2) {
            Creature* a = tournamentSelect(population);
            Creature* b = tournamentSelect(population);
            crossover(a, b, &nextPop[i]);
            nextPop[i].id = i;
            mutate(&nextPop[i]);
            if (i + 1 < POP_SIZE) {
                Creature* c = tournamentSelect(population);
                Creature* d = tournamentSelect(population);
                crossover(c, d, &nextPop[i + 1]);
                nextPop[i + 1].id = i + 1;
                mutate(&nextPop[i + 1]);
            }
        }

        memcpy(population, nextPop, sizeof(population));

        if ((gen + 1) % 10 == 0)
            std::cout << "--- generation " << (gen + 1) << " checkpoint ---\n";
    }

    printFitnessGraph();

    // ---------- Champion demo ----------
    int champIdx = 0;
    for (int i = 1; i < POP_SIZE; i++)
        if (population[i].fitness > population[champIdx].fitness)
            champIdx = i;
    Creature champ = population[champIdx];

    std::cout << "\nChampion genome: {";
    for (int g = 0; g < GENES_LEN; g++)
        std::cout << champ.genes[g] << (g + 1 < GENES_LEN ? ", " : "");
    std::cout << "}  fitness=" << champ.fitness
              << " food=" << champ.foodEaten
              << " hits=" << champ.hazardHits << "\n";

    // Save evolved population for next run / demo.
    if (savePopulation(population, POP_FILE))
        std::cout << "Saved population to " << POP_FILE << "\n";
    else
        std::cout << "Warning: could not save " << POP_FILE << "\n";

    // Live demo of the champion on a fresh map (deterministic seed).
    initRng(42);
    World demo;
    buildWorld(&demo);
    std::cout << "\n=== Champion live demo (200 ticks) ===\n";
    for (int t = 0; t < TICKS; t++) {
        if (t % 20 == 0) {
            system("cls");   // Windows; use "clear" on Linux/Mac
            std::cout << "Tick " << t
                      << "  food=" << champ.foodEaten
                      << "  hits=" << champ.hazardHits
                      << "  fitness=" << champ.fitness << "\n";
            renderWorld(&demo, &champ);
        }
        simulateTick(&champ, &demo);
    }
    std::cout << "\nFinal: food=" << champ.foodEaten
              << " hits=" << champ.hazardHits << "\n";

    return 0;
}
