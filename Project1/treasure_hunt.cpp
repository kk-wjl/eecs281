// PROJECT IDENTIFIER: 40FB54C86566B9DDEAB902CC80E8CE85C1261AAD

#include "treasure_hunt.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>



using namespace std;

namespace {
[[noreturn]] void inputError(const string &message) {
    cerr << message << '\n';
    exit(1);
}
}  // namespace

bool operator==(const Coordinate &lhs, const Coordinate &rhs) {
    return lhs.row == rhs.row && lhs.col == rhs.col;
}

TreasureHunt::TreasureHunt(const Options &optionsIn) : options(optionsIn) {}

void TreasureHunt::readInput() {
    string line;
    char inputMode = '\0';
    while (getline(cin, line)) {
        if (!line.empty() && line[0] == '#') {
            continue;
        }
        if (!line.empty()) {
            inputMode = line[0];
            break;
        }
    }

    cin >> mapSize;
    map.assign(mapSize, vector<Tile>(mapSize));

    if (inputMode == 'M') {
        readMapMode();
    } else if (inputMode == 'L') {
        readListMode();
    }

    if (!hasTreasure) {
        inputError("Map does not have treasure");
    }
    if (!hasStart) {
        inputError("Map does not have a start location");
    }
}

void TreasureHunt::hunt() {
    if (options.verbose) {
        cout << "Treasure hunt started at: " << start.row << ',' << start.col
             << '\n';
    }

    deque<Coordinate> sailContainer;
    map[start.row][start.col].parentDirection = '@';
    sailContainer.push_back(start);

    while (!sailContainer.empty() && !treasureFound) {
        const Coordinate current = takeNext(sailContainer, options.captain);
        ++waterInvestigated;

        for (const char direction : options.huntOrder) {
            Coordinate next;
            if (!neighbor(current, direction, next)) {
                continue;
            }

            Tile &nextTile = map[next.row][next.col];
            if (nextTile.parentDirection != '\0' || nextTile.terrain == '#') {
                continue;
            }

            if (nextTile.terrain == '.') {
                discover(next, direction);
                sailContainer.push_back(next);
            } else if (isLand(nextTile.terrain)) {
                discover(next, direction);
                ++timesAshore;
                if (options.verbose) {
                    cout << "Went ashore at: " << next.row << ',' << next.col
                         << '\n';
                }

                firstMateHunt(next);
                if (options.verbose) {
                    if (treasureFound) {
                        cout << "Searching island... party found treasure at "
                             << treasure.row << ',' << treasure.col << ".\n";
                    } else {
                        cout << "Searching island... party returned with no treasure.\n";
                    }
                }

                if (treasureFound) {
                    break;
                }
            }
        }
    }

    if (!treasureFound && options.verbose) {
        cout << "Treasure hunt failed\n";
    }
    if (treasureFound) {
        reconstructPath();
    }
}

void TreasureHunt::printOutput() const {
    if (options.stats) {
        printStats();
    }
    if (treasureFound && options.showPath) {
        if (options.pathMode == 'M') {
            printPathMap();
        } else {
            printPathList();
        }
    }
    printResult();
}

bool TreasureHunt::validTerrain(char terrain) {
    return terrain == '.' || terrain == 'o' || terrain == '#' ||
           terrain == '@' || terrain == '$';
}

bool TreasureHunt::isLand(char terrain) {
    return terrain == 'o' || terrain == '$';
}

void TreasureHunt::readMapMode() {
    for (uint32_t row = 0; row < mapSize; ++row) {
        string terrainRow;
        cin >> terrainRow;
        if (terrainRow.size() != mapSize) {
            inputError("Invalid terrain type");
        }
        for (uint32_t col = 0; col < mapSize; ++col) {
            if (!validTerrain(terrainRow[col])) {
                inputError("Invalid terrain type");
            }
            const char terrain = terrainRow[col];
            map[row][col].terrain = terrain;
            if (terrain == '@') {
                start = {row, col};
                hasStart = true;
            } else if (terrain == '$') {
                treasure = {row, col};
                hasTreasure = true;
            }
        }
    }
}

void TreasureHunt::readListMode() {
    uint32_t row = 0;
    uint32_t col = 0;
    char terrain = '\0';
    while (cin >> row >> col >> terrain) {
        if (row >= mapSize || col >= mapSize) {
            inputError("Invalid coordinates in list mode input");
        }
        if (!validTerrain(terrain)) {
            inputError("Invalid terrain type");
        }
        map[row][col].terrain = terrain;
        if (terrain == '@') {
            start = {row, col};
            hasStart = true;
        } else if (terrain == '$') {
            treasure = {row, col};
            hasTreasure = true;
        }
    }
}

Coordinate TreasureHunt::takeNext(deque<Coordinate> &container,
                                  ContainerMode mode) const {
    Coordinate next;
    if (mode == ContainerMode::Queue) {
        next = container.front();
        container.pop_front();
    } else {
        next = container.back();
        container.pop_back();
    }
    return next;
}

