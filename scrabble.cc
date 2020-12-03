#include <iostream>
#include <sstream>

class Tile {
private:
    char letter;
    int points;

public:
    Tile(char letter, int points) : letter(letter), points(points)
    {}

    char getLetter() { return letter; }

    int getPoints() { return points; }

    std::string toString() {
        std::stringstream ret;
        ret << "{" << letter << ", " << points << "}";
        return ret.str();
    }
};

class Cell {
public:
    enum Type { NORMAL = 0, DW, TW, DL, TL };

private:
    Tile* tile;
    Type type;

public:
    Cell(Type type) : tile(nullptr), type(type)
    {};

    Cell() : Cell(Type::NORMAL) {}

    void fill(Tile* tile) { this->tile = tile; }

    void setType(Type type) { this->type = type; }

    bool isEmpty() { return tile == nullptr; }

    Type getType() { return type; }

    std::string toString() {
        std::stringstream ret;
        ret << "{" << (tile != nullptr ? tile->toString() : "{}") << ", " << type << "}";
        return ret.str();
    }
};

class Board {
private:
    static constexpr int SIZE = 15;

    Cell board[SIZE][SIZE];

public:
    Board() {
        // setup DW cells
        board[1][1]  .setType(Cell::Type::DW);
        board[1][13] .setType(Cell::Type::DW);
        board[2][2]  .setType(Cell::Type::DW);
        board[2][12] .setType(Cell::Type::DW);
        board[3][3]  .setType(Cell::Type::DW);
        board[3][11] .setType(Cell::Type::DW);
        board[4][4]  .setType(Cell::Type::DW);
        board[4][10] .setType(Cell::Type::DW);
        board[7][7]  .setType(Cell::Type::DW);
        board[10][4] .setType(Cell::Type::DW);
        board[10][10].setType(Cell::Type::DW);
        board[11][3] .setType(Cell::Type::DW);
        board[11][11].setType(Cell::Type::DW);
        board[12][2] .setType(Cell::Type::DW);
        board[12][12].setType(Cell::Type::DW);
        board[13][1] .setType(Cell::Type::DW);
        board[13][13].setType(Cell::Type::DW);

        // setup TW cells
        board[0][0]  .setType(Cell::Type::TW);
        board[0][7]  .setType(Cell::Type::TW);
        board[0][14] .setType(Cell::Type::TW);
        board[7][0]  .setType(Cell::Type::TW);
        board[7][14] .setType(Cell::Type::TW);
        board[14][0] .setType(Cell::Type::TW);
        board[14][7] .setType(Cell::Type::TW);
        board[14][14].setType(Cell::Type::TW);

        // setup DL cells
        board[0][3]  .setType(Cell::Type::DL);
        board[0][11] .setType(Cell::Type::DL);
        board[2][6]  .setType(Cell::Type::DL);
        board[2][8]  .setType(Cell::Type::DL);
        board[3][0]  .setType(Cell::Type::DL);
        board[3][7]  .setType(Cell::Type::DL);
        board[3][14] .setType(Cell::Type::DL);
        board[6][2]  .setType(Cell::Type::DL);
        board[6][6]  .setType(Cell::Type::DL);
        board[6][8]  .setType(Cell::Type::DL);
        board[6][12] .setType(Cell::Type::DL);
        board[7][3]  .setType(Cell::Type::DL);
        board[7][11] .setType(Cell::Type::DL);
        board[8][2]  .setType(Cell::Type::DL);
        board[8][6]  .setType(Cell::Type::DL);
        board[8][8]  .setType(Cell::Type::DL);
        board[8][12] .setType(Cell::Type::DL);
        board[11][0] .setType(Cell::Type::DL);
        board[11][7] .setType(Cell::Type::DL);
        board[11][14].setType(Cell::Type::DL);
        board[12][6] .setType(Cell::Type::DL);
        board[12][8] .setType(Cell::Type::DL);
        board[14][3] .setType(Cell::Type::DL);
        board[14][11].setType(Cell::Type::DL);

        // setup TL cells
        board[1][5]  .setType(Cell::Type::TL);
        board[1][9]  .setType(Cell::Type::TL);
        board[5][1]  .setType(Cell::Type::TL);
        board[5][5]  .setType(Cell::Type::TL);
        board[5][9]  .setType(Cell::Type::TL);
        board[5][13] .setType(Cell::Type::TL);
        board[9][1]  .setType(Cell::Type::TL);
        board[9][5]  .setType(Cell::Type::TL);
        board[9][9]  .setType(Cell::Type::TL);
        board[9][13] .setType(Cell::Type::TL);
        board[13][5] .setType(Cell::Type::TL);
        board[13][9] .setType(Cell::Type::TL);
    }

    std::string toString() {
        std::stringstream ret;
        for (int i = 0; i < SIZE; i++) {
            ret << "{";
            for (int j = 0; j < SIZE; j++) {
                if (j != 0) ret << ", ";
                ret << board[i][j].toString();
            }
            ret << "}";
            if (i != SIZE - 1) ret << std::endl;
        }
        return ret.str();
    }
};

int main() {
    Board board{};

    std::cout << "Scrabble" << std::endl;
    std::cout << board.toString() << std::endl;

    return 0;
}
