#ifndef DIJKSTRA_H
#define DIJKSTRA_H

#include "../pcb/board.h"

#include <vector>

SearchResult dijkstra(const RoutingGrid& grid, Position start, Position target);

#endif
