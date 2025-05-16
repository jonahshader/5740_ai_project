#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <unordered_set>
#include <set>
#include <climits>
#include <string>
#include <ctime>

class Route {
private:
    std::vector<int> route;

public:
    Route(int size) {
        route.resize(size, -1);
    }

    Route(const std::vector<int>& r) : route(r) {}

    void setCity(int index, int city) {
        route[index] = city;
    }

    int getCity(int index) const {
        return route[index];
    }

    const std::vector<int>& getRoute() const {
        return route;
    }

    void display() const {
        std::cout << "Route: ";
        for (int city : route) {
            std::cout << city << " ";
        }
        std::cout << std::endl;
    }
};

class Population {
private:
    std::vector<Route> routes;
    int tourSize;
    int populationSize;

public:
    Population(int popSize, int tSize, bool initialize) {
        populationSize = popSize;
        tourSize = tSize;
        routes.reserve(popSize);

        for (int i = 0; i < popSize; i++) {
            routes.push_back(Route(tourSize + 1));
        }

        if (initialize) {
            // Initialize with random routes
            for (int i = 0; i < popSize; i++) {
                routes[i] = createRandomRoute();
            }
        }
    }

    Route createRandomRoute() {
        Route newRoute(tourSize + 1);
        std::vector<int> cities(tourSize);

        // Initialize the cities
        for (int i = 0; i < tourSize; i++) {
            cities[i] = i + 1;
        }

        // Shuffle the cities (excluding the first and last, which are fixed to 1)
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(cities.begin() + 1, cities.end(), g);

        // Set the route
        newRoute.setCity(0, 1); // First city is always 1
        for (int i = 1; i < tourSize; i++) {
            newRoute.setCity(i, cities[i]);
        }
        newRoute.setCity(tourSize, 1); // Last city is always 1 (return to start)

        return newRoute;
    }

    void setRoute(int index, const std::vector<int>& route) {
        for (int i = 0; i < route.size(); i++) {
            routes[index].setCity(i, route[i]);
        }
    }

    Route getRoute(int index) const {
        return routes[index];
    }

    std::vector<int> getRouteVector(int index) const {
        return routes[index].getRoute();
    }

    int getSize() const {
        return routes.size();
    }
};

class GeneticAlgorithm {
private:
    Population population;
    int populationSize;
    int tourSize;
    double crossoverRate;
    std::vector<std::vector<int>> cityMap;
    std::string crossoverFcn;
    std::vector<int> parent1Arr;
    std::vector<int> parent2Arr;

    // Helper function to check if a value is in a vector
    bool contains(const std::vector<int>& vec, int value) const {
        return std::find(vec.begin(), vec.end(), value) != vec.end();
    }

    // Helper function to find the index of a value in a vector
    int indexOf(const std::vector<int>& vec, int value) const {
        auto it = std::find(vec.begin(), vec.end(), value);
        if (it != vec.end()) {
            return std::distance(vec.begin(), it);
        }
        return -1;
    }

public:
    GeneticAlgorithm(int popSize, int tSize, double crossRate,
                     const std::vector<std::vector<int>>& cityDistances,
                     const std::string& crossFcn)
        : population(popSize, tSize, true),
          populationSize(popSize),
          tourSize(tSize),
          crossoverRate(crossRate),
          cityMap(cityDistances),
          crossoverFcn(crossFcn) {

        // Initialize parent arrays
        parent1Arr.resize(popSize / 2, -1);
        parent2Arr.resize(popSize / 2, -1);

        // Select parents (in a real GA, this would be based on fitness selection)
        selectParents();
    }

    void selectParents() {
        // For demonstration, just randomly select parents
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(0, populationSize - 1);

        for (int i = 0; i < populationSize / 2; i++) {
            parent1Arr[i] = distrib(gen);
            parent2Arr[i] = distrib(gen);
        }
    }

