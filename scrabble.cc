#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <set>
#include <deque>
#include <random>
#include <chrono>
#include <algorithm>
#include <tuple>
#include <cassert>

#include <sys/ioctl.h>
#include <unistd.h>

winsize size;
int padding = 0;

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

    TrieNode* getRoot() { return root; }
};

Trie* trie = nullptr;

enum Direction { ACROSS = 0, DOWN };

constexpr int POINTS[] = {
    1, 3, 3, 2, 1, 4, 2, 4, 1, 8, 5, 1, 3, 1, 1, 3, 10, 1, 1, 1, 1, 4, 4, 8, 4, 10
};

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
        ret << " \e[1;33m" << letter << points << "\e[0m";
        if (points < 10) ret << " ";
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
            bool update_across = (across_prefix != "" || across_postfix != "");
            bool update_down = (down_prefix != "" || down_postfix != "");
            uint32_t _across_crosses = 0;
            uint32_t _down_crosses = 0;
            for (char ch = 'A'; ch <= 'Z'; ch++) {
                int idx = ch - 'A';
                if (update_across) {
                    if (trie->isLegal(across_prefix + ch + across_postfix)) {
                        _across_crosses |= (1 << idx);
                    }
                }
                if (update_down) {
                    if (trie->isLegal(down_prefix + ch + down_postfix)) {
                        _down_crosses |= (1 << idx);
                    }
                }
            }
            if (update_across) across_crosses = _across_crosses;
            if (update_down) down_crosses = _down_crosses;
            // this->across_crosses = update_across ? _across_crosses : across_crosses;
            // this->down_crosses = update_down ? _down_crosses : down_crosses;
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
    std::stringstream blank_line;

    Cell board[SIZE][SIZE];

    bool empty;

    int getPrefixPoints(int x, int y, Direction dir) {
        int ret = 0;
        if (dir == Direction::ACROSS) x--;
        else if (dir == Direction::DOWN) y--;
        while (!board[y][x].isEmpty() && x >= 0 && x < SIZE && y >= 0 && y < SIZE) {
            ret += board[y][x].getTile().getPoints();
            if (dir == Direction::ACROSS) {
                x--;
            } else if (dir == Direction::DOWN) {
                y--;
            }
        }
        return ret;
    }

    int getPostfixPoints(int x, int y, Direction dir) {
        int ret = 0;
        if (dir == Direction::ACROSS) x++;
        else if (dir == Direction::DOWN) y++;
        while (!board[y][x].isEmpty() && x >= 0 && x < SIZE && y >= 0 && y < SIZE) {
            ret += board[y][x].getTile().getPoints();
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
        }
        if (x < SIZE - 1) {
            board[y][x + 1].updateValidCrosses(trie, 
                            getPrefix(x + 1, y, Direction::ACROSS),
                            getPostfix(x + 1, y, Direction::ACROSS),
                            getPrefix(x + 1, y, Direction::DOWN),
                            getPostfix(x + 1, y, Direction::DOWN));
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

    bool isLegalHelper(std::string word, unsigned int i, int x, int y, Direction dir, std::multiset<char>& rack) {
        if (i >= word.length()) return true;
        char ch = toupper(word[i]);
        Cell cell;
        if (dir == Direction::ACROSS) {
            cell = board[y][x + i];
            if ((!cell.isValidCross(ch, Direction::DOWN) ||
                  (rack.find(ch) == rack.end() && rack.find(' ') == rack.end())) &&
                  cell.getTile().getLetter() != ch) {
                return false;
            }
        } else if (dir == Direction::DOWN) {
            cell = board[y + i][x];
            if ((!cell.isValidCross(ch, Direction::ACROSS) ||
                  (rack.find(ch) == rack.end() && rack.find(' ') == rack.end())) &&
                  cell.getTile().getLetter() != ch) {
                return false;
            }
        }
        bool remove = cell.isEmpty();
        if (remove) {
            auto it = rack.find(ch);
            if (it == rack.end()) it = rack.find(' ');
            assert(it != rack.end());
            ch = *it;
            rack.erase(it);
        }
        bool ret = isLegalHelper(word, i + 1, x, y, dir, rack);
        if (remove) rack.insert(ch);
        return ret;
    }

    bool isLegal(std::string word, int x, int y, Direction dir, std::multiset<char>& rack) {
        bool touches_middle = false;
        bool adjacent = false;
        if (dir == Direction::ACROSS) {
            if (x + word.length() > SIZE) return false;
            for (unsigned int i = 0; i < word.length(); i++) {
                if (x + i == 7 && y == 7) touches_middle = true;
                if ((x > 0 && !board[y][x + i - 1].isEmpty()) ||
                    (x < SIZE - 1 && !board[y][x + i + 1].isEmpty()) ||
                    (y > 0 && !board[y - 1][x + i].isEmpty()) ||
                    (y < SIZE - 1 && !board[y + 1][x + i].isEmpty())) {
                    adjacent = true;
                }
            }
        } else if (dir == Direction::DOWN) {
            if (y + word.length() > SIZE) return false;
            for (unsigned int i = 0; i < word.length(); i++) {
                if (x == 7 && y + i == 7) touches_middle = true;
                if ((x > 0 && !board[y + i][x - 1].isEmpty()) ||
                    (x < SIZE - 1 && !board[y + i][x + 1].isEmpty()) ||
                    (y > 0 && !board[y + i - 1][x].isEmpty()) ||
                    (y < SIZE - 1 && !board[y + i + 1][x].isEmpty())) {
                    adjacent = true;
                }
            }
        }
        if ((empty && !touches_middle) || (!empty && !adjacent)) return false;
        return isLegalHelper(word, 0, x, y, dir, rack);
    }

public:

    Board() : empty(true) {
        // setup blank_line
        for (int i = 0; i < 4 + padding; i++) blank_line << " ";
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

    int placeWord(std::string word, int x, int y, Direction dir, std::multiset<char>& rack, bool sandbox) {
        int word_score = 0;
        int word_mul = 1;
        int tot_score = 0;
        if (!trie->isLegal(word) || !isLegal(word, x, y, dir, rack)) return -1;
        if (dir == Direction::ACROSS) {
            for (unsigned int i = 0; i < word.length(); i++) {
                char ch = toupper(word[i]);
                Cell& cell = board[y][x + i];
                if (cell.isEmpty()) {
                    int points = POINTS[ch - 'A'];
                    int cross_mul = 1;
                    int letter_mul = 1;
                    auto it = rack.find(ch);
                    if (it == rack.end()) {
                        it = rack.find(' ');
                        points = 0;
                    }
                    assert(it != rack.end());
                    switch (cell.getType()) {
                        case Cell::Type::DW: {
                            cross_mul *= 2;
                            word_mul *= 2;
                        } break;
                        case Cell::Type::TW: {
                            cross_mul *= 3;
                            word_mul *= 3;
                        } break;
                        case Cell::Type::DL: {
                            letter_mul *= 2;
                        } break;
                        case Cell::Type::TL: {
                            letter_mul *= 3;
                        } break;
                        default: break;
                    }
                    int cross_score = cross_mul *
                                     (getPrefixPoints(x + i, y, Direction::DOWN) +
                                      getPostfixPoints(x + i, y, Direction::DOWN));
                    if (cross_score > 0) {
                        tot_score += cross_score + cross_mul * letter_mul * points;
                    }
                    word_score += letter_mul * points;

                    if (!sandbox) {
                        rack.erase(it);
                        cell.fill(Tile(ch, points));
                    }
                } else word_score += cell.getTile().getPoints();
            }
        } else if (dir == Direction::DOWN) {
            for (unsigned int i = 0; i < word.length(); i++) {
                char ch = toupper(word[i]);
                Cell& cell = board[y + i][x];
                if (cell.isEmpty()) {
                    int points = POINTS[ch - 'A'];
                    int cross_mul = 1;
                    int letter_mul = 1;
                    auto it = rack.find(ch);
                    if (it == rack.end()) {
                        it = rack.find(' ');
                        points = 0;
                    }
                    assert(it != rack.end());
                    switch (cell.getType()) {
                        case Cell::Type::DW: {
                            cross_mul *= 2;
                            word_mul *= 2;
                        } break;
                        case Cell::Type::TW: {
                            cross_mul *= 3;
                            word_mul *= 3;
                        } break;
                        case Cell::Type::DL: {
                            letter_mul *= 2;
                        } break;
                        case Cell::Type::TL: {
                            letter_mul *= 3;
                        } break;
                        default: break;
                    }
                    int cross_score = cross_mul *
                                     (getPrefixPoints(x, y + i, Direction::ACROSS) +
                                      getPostfixPoints(x, y + i, Direction::ACROSS));
                    if (cross_score > 0) {
                        tot_score += cross_score + cross_mul * letter_mul * points;
                    }
                    word_score += letter_mul * points;

                    if (!sandbox) {
                        rack.erase(it);
                        cell.fill(Tile(ch, points));
                    }
                } else word_score += cell.getTile().getPoints();
            }
        }
        tot_score += word_mul * word_score;
        if (rack.size() == 0) tot_score += 50;
        if (!sandbox) empty = false;
        return tot_score;
    }

    Cell* getCell(int x, int y) { return &board[y][x]; }

    std::string toString() {
        std::stringstream ret;
        for (int i = 0; i < 4 + padding; i++) ret << " ";
        for (int i = 0; i < Board::SIZE; i++) {
            ret << "  " << i / 10 << i % 10 << " ";
        }
        ret << std::endl;
        ret << blank_line.str();
        for (int i = 0; i < SIZE; i++) {
            for (int i = 0; i < padding; i++) ret << " ";
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

class Tilebag {
private:
    std::deque<char> bag;
    std::default_random_engine rand_gen;

public:
    Tilebag() {
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        rand_gen = std::default_random_engine(seed);

        for (int i = 0; i < 9; i++)  bag.push_back('A');
        for (int i = 0; i < 2; i++)  bag.push_back('B');
        for (int i = 0; i < 2; i++)  bag.push_back('C');
        for (int i = 0; i < 4; i++)  bag.push_back('D');
        for (int i = 0; i < 12; i++) bag.push_back('E');
        for (int i = 0; i < 2; i++)  bag.push_back('F');
        for (int i = 0; i < 3; i++)  bag.push_back('G');
        for (int i = 0; i < 2; i++)  bag.push_back('H');
        for (int i = 0; i < 9; i++)  bag.push_back('I');
        for (int i = 0; i < 1; i++)  bag.push_back('J');
        for (int i = 0; i < 1; i++)  bag.push_back('K');
        for (int i = 0; i < 4; i++)  bag.push_back('L');
        for (int i = 0; i < 2; i++)  bag.push_back('M');
        for (int i = 0; i < 6; i++)  bag.push_back('N');
        for (int i = 0; i < 8; i++)  bag.push_back('O');
        for (int i = 0; i < 2; i++)  bag.push_back('P');
        for (int i = 0; i < 1; i++)  bag.push_back('Q');
        for (int i = 0; i < 6; i++)  bag.push_back('R');
        for (int i = 0; i < 4; i++)  bag.push_back('S');
        for (int i = 0; i < 6; i++)  bag.push_back('T');
        for (int i = 0; i < 4; i++)  bag.push_back('U');
        for (int i = 0; i < 2; i++)  bag.push_back('V');
        for (int i = 0; i < 2; i++)  bag.push_back('W');
        for (int i = 0; i < 1; i++)  bag.push_back('X');
        for (int i = 0; i < 2; i++)  bag.push_back('Y');
        for (int i = 0; i < 1; i++)  bag.push_back('Z');
        for (int i = 0; i < 2; i++)  bag.push_back(' ');

        std::shuffle(bag.begin(), bag.end(), rand_gen);
    }

    void draw(std::multiset<char>& rack, int num) {
        num = std::min(static_cast<size_t>(num), bag.size());
        for (int i = 0; i < num; i++) {
            rack.insert(bag.front());
            bag.pop_front();
        }
    }

    size_t size() {
        return bag.size();
    }
};

class Game {
private:
    Board board;
    Tilebag bag;
    int scores[2];
    std::multiset<char> racks[2];

    typedef std::tuple<std::string, int, int, Direction> Option;
    std::set<Option> computer_options;

    enum ComputerMode { EASY = 0, HARD, IMPOSSIBLE };
    ComputerMode difficulty = ComputerMode::HARD;

    void printBoard(bool show_diff) {
        for (int i = 0; i < 50; i++) std::cout << std::endl;

        for (int i = 0; i < 19 + padding; i++) std::cout << " ";
        for (int i = 0; i < 8; i++) std::cout << "|----";
        std::cout << "|" << std::endl;

        for (int i = 0; i < padding; i++) std::cout << " ";
        std::cout << "  Your Score: " << scores[0] / 100 << (scores[0] / 10) % 10
                  << scores[0] % 10 << "  | \e[1;33mS1\e[0m | \e[1;33mC3\e[0m "
                  << "| \e[1;33mR1\e[0m | \e[1;33mA1\e[0m | \e[1;33mB3\e[0m "
                  << "| \e[1;33mB3\e[0m | \e[1;33mL1\e[0m | \e[1;33mE1\e[0m "
                  << "|  Their Score: " << scores[1] / 100
                  << (scores[1] / 10) % 10 << scores[1] % 10 << std::endl;

        for (int i = 0; i < 19 + padding; i++) std::cout << " ";
        for (int i = 0; i < 8; i++) std::cout << "|----";
        std::cout << "|" << std::endl;

        std::cout << std::endl;

        std::string diff_string = "";
        switch (difficulty) {
            case ComputerMode::EASY: {
                diff_string = "EASY MODE";
            } break;
            case ComputerMode::HARD: {
                diff_string = "HARD MODE";
            } break;
            case ComputerMode::IMPOSSIBLE: {
                diff_string = "IMPOSSIBLE MODE";
            } break;
            default: break;
        }
        for (unsigned int i = 0; i < padding + (80 - diff_string.length()) / 2; i++) std::cout << " ";
        if (show_diff) std::cout << "\e[1;33m" << diff_string << "\e[0m" << std::endl;
        else std::cout << std::endl;

        std::cout << std::endl;

        std::cout << board.toString();

        std::cout << std::endl << std::endl;

        for (int i = 0; i < 24 + padding; i++) std::cout << " ";
        for (int i = 0; i < 7; i++) std::cout << "|----";
        std::cout << "|" << std::endl;

        for (int i = 0; i < 24 + padding; i++) std::cout << " ";
        // for (auto it = racks[0].begin(); it != racks[0].end(); it++) {
        for (unsigned int i = 0; i < 7; i++) {
            if (i >= racks[0].size()) {
                std::cout << "|    ";
            } else {
                char ch = *std::next(racks[0].begin(), i);
                std::cout << "| \e[1;33m" << ch;
                if (ch != ' ') {
                    int points = POINTS[ch - 'A'];
                    std::cout << points << "\e[0m";
                    if (points < 10) std::cout << " ";
                }
                else std::cout << " \e[0m ";
            }
        }
        std::cout << "|" << std::endl;

        for (int i = 0; i < 24 + padding; i++) std::cout << " ";
        for (int i = 0; i < 7; i++) std::cout << "|----";
        std::cout << "|" << std::endl;
    }

    void humanTurn() {
        bool done = false;
        while (!done) {
            for (int i = 0; i < 4 + padding; i++) std::cout << " ";
            std::cout << "Enter a move (word, x, y, direction): ";
            std::string move;
            getline(std::cin, move);
            if (move == "PASS") return;
            std::stringstream buf(move);
            std::string word, sdir;
            Direction dir;
            int x, y;
            buf >> word >> x >> y >> sdir;
            if (sdir == "D") dir = Direction::DOWN;
            else if (sdir == "A") dir = Direction::ACROSS;
            else {
                for (int i = 0; i < 4 + padding; i++) std::cout << " ";
                std::cout << "Invalid direction, must be [AD]" << std::endl;
                continue;
            }
            int points = board.placeWord(word, x, y, dir, racks[0], false);
            if (points > 0) {
                scores[0] += points;
                done = true;
            } else {
                for (int i = 0; i < 4 + padding; i++) std::cout << " ";
                std::cout << "Invalid move" << std::endl;
            }
        }

        bag.draw(racks[0], 7 - racks[0].size());
    }

    void extendRight(int x, int y, int anchor_x, int anchor_y, std::string partial, TrieNode* node, Direction dir) {
        if (x < 0 || y < 0 || node == nullptr) return;

        Cell* cell = board.getCell(x, y);
        if (cell->isEmpty()) {
            if (trie->isLegal(partial) && racks[1].size() < 7 &&
                (x != anchor_x || y != anchor_y)) {
                if (dir == Direction::ACROSS) {
                    computer_options.insert(std::make_tuple(partial, x - partial.length(), y, dir));
                } else if (dir == Direction::DOWN) {
                    computer_options.insert(std::make_tuple(partial, x, y - partial.length(), dir));
                }
            }
            for (char ch = 'A'; ch <= 'Z'; ch++) {
                if (node->childAt(ch) != nullptr &&
                   (racks[1].find(ch) != racks[1].end() ||
                    racks[1].find(' ') != racks[1].end())) {
                    Direction cross_dir;
                    if (dir == Direction::ACROSS) {
                        cross_dir = Direction::DOWN;
                    } else if (dir == Direction::DOWN) {
                        cross_dir = Direction::ACROSS;
                    } else assert(0);
                    if (cell->isValidCross(ch, cross_dir)) {
                        auto it = racks[1].find(ch);
                        if (it == racks[1].end()) it = racks[1].find(' ');
                        assert(it != racks[1].end());
                        char removed = *it;
                        racks[1].erase(it);
                        TrieNode* next_node = node->childAt(ch);
                        int next_x = -1, next_y = -1;
                        if (dir == Direction::ACROSS) {
                            next_x = x + 1;
                            next_y = y;
                        } else if (dir == Direction::ACROSS) {
                            next_x = x;
                            next_y = y + 1;
                        }
                        extendRight(next_x, next_y, anchor_x, anchor_y, partial + ch, next_node, dir);
                        racks[1].insert(removed);
                    }
                }
            }
        } else {
            char ch = cell->getTile().getLetter();
            if (node->childAt(ch) != nullptr) {
                TrieNode* next_node = node->childAt(ch);
                int next_x = -1, next_y = -1;
                if (dir == Direction::ACROSS) {
                    next_x = x + 1;
                    next_y = y;
                } else if (dir == Direction::ACROSS) {
                    next_x = x;
                    next_y = y + 1;
                }
                extendRight(next_x, next_y, anchor_x, anchor_y, partial + ch, next_node, dir);
            }
        }
    }

    void leftPart(int x, int y, std::string partial, TrieNode* node, int limit, Direction dir) {
        extendRight(x, y, x, y, partial, node, dir);
        if (limit > 0) {
            for (char ch = 'A'; ch <= 'Z'; ch++) {
                if (node->childAt(ch) != nullptr &&
                   (racks[1].find(ch) != racks[1].end() ||
                    racks[1].find(' ') != racks[1].end())) {
                    auto it = racks[1].find(ch);
                    if (it == racks[1].end()) it = racks[1].find(' ');
                    assert(it != racks[1].end());
                    char removed = *it;
                    racks[1].erase(it);
                    TrieNode* child = node->childAt(ch);
                    leftPart(x, y, partial + ch, child, limit - 1, dir);
                    racks[1].insert(removed);
                }
            }
        }
    }

    void genWords(int x, int y, int limit, Direction dir) {
        TrieNode* node = trie->getRoot();
        if ((dir == Direction::ACROSS && !board.getCell(x - 1, y)->isEmpty()) ||
            (dir == Direction::DOWN && !board.getCell(x, y - 1)->isEmpty())) {
            std::string prefix = board.getPrefix(x, y, dir);
            for (unsigned int i = 0; i < prefix.length(); i++) {
                assert(node != nullptr);
                node = node->childAt(prefix[i]);
            }
            extendRight(x, y, x, y, prefix, node, dir);
        }
        else leftPart(x, y, "", node, limit, dir);
    }

    void computerTurn() {
        // compute across anchors
        for (int y = 0; y < Board::SIZE; y++) {
            int last_anchor_x = 0;
            for (int x = 0; x < Board::SIZE; x++) {
                if (board.getCell(x, y)->isEmpty() &&
                    ((x > 0 && !board.getCell(x - 1, y)->isEmpty()) ||
                    (x < Board::SIZE - 1 && !board.getCell(x + 1, y)->isEmpty()) ||
                    (y > 0 && !board.getCell(x, y - 1)->isEmpty()) ||
                    (y < Board::SIZE - 1 && !board.getCell(x, y + 1)->isEmpty()))) {
                    genWords(x, y, x - last_anchor_x - 1, Direction::ACROSS);
                    last_anchor_x = x;
                }
            }
        }

        // compute down anchors
        for (int x = 0; x < Board::SIZE; x++) {
            int last_anchor_y = 0;
            for (int y = 0; y < Board::SIZE; y++) {
                if (board.getCell(x, y)->isEmpty() &&
                    ((x > 0 && !board.getCell(x - 1, y)->isEmpty()) ||
                    (x < Board::SIZE - 1 && !board.getCell(x + 1, y)->isEmpty()) ||
                    (y > 0 && !board.getCell(x, y - 1)->isEmpty()) ||
                    (y < Board::SIZE - 1 && !board.getCell(x, y + 1)->isEmpty()))) {
                    genWords(x, y, y - last_anchor_y - 1, Direction::DOWN);
                    last_anchor_y = y;
                }
            }
        }

        // pick highest-scoring option
        Option best_option = std::make_tuple("", -1, -1, Direction::ACROSS);
        int best_points = 0;
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        std::default_random_engine rand_engine(seed);
        std::uniform_int_distribution<size_t> rand_dist(0, computer_options.size() - 1);
        int consider_num = 0;
        switch (difficulty) {
            case ComputerMode::EASY: {
                consider_num = computer_options.size() / 4;
            } break;
            case ComputerMode::HARD: {
                consider_num = computer_options.size() / 2;
            } break;
            case ComputerMode::IMPOSSIBLE: {
                consider_num = computer_options.size();
            } break;
        }
        for (int i = 0; i < consider_num; i++) {
            Option option = *std::next(computer_options.begin(), rand_dist(rand_engine));
            std::string word = std::get<0>(option);
            int x = std::get<1>(option);
            int y = std::get<2>(option);
            Direction dir = std::get<3>(option);
            int points = board.placeWord(word, x, y, dir, racks[1], true);
            if (points > best_points) {
                best_points = points;
                best_option = option;
            }
        }
        if (best_points > 0) {
            std::string word = std::get<0>(best_option);
            int x = std::get<1>(best_option);
            int y = std::get<2>(best_option);
            Direction dir = std::get<3>(best_option);
            int points = board.placeWord(word, x, y, dir, racks[1], false);
            scores[1] += points;

            bag.draw(racks[1], 7 - racks[1].size());
        }
    }

    void recomputeValidCrosses() {
        for (int x = 0; x < Board::SIZE; x++) {
            for (int y = 0; y < Board::SIZE; y++) {
                board.getCell(x, y)->updateValidCrosses(trie, 
                                     board.getPrefix(x, y, Direction::ACROSS),
                                     board.getPostfix(x, y, Direction::ACROSS),
                                     board.getPrefix(x, y, Direction::DOWN),
                                     board.getPostfix(x, y, Direction::DOWN));
            }
        }
    }

    void round() {
        printBoard(true);
        humanTurn();
        recomputeValidCrosses();
        computerTurn();
        recomputeValidCrosses();
    }

public:
    Game(ComputerMode difficulty) : board(), difficulty(difficulty) {
        trie = new Trie("dict.txt");
        scores[0] = scores[1] = 0;
        racks[0] = racks[1] = {};
    }

    Game() : Game(ComputerMode::HARD) {}

    void play() {
        printBoard(false);

        for (int i = 0; i < 4 + padding; i++) std::cout << " ";
        std::cout << "What difficulty would you like?" << std::endl;
        for (int i = 0; i < 4 + padding; i++) std::cout << " ";
        std::cout << "E = Easy, H = Hard, I = Impossible" << std::endl;
        bool done = false;
        while (!done) {
            for (int i = 0; i < 4 + padding; i++) std::cout << " ";
            std::string d;
            getline(std::cin, d);
            switch (toupper(d[0])) {
                case 'E': {
                    difficulty = ComputerMode::EASY;
                    done = true;
                } break;
                case 'H': {
                    difficulty = ComputerMode::HARD;
                    done = true;
                } break;
                case 'I': {
                    difficulty = ComputerMode::IMPOSSIBLE;
                    done = true;
                } break;
                default: {
                    for (int i = 0; i < 4 + padding; i++) std::cout << " ";
                    std::cout << "Invalid difficulty, try again." << std::endl;
                } break;
            }
        }

        std::cout << std::flush;

        bag.draw(racks[0], 7);
        bag.draw(racks[1], 7);
        while (bag.size() > 0 || (racks[0].size() > 0 && racks[1].size() > 0)) {
            round();
        }

        for (char ch : racks[0]) {
            if (ch != ' ') {
                int points = POINTS[ch - 'A'];
                scores[0] -= points;
                scores[1] += points;
            }
        }

        for (char ch : racks[1]) {
            if (ch != ' ') {
                int points = POINTS[ch - 'A'];
                scores[0] += points;
                scores[1] -= points;
            }
        }

        printBoard(true);

        for (int i = 0; i < padding; i++) std::cout << " ";
        if (scores[0] > scores[1]) {
            std::cout << "You win!" << std::endl;
        } else if (scores[0] < scores[1]) {
            std::cout << "You lose!" << std::endl;
        } else {
            std::cout << "A tie!" << std::endl;
        }
    }
};

int main() {
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
    padding = (size.ws_col - 80) / 2;

    Game game;
    game.play();

    return 0;
}
