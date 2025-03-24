// genetic_algorithm.h
#ifndef GENETIC_ALGORITHM_H
#define GENETIC_ALGORITHM_H

// Constants
#define POPULATION_SIZE 50      // Start modest for FPGA implementation
#define CHROMOSOME_LENGTH 128   // Adjust based on neural network size
#define TOURNAMENT_SIZE 3
#define CROSSOVER_RATE 0.7
#define MUTATION_RATE 0.05
#define MUTATION_STRENGTH 0.2

// Structures
typedef struct {
    float genes[CHROMOSOME_LENGTH];
    float fitness;
} Chromosome;

typedef struct {
    Chromosome individuals[POPULATION_SIZE];
    int generation;
} Population;

// Function declarations
void initializePopulation(Population* population);
void setFitness(Population* population, int index, float fitness);
int tournamentSelection(Population* population);
void crossover(Chromosome* parent1, Chromosome* parent2, Chromosome* offspring1, Chromosome* offspring2);
void mutate(Chromosome* chromosome);
void evolvePopulation(Population* population);
Chromosome getBestChromosome(Population* population);

#endif // GENETIC_ALGORITHM_H
