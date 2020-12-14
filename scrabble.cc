#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>

class TrieNode {
private:
    bool terminal;
    TrieNode* children[26];

public:
    TrieNode(bool terminal) : terminal(terminal) {
        for (int i = 0; i < 26; i++) children[i] = nullptr;
    }

    void addChild(char letter, bool terminal) {
        children[letter - 'A'] = new TrieNode(terminal);
    }

    TrieNode* childAt(char letter) {
        return children[letter - 'A'];
    }

    void makeTerminal() { terminal = true; }

    bool isTerminal() { return terminal; }
};

class Trie {
private:
    TrieNode* root;

    static TrieNode* fromWords(std::vector<std::string> words) {
        TrieNode* ret = new TrieNode(false);
        for (std::string word : words) {
            TrieNode* curr = ret;
            for (char ch : word) {
                ch = toupper(ch);
                if (curr->childAt(ch) == nullptr) {
                    curr->addChild(ch, false);
                }
                curr = curr->childAt(ch);
            }
            curr->makeTerminal();
        }
        return ret;
    }

public:
    Trie(std::vector<std::string> words) : root(fromWords(words)) {}

    Trie(std::string filename) {
        std::ifstream fin(filename);
        std::vector<std::string> words;
        if (fin.is_open()) {
            std::string line;
            while (getline(fin, line)) {
                words.push_back(line);
            }
            fin.close();
        }
        root = fromWords(words);
    }

    bool isLegal(std::string word) {
        TrieNode* curr = root;
        for (char ch : word) {
            ch = toupper(ch);
            curr = curr->childAt(ch);
            if (curr == nullptr) return false;
        }
        return curr->isTerminal();
    }
};

enum Direction { ACROSS = 0, DOWN };

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
        ret << " \e[1;33m" << letter << points << "\e[0m ";
        return ret.str();
    }
};

class Cell {
public:
    enum Type { NORMAL = 0, DW, TW, DL, TL };

private:
    Tile tile;
    Type type;
    uint32_t down_crosses;
    uint32_t across_crosses;

public:
    Cell(Type type)
        : tile('\0', 0), type(type), down_crosses(0xFFFFFFFF), across_crosses(0xFFFFFFFF)
    {};

    Cell() : Cell(Type::NORMAL) {}

    void fill(Tile tile) { this->tile = tile; }

    void setType(Type type) { this->type = type; }

    bool isEmpty() { return tile.getLetter() == '\0'; }

    Tile getTile() { return tile; }

    Type getType() { return type; }

    bool isValidCross(char ch, Direction dir) {
        int idx = ch - 'A';
        if (dir == Direction::ACROSS) {
            return (across_crosses & (1 << idx)) != 0;
        } else if (dir == Direction::DOWN) {
            return (down_crosses & (1 << idx)) != 0;
        }
        return false;
    }

    void updateValidCrosses(Trie* trie, std::string across_prefix, std::string across_postfix,
                            std::string down_prefix, std::string down_postfix) {
        if (isEmpty()) {
            uint32_t _across_crosses = 0;
            uint32_t _down_crosses = 0;
            for (char ch = 'A'; ch <= 'Z'; ch++) {
                int idx = ch - 'A';
                if (!(across_prefix == "" && across_postfix == "")) {
                    if (trie->isLegal(across_prefix + ch + across_postfix)) {
                        _across_crosses |= (1 << idx);
                    }
                }
                if (!(down_prefix == "" && down_postfix == "")) {
                    if (trie->isLegal(down_prefix + ch + down_postfix)) {
                        _down_crosses |= (1 << idx);
                    }
                }
            }
            across_crosses = _across_crosses == 0 ? across_crosses : _across_crosses;
            down_crosses = _down_crosses == 0 ? down_crosses : _down_crosses;
        }
    }

    std::string toString() {
        std::stringstream ret;
        if (isEmpty()) {
            switch (type) {
                case DW: {
                    ret << " \e[1;35mDW\e[0m ";
                } break;
                case TW: {
                    ret << " \e[1;31mTW\e[0m ";
                } break;
                case DL: {
                    ret << " \e[1;36mDL\e[0m ";
                } break;
                case TL: {
                    ret << " \e[1;34mTL\e[0m ";
                } break;
                default: {
                    ret << "    ";
                } break;
            }
        }
        else {
            ret << tile.toString();
        }
        return ret.str();
    }
};

class Board {
public:
    static constexpr int SIZE = 15;

private:
    static constexpr int POINTS[] = {
        1, 3, 3, 2, 1, 4, 2, 4, 1, 8, 5, 1, 3, 1, 1, 3, 10, 1, 1, 1, 1, 4, 4, 8, 4, 10
    };

    std::stringstream blank_line;

    Cell board[SIZE][SIZE];

    Trie* trie;

