#include "board.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <random>
#include <stdexcept>

static bool samePosition(Position a, Position b) {
    return a.row == b.row && a.col == b.col;
}

Board::Board(int numberOfRows, int numberOfColumns) {
    if (numberOfRows <= 0 || numberOfColumns <= 0) {
        throw std::invalid_argument("Board dimensions must be positive.");
    }
    row = numberOfRows;
    col = numberOfColumns;
    grid.resize(row, std::vector<char>(col, '.'));
    movementCosts.resize(row, std::vector<int>(col, 1));
    wireOwners.resize(row, std::vector<int>(col, -1));
}

int Board::getRows() const { return row; }
int Board::getCols() const { return col; }

const std::vector<Connection>& Board::getConnections() const {
    return connections;
}

bool Board::getConnection(int id, Connection& connection) const {
    for (const Connection& current : connections) {
        if (current.id == id) {
            connection = current;
            return true;
        }
    }
    return false;
}

void Board::print() const {
    for (int r = 0; r < row; r++) {
        for (int c = 0; c < col; c++) {
            std::cout << grid[r][c] << ' ';
        }
        std::cout << '\n';
    }
}

bool Board::insideBoard(int cellRow, int cellCol) const {
    return cellRow >= 0 && cellRow < row && cellCol >= 0 && cellCol < col;
}

bool Board::addAnObstacle(int cellRow, int cellCol) {
    if (!insideBoard(cellRow, cellCol) || grid[cellRow][cellCol] != '.') {
        return false;
    }
    grid[cellRow][cellCol] = '#';
    return true;
}

bool Board::addConnection(int id, Position start, Position target) {
    Connection existing;
    if (id < 0 || getConnection(id, existing) || samePosition(start, target)) {
        return false;
    }
    if (!insideBoard(start.row, start.col) ||
        !insideBoard(target.row, target.col)) {
        return false;
    }
    if (grid[start.row][start.col] != '.' || grid[target.row][target.col] != '.') {
        return false;
    }
    connections.push_back({id, start, target});
    grid[start.row][start.col] = 'S';
    grid[target.row][target.col] = 'T';
    return true;
}

bool Board::setMovementCost(int cellRow, int cellCol, int cost) {
    if (!insideBoard(cellRow, cellCol) || cost < 1) {
        return false;
    }
    movementCosts[cellRow][cellCol] = cost;
    return true;
}

int Board::getMovementCost(Position position) const {
    if (!insideBoard(position.row, position.col)) {
        throw std::out_of_range("Position outside board.");
    }
    return movementCosts[position.row][position.col];
}

const int MIN_WIRE_DISTANCE = 5;
const int MAX_WIRE_DISTANCE = 60;

bool Board::generateRandom(int obstacleCount, int connectionCount,
                          unsigned int randomSeed, int maximumCost) {
    if (obstacleCount < 0 || connectionCount < 0 || maximumCost < 1) {
        return false;
    }
    std::size_t totalCells = static_cast<std::size_t>(row) * col;
    std::size_t needed = static_cast<std::size_t>(obstacleCount) +
                         2 * static_cast<std::size_t>(connectionCount);
    if (needed > totalCells) {
        return false;
    }

    Board replacement(row, col);
    replacement.seed = randomSeed;
    std::vector<Position> positions;
    for (int r = 0; r < row; r++) {
        for (int c = 0; c < col; c++) {
            positions.push_back({r, c});
        }
    }
    std::mt19937 generator(randomSeed);
    std::shuffle(positions.begin(), positions.end(), generator);
    std::uniform_int_distribution<int> randomCost(1, maximumCost);
    for (int r = 0; r < row; r++) {
        for (int c = 0; c < col; c++) {
            replacement.setMovementCost(r, c, randomCost(generator));
        }
    }

    std::size_t next = 0;
    for (int i = 0; i < obstacleCount; i++) {
        Position p = positions[next++];
        replacement.addAnObstacle(p.row, p.col);
    }

    std::vector<std::vector<bool>> used(row, std::vector<bool>(col, false));
    for (int i = 0; i < obstacleCount; i++) {
        used[positions[i].row][positions[i].col] = true;
    }

    for (int id = 0; id < connectionCount; id++) {
        while (used[positions[next].row][positions[next].col]) {
            next++;
        }
        Position start = positions[next];
        used[start.row][start.col] = true;

        Position target = start;
        bool foundNearby = false;
        std::uniform_int_distribution<int> randomOffset(-MAX_WIRE_DISTANCE, MAX_WIRE_DISTANCE);
        for (int attempt = 0; attempt < 200 && !foundNearby; attempt++) {
            int targetRow = start.row + randomOffset(generator);
            int targetCol = start.col + randomOffset(generator);
            int distance = std::abs(targetRow - start.row) + std::abs(targetCol - start.col);
            if (distance >= MIN_WIRE_DISTANCE && distance <= MAX_WIRE_DISTANCE &&
                replacement.insideBoard(targetRow, targetCol) && !used[targetRow][targetCol]) {
                target = {targetRow, targetCol};
                foundNearby = true;
            }
        }

        if (!foundNearby) {
            while (used[positions[next].row][positions[next].col]) {
                next++;
            }
            target = positions[next];
        }
        used[target.row][target.col] = true;
        replacement.addConnection(id, start, target);
    }
    *this = replacement;
    return true;
}

