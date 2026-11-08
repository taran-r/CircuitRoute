#ifndef ASTAR_H
#define ASTAR_H

#include "../pcb/board.h"

#include <vector>

struct AStarResult {
    bool found;
    std::vector<Position> path;
    double cost;
    int nodesExpand;
    int maxQueueSize;
};

AStarResult astar(const std::vector<std::vector<char>>& grid,
                  Position start, Position target);

#endif