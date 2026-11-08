#ifndef ASTAR_H
#define ASTAR_H

#include "../pcb/board.h"

SearchResult astar(const RoutingGrid& grid, Position start, Position target);

#endif