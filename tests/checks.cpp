#include "../src/pcb/board.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

void runSimulation(const Board&, SearchResult (*)(const Board&, const Connection&),
                   const std::string&);

std::string contents(const std::string& filename) {
    std::ifstream file(filename);
    std::ostringstream text;
    text << file.rdbuf();
    return text.str();
}

SearchResult manualSearch(const Board& board, const Connection& connection) {
    SearchResult result;
    if (connection.id == 0) {
        result.found = true;
        for (int c = 1; c <= 8; c++) {
            result.path.push_back({4, c});
        }
    } else {

        assert(!board.isWalkable({4, 5}, connection));
    }
    return result;
}

SearchResult invalidSearch(const Board&, const Connection& connection) {
    SearchResult result;
    result.found = true;
    result.path = {connection.start, connection.target};
    return result;
}

int main() {
    Board board(10, 10);
    assert(!board.addAnObstacle(-1, 2));
    assert(board.addAnObstacle(2, 2));
    assert(board.addConnection(0, {4, 1}, {4, 8}));
    assert(board.addConnection(1, {1, 5}, {7, 5}));
    assert(!board.addConnection(0, {0, 0}, {0, 1}));
    assert(!board.addConnection(2, {2, 2}, {0, 1}));
    assert(!board.addAnObstacle(4, 1));
    assert(!board.setMovementCost(0, 0, 0));

    Connection first, second;
    assert(board.getConnection(0, first));
    assert(board.getConnection(1, second));
    assert(!board.isWalkable(second.start, first));
    assert(board.getNeighbors({0, 0}, first).size() == 2);
    assert(board.exportCells("before.csv"));
    std::string before = contents("before.csv");
    assert(!board.blockRoute(0, {}));
    assert(!board.blockRoute(0, {{4, 1}, {4, 8}}));
    assert(!board.blockRoute(0, {{4, 1}, {-1, 1}, {4, 8}}));
    assert(!board.blockRoute(0, {{4, 1}, {4, 2}, {4, 1}, {4, 2},
                                {4, 3}, {4, 4}, {4, 5}, {4, 6}, {4, 7}, {4, 8}}));
    assert(board.exportCells("after.csv"));
    assert(before == contents("after.csv"));

    assert(board.setMovementCost(4, 2, 5));
    std::vector<Position> path;
    for (int c = 1; c <= 8; c++) { path.push_back({4, c}); }
    assert(board.getPathCost(path) == 11);
    assert(board.saveBoard("original.txt"));
    Board loaded(1, 1);
    assert(loaded.loadBoard("original.txt"));
    assert(loaded.saveBoard("roundtrip.txt"));
    assert(contents("original.txt") == contents("roundtrip.txt"));
    assert(board.blockRoute(0, path));
    assert(!board.blockRoute(0, path));
    assert(!board.saveBoard("routed_not_supported.txt"));
    assert(board.exportCells("occupied.csv"));
    std::vector<Position> crossing;
    for (int r = 1; r <= 7; r++) { crossing.push_back({r, 5}); }
    assert(!board.blockRoute(1, crossing));
    assert(board.exportCells("after_crossing.csv"));
    assert(contents("occupied.csv") == contents("after_crossing.csv"));

    std::ofstream bad("bad.txt");
    bad << "CIRCUITROUTE_1\n10 10 42\n#";
    bad.close();
    assert(!loaded.loadBoard("bad.txt"));
    assert(loaded.saveBoard("still_original.txt"));
    assert(contents("still_original.txt") == contents("original.txt"));

    Board a(400, 400), b(400, 400);
    assert(a.generateRandom(30000, 35, 42, 9));
    assert(b.generateRandom(30000, 35, 42, 9));
    assert(a.saveBoard("a.txt") && b.saveBoard("b.txt"));
    assert(contents("a.txt") == contents("b.txt"));
    assert(a.exportCells("large.csv"));
    std::ifstream csv("large.csv");
    std::string line;
    std::getline(csv, line);
    int count = 0, blocked = 0;
    while (std::getline(csv, line)) {
        count++;
        std::istringstream record(line);
        std::string field;
        std::getline(record, field, ',');
        std::getline(record, field, ',');
        std::getline(record, field, ',');
        if (field == "1") { blocked++; }
    }
    assert(count == 160000 && blocked == 30000);
    assert(!a.generateRandom(160000, 1, 42));
    assert(a.saveBoard("unchanged.txt"));
    assert(contents("unchanged.txt") == contents("a.txt"));

    runSimulation(loaded, manualSearch, "check");
    std::string firstRun = contents("check_routed_cells.csv");
    runSimulation(loaded, manualSearch, "check");
    assert(firstRun == contents("check_routed_cells.csv"));
    assert(loaded.saveBoard("unmodified_input.txt"));
    assert(contents("unmodified_input.txt") == contents("original.txt"));
    assert(contents("check_metrics.csv").find("no_path") != std::string::npos);
    runSimulation(loaded, invalidSearch, "invalid");
    assert(contents("invalid_metrics.csv").find("invalid_path") != std::string::npos);
    assert(loaded.exportCells("loaded.csv"));
    assert(contents("loaded.csv") == contents("invalid_routed_cells.csv"));
    std::cout << "All board and routing checks passed.\n";
}