bool Board::isWalkable(Position position, const Connection& connection) const {
    if (!insideBoard(position.row, position.col)) {
        return false;
    }
    char cell = grid[position.row][position.col];
    if (cell == '.') {
        return true;
    }

    return (cell == 'S' && samePosition(position, connection.start)) ||
           (cell == 'T' && samePosition(position, connection.target));
}

std::vector<Position> Board::getNeighbors(
    Position position, const Connection& connection) const {
    std::vector<Position> neighbors;
    if (!insideBoard(position.row, position.col)) {
        return neighbors;
    }

    Position candidates[4] = {
        {position.row - 1, position.col},
        {position.row + 1, position.col},
        {position.row, position.col - 1},
        {position.row, position.col + 1}
    };
    for (Position candidate : candidates) {
        if (isWalkable(candidate, connection)) {
            neighbors.push_back(candidate);
        }
    }
    return neighbors;
}

bool Board::isValidRoute(
    int connectionId,
    const std::vector<Position>& path
) const {

    Connection connection;

    bool connectionExists = getConnection(connectionId, connection);

    if (!connectionExists) {
        return false;
    }

    if (path.empty()) {
        return false;
    }

    Position first = path.front();

    if (first.row != connection.start.row ||
        first.col != connection.start.col) {
        return false;
    }

    Position last = path.back();

    if (last.row != connection.target.row ||
        last.col != connection.target.col) {
        return false;
    }

    std::vector<std::vector<bool>> visited;
    visited.resize(row);

    for (int r = 0; r < row; r++) {
        visited[r].resize(col, false);
    }

    for (std::size_t i = 0; i < path.size(); i++) {
        Position current = path[i];

        if (!isWalkable(current, connection)) {
            return false;
        }

        if (visited[current.row][current.col]) {
            return false;
        }

        if (i > 0) {
            Position previous = path[i - 1];

            int rowDistance = std::abs(current.row - previous.row);
            int colDistance = std::abs(current.col - previous.col);

            int totalDistance = rowDistance + colDistance;

            if (totalDistance != 1) {
                return false;
            }
        }

        visited[current.row][current.col] = true;
    }

    return true;
}

bool Board::blockRoute(int connectionId, const std::vector<Position>& path) {
    if (!isValidRoute(connectionId, path)) {
        return false;
    }

    for (Position position : path) {
        grid[position.row][position.col] = '*';
        wireOwners[position.row][position.col] = connectionId;
    }

    return true;
}

long long Board::getPathCost(
    const std::vector<Position>& path
) const {
    long long totalCost = 0;

    for (std::size_t i = 1; i < path.size(); i++) {
        Position position = path[i];

        int cellCost = getMovementCost(position);

        totalCost = totalCost + cellCost;
    }

    return totalCost;
}

