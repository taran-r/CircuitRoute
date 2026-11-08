#include "astar.h"
#include "min_heap.h"

#include <cstdlib>
#include <limits>

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

    return result;
}