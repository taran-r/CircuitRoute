#include "benchmark.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <random>
#include <vector>

namespace {

const int REPEATS = 3;

struct DistanceRange {
    int low;
    int high;
};

struct Experiment {
    std::string name;
    int rows;
    int cols;
    int maxCost;
    int seeds;
    int pairsPerRange;
    std::vector<DistanceRange> ranges;
};

bool pickPair(const Board& board, std::mt19937& generator, DistanceRange range,
              Position& start, Position& target) {
    const std::vector<std::vector<char>>& grid = board.getGrid();
    std::uniform_int_distribution<int> randomRow(0, board.getRows() - 1);
    std::uniform_int_distribution<int> randomCol(0, board.getCols() - 1);
    std::uniform_int_distribution<int> randomDistance(range.low, range.high);

    for (int attempt = 0; attempt < 2000; attempt++) {
        start = {randomRow(generator), randomCol(generator)};
        if (grid[start.row][start.col] != '.') {
            continue;
        }
        int distance = randomDistance(generator);
        std::uniform_int_distribution<int> randomRowShift(-distance, distance);
        int rowShift = randomRowShift(generator);
        int colShift = distance - std::abs(rowShift);
        if (generator() % 2 == 0) {
            colShift = -colShift;
        }
        target = {start.row + rowShift, start.col + colShift};
        if (board.insideBoard(target.row, target.col) &&
            grid[target.row][target.col] == '.') {
            return true;
        }
    }
    return false;
}

double bestTimeMs(SearchFunction search, const RoutingGrid& grid, Position start,
                  Position target, SearchResult& result) {
    double best = 0;
    for (int repeat = 0; repeat < REPEATS; repeat++) {
        auto begin = std::chrono::steady_clock::now();
        result = search(grid, start, target);
        auto end = std::chrono::steady_clock::now();
        double milliseconds = std::chrono::duration<double, std::milli>(end - begin).count();
        if (repeat == 0 || milliseconds < best) {
            best = milliseconds;
        }
    }
    return best;
}

void writeRow(std::ofstream& file, const Experiment& experiment, const std::string& range,
              int seed, int pairId, int distance, const std::string& algorithm,
              const SearchResult& result, double milliseconds) {
    file << experiment.name << ',' << experiment.rows << ',' << experiment.maxCost << ','
         << range << ',' << seed << ',' << pairId << ',' << distance << ',' << algorithm << ','
         << (result.found ? 1 : 0) << ',' << milliseconds << ',' << result.nodesExpanded << ','
         << result.maxQueueSize << ',' << result.estimatedMemoryBytes << ',' << result.cost << '\n';
}

int runExperiment(std::ofstream& file, const Experiment& experiment, SearchFunction dijkstraSearch,
                  SearchFunction aStarSearch) {
    int mismatches = 0;
    int obstacles = experiment.rows * experiment.cols * 3 / 16;

    for (int seed = 1; seed <= experiment.seeds; seed++) {
        Board board(experiment.rows, experiment.cols);
        board.generateRandom(obstacles, 0, seed, experiment.maxCost);
        std::mt19937 generator(seed * 7919);

        int pairId = 0;
        std::vector<int> rangeOfPair;
        for (std::size_t r = 0; r < experiment.ranges.size(); r++) {
            for (int i = 0; i < experiment.pairsPerRange; i++) {
                Position start, target;
                if (pickPair(board, generator, experiment.ranges[r], start, target) &&
                    board.addConnection(pairId, start, target)) {
                    rangeOfPair.push_back(static_cast<int>(r));
                    pairId++;
                }
            }
        }

        for (const Connection& connection : board.getConnections()) {
            RoutingGrid grid = board.getRoutingGrid(connection.id);
            int distance = std::abs(connection.start.row - connection.target.row) +
                           std::abs(connection.start.col - connection.target.col);
            DistanceRange range = experiment.ranges[rangeOfPair[connection.id]];
            std::string label = std::to_string(range.low) + "-" + std::to_string(range.high);

            SearchResult dijkstraResult, aStarResult;
            double dijkstraTime = bestTimeMs(dijkstraSearch, grid, connection.start,
                                             connection.target, dijkstraResult);
            double aStarTime = bestTimeMs(aStarSearch, grid, connection.start,
                                          connection.target, aStarResult);
            if (dijkstraResult.found != aStarResult.found ||
                (dijkstraResult.found && dijkstraResult.cost != aStarResult.cost)) {
                mismatches++;
            }
            writeRow(file, experiment, label, seed, connection.id, distance, "dijkstra",
                     dijkstraResult, dijkstraTime);
            writeRow(file, experiment, label, seed, connection.id, distance, "astar",
                     aStarResult, aStarTime);
        }
        std::cout << "  " << experiment.name << " " << experiment.rows << "x" << experiment.cols
                  << ", board " << seed << "/" << experiment.seeds << " done\n";
    }
    return mismatches;
}

}

bool runBenchmark(const std::string& filename, SearchFunction dijkstraSearch,
                  SearchFunction aStarSearch) {
    if (dijkstraSearch == nullptr || aStarSearch == nullptr) {
        std::cout << "Both algorithms must be connected to run the benchmark.\n";
        return false;
    }
    std::ofstream file(filename);
    if (!file) {
        std::cout << "Could not open " << filename << ".\n";
        return false;
    }
    file.precision(9);
    file << "experiment,board_size,max_cost,distance_range,seed,pair_id,manhattan_distance,"
            "algorithm,found,time_ms,nodes_expanded,max_queue_size,memory_bytes,path_cost\n";

    std::vector<DistanceRange> ranges = {{5, 60}, {61, 150}, {151, 300}, {301, 600}};
    std::vector<Experiment> experiments = {
        {"distance_uniform", 400, 400, 1, 5, 8, ranges},
        {"distance_weighted", 400, 400, 9, 5, 8, ranges},
    };
    for (int size : {100, 200, 300, 400}) {
        experiments.push_back({"size", size, size, 1, 3, 12, {{size / 2, size}}});
    }

    std::cout << "Running the performance benchmark (each search is timed "
              << REPEATS << " times and the fastest is kept)...\n";
    int mismatches = 0;
    for (const Experiment& experiment : experiments) {
        mismatches += runExperiment(file, experiment, dijkstraSearch, aStarSearch);
    }
    file.flush();
    std::cout << "Saved " << filename << ".\n";
    std::cout << "Searches where the two algorithms disagreed on the path cost: "
              << mismatches << " (expected 0)\n";
    return file.good();
}
