#include "../src/algorithms/astar.h"
#include "../src/algorithms/dijkstra.h"

#include <cassert>
#include <iostream>

int main() {
    const Position start{0, 0};
    const Position target{0, 2};
    RoutingGrid grid(2, std::vector<RoutingCell>(3));

    SearchResult a = astar(grid, start, target);
    SearchResult d = dijkstra(grid, start, target);
    assert(a.found && d.found);
    assert(a.cost == 2 && d.cost == 2);

    grid[0][1].movementCost = 9;
    a = astar(grid, start, target);
    d = dijkstra(grid, start, target);
    assert(a.found && d.found);
    assert(a.cost == 4 && d.cost == 4);

    grid[0][1].movementCost = 1;
    grid[0][1].symbol = '#';
    a = astar(grid, start, target);
    d = dijkstra(grid, start, target);
    assert(a.found && d.found);
    assert(a.cost == 4 && d.cost == 4);
    assert(a.path.size() == 5);
    assert(a.path.front().row == start.row && a.path.front().col == start.col);
    assert(a.path.back().row == target.row && a.path.back().col == target.col);

    grid[1][1].symbol = '#';
    a = astar(grid, start, target);
    d = dijkstra(grid, start, target);
    assert(!a.found && !d.found);
    assert(a.path.empty() && d.path.empty());

    std::cout << "Passed: straight route, weighted detour, obstacle detour, "
                 "and unreachable target.\n";
}