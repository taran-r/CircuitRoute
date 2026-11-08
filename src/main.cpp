#include "pcb/board.h"
#include "algorithms/dijkstra.h"
#include "algorithms/astar.h"
#include <chrono>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>

using SearchFunction = SearchResult (*)(const RoutingGrid&, Position, Position);

SearchFunction dijkstraAlgorithm = dijkstra;
SearchFunction aStarAlgorithm = astar;

bool readNumber(const std::string& prompt, int minimum, int maximum, int& value) {
    while (true) {
        std::cout << prompt;
        if (std::cin >> value) {
            if (value >= minimum && value <= maximum) {
                return true;
            }
        } else if (std::cin.eof()) {
            return false;
        }
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Enter a whole number from " << minimum
                  << " to " << maximum << ".\n";
    }
}

void runSimulation(const Board& original, SearchFunction search,
                   const std::string& algorithmName) {
    if (search == nullptr) {
        std::cout << "This algorithm has not been connected yet.\n";
        std::cout << "Set aStarAlgorithm in main.cpp to enable it.\n";
        return;
    }

    Board workingBoard = original;
    if (!original.saveBoard(algorithmName + "_input.txt") ||
        !original.exportCells(algorithmName + "_input_cells.csv")) {
        std::cout << "Could not save the input dataset. Run cancelled.\n";
        return;
    }

    std::ofstream metrics(algorithmName + "_metrics.csv");
    std::ofstream paths(algorithmName + "_paths.csv");
    if (!metrics || !paths) {
        std::cout << "Could not open result files. Run cancelled.\n";
        return;
    }
    metrics << "algorithm,connection_id,source_row,source_col,target_row,target_col,"
            << "status,search_time_ms,nodes_expanded,max_queue_size,"
            << "estimated_search_memory_bytes,path_cost\n";
    paths << "connection_id,step,row,col\n";
    metrics.precision(12);

    int successful = 0;
    int invalidResults = 0;
    double totalSearchTime = 0;
    long long totalCost = 0;

    for (const Connection& connection : original.getConnections()) {
        RoutingGrid routingGrid = workingBoard.getRoutingGrid(connection.id);
        auto startTime = std::chrono::steady_clock::now();
        SearchResult result = search(routingGrid, connection.start, connection.target);
        auto endTime = std::chrono::steady_clock::now();
        double milliseconds =
            std::chrono::duration<double, std::milli>(endTime - startTime).count();
        totalSearchTime += milliseconds;

        std::string status = "no_path";
        long long cost = -1;
        if (result.found) {
            if (workingBoard.isValidRoute(connection.id, result.path)) {
                cost = workingBoard.getPathCost(result.path);
                if (result.cost != cost) {
                    status = "invalid_cost";
                    invalidResults++;
                    cost = -1;
                } else {
                    workingBoard.blockRoute(connection.id, result.path);
                    status = "success";
                    successful++;
                    totalCost += cost;
                    for (std::size_t step = 0; step < result.path.size(); step++) {
                        paths << connection.id << ',' << step << ','
                              << result.path[step].row << ','
                              << result.path[step].col << '\n';
                    }
                }
            } else {
                status = "invalid_path";
                invalidResults++;
            }
        } else if (!result.path.empty() || result.cost != -1) {
            status = "invalid_result";
            invalidResults++;
        }

        metrics << algorithmName << ',' << connection.id << ','
                << connection.start.row << ',' << connection.start.col << ','
                << connection.target.row << ',' << connection.target.col << ','
                << status << ',' << milliseconds << ',' << result.nodesExpanded
                << ',' << result.maxQueueSize << ',';

        if (result.estimatedMemoryBytes != 0) {
            metrics << result.estimatedMemoryBytes;
        }
        metrics << ',' << cost << '\n';
        std::cout << "Connection " << connection.id << ": " << status << '\n';
    }

    metrics.flush();
    paths.flush();
    bool cellsSaved = workingBoard.exportCells(algorithmName + "_routed_cells.csv");
    std::size_t attempted = original.getConnections().size();
    std::cout << "\nSuccessful: " << successful << '/' << attempted << '\n';
    std::cout << "Unsuccessful: " << attempted - successful << '\n';
    std::cout << "Invalid algorithm results: " << invalidResults << '\n';
    std::cout << "Total search time: " << totalSearchTime << " ms\n";
    std::cout << "Total successful path cost: " << totalCost << '\n';
    std::cout << "Completion rate: "
              << (attempted == 0 ? 0.0 : 100.0 * successful / attempted)
              << "%\n";
    if (!metrics.good() || !paths.good() || !cellsSaved) {
        std::cout << "Warning: some result files could not be fully written.\n";
    } else {
        std::cout << "Saved input, metrics, paths, and routed cells for "
                  << algorithmName << ".\n";
    }
}