    std::string getPrefix(int x, int y, Direction dir) {
        std::string ret = "";
        if (dir == Direction::ACROSS) x--;
        else if (dir == Direction::DOWN) y--;
        while (!board[y][x].isEmpty() && x >= 0 && x < SIZE && y >= 0 && y < SIZE) {
            ret = board[y][x].getTile().getLetter() + ret;
            if (dir == Direction::ACROSS) {
                x--;
            } else if (dir == Direction::DOWN) {
                y--;
            }
        }
        return ret;
    }

    std::string getPostfix(int x, int y, Direction dir) {
        std::string ret = "";
        if (dir == Direction::ACROSS) x++;
        else if (dir == Direction::DOWN) y++;
        while (!board[y][x].isEmpty() && x >= 0 && x < SIZE && y >= 0 && y < SIZE) {
            ret += board[y][x].getTile().getLetter();
            if (dir == Direction::ACROSS) {
                x++;
            } else if (dir == Direction::DOWN) {
                y++;
            }
        }
        return ret;
    }

    void updateAdjacentValidCrosses(int x, int y) {
        if (x > 0) {
            board[y][x - 1].updateValidCrosses(trie, 
                            getPrefix(x - 1, y, Direction::ACROSS),
                            getPostfix(x - 1, y, Direction::ACROSS),
                            getPrefix(x - 1, y, Direction::DOWN),
                            getPostfix(x - 1, y, Direction::DOWN));
            // if (y > 0) {
                // board[y - 1][x - 1].updateValidCrosses(trie,
                                    // getPrefix(x - 1, y - 1, Direction::ACROSS),
                                    // getPostfix(x - 1, y - 1, Direction::ACROSS),
                                    // getPrefix(x - 1, y - 1, Direction::DOWN),
                                    // getPostfix(x - 1, y - 1, Direction::DOWN));
            // }
            // if (y < SIZE - 1) {
                // board[y + 1][x - 1].updateValidCrosses(trie,
                                    // getPrefix(x - 1, y + 1, Direction::ACROSS),
                                    // getPostfix(x - 1, y + 1, Direction::ACROSS),
                                    // getPrefix(x - 1, y + 1, Direction::DOWN),
                                    // getPostfix(x - 1, y + 1, Direction::DOWN));
            // }
        }
        if (x < SIZE - 1) {
            board[y][x + 1].updateValidCrosses(trie, 
                            getPrefix(x + 1, y, Direction::ACROSS),
                            getPostfix(x + 1, y, Direction::ACROSS),
                            getPrefix(x + 1, y, Direction::DOWN),
                            getPostfix(x + 1, y, Direction::DOWN));
            // if (y > 0) {
                // board[y - 1][x - 1].updateValidCrosses(trie,
                                    // getPrefix(x + 1, y - 1, Direction::ACROSS),
                                    // getPostfix(x + 1, y - 1, Direction::ACROSS),
                                    // getPrefix(x + 1, y - 1, Direction::DOWN),
                                    // getPostfix(x + 1, y - 1, Direction::DOWN));
            // }
            // if (y < SIZE - 1) {
                // board[y + 1][x - 1].updateValidCrosses(trie,
                                    // getPrefix(x + 1, y + 1, Direction::ACROSS),
                                    // getPostfix(x + 1, y + 1, Direction::ACROSS),
                                    // getPrefix(x + 1, y + 1, Direction::DOWN),
                                    // getPostfix(x + 1, y + 1, Direction::DOWN));
            // }
        }
        if (y > 0) {
            board[y - 1][x].updateValidCrosses(trie, 
                            getPrefix(x, y - 1, Direction::ACROSS),
                            getPostfix(x, y - 1, Direction::ACROSS),
                            getPrefix(x, y - 1, Direction::DOWN),
                            getPostfix(x, y - 1, Direction::DOWN));
        }
        if (y < SIZE - 1) {
            board[y + 1][x].updateValidCrosses(trie, 
                            getPrefix(x, y + 1, Direction::ACROSS),
                            getPostfix(x, y + 1, Direction::ACROSS),
                            getPrefix(x, y + 1, Direction::DOWN),
                            getPostfix(x, y + 1, Direction::DOWN));
        }
    }

public:

