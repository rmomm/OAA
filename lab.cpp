#include <iostream>
#include <string>
#include <algorithm>
#include <map>
#include <vector>

using namespace std;

enum class CommandType {
    CREATE, INSERT, PRINT_INDEX, SEARCH, UNKNOWN
};

struct Command {
    CommandType type = CommandType::UNKNOWN;
    string collectionName;
    string text;
    string query;
};

struct Document {
    string text;
};

map<string, vector<Document>> collection;


string trim(const string& s) {
    size_t a = s.find_first_not_of(" \t\n\r");
    size_t b = s.find_last_not_of(" \t\n\r");
    if (a == string::npos) return "";
    return s.substr(a, b - a + 1);
}

Command parseCommand(const string& input) {
    Command cmd;

    string s = trim(input);
    if (s.empty() || s.back() != ';')
        return cmd;

    s.pop_back();
    s = trim(s);

    size_t pos = s.find(' ');
    string keyword = (pos == string::npos) ? s : s.substr(0, pos);
    string rest = (pos == string::npos) ? "" : trim(s.substr(pos + 1));

    transform(keyword.begin(), keyword.end(), keyword.begin(), ::toupper);

    if (keyword == "CREATE") cmd.type = CommandType::CREATE;
    else if (keyword == "INSERT") cmd.type = CommandType::INSERT;
    else if (keyword == "PRINT_INDEX") cmd.type = CommandType::PRINT_INDEX;
    else if (keyword == "SEARCH") cmd.type = CommandType::SEARCH;
    else cmd.type = CommandType::UNKNOWN;

    if (cmd.type == CommandType::CREATE) {
        cmd.collectionName = rest;
        return cmd;
    }

    if (cmd.type == CommandType::PRINT_INDEX) {
        cmd.collectionName = rest;
        return cmd;
    }

    if (cmd.type == CommandType::INSERT) {
        size_t p = rest.find(' ');
        if (p == string::npos) return cmd;

        cmd.collectionName = rest.substr(0, p);

        string doc = trim(rest.substr(p + 1));

        if (doc.size() >= 2 && doc.front() == '"' && doc.back() == '"')
            cmd.text = doc.substr(1, doc.size() - 2);

        return cmd;
    }

    if (cmd.type == CommandType::SEARCH) {
        size_t p = rest.find(' ');
        if (p == string::npos) {
            cmd.collectionName = rest;
            return cmd;
        }

        cmd.collectionName = rest.substr(0, p);
        string tail = trim(rest.substr(p + 1));

        string pref = tail.substr(0, 5);
        transform(pref.begin(), pref.end(), pref.begin(), ::toupper);

        if (pref == "WHERE")
            cmd.query = trim(tail.substr(5));

        return cmd;
    }

    return cmd;
}

int main() {
    string input;

    cout << "Enter commands:" << endl;

    while (true) {
        getline(cin, input);
        if (input == "exit") break;

        Command cmd = parseCommand(input);

        switch (cmd.type) {

        case CommandType::CREATE:
            if (collection.find(cmd.collectionName) != collection.end()) {
                cout << "Collection already exists\n";
                break;
            }
            collection[cmd.collectionName] = {};
            cout << "Collection " << cmd.collectionName << " has been created\n";
            break;

        case CommandType::INSERT:
            if (collection.find(cmd.collectionName) == collection.end()) {
                cout << "Collection does not exist\n";
                break;
            }
            collection[cmd.collectionName].push_back({ cmd.text });
            cout << "Document added to " << cmd.collectionName << " \n";
            break;

        case CommandType::PRINT_INDEX:
            if (collection.find(cmd.collectionName) == collection.end()) {
                cout << "Collection does not exist\n";
                break;
            }
            cout << "Documents in " << cmd.collectionName << ":\n";
            if (collection[cmd.collectionName].empty()) {
                cout << " (empty)\n";
            }
            else {
                for (const auto& d : collection[cmd.collectionName])
                    cout << " - " << d.text << "\n";
            }
            break;

        case CommandType::SEARCH:
            if (collection.find(cmd.collectionName) == collection.end()) {
                cout << "Collection does not exist\n";
                break;
            }
            cout << "SEARCH in " << cmd.collectionName << " ";
            if (!cmd.query.empty()) cout << " WHERE " << cmd.query;
            cout << "\n";
            break;

        default:
            cout << "Unknown or invalid command\n";
        }
    }
}