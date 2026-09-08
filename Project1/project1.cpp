// PROJECT IDENTIFIER: 40FB54C86566B9DDEAB902CC80E8CE85C1261AAD

#include "treasure_hunt.hpp"

#include <algorithm>
#include <cstdlib>
#include <getopt.h>
#include <iostream>
#include <string>

using namespace std;

[[noreturn]] void commandLineError(const string &message) {
    cerr << message << '\n';
    exit(1);
}

void printHelp(const char *command) {
    cout << "Usage: " << command << " [options] < inputfile\n"
         << "Find treasure on a map of water and islands.\n\n"
         << "Options:\n"
         << "  -h, --help                 Print this message and exit\n"
         << "  -c, --captain MODE         Water container: QUEUE or STACK\n"
         << "  -f, --first-mate MODE      Land container: QUEUE or STACK\n"
         << "  -o, --hunt-order ORDER     Discovery order using N, E, S, W\n"
         << "  -v, --verbose              Print hunt progress\n"
         << "  -s, --stats                Print hunt statistics\n"
         << "  -p, --show-path M|L        Print path as a map or list\n";
}

bool helpWasRequested(int argc, char *argv[]) {
    for (int i = 1; i < argc; ++i) {
        const string argument = argv[i];
        if (argument == "--help") {
            return true;
        }
        if (argument.size() >= 2 && argument[0] == '-' && argument[1] != '-' &&
            argument.find('h', 1) != string::npos) {
            return true;
        }
    }
    return false;
}

ContainerMode parseContainer(const char *argument, const string &errorMessage) {
    const string mode = argument;
    if (mode == "QUEUE") {
        return ContainerMode::Queue;
    }
    if (mode == "STACK") {
        return ContainerMode::Stack;
    }
    commandLineError(errorMessage);
}

bool validHuntOrder(const string &order) {
    if (order.size() != 4) {
        return false;
    }
    string sorted = order;
    sort(sorted.begin(), sorted.end());
    return sorted == "ENSW";
}

void getOptions(int argc, char *argv[], Options &options) {
    if (helpWasRequested(argc, argv)) {
        printHelp(argv[0]);
        exit(0);
    }

    opterr = 0;
    int choice = 0;
    int optionIndex = 0;
    bool showPathSeen = false;

    option longOptions[] = {
        {"help", no_argument, nullptr, 'h'},
        {"captain", required_argument, nullptr, 'c'},
        {"first-mate", required_argument, nullptr, 'f'},
        {"hunt-order", required_argument, nullptr, 'o'},
        {"verbose", no_argument, nullptr, 'v'},
        {"stats", no_argument, nullptr, 's'},
        {"show-path", required_argument, nullptr, 'p'},
        {nullptr, 0, nullptr, '\0'},
    };

    while ((choice = getopt_long(argc, argv, "hc:f:o:vsp:", longOptions,
                                 &optionIndex)) != -1) {
        switch (choice) {
        case 'h':
            printHelp(argv[0]);
            exit(0);
        case 'c':
            options.captain = parseContainer(optarg, "Invalid argument to --captain");
            break;
        case 'f':
            options.firstMate =
                parseContainer(optarg, "Invalid argument to --first-mate");
            break;
        case 'o':
            options.huntOrder = optarg;
            if (!validHuntOrder(options.huntOrder)) {
                commandLineError("Invalid argument to --hunt-order");
            }
            break;
        case 'v':
            options.verbose = true;
            break;
        case 's':
            options.stats = true;
            break;
        case 'p': {
            if (showPathSeen) {
                commandLineError("Specify --show-path only once");
            }
            showPathSeen = true;
            const string pathArgument = optarg;
            if (pathArgument.size() != 1 ||
                (pathArgument[0] != 'M' && pathArgument[0] != 'L')) {
                commandLineError("Invalid argument to --show-path");
            }
            options.showPath = true;
            options.pathMode = pathArgument[0];
            break;
        }
        default:
            commandLineError("Unknown option");
        }
    }
}

int main(int argc, char *argv[]) {
    ios_base::sync_with_stdio(false);

    Options options;
    getOptions(argc, argv, options);

    TreasureHunt treasureHunt(options);
    treasureHunt.readInput();
    treasureHunt.hunt();
    treasureHunt.printOutput();
    return 0;
}