    Board() : trie(nullptr) {
        // setup trie
        trie = new Trie("dict.txt");

        // setup blank_line
        blank_line << "    ";
        for (int i = 0; i < SIZE; i++) {
            blank_line << "|----";
        }
        blank_line << "|" << std::endl;

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

    int placeWord(std::string word, int x, int y, Direction dir) {
        int score = 0;
        if (!trie->isLegal(word)) return -1;
        if (dir == Direction::ACROSS) {
            if (x + word.length() > SIZE) return -1;
            for (unsigned int i = 0; i < word.length(); i++) {
                char ch = toupper(word[i]);
                if (!board[y][x + i].isValidCross(ch, Direction::DOWN) &&
                     board[y][x + i].getTile().getLetter() != ch) {
                    return -1;
                }
            }
            for (unsigned int i = 0; i < word.length(); i++) {
                char ch = toupper(word[i]);
                int points = POINTS[ch - 'A'];
                Cell& cell = board[y][x + i];
                cell.fill(Tile(ch, points));
                updateAdjacentValidCrosses(x + i, y);
                score += points;
            }
        } else if (dir == Direction::DOWN) {
            if (y + word.length() > SIZE) return -1;
            for (unsigned int i = 0; i < word.length(); i++) {
                char ch = toupper(word[i]);
                if (!board[y + i][x].isValidCross(ch, Direction::ACROSS) &&
                     board[y + i][x].getTile().getLetter() != ch) {
                    return -1;
                }
            }
            for (unsigned int i = 0; i < word.length(); i++) {
                char ch = toupper(word[i]);
                int points = POINTS[ch - 'A'];
                Cell& cell = board[y + i][x];
                cell.fill(Tile(ch, points));
                updateAdjacentValidCrosses(x, y + i);
                score += points;
            }
        }
        return score;
    }

    Cell getCell(int x, int y) { return board[y][x]; }

    std::string toString() {
        std::stringstream ret;
        ret << "    ";
        for (int i = 0; i < Board::SIZE; i++) {
            ret << "  " << i / 10 << i % 10 << " ";
        }
        ret << std::endl;
        ret << blank_line.str();
        for (int i = 0; i < SIZE; i++) {
            ret << " " << i / 10 << i % 10 << " ";
            for (int j = 0; j < SIZE; j++) {
                ret << "|";
                ret << board[i][j].toString();
            }
            ret << "|";
            ret << std::endl << blank_line.str();
        }
        return ret.str();
    }
};

class Game {
private:
    Board board;
    int scores[2];
    std::vector<char> racks[2];

    void printBoard() {
        for (int i = 0; i < 50; i++) std::cout << std::endl;

        for (int i = 0; i < 19; i++) std::cout << " ";
        for (int i = 0; i < 8; i++) std::cout << "|----";
        std::cout << "|" << std::endl;

        std::cout << "  Your Score: " << scores[0] / 100 << (scores[0] / 10) % 10
                  << scores[0] % 10 << "  | \e[1;33mS1\e[0m | \e[1;33mC3\e[0m "
                  << "| \e[1;33mR1\e[0m | \e[1;33mA1\e[0m | \e[1;33mB3\e[0m "
                  << "| \e[1;33mB3\e[0m | \e[1;33mL1\e[0m | \e[1;33mE1\e[0m "
                  << "|  Their Score: " << scores[1] / 100
                  << (scores[1] / 10) % 10 << scores[1] % 10 << std::endl;

        for (int i = 0; i < 19; i++) std::cout << " ";
        for (int i = 0; i < 8; i++) std::cout << "|----";
        std::cout << "|" << std::endl;

        std::cout << std::endl << std::endl;

        std::cout << board.toString();
    }

    void humanTurn() {
        std::cout << "Enter a move (word, x, y, direction): ";
        std::string move;
        getline(std::cin, move);
        std::stringstream buf(move);
        std::string word, sdir;
        Direction dir;
        int x, y;
        buf >> word >> x >> y >> sdir;
        if (sdir == "D") dir = Direction::DOWN;
        else if (sdir == "A") dir = Direction::ACROSS;
        else {
            std::cerr << "Invalid direction, must be [AD]" << std::endl;
            return;
        }
        int points = board.placeWord(word, x, y, dir);
        if (points > 0) {
            scores[0] += points;
        } else {
            std::cerr << "Invalid move" << std::endl;
        }
    }

    void computerTurn() {
    }

    void round() {
        printBoard();
        humanTurn();
        computerTurn();
    }

public:
    Game() : board() {
        scores[0] = scores[1] = 0;
        racks[0] = racks[1] = { '\0', '\0', '\0', '\0', '\0', '\0', '\0' };
    }

    void play() {
        while (true) {
            round();
        }
    }
};

int main() {
    Game game;
    game.play();


    // std::cout << "Scrabble" << std::endl;

    // Board board;

    // std::cout << board.placeWord("bababab", 5, 10, Direction::DOWN) << std::endl;
    // std::cout << board.placeWord("foo", 0, 0, Direction::ACROSS) << std::endl;
    // std::cout << board.placeWord("bar", 7, 7, Direction::DOWN) << std::endl;
    // std::cout << board.placeWord("box", 8, 5, Direction::DOWN) << std::endl;
    
    // for (char ch = 'A'; ch <= 'Z'; ch++) {
        // std::cout << ch << ": " << board.getCell(8, 7).isValidCross(ch) << std::endl;
    // }

    // std::cout << board.toString();

    return 0;
}
