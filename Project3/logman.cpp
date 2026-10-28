// Project Identifier: 01BD41C3BF016AD7E8B6F837DF18926EC3261350

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <numeric>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

using namespace std;

namespace {

// A master log with 2^32 entries would already exceed practical memory limits.
using EntryId = uint32_t;

struct Entry {
    string timestamp;
    string category;
    string message;
    string category_lower;
    uint64_t time = 0;
};

char lower_ascii(char ch) {
    return ch >= 'A' && ch <= 'Z' ? static_cast<char>(ch - 'A' + 'a') : ch;
}

bool alphanumeric(char ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
           (ch >= '0' && ch <= '9');
}

string lowercase(string value) {
    for (char &ch : value) ch = lower_ascii(ch);
    return value;
}

uint64_t timestamp_value(const string &value) {
    uint64_t result = 0;
    for (char ch : value) {
        if (ch >= '0' && ch <= '9') {
            result = result * 10 + static_cast<unsigned>(ch - '0');
        }
    }
    return result;
}

bool parse_position(const string &argument, size_t limit, size_t &position) {
    if (argument.empty() || argument[0] == '-') return false;
    istringstream input(argument);
    unsigned long long value = 0;
    char extra = 0;
    if (!(input >> value) || (input >> extra) || value >= limit) return false;
    position = static_cast<size_t>(value);
    return true;
}

class LogManager {
  public:
    bool load(const string &filename) {
        ifstream input(filename);
        if (input.fail()) {
            cerr << "Could not open master log file\n";
            return false;
        }

        string line;
        while (getline(input, line)) {
            if (entries.size() == numeric_limits<EntryId>::max()) {
                cerr << "Master log has too many entries\n";
                return false;
            }
            const size_t first = line.find('|');
            const size_t second = line.find('|', first + 1);
            Entry entry;
            entry.timestamp = line.substr(0, first);
            entry.category = line.substr(first + 1, second - first - 1);
            entry.message = line.substr(second + 1);
            entry.category_lower = lowercase(entry.category);
            entry.time = timestamp_value(entry.timestamp);
            entries.push_back(std::move(entry));
        }

        ordered.resize(entries.size());
        iota(ordered.begin(), ordered.end(), EntryId{0});
        sort(ordered.begin(), ordered.end(), [this](EntryId lhs, EntryId rhs) {
            if (entries[lhs].time != entries[rhs].time)
                return entries[lhs].time < entries[rhs].time;
            if (entries[lhs].category_lower != entries[rhs].category_lower)
                return entries[lhs].category_lower < entries[rhs].category_lower;
            return lhs < rhs;
        });

        rank.resize(entries.size());
        for (size_t index = 0; index < ordered.size(); ++index) {
            const EntryId id = ordered[index];
            rank[id] = static_cast<EntryId>(index);
            categories[entries[id].category_lower].push_back(id);
            index_words(entries[id].category, id);
            index_words(entries[id].message, id);
        }
        cout << entries.size() << " entries read\n";
        return true;
    }

    void run() {
        string line;
        while (true) {
            cout << "% ";
            if (!getline(cin, line)) break;
            if (line.empty()) {
                cerr << "Unknown command\n";
                continue;
            }
            const char command = line[0];
            const string argument = line.size() > 2 ? line.substr(2) : string();
            if (command == 'q') break;
            if (command == '#') continue;
            execute(command, argument);
        }
    }

  private:
    vector<Entry> entries;
    vector<EntryId> ordered;
    vector<EntryId> rank;
    unordered_map<string, vector<EntryId>> categories;
    unordered_map<string, vector<EntryId>> keywords;
    vector<EntryId> excerpt;
    vector<EntryId> keyword_result;
    const vector<EntryId> empty_result;
    const vector<EntryId> *last_source = nullptr;
    size_t last_begin = 0;
    size_t last_end = 0;

    void index_words(const string &text, EntryId id) {
        string word;
        for (size_t index = 0; index <= text.size(); ++index) {
            const char ch = index < text.size() ? text[index] : ' ';
            if (alphanumeric(ch)) {
                word.push_back(lower_ascii(ch));
            } else if (!word.empty()) {
                vector<EntryId> &postings = keywords[word];
                if (postings.empty() || postings.back() != id) postings.push_back(id);
                word.clear();
            }
        }
    }

    static vector<string> words(const string &text) {
        vector<string> result;
        string word;
        for (size_t index = 0; index <= text.size(); ++index) {
            const char ch = index < text.size() ? text[index] : ' ';
            if (alphanumeric(ch)) {
                word.push_back(lower_ascii(ch));
            } else if (!word.empty()) {
                result.push_back(std::move(word));
                word.clear();
            }
        }
        return result;
    }

    void save_search(const vector<EntryId> &source, size_t begin, size_t end) {
        last_source = &source;
        last_begin = begin;
        last_end = end;
    }

    size_t result_count() const { return last_end - last_begin; }

    void print_entry(EntryId id) const {
        const Entry &entry = entries[id];
        cout << id << '|' << entry.timestamp << '|' << entry.category << '|'
             << entry.message << '\n';
    }

    void print_excerpt_entry(size_t position) const {
        cout << position << '|';
        print_entry(excerpt[position]);
    }

    void print_summary() const {
        print_excerpt_entry(0);
        cout << "...\n";
        print_excerpt_entry(excerpt.size() - 1);
    }

