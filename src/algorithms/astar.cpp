#include "astar.h"
#include "min_heap.h"
#include <cstdlib>
#include <limits>
#include <algorithm>

namespace {

constexpr double MIN_MOVEMENT_COST = 1.0;

double manhattanDistance(Position current, Position target) {
    int rowDistance = std::abs(current.row - target.row);
    int colDistance = std::abs(current.col - target.col);
    return (rowDistance + colDistance) * MIN_MOVEMENT_COST;
}

} 

AStarResult astar(const std::vector<std::vector<char>>& grid,
                  Position start, Position target) {
    AStarResult result{};
    result.cost = std::numeric_limits<double>::infinity();

    if (grid.empty() || grid[0].empty()) {
        return result;
    }

    int rows = static_cast<int>(grid.size());
    int cols = static_cast<int>(grid[0].size());

    for (int row = 0; row < rows; ++row) {
        if (grid[row].size() != grid[0].size()) {
            return result;
        }
    }

    if (start.row < 0 || start.row >= rows ||
        start.col < 0 || start.col >= cols ||
        target.row < 0 || target.row >= rows ||
        target.col < 0 || target.col >= cols) {
        return result;
    }

    if (grid[start.row][start.col] == '#' ||
        grid[target.row][target.col] == '#') {
        return result;
    }

    const int ROW_DELTA[4] = {-1, 1, 0, 0};
    const int COL_DELTA[4] = {0, 0, -1, 1};

    int startIndex = start.row * cols + start.col;
    int targetIndex = target.row * cols + target.col;
    int cellCount = rows * cols;

    std::vector<double> distance(cellCount, result.cost);
    std::vector<int> previous(cellCount, -1);
    MinHeap heap;

    distance[startIndex] = 0.0;
    heap.insert(manhattanDistance(start, target), startIndex);
    result.maxQueueSize = 1;

    while (!heap.empty()) {
        HeapEntry current = heap.extractMin();
        Position cell{current.vertex / cols, current.vertex % cols};

        double bestPriority = distance[current.vertex]
                            + manhattanDistance(cell, target);

        if (current.priority > bestPriority) {
            continue;
        }

        ++result.nodesExpand;

        if (current.vertex == targetIndex) {
            result.found = true;
            result.cost = distance[targetIndex];

            for (int index = targetIndex; index != -1; index = previous[index]) {
                result.path.push_back(Position{index / cols, index % cols});
            }
            std::reverse(result.path.begin(), result.path.end());
            return result;
        }

        for (int i = 0; i < 4; ++i) {
            int nextRow = cell.row + ROW_DELTA[i];
            int nextCol = cell.col + COL_DELTA[i];

            if (nextRow < 0 || nextRow >= rows ||
                nextCol < 0 || nextCol >= cols) {
                continue;
            }

            if (grid[nextRow][nextCol] == '#') {
                continue;
            }

            int nextIndex = nextRow * cols + nextCol;
            double newDistance = distance[current.vertex] + 1.0;

            if (newDistance < distance[nextIndex]) {
                distance[nextIndex] = newDistance;
                previous[nextIndex] = current.vertex;

                Position next{nextRow, nextCol};
                heap.insert(newDistance + manhattanDistance(next, target),
                            nextIndex);

                int queueSize = static_cast<int>(heap.size());
                if (queueSize > result.maxQueueSize) {
                    result.maxQueueSize = queueSize;
                }
            }
        }
    }

    return result;
}