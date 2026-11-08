#ifndef BOARD_H
#define BOARD_H
#include <cstddef>
#include <string>
#include <vector>

struct Position {
    int row;
    int col;
};

struct Connection {
    int id;
    Position start;
    Position target;
};

struct RoutingCell {
    char symbol = '.';
    int movementCost = 1;
};

using RoutingGrid = std::vector<std::vector<RoutingCell>>;

struct SearchResult {
    bool found = false;
    std::vector<Position> path;
    long long cost = -1;
    std::size_t nodesExpanded = 0;
    std::size_t maxQueueSize = 0;

    std::size_t estimatedMemoryBytes = 0;
};

class Board {
private:
    int row;
    int col;
    unsigned int seed = 0;
    std::vector<std::vector<char>> grid;
    std::vector<std::vector<int>> movementCosts;
    std::vector<std::vector<int>> wireOwners;
    std::vector<Connection> connections;

public:
    Board(int numberOfRows, int numberOfColumns);
    void print() const;
    const std::vector<std::vector<char>>& getGrid() const;
    const std::vector<std::vector<int>>& getMovementCosts() const;
    RoutingGrid getRoutingGrid(int connectionId) const;
    int getRows() const;
    int getCols() const;
    const std::vector<Connection>& getConnections() const;
    bool getConnection(int id, Connection& connection) const;
    bool insideBoard(int cellRow, int cellCol) const;
    bool addAnObstacle(int cellRow, int cellCol);
    bool addConnection(int id, Position start, Position target);
    bool setMovementCost(int cellRow, int cellCol, int cost);
    int getMovementCost(Position position) const;

    bool generateRandom(int obstacleCount, int connectionCount,
                        unsigned int randomSeed, int maximumCost = 1);

    bool isWalkable(Position position, const Connection& connection) const;
    std::vector<Position> getNeighbors(
        Position position, const Connection& connection) const;

    bool isValidRoute(int connectionId, const std::vector<Position>& path) const;
    bool blockRoute(int connectionId, const std::vector<Position>& path);
    long long getPathCost(const std::vector<Position>& path) const;

    bool saveBoard(const std::string& filename) const;
    bool loadBoard(const std::string& filename);
    bool exportCells(const std::string& filename) const;
};

#endif