    // Crossover population
    void crossover() {
        // Create a temporary population to hold children of current population
        Population childPop(populationSize, tourSize, false);
        // Each crossover returns two children, hold them in this temp array
        std::vector<std::vector<int>> children(2, std::vector<int>(tourSize + 1));

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dist(0.0, 1.0);

        if (crossoverFcn == "One-Point Crossover") {
            // Based on the crossover rate, create a new child or pass
            // a current member of the population forward
            for (int i = 0; i < populationSize; i += 2) {
                if (dist(gen) < crossoverRate) {
                    children = onePointCrossover(parent1Arr[i/2], parent2Arr[i/2]);
                    // Each set of parents should create two children
                    if (i == populationSize - 1 && i % 2 == 0) {
                        childPop.setRoute(i, children[0]);
                    } else {
                        for (int j = 0; j < 2; j++) {
                            childPop.setRoute(i + j, children[j]);
                        }
                    }
                } else {
                    if (i == populationSize - 1 && i % 2 == 0) {
                        childPop.setRoute(i, population.getRouteVector(parent1Arr[i/2]));
                    } else {
                        for (int j = 0; j < 2; j++) {
                            if (j == 0) {
                                childPop.setRoute(i + j, population.getRouteVector(parent1Arr[i/2]));
                            } else {
                                childPop.setRoute(i + j, population.getRouteVector(parent2Arr[i/2]));
                            }
                        }
                    }
                }
            }
            // Save the new population
            population = childPop;
        } else if (crossoverFcn == "Two-Point Crossover") {
            // Based on the crossover rate, create a new child or pass
            // a current member of the population forward
            for (int i = 0; i < populationSize; i += 2) {
                if (dist(gen) < crossoverRate) {
                    children = twoPointCrossover(parent1Arr[i/2], parent2Arr[i/2]);
                    // Each set of parents should create two children
                    if (i == populationSize - 1 && i % 2 == 0) {
                        childPop.setRoute(i, children[0]);
                    } else {
                        for (int j = 0; j < 2; j++) {
                            childPop.setRoute(i + j, children[j]);
                        }
                    }
                } else {
                    if (i == populationSize - 1 && i % 2 == 0) {
                        childPop.setRoute(i, population.getRouteVector(parent1Arr[i/2]));
                    } else {
                        for (int j = 0; j < 2; j++) {
                            if (j == 0) {
                                childPop.setRoute(i + j, population.getRouteVector(parent1Arr[i/2]));
                            } else {
                                childPop.setRoute(i + j, population.getRouteVector(parent2Arr[i/2]));
                            }
                        }
                    }
                }
            }
            // Save the new population
            population = childPop;
        } else if (crossoverFcn == "CX Crossover") {
            // Based on the crossover rate, create a new child or pass
            // a current member of the population forward
            for (int i = 0; i < populationSize; i += 2) {
                if (dist(gen) < crossoverRate) {
                    children = cxCrossover(parent1Arr[i/2], parent2Arr[i/2]);
                    if (i == populationSize - 1 && i % 2 == 0) {
                        childPop.setRoute(i, children[0]);
                    } else {
                        for (int j = 0; j < 2; j++) {
                            childPop.setRoute(i + j, children[j]);
                        }
                    }
                } else {
                    if (i == populationSize - 1 && i % 2 == 0) {
                        childPop.setRoute(i, population.getRouteVector(parent1Arr[i/2]));
                    } else {
                        for (int j = 0; j < 2; j++) {
                            if (j == 0) {
                                childPop.setRoute(i + j, population.getRouteVector(parent1Arr[i/2]));
                            } else {
                                childPop.setRoute(i + j, population.getRouteVector(parent2Arr[i/2]));
                            }
                        }
                    }
                }
            }
            // Save the new population
            population = childPop;
        } else if (crossoverFcn == "CX2 Crossover") {
            // Based on the crossover rate, create a new child or pass
            // a current member of the population forward
            for (int i = 0; i < populationSize; i += 2) {
                if (dist(gen) < crossoverRate) {
                    children = cx2Crossover(parent1Arr[i/2], parent2Arr[i/2]);
                    if (i == populationSize - 1 && i % 2 == 0) {
                        childPop.setRoute(i, children[0]);
                    } else {
                        for (int j = 0; j < 2; j++) {
                            childPop.setRoute(i + j, children[j]);
                        }
                    }
                } else {
                    if (i == populationSize - 1 && i % 2 == 0) {
                        childPop.setRoute(i, population.getRouteVector(parent1Arr[i/2]));
                    } else {
                        for (int j = 0; j < 2; j++) {
                            if (j == 0) {
                                childPop.setRoute(i + j, population.getRouteVector(parent1Arr[i/2]));
                            } else {
                                childPop.setRoute(i + j, population.getRouteVector(parent2Arr[i/2]));
                            }
                        }
                    }
                }
            }
            // Save the new population
            population = childPop;
        } else if (crossoverFcn == "Greedy Crossover") {
            // Based on the crossover rate, create a new child or pass
            // a current member of the population forward
            for (int i = 0; i < populationSize; i += 2) {
                if (dist(gen) < crossoverRate) {
                    children = greedyCrossover(parent1Arr[i/2], parent2Arr[i/2]);
                    // Each set of parents should create two children
                    if (i == populationSize - 1 && i % 2 == 0) {
                        childPop.setRoute(i, children[0]);
                    } else {
                        for (int j = 0; j < 2; j++) {
                            childPop.setRoute(i + j, children[j]);
                        }
                    }
                } else {
                    if (i == populationSize - 1 && i % 2 == 0) {
                        childPop.setRoute(i, population.getRouteVector(parent1Arr[i/2]));
                    } else {
                        for (int j = 0; j < 2; j++) {
                            if (j == 0) {
                                childPop.setRoute(i + j, population.getRouteVector(parent1Arr[i/2]));
                            } else {
                                childPop.setRoute(i + j, population.getRouteVector(parent2Arr[i/2]));
                            }
                        }
                    }
                }
            }
            // Save the new population
            population = childPop;
        } else if (crossoverFcn == "PMX Crossover") {
            // Based on the crossover rate, create a new child or pass
            // a current member of the population forward
            for (int i = 0; i < populationSize; i += 2) {
                if (dist(gen) < crossoverRate) {
                    children = pmxCrossover(parent1Arr[i/2], parent2Arr[i/2]);
                    // Each set of parents should create two children
                    if (i == populationSize - 1 && i % 2 == 0) {
                        childPop.setRoute(i, children[0]);
                    } else {
                        for (int j = 0; j < 2; j++) {
                            childPop.setRoute(i + j, children[j]);
                        }
                    }
                } else {
                    if (i == populationSize - 1 && i % 2 == 0) {
                        childPop.setRoute(i, population.getRouteVector(parent1Arr[i/2]));
                    } else {
                        for (int j = 0; j < 2; j++) {
                            if (j == 0) {
                                childPop.setRoute(i + j, population.getRouteVector(parent1Arr[i/2]));
                            } else {
                                childPop.setRoute(i + j, population.getRouteVector(parent2Arr[i/2]));
                            }
                        }
                    }
                }
            }
            // Save the new population
            population = childPop;
        } else if (crossoverFcn == "OX Crossover") {
            // Based on the crossover rate, create a new child or pass
            // a current member of the population forward
            for (int i = 0; i < populationSize; i += 2) {
                if (dist(gen) < crossoverRate) {
                    children = oxCrossover(parent1Arr[i/2], parent2Arr[i/2]);
                    // Each set of parents should create two children
                    if (i == populationSize - 1 && i % 2 == 0) {
                        childPop.setRoute(i, children[0]);
                    } else {
                        for (int j = 0; j < 2; j++) {
                            childPop.setRoute(i + j, children[j]);
                        }
                    }
                } else {
                    if (i == populationSize - 1 && i % 2 == 0) {
                        childPop.setRoute(i, population.getRouteVector(parent1Arr[i/2]));
                    } else {
                        for (int j = 0; j < 2; j++) {
                            if (j == 0) {
                                childPop.setRoute(i + j, population.getRouteVector(parent1Arr[i/2]));
                            } else {
                                childPop.setRoute(i + j, population.getRouteVector(parent2Arr[i/2]));
                            }
                        }
                    }
                }
            }
            // Save the new population
            population = childPop;
        } else if (crossoverFcn == "SCX Crossover") {
            // This one only produces one child so it's quirky
            // Based on the crossover rate, create a new child or pass
            // a current member of the population forward
            for (int i = 0; i < populationSize; i += 2) {
                if (dist(gen) < crossoverRate) {
                    // Each set of parents should create two children
                    if (i == populationSize - 1 && i % 2 == 0) {
                        std::vector<int> child = scxCrossover(parent1Arr[i/2], parent2Arr[i/2]);
                        childPop.setRoute(i, child);
                    } else {
                        std::vector<int> child = scxCrossover(parent1Arr[i/2], parent2Arr[i/2]);
                        childPop.setRoute(i, child);
                        if (i == populationSize - 2) {
                            child = scxCrossover(parent1Arr[i/2], parent2Arr[(i/2) - 1]);
                            childPop.setRoute(i + 1, child);
                        } else {
                            child = scxCrossover(parent1Arr[i/2], parent2Arr[(i/2) + 1]);
                            childPop.setRoute(i + 1, child);
                        }
                    }
                } else {
                    if (i == populationSize - 1 && i % 2 == 0) {
                        childPop.setRoute(i, population.getRouteVector(parent1Arr[i/2]));
                    } else {
                        for (int j = 0; j < 2; j++) {
                            if (j == 0) {
                                childPop.setRoute(i + j, population.getRouteVector(parent1Arr[i/2]));
                            } else {
                                childPop.setRoute(i + j, population.getRouteVector(parent2Arr[i/2]));
                            }
                        }
                    }
                }
            }
            // Save the new population
            population = childPop;
        } else {
            std::cout << "Invalid crossover function passed into GA" << std::endl;
        }
    }