bool TreasureHunt::neighbor(const Coordinate &current, char direction,
                            Coordinate &next) const {
    next = current;
    switch (direction) {
    case 'N':
        if (current.row == 0) {
            return false;
        }
        --next.row;
        break;
    case 'E':
        if (current.col + 1 >= mapSize) {
            return false;
        }
        ++next.col;
        break;
    case 'S':
        if (current.row + 1 >= mapSize) {
            return false;
        }
        ++next.row;
        break;
    case 'W':
        if (current.col == 0) {
            return false;
        }
        --next.col;
        break;
    default:
        return false;
    }
    return true;
}

char TreasureHunt::opposite(char direction) {
    switch (direction) {
    case 'N':
        return 'S';
    case 'E':
        return 'W';
    case 'S':
        return 'N';
    case 'W':
        return 'E';
    default:
        return '\0';
    }
}

void TreasureHunt::discover(const Coordinate &location,
                            char directionFromParent) {
    Tile &tile = map[location.row][location.col];
    tile.parentDirection = opposite(directionFromParent);
}

void TreasureHunt::firstMateHunt(const Coordinate &landing) {
    deque<Coordinate> searchContainer;
    searchContainer.push_back(landing);

    if (map[landing.row][landing.col].terrain == '$') {
        ++landInvestigated;
        treasureFound = true;
        treasure = landing;
        return;
    }

    while (!searchContainer.empty()) {
        const Coordinate current = takeNext(searchContainer, options.firstMate);
        ++landInvestigated;

        for (const char direction : options.huntOrder) {
            Coordinate next;
            if (!neighbor(current, direction, next)) {
                continue;
            }

            Tile &nextTile = map[next.row][next.col];
        if (nextTile.parentDirection != '\0' || !isLand(nextTile.terrain)) {
                continue;
            }

            discover(next, direction);
            searchContainer.push_back(next);
            if (nextTile.terrain == '$') {
                ++landInvestigated;
                treasureFound = true;
                treasure = next;
                return;
            }
        }
    }
}

Coordinate TreasureHunt::parentOf(const Coordinate &child) const {
    Coordinate parent = child;
    const char direction = map[child.row][child.col].parentDirection;
    switch (direction) {
    case 'N':
        --parent.row;
        break;
    case 'E':
        ++parent.col;
        break;
    case 'S':
        ++parent.row;
        break;
    case 'W':
        --parent.col;
        break;
    default:
        break;
    }
    return parent;
}

void TreasureHunt::reconstructPath() {
    Coordinate current = treasure;
    if (options.showPath) {
        path.push_back(current);
    }

    while (!(current == start)) {
        current = parentOf(current);
        ++finalPathLength;
        if (options.showPath) {
            path.push_back(current);
        }
    }

    if (options.showPath) {
        reverse(path.begin(), path.end());
    }
}

size_t TreasureHunt::pathLength() const {
    return finalPathLength;
}

void TreasureHunt::printStats() const {
    cout << "--- STATS ---\n"
         << "Starting location: " << start.row << ',' << start.col << '\n'
         << "Water locations investigated: " << waterInvestigated << '\n'
         << "Land locations investigated: " << landInvestigated << '\n'
         << "Went ashore: " << timesAshore << '\n';
    if (treasureFound) {
        cout << "Path length: " << pathLength() << '\n'
             << "Treasure location: " << treasure.row << ',' << treasure.col
             << '\n';
    }
    cout << "--- STATS ---\n";
}

void TreasureHunt::printPathList() const {
    cout << "Sail:\n";
    bool searchLabelPrinted = false;
    for (const Coordinate &location : path) {
        if (!searchLabelPrinted &&
            isLand(map[location.row][location.col].terrain)) {
            cout << "Search:\n";
            searchLabelPrinted = true;
        }
        cout << location.row << ',' << location.col << '\n';
    }
}

void TreasureHunt::printPathMap() const {
    vector<string> outputMap(mapSize, string(mapSize, '.'));
    for (uint32_t row = 0; row < mapSize; ++row) {
        for (uint32_t col = 0; col < mapSize; ++col) {
            outputMap[row][col] = map[row][col].terrain;
        }
    }

    for (size_t i = 1; i + 1 < path.size(); ++i) {
        const Coordinate &previous = path[i - 1];
        const Coordinate &current = path[i];
        const Coordinate &next = path[i + 1];
        if (previous.row == next.row) {
            outputMap[current.row][current.col] = '-';
        } else if (previous.col == next.col) {
            outputMap[current.row][current.col] = '|';
        } else {
            outputMap[current.row][current.col] = '+';
        }
    }
    outputMap[treasure.row][treasure.col] = 'X';

    for (const string &row : outputMap) {
        cout << row << '\n';
    }
}

void TreasureHunt::printResult() const {
    if (treasureFound) {
        cout << "Treasure found at " << treasure.row << ',' << treasure.col
             << " with path length " << pathLength() << ".\n";
    } else {
        cout << "No treasure found after investigating "
             << waterInvestigated + landInvestigated << " locations.\n";
    }
}
