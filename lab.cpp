#include <iostream>
#include <string>
#include <algorithm>
#include <map>
#include <vector>
#include <regex>
#include <set>

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
    int id;
    string text;
};

map<string, vector<Document>> collection;
map<string, map<string, map<int, vector<int>>>> index;


string trim(const string& s) {
    size_t a = s.find_first_not_of(" \t\n\r");
    size_t b = s.find_last_not_of(" \t\n\r");
    if (a == string::npos) return "";
    return s.substr(a, b - a + 1);
}

string LowerStr(const string& s) {
    string r = s;
    transform(r.begin(), r.end(), r.begin(), [](unsigned char c) { return (char)tolower(c); });
    return r;
}

bool ValidIdentifier(const string& id) {
    if (id.empty()) return false;
    if (!isalpha((unsigned char)id[0])) return false;
    for (char c : id) if (!(isalnum((unsigned char)c) || c == '_')) return false;
    return true;
}

string extractText(const string& s) {
    string t = trim(s);
    size_t f = t.find('"');
    size_t l = t.rfind('"');
    if (f != string::npos && l != string::npos && l > f) {
        return t.substr(f + 1, l - f - 1);
    }
    return t;
}


vector<pair<string, int>> tokenize_with_positions(const string& text) {
    vector<pair<string, int>> out;
    regex word_re("([a-zA-Z0-9_]+)");
    auto it = sregex_iterator(text.begin(), text.end(), word_re);
    auto end = sregex_iterator();
    int pos = 1;
    for (; it != end; ++it) {
        string w = (*it)[1].str();
        transform(w.begin(), w.end(), w.begin(), [](unsigned char c) { return (char)tolower(c); });
        out.emplace_back(w, pos++);
    }
    return out;
}



Command parseCommand(const string& input) {
    Command cmd;

    string s = trim(input);
    if (s.empty() || s.back() != ';')
        return cmd;

    if (s.back() == ';') {
        s.pop_back();
        s = trim(s);
    }

    size_t pos = s.find_first_of(" \t\n\r");
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
        size_t p = rest.find_first_of(" \t\n\r");
        if (p == string::npos) 
            return cmd;
        cmd.collectionName = trim(rest.substr(0, p));
        string tail = trim(rest.substr(p + 1));
        cmd.text = extractText(tail);
        return cmd;
    }

    if (cmd.type == CommandType::SEARCH) {
        size_t p = rest.find_first_of(" \t\n\r");
        if (p == string::npos) {
            cmd.collectionName = trim(rest);
            cmd.query = "";
            return cmd;
        }
        cmd.collectionName = trim(rest.substr(0, p));
        string tail = trim(rest.substr(p + 1));
     
        string pref = tail.substr(0, min<size_t>(5, tail.size()));
        transform(pref.begin(), pref.end(), pref.begin(), [](unsigned char c) { 
            return (char)toupper(c);});

        if (pref == "WHERE") {
            cmd.query = trim(tail.substr(5));
        }
        else {
            cmd.query = tail;
        }
        return cmd;
    }

    return cmd;
}


void handleCreate(const string& name) {
    string n = trim(name);
    if (!ValidIdentifier(n)) {
        cout << "Error: invalid collection name\n";
        return;
    }
    if (collection.count(n)) {
        cout << "Error: collection already exists\n";
        return;
    }
    collection[n] = {};
    index[n] = {};
    cout << "Collection " << n << " has been created\n";
}


void handleInsert(const string& name, const string& text) {
    string n = trim(name);
    if (!ValidIdentifier(n)) {
        cout << "Error: invalid collection name\n";
        return;
    }
    if (!collection.count(n)) {
        cout << "Error: collection does not exist\n";
        return;
    }
    int newId = (int)collection[n].size() + 1;
    collection[n].push_back({ newId, text });

    auto tokens = tokenize_with_positions(text);
    for (auto& pr : tokens) {
        const string& word = pr.first;
        int position = pr.second;
        index[n][word][newId].push_back(position);
    }

    cout << "Document has been added to " << n << "\n";
    cout << " TEXT=[" << text << "]\n";
}


void handlePrintIndex(const string& name) {
    string n = trim(name);
    if (!ValidIdentifier(n)) {
        cout << "Error: invalid collection name\n";
        return;
    }
    if (!collection.count(n)) {
        cout << "Error: collection does not exist\n";
        return;
    }

    auto& idx = index[n];
    if (idx.empty()) {
        cout << "(empty index)\n";
        return;
    }

    for (const auto& wordEntry : idx) {
        cout << "\"" << wordEntry.first << "\":" << "\n";
        for (const auto& docEntry : wordEntry.second) {
            int docID = docEntry.first; 
            const vector<int>& positions = docEntry.second;
            cout << "  d" << docID << " -> [";
            for (size_t i = 0; i < positions.size(); ++i) {
                cout << positions[i];
                if (i + 1 < positions.size()) cout << ", ";
            }
            cout << "]\n";
        }
    }
}


void handleSearchAll(const string& name) {
    string n = trim(name);
    if (!ValidIdentifier(n)) { 
        cout << "Error: invalid collection name\n"; 
        return; 
    }
    if (!collection.count(n)) { 
        cout << "Error: collection does not exist\n";
        return; 
    }
    cout << "All documents:\n";
    for (const auto& doc : collection[n]) {
        cout << "  d" << doc.id << ": \"" << doc.text << "\"\n";
    }
}