    // Single child crossover for SCX
    std::vector<int> scxCrossover(int parent1Idx, int parent2Idx) {
        std::vector<int> parent1 = population.getRouteVector(parent1Idx);
        std::vector<int> parent2 = population.getRouteVector(parent2Idx);
        std::vector<int> child(tourSize + 1, -1);

        // First and last city must always be the starting city
        child[0] = 1;
        child[tourSize] = 1;
        int childIdx = 1;

        int p = parent1[0];
        int alpha;
        int beta;

        do {
            alpha = -1;
            beta = -1;
            for (int i = indexOf(parent1, p) + 1; i < tourSize + 1; i++) {
                if (!contains(child, parent1[i])) {
                    alpha = parent1[i];
                    break;
                }
            }
            if (alpha == -1) {
                for (int i = 1; i < tourSize + 1; i++) {
                    if (!contains(child, i)) {
                        alpha = i;
                        break;
                    }
                }
            }
            for (int i = indexOf(parent2, p) + 1; i < tourSize + 1; i++) {
                if (!contains(child, parent2[i])) {
                    beta = parent2[i];
                    break;
                }
            }
            if (beta == -1) {
                for (int i = 1; i < tourSize + 1; i++) {
                    if (!contains(child, i)) {
                        beta = i;
                        break;
                    }
                }
            }

            if (cityMap[p - 1][alpha - 1] < cityMap[p - 1][beta - 1]) {
                p = alpha;
                child[childIdx] = alpha;
            } else {
                p = beta;
            }
            child[childIdx] = p;
            childIdx++;
        } while (contains(child, -1));

        return child;
    }

