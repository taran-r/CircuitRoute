#ifndef BOARD_H
#define BOARD_H
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


class Board{
private:
    int row;
    int col;
    std::vector<std::vector<char>> grid;
    std::vector<Connection> connections;
public:
    Board(int numberOfRows, int numberOfColumns);
    bool addAnObstacle(int row, int col);
    bool addConnection(int id, Position start, Position target);


    void print() const;

};


#endif