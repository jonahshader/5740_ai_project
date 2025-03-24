// genetic_algorithm.cpp
#include "genetic_algorithm.h"
#include <stdlib.h>
#include <math.h>

// Helper function for random float between min and max
float randomFloat(float min, float max) {
    return min + (float)rand() / (float)RAND_MAX * (max - min);
}

// Initialize population with random values
void initializePopulation(Population* population) {
    population->generation = 0;

    for (int i = 0; i < POPULATION_SIZE; i++) {
        for (int j = 0; j < CHROMOSOME_LENGTH; j++) {
            population->individuals[i].genes[j] = randomFloat(-1.0f, 1.0f);
        }
        population->individuals[i].fitness = 0.0f;
    }
}

// Set fitness for a specific individual (based on game results)
void setFitness(Population* population, int index, float fitness) {
    if (index >= 0 && index < POPULATION_SIZE) {
        population->individuals[index].fitness = fitness;
    }
}

// Tournament selection
int tournamentSelection(Population* population) {
    int bestIndex = rand() % POPULATION_SIZE;
    float bestFitness = population->individuals[bestIndex].fitness;

    for (int i = 1; i < TOURNAMENT_SIZE; i++) {
        int index = rand() % POPULATION_SIZE;
        if (population->individuals[index].fitness > bestFitness) {
            bestIndex = index;
            bestFitness = population->individuals[index].fitness;
        }
    }

    return bestIndex;
}

// Uniform crossover
void crossover(Chromosome* parent1, Chromosome* parent2, Chromosome* offspring1, Chromosome* offspring2) {
    if (randomFloat(0.0f, 1.0f) < CROSSOVER_RATE) {
        // Perform crossover
        for (int i = 0; i < CHROMOSOME_LENGTH; i++) {
            if (rand() % 2 == 0) {
                offspring1->genes[i] = parent1->genes[i];
                offspring2->genes[i] = parent2->genes[i];
            } else {
                offspring1->genes[i] = parent2->genes[i];
                offspring2->genes[i] = parent1->genes[i];
            }
        }
    } else {
        // Just copy parents
        for (int i = 0; i < CHROMOSOME_LENGTH; i++) {
            offspring1->genes[i] = parent1->genes[i];
            offspring2->genes[i] = parent2->genes[i];
        }
    }
}

// Mutation with gaussian noise
void mutate(Chromosome* chromosome) {
    for (int i = 0; i < CHROMOSOME_LENGTH; i++) {
        if (randomFloat(0.0f, 1.0f) < MUTATION_RATE) {
            // Add gaussian noise
            float noise = randomFloat(-MUTATION_STRENGTH, MUTATION_STRENGTH);
            chromosome->genes[i] += noise;

            // Clamp to [-1, 1]
            if (chromosome->genes[i] > 1.0f) chromosome->genes[i] = 1.0f;
            if (chromosome->genes[i] < -1.0f) chromosome->genes[i] = -1.0f;
        }
    }
}

// Evolve population to next generation
void evolvePopulation(Population* population) {
    Chromosome newPopulation[POPULATION_SIZE];

    // Elitism: keep the best individual
    int bestIndex = 0;
    float bestFitness = population->individuals[0].fitness;

    for (int i = 1; i < POPULATION_SIZE; i++) {
        if (population->individuals[i].fitness > bestFitness) {
            bestIndex = i;
            bestFitness = population->individuals[i].fitness;
        }
    }

    // Copy best individual to new population
    for (int i = 0; i < CHROMOSOME_LENGTH; i++) {
        newPopulation[0].genes[i] = population->individuals[bestIndex].genes[i];
    }
    newPopulation[0].fitness = population->individuals[bestIndex].fitness;

    // Create rest of new population
    for (int i = 1; i < POPULATION_SIZE; i += 2) {
        int parent1Index = tournamentSelection(population);
        int parent2Index = tournamentSelection(population);

        // Make sure parents are different
        while (parent2Index == parent1Index) {
            parent2Index = tournamentSelection(population);
        }

        // Perform crossover and mutation
        crossover(&population->individuals[parent1Index],
                 &population->individuals[parent2Index],
                 &newPopulation[i],
                 &newPopulation[i + 1 < POPULATION_SIZE ? i + 1 : i]);

        mutate(&newPopulation[i]);
        if (i + 1 < POPULATION_SIZE) {
            mutate(&newPopulation[i + 1]);
        }
    }

    // Copy new population back
    for (int i = 0; i < POPULATION_SIZE; i++) {
        for (int j = 0; j < CHROMOSOME_LENGTH; j++) {
            population->individuals[i].genes[j] = newPopulation[i].genes[j];
        }
        population->individuals[i].fitness = 0.0f; // Reset fitness for next evaluation
    }

    population->generation++;
}

// Get the best chromosome from the population
Chromosome getBestChromosome(Population* population) {
    int bestIndex = 0;
    float bestFitness = population->individuals[0].fitness;

    for (int i = 1; i < POPULATION_SIZE; i++) {
        if (population->individuals[i].fitness > bestFitness) {
            bestIndex = i;
            bestFitness = population->individuals[i].fitness;
        }
    }

    return population->individuals[bestIndex];
}