    // Paper four Order Crossover
    std::vector<std::vector<int>> oxCrossover(int parent1Idx, int parent2Idx) {
        std::vector<int> parent1 = population.getRouteVector(parent1Idx);
        std::vector<int> parent2 = population.getRouteVector(parent2Idx);
        std::vector<std::vector<int>> children(2, std::vector<int>(tourSize + 1, -1));
        std::vector<int> subTour1;
        std::vector<int> subTour2;

        // Select the two points to crossover
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(1, tourSize - 1);
        int startPoint = dist(gen);
        int endPoint = dist(gen);

        // Ensure the start point is before the end point
        if (startPoint > endPoint) {
            std::swap(startPoint, endPoint);
        }

        // Children take the matching parents between the points
        for (int i = startPoint; i < endPoint; i++) {
            children[0][i] = parent1[i];
            children[1][i] = parent2[i];
        }

        // Create sub tours
        int idx = endPoint;
        do {
            if (!contains(children[0], parent2[idx])) {
                subTour1.push_back(parent2[idx]);
            }
            if (!contains(children[1], parent1[idx])) {
                subTour2.push_back(parent1[idx]);
            }
            idx++;
            if (idx == tourSize) {
                idx = 1;
            }
        } while (idx != endPoint);

        // Finish populating the children from the sub tours
        int subTourIdx = 0;
        do {
            children[0][idx] = subTour1[subTourIdx];
            children[1][idx] = subTour2[subTourIdx];
            subTourIdx++;
            idx++;
            if (idx == tourSize) {
                idx = 1;
            }
        } while (idx != startPoint);

        // First and last city must always be the starting city
        children[0][0] = 1;
        children[1][0] = 1;
        children[0][tourSize] = 1;
        children[1][tourSize] = 1;

        return children;
    }

