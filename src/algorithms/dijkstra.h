#ifndef DIJKSTRA_H
#define DIJKSTRA_H

#include "../pcb/board.h"

#include <vector>

struct DijkstraResult {
    bool found;
    std::vector<Position> path;
    double cost;
    int nodesExpand;
    int maxQueueSize;
};

DijkstraResult dijkstra(const std::vector<std::vector<char>>& grid, Position start, Position target);

#endif
