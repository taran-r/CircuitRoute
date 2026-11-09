#include "../src/algorithms/dijkstra.h"

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <vector>

RoutingGrid makeGrid(int rows, int cols) {
    return RoutingGrid(rows, std::vector<RoutingCell>(cols));
}

bool samePosition(Position a, Position b) {
    return a.row == b.row && a.col == b.col;
}

bool isAdjacent(Position a, Position b) {
    int rowDiff = std::abs(a.row - b.row);
    int colDiff = std::abs(a.col - b.col);
    return (rowDiff + colDiff) == 1;
}

long long pathEnteredCost(const RoutingGrid& grid, const std::vector<Position>& path) {
    long long total = 0;
    for (std::size_t i = 1; i < path.size(); i++) {
        total += grid[path[i].row][path[i].col].movementCost;
    }
    return total;
}

void assertValidPath(const RoutingGrid& grid, const SearchResult& result,
                     Position start, Position target, long long expectedCost) {
    assert(result.found);
    assert(!result.path.empty());
    assert(samePosition(result.path.front(), start));
    assert(samePosition(result.path.back(), target));
    assert(result.cost == expectedCost);
    assert(pathEnteredCost(grid, result.path) == expectedCost);

    for (std::size_t i = 0; i < result.path.size(); i++) {
        Position cell = result.path[i];
        assert(cell.row >= 0 && cell.row < static_cast<int>(grid.size()));
        assert(cell.col >= 0 && cell.col < static_cast<int>(grid[cell.row].size()));
        assert(grid[cell.row][cell.col].symbol != '#');
        if (i > 0) {
            assert(isAdjacent(result.path[i - 1], cell));
        }
    }
}

int main() {
    {
        RoutingGrid grid = makeGrid(1, 4);
        Position start{0, 0};
        Position target{0, 3};
        SearchResult result = dijkstra(grid, start, target);
        assertValidPath(grid, result, start, target, 3);
        assert(result.path.size() == 4);
        assert(result.nodesExpanded > 0);
        assert(result.maxQueueSize > 0);
        assert(result.estimatedMemoryBytes > 0);
    }

    {
        RoutingGrid grid = makeGrid(2, 3);
        grid[0][1].movementCost = 9;
        Position start{0, 0};
        Position target{0, 2};
        SearchResult result = dijkstra(grid, start, target);
        assertValidPath(grid, result, start, target, 4);
        assert(result.path.size() == 5);
    }

    {
        RoutingGrid grid = makeGrid(3, 3);
        grid[0][1].movementCost = 30;
        grid[1][1].movementCost = 30;
        Position start{0, 0};
        Position target{2, 2};
        SearchResult result = dijkstra(grid, start, target);
        assertValidPath(grid, result, start, target, 4);
        assert(result.path.size() == 5);
    }

    {
        RoutingGrid grid = makeGrid(3, 3);
        grid[0][1].symbol = '#';
        grid[1][1].symbol = '#';
        Position start{0, 0};
        Position target{0, 2};
        SearchResult result = dijkstra(grid, start, target);
        assertValidPath(grid, result, start, target, 6);
    }

    {
        RoutingGrid grid = makeGrid(2, 3);
        grid[0][1].symbol = '#';
        grid[1][1].symbol = '#';
        SearchResult result = dijkstra(grid, {0, 0}, {0, 2});
        assert(!result.found);
        assert(result.path.empty());
        assert(result.cost == -1);
    }

    {
        RoutingGrid grid = makeGrid(2, 2);
        grid[0][0].symbol = '#';
        SearchResult blockedStart = dijkstra(grid, {0, 0}, {1, 1});
        assert(!blockedStart.found);

        grid[0][0].symbol = '.';
        grid[1][1].symbol = '#';
        SearchResult blockedTarget = dijkstra(grid, {0, 0}, {1, 1});
        assert(!blockedTarget.found);

        SearchResult outOfBounds = dijkstra(grid, {0, 0}, {5, 5});
        assert(!outOfBounds.found);
    }

    {
        RoutingGrid grid = makeGrid(2, 2);
        Position cell{1, 1};
        SearchResult result = dijkstra(grid, cell, cell);
        assertValidPath(grid, result, cell, cell, 0);
        assert(result.path.size() == 1);
        assert(result.nodesExpanded == 1);
    }

    {
        RoutingGrid grid = makeGrid(3, 1);
        Position start{0, 0};
        Position target{2, 0};
        SearchResult result = dijkstra(grid, start, target);
        assertValidPath(grid, result, start, target, 2);
    }

    {
        RoutingGrid jagged = {
            {RoutingCell{'.', 1}, RoutingCell{'.', 1}},
            {RoutingCell{'.', 1}}
        };
        SearchResult result = dijkstra(jagged, {0, 0}, {1, 0});
        assert(!result.found);
    }

    std::cout << "Passed: uniform, weighted detours, obstacles, unreachable, "
                 "invalid endpoints, start==target, and boundary cases.\n";
    return 0;
}