    // Paper four partially-mapped crossover
    std::vector<std::vector<int>> pmxCrossover(int parent1Idx, int parent2Idx) {
        std::vector<int> parent1 = population.getRouteVector(parent1Idx);
        std::vector<int> parent2 = population.getRouteVector(parent2Idx);
        std::vector<std::vector<int>> children(2, std::vector<int>(tourSize + 1, -1));

        // Select the two points to crossover
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(0, tourSize - 1);
        int startPoint = dist(gen);
        int endPoint = dist(gen);

        // Ensure the start point is before the end point
        if (startPoint > endPoint) {
            std::swap(startPoint, endPoint);
        }

        // Children take the opposing parents between the points
        for (int i = startPoint; i < endPoint; i++) {
            children[0][i] = parent2[i];
            children[1][i] = parent1[i];
        }

        // If possible, take the matching parent outside of the two points
        for (int i = 0; i < tourSize; i++) {
            if (i < startPoint || i >= endPoint) {
                if (!contains(children[0], parent1[i])) {
                    children[0][i] = parent1[i];
                }
                if (!contains(children[1], parent2[i])) {
                    children[1][i] = parent2[i];
                }
            }
        }

        // Fill in the remaining indices by swapping the locations
        // of missing cities in parent 1 and parent 2.
        for (int i = 0; i < tourSize; i++) {
            if (children[0][i] == -1) {
                int index = i;
                int value = parent1[index];
                index = indexOf(parent2, value);
                while (true) {
                    value = parent1[index];
                    if (!contains(children[0], value)) {
                        children[0][i] = value;
                        break;
                    }
                    index = indexOf(parent2, value);
                }
            }
            if (children[1][i] == -1) {
                int index = i;
                int value = parent2[index];
                index = indexOf(parent1, value);
                while (true) {
                    value = parent2[index];
                    if (!contains(children[1], value)) {
                        children[1][i] = value;
                        break;
                    }
                    index = indexOf(parent1, value);
                }
            }
        }

        // Last city must always be the starting city
        children[0][tourSize] = 1;
        children[1][tourSize] = 1;

        return children;
    }

    std::vector<std::vector<int>> cxCrossover(int parent1Idx, int parent2Idx) {
        std::vector<int> parent1 = population.getRouteVector(parent1Idx);
        std::vector<int> parent2 = population.getRouteVector(parent2Idx);
        std::vector<std::vector<int>> children(2, std::vector<int>(tourSize + 1, -1));
        std::vector<int> cycle1;
        std::vector<int> values1;
        std::vector<int> cycle2;
        std::vector<int> values2;
        int startPos = 1;

        // Create the first cycle.
        cycle1.push_back(startPos);
        values1.push_back(parent1[startPos]);
        int value = parent2[startPos];
        values1.push_back(value);
        int index = indexOf(parent1, value);
        while (index != startPos) {
            cycle1.push_back(index);
            value = parent2[index];
            values1.push_back(value);
            index = indexOf(parent1, value);
        }

        // Create the second cycle.
        startPos = 1;
        cycle2.push_back(startPos);
        values2.push_back(parent2[startPos]);
        value = parent1[startPos];
        values2.push_back(value);
        index = indexOf(parent2, value);
        while (index != startPos) {
            cycle2.push_back(index);
            value = parent1[index];
            values2.push_back(value);
            index = indexOf(parent2, value);
        }

        // Populate the first child using the first cycle
        for (size_t i = 0; i < cycle1.size(); i++) {
            children[0][cycle1[i]] = values1[i];
        }

        // Populate the second child using the second child
        for (size_t i = 0; i < cycle2.size(); i++) {
            children[1][cycle2[i]] = values2[i];
        }

        // Fill in missing cities from opposing parent
        for (int i = 1; i < tourSize; i++) {
            if (children[0][i] == -1) {
                children[0][i] = parent2[i];
            }
            if (children[1][i] == -1) {
                children[1][i] = parent1[i];
            }
        }

        // First and last city must always be the starting city
        children[0][0] = 1;
        children[1][0] = 1;
        children[0][tourSize] = 1;
        children[1][tourSize] = 1;

        return children;
    }

