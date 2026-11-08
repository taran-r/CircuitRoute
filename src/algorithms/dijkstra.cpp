#include "dijkstra.h"
#include "min_heap.h"

#include <limits>
#include <vector>

namespace {

const int ROW_DELTA[4] = {-1, 1, 0, 0};
const int COL_DELTA[4] = {0, 0, -1, 1};

int toIndex(int row, int col, int cols) {
    return row * cols + col;
}
Position toPosition(int index, int cols) {
    return Position{index / cols, index % cols};
}
bool inBounds(int row, int col, int rows, int cols) {
    return row >= 0 && col >= 0 && row < rows && col < cols;
}

bool isBlocked(const RoutingCell& cell) {
    return cell.symbol == '#';
}

}

SearchResult dijkstra(const RoutingGrid& grid, Position start, Position target) {
    SearchResult result;
    result.found = false;
    result.cost = -1;
    result.nodesExpanded = 0;
    result.maxQueueSize = 0;
    result.estimatedMemoryBytes = 0;

    if (grid.empty() || grid[0].empty()) {
        return result;
    }

    int rows = static_cast<int>(grid.size());
    int cols = static_cast<int>(grid[0].size());
    for (int row = 1; row < rows; row++) {
        if (static_cast<int>(grid[row].size()) != cols) {
            return result;
        }
    }

    if (!inBounds(start.row, start.col, rows, cols) ||
        !inBounds(target.row, target.col, rows, cols)) {
        return result;
    }
    if (isBlocked(grid[start.row][start.col]) ||
        isBlocked(grid[target.row][target.col])) {
        return result;
    }

    int cellCount = rows * cols;
    int startIndex = toIndex(start.row, start.col, cols);
    int targetIndex = toIndex(target.row, target.col, cols);
    const double infinity = std::numeric_limits<double>::infinity();
    std::vector<double> distance(cellCount, infinity);
    std::vector<int> previous(cellCount, -1);

    MinHeap heap;
    distance[startIndex] = 0.0;
    heap.insert(0.0, startIndex);
    result.maxQueueSize = 1;

    while (!heap.empty()) {
        HeapEntry current = heap.extractMin();

        if (current.priority > distance[current.vertex]) {
            continue;
        }

        result.nodesExpanded++;

        if (current.vertex == targetIndex) {
            break;
        }

        Position cell = toPosition(current.vertex, cols);

        for (int i = 0; i < 4; i++) {
            int nextRow = cell.row + ROW_DELTA[i];
            int nextCol = cell.col + COL_DELTA[i];

            if (!inBounds(nextRow, nextCol, rows, cols)) {
                continue;
            }
            if (isBlocked(grid[nextRow][nextCol])) {
                continue;
            }

            int nextIndex = toIndex(nextRow, nextCol, cols);
            double newDistance = distance[current.vertex] + grid[nextRow][nextCol].movementCost;

            if (newDistance < distance[nextIndex]) {
                distance[nextIndex] = newDistance;
                previous[nextIndex] = current.vertex;
                heap.insert(newDistance, nextIndex);

                std::size_t queueSize = heap.size();
                if (queueSize > result.maxQueueSize) {
                    result.maxQueueSize = queueSize;
                }
            }
        }
    }

    // Estimate of primary search storage, not exact process memory.
    result.estimatedMemoryBytes =
        distance.size() * sizeof(double) +
        previous.size() * sizeof(int) +
        result.maxQueueSize * sizeof(HeapEntry);

    if (distance[targetIndex] == infinity) {
        return result;
    }



    result.found = true;
    result.cost = static_cast<long long>(distance[targetIndex]);
    std::vector<Position> reversedPath;
    for (int index = targetIndex; index != -1; index = previous[index]) {
        reversedPath.push_back(toPosition(index, cols));
    }
    result.path.resize(reversedPath.size());
    for (std::size_t i = 0; i < reversedPath.size(); i++) {
        result.path[i] = reversedPath[reversedPath.size() - 1 - i];
    }




    return result;
}
