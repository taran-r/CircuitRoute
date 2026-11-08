#include "../src/algorithms/astar.h"
#include "../src/algorithms/dijkstra.h"

#include <cassert>
#include <iostream>

int main() {
    const Position start{0, 0};
    const Position target{0, 2};

    std::vector<std::vector<char>> grid{
        {'.', '.', '.'},
        {'.', '.', '.'}
    };

    AStarResult a = astar(grid, start, target);
    DijkstraResult d = dijkstra(grid, start, target);
    assert(a.found && d.found);
    assert(a.cost == 2.0 && d.cost == 2.0);

    grid[0][1] = '#';
    a = astar(grid, start, target);
    d = dijkstra(grid, start, target);
    assert(a.found && d.found);
    assert(a.cost == 4.0 && d.cost == 4.0);
    assert(a.path.size() == 5);
    assert(a.path.front().row == start.row && a.path.front().col == start.col);
    assert(a.path.back().row == target.row && a.path.back().col == target.col);

    grid[1][1] = '#';
    a = astar(grid, start, target);
    d = dijkstra(grid, start, target);
    assert(!a.found && !d.found);
    assert(a.path.empty() && d.path.empty());

    std::cout << "Passed: straight route, obstacle detour, and unreachable target.\n";
}