void handleSearchKeyword(const string& name, const string& rawKeyword) {
    string n = trim(name);
    if (!ValidIdentifier(n)) { 
        cout << "Error: invalid collection name\n"; 
        return; 
    }

    if (!collection.count(n)) { 
        cout << "Error: collection does not exist\n"; 
        return; 
    }

    string key = extractText(rawKeyword);
    key = LowerStr(trim(key));
    if (key.empty()) { 
        cout << "No documents found\n"; 
        return; 
    }

    auto& idx = index[n];
    if (!idx.count(key)) {
        cout << "No documents found\n";
        return;
    }

    cout << "Documents found:\n";
    for (const auto& entry : idx[key]) {
        int docID = entry.first;
        cout << "  d" << docID << ": \"" << collection[n][docID - 1].text << "\"\n";
    }
}


void handleSearchRange(const string& name, const string& rawA, const string& rawB) {
    string n = trim(name);
    if (!ValidIdentifier(n)) { 
        cout << "Error: invalid collection name\n"; 
        return; 
    }

    if (!collection.count(n)) { 
        cout << "Error: collection does not exist\n"; 
        return; 
    }

    string a = LowerStr(trim(extractText(rawA)));
    string b = LowerStr(trim(extractText(rawB)));
    if (a.empty() || b.empty()) { 
        cout << "No documents found\n"; 
        return; 
    }

    if (a > b) swap(a, b);

    auto& idx = index[n];
    set<int> foundDocs;
    for (auto it = idx.lower_bound(a); it != idx.end() && it->first <= b; ++it) {
        for (const auto& docEntry : it->second) foundDocs.insert(docEntry.first);
    }
    if (foundDocs.empty()) { 
        cout << "No documents found\n"; 
        return; 
    }

    cout << "Documents found:\n";
    for (int docID : foundDocs) {
        cout << "  d" << docID << ": \"" << collection[n][docID - 1].text << "\"\n";
    }
}


void handleSearchDistance(const string& name, const string& rawW1, int N, const string& rawW2) {
    string n = trim(name);
    if (!ValidIdentifier(n)) { 
        cout << "Error: invalid collection name\n"; 
        return; 
    }

    if (!collection.count(n)) { 
        cout << "Error: collection does not exist\n"; 
        return; 
    }

    string w1 = LowerStr(trim(extractText(rawW1)));
    string w2 = LowerStr(trim(extractText(rawW2)));

    if (w1.empty() || w2.empty()) { 
        cout << "No documents found\n";
        return; 
    }

    auto& idx = index[n];
    set<int> resultDocs;

    if (!idx.count(w1) || !idx.count(w2)) { 
        cout << "No documents found\n"; 
        return; 
    }

    auto& map1 = idx[w1];
    auto& map2 = idx[w2];

    for (const auto& p1 : map1) {
        int docID = p1.first;
        if (!map2.count(docID)) continue;
        const vector<int>& pos1 = p1.second;
        const vector<int>& pos2 = map2.at(docID);
        size_t i = 0, j = 0;
        while (i < pos1.size() && j < pos2.size()) {
            int a = pos1[i], b = pos2[j];
            int diff = abs(a - b);
            if (diff <= N) { 
                resultDocs.insert(docID); 
                break; 
            }
            if (a < b) ++i; else ++j;
        }
    }

    if (resultDocs.empty()) { 
        cout << "No documents found\n"; 
        return; 
    }
    cout << "Documents found:\n";
    for (int docID : resultDocs) {
        cout << "  d" << docID << ": \"" << collection[n][docID - 1].text << "\"\n";
    }
}




int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    cout << "Enter commands:" << endl;
    string line, buffer;

    while (true) {
        if (!getline(cin, line))
            break;
        if (line == "exit") 
            break;

        if (!buffer.empty()) buffer += "\n";
        buffer += line;

        size_t semi;
        while ((semi = buffer.find(';')) != string::npos) {
            string cmdText = buffer.substr(0, semi + 1);
            buffer = (semi + 1 < buffer.size()) ? buffer.substr(semi + 1) : string();

            Command cmd = parseCommand(cmdText);

            switch (cmd.type) {
            case CommandType::CREATE:
                handleCreate(cmd.collectionName);
                break;

            case CommandType::INSERT:
                handleInsert(cmd.collectionName, cmd.text);
                break;

            case CommandType::PRINT_INDEX:
                handlePrintIndex(cmd.collectionName);
                break;

            case CommandType::SEARCH: {
                string q = trim(cmd.query);
                if (q.empty()) {
                    handleSearchAll(cmd.collectionName);
                    break;
                }
                size_t posLt = q.find('<');
                size_t posDash = q.find('-');
                if (posLt != string::npos) {
                    string left = trim(q.substr(0, posLt));
                    string rest = trim(q.substr(posLt + 1));
                    size_t sp = rest.find_first_of(" \t\n\r");
                    if (sp == string::npos) { 
                        cout << "Error: invalid distance query\n"; 
                        break; 
                    }
                    string numStr = trim(rest.substr(0, sp));
                    int N = 0;
                    try { N = stoi(numStr); }
                    catch (...) { 
                        cout << "Error: invalid number\n"; 
                        break; 
                    }
                    string right = trim(rest.substr(sp + 1));
                    handleSearchDistance(cmd.collectionName, left, N, right);
                }
                else if (posDash != string::npos) {
                    string left = trim(q.substr(0, posDash));
                    string right = trim(q.substr(posDash + 1));
                    handleSearchRange(cmd.collectionName, left, right);
                }
                else {
                    handleSearchKeyword(cmd.collectionName, q);
                }
                break;
            }

            default:
                cout << "Unknown or invalid command\n";
            }
        }
    }
    return 0;
}