    // Paper four crossover
    std::vector<std::vector<int>> cx2Crossover(int parent1Idx, int parent2Idx) {
        std::vector<int> parent1 = population.getRouteVector(parent1Idx);
        std::vector<int> parent2 = population.getRouteVector(parent2Idx);
        std::vector<std::vector<int>> children(2, std::vector<int>(tourSize + 1));
        std::vector<int> cycle1;
        std::vector<int> cycle2;

        // This crossover creates children by following
        // "cycles" of cities and their corresponding indices
        // in both parents. Essentially, you take a starting city
        // from parent2. You find that cities index in parent 1, then
        // look at the city in the corresponding index in parent 2.
        // Once again find this cities index in parent 1, then take the
        // corresponding city in parent 2. Through a repetition of this
        // (do once, take for child 1, do twice, take for child 2, repeat)
        // the children are created.
        int startPos = 0;
        while (true) {
            // Create the cycles.
            cycle1.push_back(startPos);
            int value = parent2[startPos];
            int index = indexOf(parent1, value);
            value = parent2[index];
            index = indexOf(parent1, value);
            cycle2.push_back(index);
            value = parent2[index];
            index = indexOf(parent1, value);
            int startIdx = -1;
            // Continue finding the cycle until it loops back on itself
            while (index != startPos && startIdx != index) {
                startIdx = index;
                cycle1.push_back(index);
                value = parent2[index];
                index = indexOf(parent1, value);
                value = parent2[index];
                index = indexOf(parent1, value);
                cycle2.push_back(index);
                value = parent2[index];
                index = indexOf(parent1, value);
            }

            // Check if the cycles are complete. If not,
            // find the starting point for the next cycle
            if (cycle1.size() == tourSize) {
                break;
            } else {
                std::set<int> remainingIndices;
                for (int i = 0; i < tourSize; i++) {
                    if (std::find(cycle1.begin(), cycle1.end(), i) == cycle1.end()) {
                        remainingIndices.insert(i);
                    }
                }
                if (!remainingIndices.empty()) {
                    startPos = *remainingIndices.begin();
                } else {
                    break; // No more indices to process
                }
            }
        }

        // Generate the children using the cycles
        for (size_t i = 0; i < tourSize; i++) {
            if (i < cycle1.size()) {
                int tempIdx = cycle1[i];
                children[0][i] = parent2[tempIdx];
            }
            if (i < cycle2.size()) {
                int tempIdx = cycle2[i];
                children[1][i] = parent2[tempIdx];
            }
        }

        // Last city must always be the starting city
        children[0][tourSize] = 1;
        children[1][tourSize] = 1;

        return children;
    }

