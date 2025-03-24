// main.cpp
#include "genetic_algorithm.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Simple neural network for demonstration
// In a real implementation, this would be more complex
typedef struct {
    float* weights;
    int numWeights;
} NeuralNetwork;

// Initialize a neural network with weights from a chromosome
void initializeNetwork(NeuralNetwork* network, float* genes, int length) {
    network->weights = genes;
    network->numWeights = length;
}

// This would be your game function that uses the neural network
// Returns the scores after playing the game
void playGame(NeuralNetwork* network, int* player1_score, int* player2_score) {
    // Simulate a game result based on network weights
    // For demonstration, we'll just generate random scores
    // In your real implementation, this would run your actual game

    // Simple dummy implementation - weights sum influences score
    float weightSum = 0.0f;
    for (int i = 0; i < network->numWeights; i++) {
        weightSum += network->weights[i];
    }

    // Normalize to reasonable score range and add randomness
    *player1_score = (int)(15.0f + weightSum * 5.0f + (rand() % 5));
    *player2_score = (int)(15.0f + (rand() % 10)); // Opponent score
}

int main() {
    // Seed random number generator
    srand((unsigned int)time(NULL));

    // Initialize population
    Population population;
    initializePopulation(&population);

    // Run for a number of generations
    int numGenerations = 50;
    NeuralNetwork network;

    for (int gen = 0; gen < numGenerations; gen++) {
        printf("Generation %d:\n", gen);

        // Evaluate each individual by playing the game
        for (int i = 0; i < POPULATION_SIZE; i++) {
            // Setup neural network with chromosome weights
            initializeNetwork(&network, population.individuals[i].genes, CHROMOSOME_LENGTH);

            // Play the game to get scores
            int player1_score, player2_score;
            playGame(&network, &player1_score, &player2_score);

            // Set fitness based on score difference
            float fitness = (float)(player1_score - player2_score);
            setFitness(&population, i, fitness);

            // Optionally print progress for some individuals
            if (i % 10 == 0) {
                printf("  Individual %d: Score %d-%d, Fitness: %.2f\n",
                       i, player1_score, player2_score, fitness);
            }
        }

        // Print best individual of this generation
        Chromosome best = getBestChromosome(&population);
        printf("  Best fitness: %.2f\n", best.fitness);

        // Evolve to next generation
        evolvePopulation(&population);
    }

    return 0;
}
