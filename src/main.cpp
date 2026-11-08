#include "pcb/board.h"

#include "pcb/board.h"

#include <iostream>

int main() {
    Board board(5, 8);

    board.addAnObstacle(1, 2);
    board.addAnObstacle(2, 2);
    board.addAnObstacle(3, 2);

    bool firstAdded = board.addConnection(0, {0, 0}, {4, 7});
    bool secondAdded = board.addConnection(1, {0, 7}, {4, 0});

    std::cout << std::boolalpha;
    std::cout << "First connection added: " << firstAdded << '\n';
    std::cout << "Second connection added: " << secondAdded << '\n';

    board.print();

    // These should fail without changing the board.
    std::cout << "Connection on obstacle: "
              << board.addConnection(2, {1, 2}, {2, 7}) << '\n';

    std::cout << "Duplicate ID: "
              << board.addConnection(0, {0, 1}, {4, 6}) << '\n';

    std::cout << "Obstacle on endpoint: "
              << board.addAnObstacle(0, 0) << '\n';

    return 0;
}