    // Paper five
    std::vector<std::vector<int>> greedyCrossover(int parent1Idx, int parent2Idx) {
        std::vector<int> parent1 = population.getRouteVector(parent1Idx);
        std::vector<int> parent2 = population.getRouteVector(parent2Idx);
        std::vector<std::vector<int>> children(2, std::vector<int>(tourSize + 1));
        std::vector<int> illegal_cities(tourSize);
        int last_city;
        int shortestDistanceTracker; // covert to double after fix
        int temporary_best_index = 0;

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(1, tourSize - 1);
        // Do not crossover at the first or last index
        // as they must be the starting city
        int crossover = dist(gen);

        // Create the first child
        for (int i = 0; i < tourSize; i++) {
            // Before the crossover point, take the parent
            if (i < crossover) {
                children[0][i] = parent1[i];
            }
            // After the crossover point, greedily look for the nearest neighbor
            else {
                // setup
                last_city = children[0][i - 1] - 1;
                std::fill(illegal_cities.begin(), illegal_cities.end(), -1);
                shortestDistanceTracker = INT_MAX;

                // find the nearest neighbor.
                for (int j = 0; j < tourSize; j++) {
                    // any time that we finds a city that is already in the child, we skip it
                    if (!contains(children[0], j + 1)) {
                        if (cityMap[last_city][j] < shortestDistanceTracker) {
                            shortestDistanceTracker = cityMap[last_city][j];
                            temporary_best_index = j;
                        }
                    }
                }
                children[0][i] = temporary_best_index + 1;
            }
        }
        // Set the final index to the first city (untouched in for loop)
        children[0][tourSize] = 1;

        // Create the second child
        for (int i = 0; i < tourSize; i++) {
            // Before the crossover point, take the parent
            if (i < crossover) {
                children[1][i] = parent2[i];
            }
            // After the crossover point, greedily look for the nearest neighbor
            else {
                // setup
                last_city = children[1][i - 1] - 1;
                std::fill(illegal_cities.begin(), illegal_cities.end(), -1);
                shortestDistanceTracker = INT_MAX;

                // find the nearest neighbor.
                for (int j = 0; j < tourSize; j++) {
                    // any time that we finds a city that is already in the child, we skip it
                    if (!contains(children[1], j + 1)) {
                        if (cityMap[last_city][j] < shortestDistanceTracker) {
                            shortestDistanceTracker = cityMap[last_city][j];
                            temporary_best_index = j;
                        }
                    }
                }
                children[1][i] = temporary_best_index + 1;
            }
        }
        // Set the final index to the first city (untouched in for loop)
        children[1][tourSize] = 1;

        return children;
    }

    // Paper three
    std::vector<std::vector<int>> twoPointCrossover(int parent1Idx, int parent2Idx) {
        // Initialize the two child chromosomes
        std::vector<std::vector<int>> children(2, std::vector<int>(tourSize + 1, -1));
        std::vector<int> parent1 = population.getRouteVector(parent1Idx);
        std::vector<int> parent2 = population.getRouteVector(parent2Idx);

        // Select the two points to crossover
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(0, tourSize - 1);
        int startPoint = dist(gen);
        int endPoint = dist(gen);

        // Ensure the start point is before the end point
        if (startPoint > endPoint) {
            std::swap(startPoint, endPoint);
        }

        // Create the first child
        // Take parent one inside of the two crossover points
        for (int i = startPoint; i < endPoint; i++) {
            children[0][i] = parent1[i];
        }

        // Take parent two outside of the two crossover points if possible
        for (int i = 0; i < tourSize; i++) {
            if (children[0][i] == -1) {
                for (int city : parent2) {
                    if (!contains(children[0], city)) {
                        children[0][i] = city;
                        break;
                    }
                }
            }
        }
        // Final city must be the starting city
        children[0][tourSize] = 1;

        // Create the second child
        // Take parent two inside of the two crossover points
        for (int i = startPoint; i < endPoint; i++) {
            children[1][i] = parent2[i];
        }

        // Take parent one outside of the two crossover points if possible
        for (int i = 0; i < tourSize; i++) {
            if (children[1][i] == -1) {
                for (int city : parent1) {
                    if (!contains(children[1], city)) {
                        children[1][i] = city;
                        break;
                    }
                }
            }
        }
        // Final city must be the starting city
        children[1][tourSize] = 1;

        return children;
    }