void runCollisionDemo() {
    Board demo(10, 10);
    demo.addAnObstacle(2, 2);
    demo.addAnObstacle(2, 3);
    demo.addConnection(0, {4, 1}, {4, 8});
    demo.addConnection(1, {1, 5}, {7, 5});
    std::vector<Position> firstPath;
    for (int c = 1; c <= 8; c++) {
        firstPath.push_back({4, c});
    }
    std::vector<Position> crossingPath;
    for (int r = 1; r <= 7; r++) {
        crossingPath.push_back({r, 5});
    }
    std::cout << "First wire accepted (expected 1): "
              << demo.blockRoute(0, firstPath) << '\n';
    std::cout << "Crossing wire accepted (expected 0): "
              << demo.blockRoute(1, crossingPath) << '\n';
    demo.print();
    std::cout << "This demonstration does not change your current dataset.\n";
}

int main() {
    Board board(10, 10);
    board.generateRandom(15, 3, 42);

    while (true) {
        std::cout << "\nCircuitRoute - " << board.getRows() << " x "
                  << board.getCols() << ", " << board.getConnections().size()
                  << " connections\n"
                  << "1. Generate a board\n"
                  << "2. Print board and connection pairs\n"
                  << "3. Save input board and cell CSV\n"
                  << "4. Load saved input board\n"
                  << "5. Run Dijkstra\n"
                  << "6. Run A*\n"
                  << "7. Run manual collision demonstration\n"
                  << "0. Exit\n";
        int choice;
        if (!readNumber("Choice: ", 0, 7, choice) || choice == 0) {
            break;
        }
        if (choice == 1) {
            int row, col, obstacles, wires, seed, maximumCost;
            if (!readNumber("Rows (project: 400): ", 1, 1000, row) ||
                !readNumber("Columns (project: 400): ", 1, 1000, col) ||
                !readNumber("Obstacle cells (project: 30000): ", 0, 1000000, obstacles) ||
                !readNumber("Connections (project: 20-50): ", 0, 500000, wires) ||
                !readNumber("Seed (example: 42): ", 0, 2000000000, seed) ||
                !readNumber("Maximum cell cost (1 = uniform): ", 1, 1000, maximumCost)) {
                break;
            }
            Board replacement(row, col);
            if (replacement.generateRandom(obstacles, wires,
                                            static_cast<unsigned int>(seed), maximumCost)) {
                board = replacement;
                std::cout << "Generated. Non-obstacle cells: " << row * col - obstacles
                          << '\n';
            } else {
                std::cout << "Not enough room. Previous board kept.\n";
            }
        } else if (choice == 2) {
            if (board.getRows() * board.getCols() <= 1600) {
                board.print();
                std::cout << ". empty, # obstacle, S start, T target, * wire\n";
            } else {
                std::cout << "Board too large for terminal display. Save the CSV instead.\n";
            }
            for (const Connection& connection : board.getConnections()) {
                std::cout << "ID " << connection.id << ": ("
                          << connection.start.row << ',' << connection.start.col
                          << ") -> (" << connection.target.row << ','
                          << connection.target.col << ")\n";
            }
        } else if (choice == 3) {
            bool boardSaved = board.saveBoard("dataset.txt");
            bool cellsSaved = board.exportCells("dataset_cells.csv");
            std::cout << (boardSaved && cellsSaved
                ? "Saved dataset.txt and dataset_cells.csv.\n"
                : "Could not save all files.\n");
        } else if (choice == 4) {
            std::cout << (board.loadBoard("dataset.txt")
                ? "Loaded dataset.txt.\n"
                : "Load failed. Previous board kept.\n");
        } else if (choice == 5) {
            runSimulation(board, dijkstraAlgorithm, "dijkstra");
        } else if (choice == 6) {
            runSimulation(board, aStarAlgorithm, "astar");
        } else if (choice == 7) {
            runCollisionDemo();
        }
    }
    return 0;
}
