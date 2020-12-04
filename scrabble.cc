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

    static constexpr int POINTS[] = {
        1, 3, 3, 2, 1, 4, 2, 4, 1, 8, 5, 1, 3, 1, 1, 3, 10, 1, 1, 1, 1, 4, 4, 8, 4, 10
    };

    Cell board[SIZE][SIZE];

public:
    enum Direction { ACROSS = 0, DOWN };

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

    int placeWord(std::string word, int x, int y, Direction dir) {
        std::cerr << "warning: placeWord not considering legality or cross-words" << std::endl;
        int score = 0;
        if (dir == Direction::ACROSS) {
            if (x + word.length() >= SIZE) return 0;
            for (unsigned int i = 0; i < word.length(); i++) {
                char ch = toupper(word[i]);
                int points = POINTS[ch - 'A'];
                board[y][x + i].fill(new Tile(ch, points));
                score += points;
            }
        } else if (dir == Direction::DOWN) {
            if (y + word.length() >= SIZE) return 0;
            for (unsigned int i = 0; i < word.length(); i++) {
                char ch = toupper(word[i]);
                int points = POINTS[ch - 'A'];
                board[y + i][x].fill(new Tile(ch, points));
                score += points;
            }
        }
        return score;
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
    std::cout << "Scrabble" << std::endl;

    Trie* trie = new Trie("dict.txt");
    std::vector<std::pair<std::string, bool>> tests = {{"foo", false}, {"bar", true}, {"this", true}, {"is", true}, {"a", false}, {"trie", true}, {"thi", false}, {"tri", false}, {"fo", false}, {"ba", true}, {"nonsense", true}, {"garbage", true}};
    for (std::pair<std::string, bool> test : tests) {
        if (trie->isLegal(test.first) != test.second) std::cout << "Failed test {" << test.first << ", " << test.second << "}" << std::endl;
    }

    Board board{};

    // std::cout << board.toString() << std::endl;

    board.placeWord("foo", 0, 0, Board::Direction::ACROSS);
    board.placeWord("bar", 7, 7, Board::Direction::DOWN);

    std::cout << board.toString() << std::endl;

    return 0;
}
