// PROJECT IDENTIFIER: 40FB54C86566B9DDEAB902CC80E8CE85C1261AAD

#ifndef TREASURE_HUNT_HPP
#define TREASURE_HUNT_HPP

#include <cstdint>
#include <deque>
#include <string>
#include <vector>

enum class ContainerMode {
    Stack,
    Queue,
};

struct Options {
    ContainerMode captain = ContainerMode::Stack;
    ContainerMode firstMate = ContainerMode::Queue;
    std::string huntOrder = "NESW";
    bool verbose = false;
    bool stats = false;
    bool showPath = false;
    char pathMode = '\0';
};

struct Coordinate {
    std::uint32_t row = 0;
    std::uint32_t col = 0;
};

bool operator==(const Coordinate &lhs, const Coordinate &rhs);

struct Tile {
    char terrain = '.';
    // '\0' means undiscovered; N/E/S/W point back to the parent; '@' marks start.
    char parentDirection = '\0';
};

class TreasureHunt {
  public:
    explicit TreasureHunt(const Options &optionsIn);

    void readInput();
    void hunt();
    void printOutput() const;

  private:
    Options options;
    std::uint32_t mapSize = 0;
    std::vector<std::vector<Tile>> map;
    Coordinate start;
    Coordinate treasure;
    bool hasStart = false;
    bool hasTreasure = false;
    bool treasureFound = false;
    size_t waterInvestigated = 0;
    size_t landInvestigated = 0;
    size_t timesAshore = 0;
    size_t finalPathLength = 0;
    std::vector<Coordinate> path;

    static bool validTerrain(char terrain);
    static bool isLand(char terrain);
    static char opposite(char direction);

    void readMapMode();
    void readListMode();
    Coordinate takeNext(std::deque<Coordinate> &container,
                        ContainerMode mode) const;
    bool neighbor(const Coordinate &current, char direction,
                  Coordinate &next) const;
    void discover(const Coordinate &location, char directionFromParent);
    void firstMateHunt(const Coordinate &landing);
    Coordinate parentOf(const Coordinate &child) const;
    void reconstructPath();
    size_t pathLength() const;
    void printStats() const;
    void printPathList() const;
    void printPathMap() const;
    void printResult() const;
};

#endif