    void timestamp_search(const string &argument, bool range) {
        string first = argument;
        string second = argument;
        if (range) {
            const size_t separator = argument.find('|');
            if (separator == string::npos) {
                cerr << "Invalid timestamp range\n";
                return;
            }
            first = argument.substr(0, separator);
            second = argument.substr(separator + 1);
        }
        if (first.size() != 14 || second.size() != 14) {
            cerr << "Timestamps must be 14 characters\n";
            return;
        }

        const uint64_t start = timestamp_value(first);
        const uint64_t finish = timestamp_value(second);
        const auto lower = lower_bound(ordered.begin(), ordered.end(), start,
                                       [this](EntryId id, uint64_t time) {
                                           return entries[id].time < time;
                                       });
        const auto upper = upper_bound(ordered.begin(), ordered.end(), finish,
                                       [this](uint64_t time, EntryId id) {
                                           return time < entries[id].time;
                                       });
        save_search(ordered, static_cast<size_t>(lower - ordered.begin()),
                    static_cast<size_t>(upper - ordered.begin()));
        cout << (range ? "Timestamps search: " : "Timestamp search: ")
             << result_count() << " entries found\n";
    }

    void category_search(const string &argument) {
        const auto found = categories.find(lowercase(argument));
        const vector<EntryId> &result = found == categories.end() ? empty_result : found->second;
        save_search(result, 0, result.size());
        cout << "Category search: " << result_count() << " entries found\n";
    }

    void keyword_search(const string &argument) {
        const vector<string> query = words(argument);
        unordered_set<string> seen;
        keyword_result.clear();
        bool first_word = true;
        for (const string &word : query) {
            if (!seen.insert(word).second) continue;
            const auto found = keywords.find(word);
            if (first_word) {
                first_word = false;
                if (found != keywords.end()) keyword_result = found->second;
            } else if (!keyword_result.empty()) {
                if (found == keywords.end()) {
                    keyword_result.clear();
                    break;
                }
                vector<EntryId> intersection;
                intersection.reserve(min(keyword_result.size(), found->second.size()));
                set_intersection(keyword_result.begin(), keyword_result.end(),
                                 found->second.begin(), found->second.end(),
                                 back_inserter(intersection),
                                 [this](EntryId lhs, EntryId rhs) {
                                     return rank[lhs] < rank[rhs];
                                 });
                keyword_result.swap(intersection);
            }
        }
        save_search(keyword_result, 0, keyword_result.size());
        cout << "Keyword search: " << result_count() << " entries found\n";
    }

    void execute(char command, const string &argument) {
        size_t position = 0;
        switch (command) {
        case 't': timestamp_search(argument, true); break;
        case 'm': timestamp_search(argument, false); break;
        case 'c': category_search(argument); break;
        case 'k': keyword_search(argument); break;
        case 'a':
            if (!parse_position(argument, entries.size(), position)) {
                cerr << "Invalid log entry ID\n";
                break;
            }
            excerpt.push_back(static_cast<EntryId>(position));
            cout << "log entry " << position << " appended\n";
            break;
        case 'r':
            if (!last_source) {
                cerr << "No previous search\n";
                break;
            }
            excerpt.insert(excerpt.end(), last_source->begin() + static_cast<ptrdiff_t>(last_begin),
                           last_source->begin() + static_cast<ptrdiff_t>(last_end));
            cout << result_count() << " log entries appended\n";
            break;
        case 'd':
        case 'b':
        case 'e':
            if (!parse_position(argument, excerpt.size(), position)) {
                cerr << "Invalid excerpt list position\n";
                break;
            }
            if (command == 'd') {
                excerpt.erase(excerpt.begin() + static_cast<ptrdiff_t>(position));
                cout << "Deleted excerpt list entry " << position << '\n';
            } else {
                const EntryId id = excerpt[position];
                excerpt.erase(excerpt.begin() + static_cast<ptrdiff_t>(position));
                excerpt.insert(command == 'b' ? excerpt.begin() : excerpt.end(), id);
                cout << "Moved excerpt list entry " << position << '\n';
            }
            break;
        case 's':
            cout << "excerpt list sorted\n";
            if (excerpt.empty()) {
                cout << "(previously empty)\n";
            } else {
                cout << "previous ordering:\n";
                print_summary();
                sort(excerpt.begin(), excerpt.end(), [this](EntryId lhs, EntryId rhs) {
                    return rank[lhs] < rank[rhs];
                });
                cout << "new ordering:\n";
                print_summary();
            }
            break;
        case 'l':
            cout << "excerpt list cleared\n";
            if (excerpt.empty()) {
                cout << "(previously empty)\n";
            } else {
                cout << "previous contents:\n";
                print_summary();
                excerpt.clear();
            }
            break;
        case 'g':
            if (!last_source) {
                cerr << "No previous search\n";
                break;
            }
            for (size_t index = last_begin; index < last_end; ++index)
                print_entry((*last_source)[index]);
            break;
        case 'p':
            for (size_t index = 0; index < excerpt.size(); ++index)
                print_excerpt_entry(index);
            break;
        default: cerr << "Unknown command\n"; break;
        }
    }
};

} // namespace

int main(int argc, char **argv) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    string filename;
    for (int index = 1; index < argc; ++index) {
        const string option = argv[index];
        if (option == "-h" || option == "--help") {
            cout << "Usage: logman -f LOGFILE\n"
                 << "Search a log and manage an excerpt list.\n";
            return 0;
        }
        if ((option == "-f" || option == "--file") && index + 1 < argc)
            filename = argv[++index];
    }
    LogManager manager;
    if (!manager.load(filename)) return 1;
    manager.run();
    return 0;
}
