#include "astar.h"
#include "min_heap.h"
#include <algorithm>
#include <cstdlib>
#include <limits>
#include <vector>

namespace {

const int ROW_DELTA[4] = {-1, 1, 0, 0};
const int COL_DELTA[4] = {0, 0, -1, 1};
constexpr double MIN_MOVEMENT_COST = 1.0;

bool inBounds(Position cell, int rows, int cols) {
    return cell.row >= 0 && cell.row < rows &&
           cell.col >= 0 && cell.col < cols;
}

double manhattanDistance(Position current, Position target) {
    int rowDistance = std::abs(current.row - target.row);
    int colDistance = std::abs(current.col - target.col);
    return (rowDistance + colDistance) * MIN_MOVEMENT_COST;
}

}

SearchResult astar(const RoutingGrid& grid, Position start, Position target) {
    SearchResult result{};

    if (grid.empty() || grid[0].empty()) {
        return result;
    }

    int rows = static_cast<int>(grid.size());
    int cols = static_cast<int>(grid[0].size());

    for (const auto& row : grid) {
        if (row.size() != grid[0].size()) {
            return result;
        }
    }

    if (!inBounds(start, rows, cols) || !inBounds(target, rows, cols)) {
        return result;
    }

    if (grid[start.row][start.col].symbol == '#' ||
        grid[target.row][target.col].symbol == '#') {
        return result;
    }

    int startIndex = start.row * cols + start.col;
    int targetIndex = target.row * cols + target.col;
    int cellCount = rows * cols;

    const double infinity = std::numeric_limits<double>::infinity();
    std::vector<double> distance(cellCount, infinity);
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

        ++result.nodesExpanded;

        if (current.vertex == targetIndex) {
            result.found = true;
            result.cost = static_cast<long long>(distance[targetIndex]);

            for (int index = targetIndex; index != -1; index = previous[index]) {
                result.path.push_back(Position{index / cols, index % cols});
            }
            std::reverse(result.path.begin(), result.path.end());
            return result;
        }

        for (int i = 0; i < 4; ++i) {
            Position next{cell.row + ROW_DELTA[i], cell.col + COL_DELTA[i]};
            if (!inBounds(next, rows, cols)) {
                continue;
            }

            const RoutingCell& nextCell = grid[next.row][next.col];
            if (nextCell.symbol == '#') {
                continue;
            }

            int nextIndex = next.row * cols + next.col;
            double newDistance = distance[current.vertex] + nextCell.movementCost;

            if (newDistance < distance[nextIndex]) {
                distance[nextIndex] = newDistance;
                previous[nextIndex] = current.vertex;
                heap.insert(newDistance + manhattanDistance(next, target),
                            nextIndex);

                if (heap.size() > result.maxQueueSize) {
                    result.maxQueueSize = heap.size();
                }
            }
        }
    }

    return result;
}