    // One point crossover - basic implementation
    std::vector<std::vector<int>> onePointCrossover(int parent1Idx, int parent2Idx) {
        std::vector<int> parent1 = population.getRouteVector(parent1Idx);
        std::vector<int> parent2 = population.getRouteVector(parent2Idx);
        std::vector<std::vector<int>> children(2, std::vector<int>(tourSize + 1, -1));

        // Select a random crossover point, avoiding the first and last positions
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(1, tourSize - 1);
        int crossoverPoint = dist(gen);

        // Create first child
        // Copy from parent1 up to crossover point
        for (int i = 0; i <= crossoverPoint; i++) {
            children[0][i] = parent1[i];
        }

        // Fill in remaining cities from parent2
        int index = crossoverPoint + 1;
        for (int i = 0; i < tourSize + 1; i++) {
            if (index >= tourSize + 1) break;

            int city = parent2[i];
            if (!contains(children[0], city)) {
                children[0][index++] = city;
            }
        }

        // Create second child
        // Copy from parent2 up to crossover point
        for (int i = 0; i <= crossoverPoint; i++) {
            children[1][i] = parent2[i];
        }

        // Fill in remaining cities from parent1
        index = crossoverPoint + 1;
        for (int i = 0; i < tourSize + 1; i++) {
            if (index >= tourSize + 1) break;

            int city = parent1[i];
            if (!contains(children[1], city)) {
                children[1][index++] = city;
            }
        }

        // Ensure the last city is the same as the first (complete the tour)
        children[0][tourSize] = children[0][0];
        children[1][tourSize] = children[1][0];

        return children;
    }

    // Get population for testing
    const Population& getPopulation() const {
        return population;
    }

    void displayRoutes() const {
        std::cout << "Current Population Routes:" << std::endl;
        for (int i = 0; i < populationSize; i++) {
            std::cout << "Route " << i << ": ";
            for (int city : population.getRouteVector(i)) {
                std::cout << city << " ";
            }
            std::cout << std::endl;
        }
    }
};

// Test function to verify the crossover operations
void testCrossover(const std::string& crossoverType, int populationSize = 10, int tourSize = 5) {
    std::cout << "\nTesting " << crossoverType << " crossover..." << std::endl;

    // Create a city distance map for testing
    std::vector<std::vector<int>> cityMap(tourSize, std::vector<int>(tourSize, 0));

    // Fill with some test distances
    for (int i = 0; i < tourSize; i++) {
        for (int j = 0; j < tourSize; j++) {
            if (i != j) {
                cityMap[i][j] = (i + 1) * 10 + (j + 1); // Just a simple pattern for testing
            }
        }
    }

    // Create the GA with the specified crossover function
    GeneticAlgorithm ga(populationSize, tourSize, 0.8, cityMap, crossoverType);

    std::cout << "Before crossover:" << std::endl;
    ga.displayRoutes();

    // Perform crossover
    ga.crossover();

    std::cout << "\nAfter " << crossoverType << " crossover:" << std::endl;
    ga.displayRoutes();

    std::cout << "Verification:" << std::endl;
    // Verify that all routes are valid (contain each city exactly once, except for city 1 which is at start and end)
    for (int i = 0; i < populationSize; i++) {
        std::vector<int> route = ga.getPopulation().getRouteVector(i);

        // Check if first and last cities are 1
        if (route[0] != 1 || route[tourSize] != 1) {
            std::cout << "Error in route " << i << ": First and last cities must be 1" << std::endl;
            continue;
        }

        // Check if all cities are present exactly once (except city 1 which is at both ends)
        std::vector<int> cityCounts(tourSize + 1, 0);
        for (int j = 0; j < tourSize + 1; j++) {
            cityCounts[route[j]]++;
        }

        bool valid = true;
        for (int j = 1; j <= tourSize; j++) {
            if (j == 1 && cityCounts[j] != 2) {
                std::cout << "Error in route " << i << ": City 1 must appear exactly twice" << std::endl;
                valid = false;
                break;
            }
            if (j != 1 && cityCounts[j] != 1) {
                std::cout << "Error in route " << i << ": City " << j << " must appear exactly once" << std::endl;
                valid = false;
                break;
            }
        }

        if (valid) {
            std::cout << "Route " << i << " is valid" << std::endl;
        }
    }
}

int main() {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    int populationSize = 10;
    int tourSize = 5;

    // Test each crossover function
    testCrossover("One-Point Crossover", populationSize, tourSize);
    testCrossover("Two-Point Crossover", populationSize, tourSize);
    testCrossover("CX Crossover", populationSize, tourSize);
    testCrossover("CX2 Crossover", populationSize, tourSize);
    testCrossover("Greedy Crossover", populationSize, tourSize);
    testCrossover("PMX Crossover", populationSize, tourSize);
    testCrossover("OX Crossover", populationSize, tourSize);
    testCrossover("SCX Crossover", populationSize, tourSize);

    return 0;
}
