#include "board.h"

#include <iostream>
#include <stdexcept>

Board::Board(int numberOfRows, int numberOfColumns) {
    if (numberOfRows <= 0 || numberOfColumns <= 0) {
        throw std::invalid_argument("Board dimensions must be positive.");
    }

    int rows = numberOfRows;
    int cols = numberOfColumns;
    this->row = rows;
    this->col = cols;

    grid.resize(rows);

    for (int row = 0; row < rows; row++) {
        grid[row].resize(cols, '.');
    }
}

void Board::print() const {
    for (std::size_t row = 0; row < grid.size(); row++) {
        for (std::size_t col = 0; col < grid[row].size(); col++) {
            std::cout << grid[row][col] << ' ';
        }

        std::cout << '\n';
    }
}

bool Board::addAnObstacle(int row, int col) {
    if (row < 0 || col < 0) {
        return false;
    }

    if (row >= static_cast<int>(grid.size()) ||
        col >= static_cast<int>(grid[row].size())) {
        return false;
    }

    if (grid[row][col] != '.') {
        return false;
    }

    grid[row][col] = '#';
    return true;
}

bool Board::addConnection(int id, Position start, Position target) {
    // IDs must be nonnegative and unique.
    if (id < 0) {
        return false;
    }

    for (const Connection& connection : connections) {
        if (connection.id == id) {
            return false;
        }
    }
    for (Position position : {start, target}) {
        if (position.row < 0 || position.col < 0) {
            return false;
        }

        if (position.row >= static_cast<int>(grid.size()) ||
            position.col >=
                static_cast<int>(grid[position.row].size())) {
            return false;
        }
        if (grid[position.row][position.col] != '.') {
            return false;
        }
    }

    if (start.row == target.row && start.col == target.col) {
        return false;
    }
    Connection connection{id, start, target};
    connections.push_back(connection);

    grid[start.row][start.col] = 'S';
    grid[target.row][target.col] = 'T';

    return true;
}