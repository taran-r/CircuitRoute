#include "../src/algorithms/astar.h"
#include "../src/algorithms/dijkstra.h"

#include <cassert>
#include <iostream>

void assertSameOptimalCost(const RoutingGrid& grid, Position start, Position target,
                           long long expectedCost, bool expectFound) {
    SearchResult a = astar(grid, start, target);
    SearchResult d = dijkstra(grid, start, target);

    assert(a.found == expectFound);
    assert(d.found == expectFound);

    if (!expectFound) {
        assert(a.cost == -1 || a.path.empty());
        assert(d.cost == -1);
        assert(a.path.empty() && d.path.empty());
        return;
    }

    assert(a.cost == expectedCost);
    assert(d.cost == expectedCost);
    assert(a.cost == d.cost);
}

int main() {
    const Position start{0, 0};
    const Position target{0, 2};
    RoutingGrid grid(2, std::vector<RoutingCell>(3));

    assertSameOptimalCost(grid, start, target, 2, true);

    grid[0][1].movementCost = 9;
    assertSameOptimalCost(grid, start, target, 4, true);

    grid[0][1].movementCost = 1;
    grid[0][1].symbol = '#';
    {
        SearchResult a = astar(grid, start, target);
        SearchResult d = dijkstra(grid, start, target);
        assert(a.found && d.found);
        assert(a.cost == 4 && d.cost == 4);
        assert(a.path.size() == 5);
        assert(a.path.front().row == start.row && a.path.front().col == start.col);
        assert(a.path.back().row == target.row && a.path.back().col == target.col);
        assert(d.path.front().row == start.row && d.path.front().col == start.col);
        assert(d.path.back().row == target.row && d.path.back().col == target.col);
    }

    grid[1][1].symbol = '#';
    assertSameOptimalCost(grid, start, target, -1, false);

    {
        RoutingGrid taller(3, std::vector<RoutingCell>(3));
        taller[0][1].movementCost = 50;
        taller[1][1].movementCost = 50;
        assertSameOptimalCost(taller, {0, 0}, {0, 2}, 6, true);
        assertSameOptimalCost(taller, {2, 0}, {2, 2}, 2, true);
    }

    {
        RoutingGrid blockedStart(2, std::vector<RoutingCell>(2));
        blockedStart[0][0].symbol = '#';
        assertSameOptimalCost(blockedStart, {0, 0}, {1, 1}, -1, false);
    }

    {
        RoutingGrid same(1, std::vector<RoutingCell>(1));
        assertSameOptimalCost(same, {0, 0}, {0, 0}, 0, true);
    }

    {
        RoutingGrid corridor(1, std::vector<RoutingCell>(5));
        corridor[0][2].movementCost = 7;
        assertSameOptimalCost(corridor, {0, 0}, {0, 4}, 10, true);
    }

    std::cout << "Passed: Dijkstra and A* matching optimal costs on uniform, "
                 "weighted, obstacle, unreachable, and extra deterministic cases.\n";
    return 0;
}