bool Board::saveBoard(const std::string& filename) const {

    for (int r = 0; r < row; r++) {
        for (int c = 0; c < col; c++) {
            if (grid[r][c] == '*') {
                return false;
            }
        }
    }

    std::ofstream file(filename);

    if (!file.is_open()) {
        return false;
    }

    file << "CIRCUITROUTE_1\n";
    file << row << ' ' << col << ' ' << seed << '\n';

    for (int r = 0; r < row; r++) {
        for (int c = 0; c < col; c++) {
            char cellToSave = '.';

            if (grid[r][c] == '#') {
                cellToSave = '#';
            }

            file << cellToSave << ' ';
            file << movementCosts[r][c] << '\n';
        }
    }

    file << connections.size() << '\n';

    for (const Connection& connection : connections) {
        file << connection.id << ' ';
        file << connection.start.row << ' ';
        file << connection.start.col << ' ';
        file << connection.target.row << ' ';
        file << connection.target.col << '\n';
    }

    file.flush();

    return file.good();
}

bool Board::loadBoard(const std::string& filename) {
    std::ifstream file(filename);

    if (!file.is_open()) {
        return false;
    }

    std::string version;
    int newRow;
    int newCol;
    unsigned int newSeed;

    file >> version >> newRow >> newCol >> newSeed;

    if (file.fail()) {
        return false;
    }

    if (version != "CIRCUITROUTE_1") {
        return false;
    }

    if (newRow < 1 || newRow > 1000) {
        return false;
    }

    if (newCol < 1 || newCol > 1000) {
        return false;
    }

    Board replacement(newRow, newCol);
    replacement.seed = newSeed;

    for (int r = 0; r < newRow; r++) {
        for (int c = 0; c < newCol; c++) {
            char cell;
            int cost;

            file >> cell >> cost;

            if (file.fail()) {
                return false;
            }

            if (cell != '.' && cell != '#') {
                return false;
            }

            bool costAccepted = replacement.setMovementCost(r, c, cost);

            if (!costAccepted) {
                return false;
            }

            if (cell == '#') {
                replacement.addAnObstacle(r, c);
            }
        }
    }

    int connectionCount;
    file >> connectionCount;

    if (file.fail()) {
        return false;
    }

    int maximumConnections = (newRow * newCol) / 2;

    if (connectionCount < 0 ||
        connectionCount > maximumConnections) {
        return false;
    }

    for (int i = 0; i < connectionCount; i++) {
        Connection connection;

        file >> connection.id;
        file >> connection.start.row;
        file >> connection.start.col;
        file >> connection.target.row;
        file >> connection.target.col;

        if (file.fail()) {
            return false;
        }

        bool connectionAdded = replacement.addConnection(
            connection.id,
            connection.start,
            connection.target
        );

        if (!connectionAdded) {
            return false;
        }
    }

    std::string extraContent;

    if (file >> extraContent) {
        return false;
    }

    *this = replacement;

    return true;
}

bool Board::exportCells(const std::string& filename) const {
    std::ofstream file(filename);

    if (!file.is_open()) {
        return false;
    }
    file << "row,col,blocked,movement_cost,wire_id,cell\n";

    for (int r = 0; r < row; r++) {
        for (int c = 0; c < col; c++) {
            int blocked = 0;

            if (grid[r][c] == '#') {
                blocked = 1;
            }

            file << r << ',';
            file << c << ',';
            file << blocked << ',';
            file << movementCosts[r][c] << ',';
            file << wireOwners[r][c] << ',';
            file << grid[r][c] << '\n';
        }
    }

    file.flush();

    return file.good();
}

const std::vector<std::vector<char>>& Board::getGrid() const {
    return grid;
}

const std::vector<std::vector<int>>& Board::getMovementCosts() const {
    return movementCosts;
}

RoutingGrid Board::getRoutingGrid(int connectionId) const {
    Connection connection;
    if (!getConnection(connectionId, connection)) {
        throw std::invalid_argument("Connection ID does not exist.");
    }

    RoutingGrid routingGrid(row, std::vector<RoutingCell>(col));
    for (int r = 0; r < row; r++) {
        for (int c = 0; c < col; c++) {
            Position position{r, c};
            if (isWalkable(position, connection)) {
                routingGrid[r][c].symbol = '.';
            } else {
                routingGrid[r][c].symbol = '#';
            }
            routingGrid[r][c].movementCost = movementCosts[r][c];
        }
    }
    return routingGrid;
}
