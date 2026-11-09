#ifndef BENCHMARK_H
#define BENCHMARK_H

#include "../pcb/board.h"

#include <string>

using SearchFunction = SearchResult (*)(const RoutingGrid&, Position, Position);

bool runBenchmark(const std::string& filename, SearchFunction dijkstraSearch,
                  SearchFunction aStarSearch);

#endif
