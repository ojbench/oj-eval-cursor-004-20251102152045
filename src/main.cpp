// Bookstore Management System - Minimal but functional implementation
// File-backed storage, no long-lived in-memory main datasets

#include <bits/stdc++.h>
using namespace std;

namespace fsutil {
    static const string kAccountsFile = "accounts.db";
    static const string kBooksFile = "books.db";
    static const string kFinanceFile = "finance.db";
    static const string kOpsLogFile = "ops.log";

    inline bool fileExists(const string &path) {
        FILE *f = fopen(path.c_str(), "rb");
        if (!f) return false;
        fclose(f);
        return true;
    }

    inline bool writeAll(const string &path, const string &data) {
        FILE *f = fopen(path.c_str(), "wb");
        if (!f) return false;
        size_t w = fwrite(data.data(), 1, data.size(), f);
        fclose(f);
        return w == data.size();
    }

    inline bool appendLine(const string &path, const string &line) {
        FILE *f = fopen(path.c_str(), "ab");
        if (!f) return false;
        size_t w = fwrite(line.data(), 1, line.size(), f);
        fclose(f);
        return w == line.size();
    }

    inline vector<string> readAllLines(const string &path) {
        vector<string> lines;
        FILE *f = fopen(path.c_str(), "rb");
        if (!f) return lines;
        string cur;
        const size_t BUFSZ = 1 << 16;
        vector<char> buf(BUFSZ);
        while (true) {
            size_t r = fread(buf.data(), 1, BUFSZ, f);
            if (r == 0) break;
            for (size_t i = 0; i < r; ++i) {
                char c = buf[i];
                if (c == '\n') {
                    lines.push_back(cur);
                    cur.clear();
                } else if (c != '\r') {
                    cur.push_back(c);
                }
            }
        }
        fclose(f);
        if (!cur.empty()) lines.push_back(cur);
        return lines;
    }
}

namespace strutil {
    inline string ltrim(const string &s) {
        size_t i = 0; while (i < s.size() && isspace(static_cast<unsigned char>(s[i]))) ++i; return s.substr(i);
    }
    inline string rtrim(const string &s) {
        if (s.empty()) return s;
        size_t i = s.size();
        while (i > 0 && isspace(static_cast<unsigned char>(s[i-1]))) --i;
        return s.substr(0, i);
    }
    inline string trim(const string &s) { return rtrim(ltrim(s)); }

    inline vector<string> split(const string &s, char delim) {
        vector<string> out; string cur;
        for (char c : s) {
            if (c == delim) { out.push_back(cur); cur.clear(); }
            else { cur.push_back(c); }
        }
        out.push_back(cur);
        return out;
    }

    inline string escapeField(const string &s) {
        string t; t.reserve(s.size());
        for (char c : s) {
            if (c == '\\') { t += "\\\\"; }
            else if (c == '\t') { t += "\\t"; }
            else if (c == '\n') { t += "\\n"; }
            else { t += c; }
        }
        return t;
    }
    inline string unescapeField(const string &s) {
        string t; t.reserve(s.size());
        for (size_t i = 0; i < s.size(); ++i) {
            char c = s[i];
            if (c == '\\' && i + 1 < s.size()) {
                char n = s[i+1];
                if (n == 't') { t.push_back('\t'); ++i; }
                else if (n == 'n') { t.push_back('\n'); ++i; }
                else if (n == '\\') { t.push_back('\\'); ++i; }
                else { t.push_back(c); }
            } else {
                t.push_back(c);
            }
        }
        return t;
    }

    inline bool isASCIIVisible(char c) {
        unsigned char uc = static_cast<unsigned char>(c);
        return uc >= 32 && uc <= 126;
    }

    inline bool isUserIdOrPasswordValid(const string &s) {
        if (s.empty() || s.size() > 30) return false;
        for (char c : s) {
            if (!(isdigit(static_cast<unsigned char>(c)) || isalpha(static_cast<unsigned char>(c)) || c == '_')) return false;
        }
        return true;
    }

    inline bool isUsernameValid(const string &s) {
        if (s.empty() || s.size() > 30) return false;
        for (char c : s) {
            if (!isASCIIVisible(c)) return false;
        }
        return true;
    }

    inline bool isISBNValid(const string &s) {
        if (s.empty() || s.size() > 20) return false;
        for (char c : s) if (!isASCIIVisible(c)) return false;
        return true;
    }
    inline bool isBookNameOrAuthorValid(const string &s) {
        if (s.size() > 60) return false;
        for (char c : s) {
            if (!isASCIIVisible(c) || c == '"') return false;
        }
        return true;
    }
    inline bool isKeywordValid(const string &s) {
        if (s.size() > 60) return false;
        for (char c : s) {
            if (!isASCIIVisible(c) || c == '"') return false;
        }
        return true;
    }

    inline bool parseInt(const string &s, long long &out) {
        if (s.empty()) return false;
        if (s.size() > 1 && s[0] == '+') return false;
        long long sign = 1; size_t i = 0;
        if (s[0] == '-') { sign = -1; i = 1; }
        if (i >= s.size()) return false;
        long long v = 0;
        for (; i < s.size(); ++i) {
            char c = s[i];
            if (!isdigit(static_cast<unsigned char>(c))) return false;
            v = v * 10 + (c - '0');
            if (v > LLONG_MAX / 2) { /* rough overflow guard */ }
        }
        out = v * sign;
        return true;
    }

    inline bool parseMoneyToCents(const string &s, long long &cents) {
        // Accept forms: D, D.D, D.DD ; non-negative
        if (s.empty()) return false;
        if (s[0] == '+') return false;
        size_t pos = s.find('.');
        string a = s, b = "";
        if (pos != string::npos) { a = s.substr(0, pos); b = s.substr(pos + 1); }
        long long ia = 0;
        if (a.empty()) ia = 0; else {
            for (char c : a) if (!isdigit(static_cast<unsigned char>(c))) return false;
            if (!a.empty()) {
                // strip leading zeros ok
                for (char c : a) ia = ia * 10 + (c - '0');
            }
        }
        if (b.size() > 2) return false;
        long long ib = 0;
        for (char c : b) if (!isdigit(static_cast<unsigned char>(c))) return false;
        if (b.size() == 1) ib = (b[0] - '0') * 10;
        else if (b.size() == 2) ib = (b[0] - '0') * 10 + (b[1] - '0');
        cents = ia * 100 + ib;
        return true;
    }

    inline string centsToMoney(long long cents) {
        bool neg = cents < 0; if (neg) cents = -cents;
        long long a = cents / 100; long long b = cents % 100;
        string s = to_string(a) + "." + (b < 10 ? string("0") + to_string(b) : to_string(b));
        if (neg) s = "-" + s;
        return s;
    }
}

struct Account {
    string userId;
    string password;
    int privilege = 1;
    string username;
    bool active = true;
};

struct Book {
    string isbn;
    string name;
    string author;
    string keywords; // segments separated by '|'
    long long priceCents = 0;
    long long stock = 0;
};

enum class TxType { BUY, IMPORT };
struct Tx { TxType type; long long amountCents; };

namespace store {
    using namespace fsutil; using namespace strutil;

    inline vector<Account> readAllAccounts() {
        vector<Account> out;
        if (!fileExists(kAccountsFile)) return out;
        auto lines = readAllLines(kAccountsFile);
        for (auto &ln : lines) {
            if (ln.empty()) continue;
            auto parts = split(ln, '\t');
            if (parts.size() < 5) continue;
            Account a;
            a.userId = unescapeField(parts[0]);
            a.password = unescapeField(parts[1]);
            long long p = 1; parseInt(parts[2], p); a.privilege = (int)p;
            a.username = unescapeField(parts[3]);
            a.active = (parts[4] == "1");
            out.push_back(a);
        }
        return out;
    }

    inline bool writeAllAccounts(const vector<Account>& accs) {
        string data;
        for (auto &a : accs) {
            data += escapeField(a.userId);
            data += '\t';
            data += escapeField(a.password);
            data += '\t';
            data += to_string(a.privilege);
            data += '\t';
            data += escapeField(a.username);
            data += '\t';
            data += a.active ? "1" : "0";
            data += '\n';
        }
        return writeAll(kAccountsFile, data);
    }

    inline vector<Book> readAllBooks() {
        vector<Book> out;
        if (!fileExists(kBooksFile)) return out;
        auto lines = readAllLines(kBooksFile);
        for (auto &ln : lines) {
            if (ln.empty()) continue;
            auto parts = split(ln, '\t');
            if (parts.size() < 6) continue;
            Book b;
            b.isbn = unescapeField(parts[0]);
            b.name = unescapeField(parts[1]);
            b.author = unescapeField(parts[2]);
            b.keywords = unescapeField(parts[3]);
            long long pc = 0; parseInt(parts[4], pc); b.priceCents = pc;
            long long st = 0; parseInt(parts[5], st); b.stock = st;
            out.push_back(b);
        }
        return out;
    }

    inline bool writeAllBooks(const vector<Book>& books) {
        string data;
        for (auto &b : books) {
            data += escapeField(b.isbn); data += '\t';
            data += escapeField(b.name); data += '\t';
            data += escapeField(b.author); data += '\t';
            data += escapeField(b.keywords); data += '\t';
            data += to_string(b.priceCents); data += '\t';
            data += to_string(b.stock); data += '\n';
        }
        return writeAll(kBooksFile, data);
    }

    inline vector<Tx> readAllTx() {
        vector<Tx> out; if (!fileExists(kFinanceFile)) return out;
        auto lines = readAllLines(kFinanceFile);
        for (auto &ln : lines) {
            if (ln.empty()) continue;
            auto parts = split(ln, '\t');
            if (parts.size() != 2) continue;
            Tx t; t.type = (parts[0] == string("BUY") ? TxType::BUY : TxType::IMPORT);
            long long v = 0; strutil::parseInt(parts[1], v); t.amountCents = v;
            out.push_back(t);
        }
        return out;
    }

    inline bool appendTx(const Tx &t) {
        string line = (t.type == TxType::BUY ? "BUY" : "IMPORT");
        line += '\t'; line += to_string(t.amountCents); line += '\n';
        return appendLine(kFinanceFile, line);
    }

    inline void ensureInitialized() {
        if (!fileExists(kAccountsFile)) {
            Account root; root.userId = "root"; root.password = "sjtu"; root.privilege = 7; root.username = "root"; root.active = true;
            writeAllAccounts({root});
        }
        if (!fileExists(kBooksFile)) {
            writeAll(kBooksFile, "");
        }
        if (!fileExists(kFinanceFile)) {
            writeAll(kFinanceFile, "");
        }
        if (!fileExists(kOpsLogFile)) {
            writeAll(kOpsLogFile, "");
        }
    }

    inline void appendOpLog(const string &user, const string &rawCmd) {
        // timestamp optional; keep concise
        string u = user.empty() ? string("guest") : user;
        string line = u + "\t" + rawCmd + "\n";
        appendLine(kOpsLogFile, line);
    }

    inline vector<pair<string,string>> readOpLog() {
        vector<pair<string,string>> out; if (!fileExists(kOpsLogFile)) return out;
        auto lines = readAllLines(kOpsLogFile);
        for (auto &ln : lines) {
            auto p = split(ln, '\t');
            if (p.size() >= 2) {
                out.emplace_back(p[0], ln.substr(p[0].size()+1));
            }
        }
        return out;
    }
}

struct SessionUser {
    string userId;
    int privilege = 0;
};

struct RuntimeState {
    vector<SessionUser> loginStack;
    vector<string> selectedISBN; // per stack frame

    SessionUser current() const { return loginStack.empty() ? SessionUser{} : loginStack.back(); }
    bool isLoggedIn() const { return !loginStack.empty(); }
};

namespace parser {
    using namespace strutil;

    inline vector<string> tokenize(const string &line) {
        vector<string> tokens; string cur; bool inQuotes = false;
        for (size_t i = 0; i < line.size(); ++i) {
            char c = line[i];
            if (c == '"') { inQuotes = !inQuotes; }
            else if (!inQuotes && isspace(static_cast<unsigned char>(c))) {
                if (!cur.empty()) { tokens.push_back(cur); cur.clear(); }
            } else {
                cur.push_back(c);
            }
        }
        if (!cur.empty()) tokens.push_back(cur);
        return tokens;
    }
}

class Engine {
public:
    Engine() { store::ensureInitialized(); }

    void run(istream &in, ostream &out) {
        string line;
        while (std::getline(in, line)) {
            string raw = line;
            line = strutil::trim(line);
            if (line.empty()) { // Commands containing only spaces are legal and produce no output
                continue;
            }

            auto tokens = parser::tokenize(line);
            if (tokens.empty()) continue;
            string cmd = tokens[0];

            if (cmd == "quit" || cmd == "exit") {
                // Terminate normally
                break;
            }

            bool ok = false;
            string output;
            if (cmd == "su") ok = cmd_su(tokens);
            else if (cmd == "logout") ok = cmd_logout();
            else if (cmd == "register") ok = cmd_register(tokens);
            else if (cmd == "passwd") ok = cmd_passwd(tokens);
            else if (cmd == "useradd") ok = cmd_useradd(tokens);
            else if (cmd == "delete") ok = cmd_delete(tokens);
            else if (cmd == "show" && tokens.size() >= 2 && tokens[1] == "finance") ok = cmd_show_finance(tokens, output);
            else if (cmd == "show") ok = cmd_show(tokens, output);
            else if (cmd == "buy") ok = cmd_buy(tokens, output);
            else if (cmd == "select") ok = cmd_select(tokens);
            else if (cmd == "modify") ok = cmd_modify(tokens);
            else if (cmd == "import") ok = cmd_import(tokens);
            else if (cmd == "log") ok = cmd_log(output);
            else if (cmd == "report") ok = cmd_report(tokens, output);
            else {
                ok = false;
            }

            if (!ok) {
                out << "Invalid\n";
            } else {
                if (!output.empty()) out << output;
            }

            // Append op log for auditable commands
            store::appendOpLog(state.isLoggedIn() ? state.current().userId : string(), raw);
        }
    }

private:
    RuntimeState state;

    // Helpers
    static bool findAccountById(const string &uid, Account &acc, size_t &index) {
        auto all = store::readAllAccounts();
        for (size_t i = 0; i < all.size(); ++i) {
            if (all[i].userId == uid && all[i].active) { acc = all[i]; index = i; return true; }
        }
        return false;
    }
    static bool findBookByISBN(const string &isbn, Book &book, size_t &index) {
        auto all = store::readAllBooks();
        for (size_t i = 0; i < all.size(); ++i) {
            if (all[i].isbn == isbn) { book = all[i]; index = i; return true; }
        }
        return false;
    }

    bool requirePrivilege(int need) const {
        return state.current().privilege >= need;
    }

    // Commands
    bool cmd_su(const vector<string>& t) {
        // su [UserID] ([Password])?
        if (t.size() != 2 && t.size() != 3) return false;
        string uid = t[1];
        if (!strutil::isUserIdOrPasswordValid(uid)) return false;
        Account acc; size_t idx = 0;
        if (!findAccountById(uid, acc, idx)) return false;

        if (t.size() == 3) {
            string pw = t[2];
            if (pw != acc.password) return false;
            state.loginStack.push_back({acc.userId, acc.privilege});
            state.selectedISBN.push_back("");
            return true;
        } else {
            // no password provided: only allowed if current account's privilege is higher than target
            if (!state.isLoggedIn()) return false;
            if (state.current().privilege > acc.privilege) {
                state.loginStack.push_back({acc.userId, acc.privilege});
                state.selectedISBN.push_back("");
                return true;
            }
            return false;
        }
    }

    bool cmd_logout() {
        // {1}
        if (!requirePrivilege(1)) return false;
        if (!state.isLoggedIn()) return false;
        state.loginStack.pop_back();
        state.selectedISBN.pop_back();
        return true;
    }

    bool cmd_register(const vector<string>& t) {
        // {0} register [UserID] [Password] [Username]
        if (t.size() != 4) return false;
        string uid = t[1], pw = t[2], uname = t[3];
        if (!strutil::isUserIdOrPasswordValid(uid) || !strutil::isUserIdOrPasswordValid(pw) || !strutil::isUsernameValid(uname)) return false;
        auto all = store::readAllAccounts();
        for (auto &a : all) if (a.active && a.userId == uid) return false;
        all.push_back({uid, pw, 1, uname, true});
        return store::writeAllAccounts(all);
    }

    bool cmd_passwd(const vector<string>& t) {
        // {1} passwd [UserID] ([CurrentPassword])? [NewPassword]
        if (!requirePrivilege(1)) return false;
        if (t.size() != 3 && t.size() != 4) return false;
        string uid = t[1];
        string curpw, newpw;
        if (t.size() == 3) {
            if (!requirePrivilege(7)) return false; // only superuser can omit current password
            newpw = t[2];
        } else {
            curpw = t[2]; newpw = t[3];
        }
        if (!strutil::isUserIdOrPasswordValid(uid) || !strutil::isUserIdOrPasswordValid(newpw) || (!curpw.empty() && !strutil::isUserIdOrPasswordValid(curpw))) return false;
        auto all = store::readAllAccounts();
        bool found = false;
        for (auto &a : all) {
            if (a.active && a.userId == uid) {
                if (curpw.empty() || a.password == curpw) {
                    a.password = newpw; found = true; break;
                } else {
                    return false;
                }
            }
        }
        if (!found) return false;
        return store::writeAllAccounts(all);
    }

    bool cmd_useradd(const vector<string>& t) {
        // {3} useradd [UserID] [Password] [Privilege] [Username]
        if (!requirePrivilege(3)) return false;
        if (t.size() != 5) return false;
        string uid = t[1], pw = t[2], pr = t[3], uname = t[4];
        long long priv = 0; if (!strutil::parseInt(pr, priv)) return false;
        if (!(priv == 1 || priv == 3 || priv == 7)) return false;
        if (priv >= state.current().privilege) return false;
        if (!strutil::isUserIdOrPasswordValid(uid) || !strutil::isUserIdOrPasswordValid(pw) || !strutil::isUsernameValid(uname)) return false;
        auto all = store::readAllAccounts();
        for (auto &a : all) if (a.active && a.userId == uid) return false;
        all.push_back({uid, pw, (int)priv, uname, true});
        return store::writeAllAccounts(all);
    }

    bool cmd_delete(const vector<string>& t) {
        // {7} delete [UserID]
        if (!requirePrivilege(7)) return false;
        if (t.size() != 2) return false;
        string uid = t[1];
        auto all = store::readAllAccounts();
        bool found = false;
        for (auto &a : all) if (a.active && a.userId == uid) { found = true; break; }
        if (!found) return false;
        // cannot delete if logged in
        for (auto &s : state.loginStack) if (s.userId == uid) return false;
        for (auto &a : all) if (a.userId == uid) a.active = false;
        return store::writeAllAccounts(all);
    }

    static bool parseShowArgs(const vector<string>& t, string &field, string &value) {
        // show (-ISBN=[ISBN] | -name="[BookName]" | -author="[Author]" | -keyword="[Keyword]")?
        field.clear(); value.clear();
        if (t.size() == 1) return true;
        if (t.size() != 2) return false;
        string arg = t[1];
        auto pos = arg.find('=');
        if (pos == string::npos) return false;
        field = arg.substr(0, pos);
        value = arg.substr(pos + 1);
        // strip optional quotes around value
        if (value.size() >= 2 && value.front() == '"' && value.back() == '"') value = value.substr(1, value.size() - 2);
        if (field != "-ISBN" && field != "-name" && field != "-author" && field != "-keyword") return false;
        if (value.empty()) return false;
        return true;
    }

    bool cmd_show(const vector<string>& t, string &out) {
        // {1}
        if (!requirePrivilege(1)) return false;
        string field, val;
        if (!parseShowArgs(t, field, val)) return false;
        auto books = store::readAllBooks();
        vector<Book> matched;
        for (auto &b : books) {
            bool ok = true;
            if (!field.empty()) {
                if (field == "-ISBN") ok = (b.isbn == val);
                else if (field == "-name") ok = (b.name == val);
                else if (field == "-author") ok = (b.author == val);
                else if (field == "-keyword") {
                    if (val.find('|') != string::npos) return false; // multiple keywords not allowed
                    // check segment exact match
                    auto segs = strutil::split(b.keywords, '|');
                    ok = false;
                    for (auto &s : segs) { if (s == val) { ok = true; break; } }
                }
            }
            if (ok) matched.push_back(b);
        }
        sort(matched.begin(), matched.end(), [](const Book& a, const Book& b){ return a.isbn < b.isbn; });
        if (matched.empty()) { out += "\n"; return true; }
        for (auto &b : matched) {
            out += b.isbn; out += '\t';
            out += b.name; out += '\t';
            out += b.author; out += '\t';
            out += b.keywords; out += '\t';
            out += strutil::centsToMoney(b.priceCents); out += '\t';
            out += to_string(b.stock); out += '\n';
        }
        return true;
    }

    bool cmd_buy(const vector<string>& t, string &out) {
        // {1} buy [ISBN] [Quantity]
        if (!requirePrivilege(1)) return false;
        if (t.size() != 3) return false;
        string isbn = t[1]; long long qty = 0;
        if (!strutil::isISBNValid(isbn)) return false;
        if (!strutil::parseInt(t[2], qty) || qty <= 0) return false;
        auto books = store::readAllBooks();
        bool found = false; for (auto &b : books) if (b.isbn == isbn) { found = true; break; }
        if (!found) return false;
        for (auto &b : books) if (b.isbn == isbn) {
            if (b.stock < qty) return false;
            b.stock -= qty;
            long long total = b.priceCents * qty;
            if (!store::writeAllBooks(books)) return false;
            store::appendTx({TxType::BUY, total});
            out += strutil::centsToMoney(total); out += '\n';
            return true;
        }
        return false;
    }

    bool cmd_select(const vector<string>& t) {
        // {3} select [ISBN]
        if (!requirePrivilege(3)) return false;
        if (t.size() != 2) return false;
        string isbn = t[1]; if (!strutil::isISBNValid(isbn)) return false;
        auto books = store::readAllBooks();
        bool found = false; for (auto &b : books) if (b.isbn == isbn) { found = true; break; }
        if (!found) {
            // create new book with only ISBN
            Book nb; nb.isbn = isbn; books.push_back(nb);
            if (!store::writeAllBooks(books)) return false;
        }
        if (!state.isLoggedIn()) return false; // should not happen because requirePrivilege(3)
        state.selectedISBN.back() = isbn;
        return true;
    }

    static bool parseModifyArgs(const vector<string>& t, map<string,string> &kv) {
        // modify (-ISBN=[ISBN] | -name="[BookName]" | -author="[Author]" | -keyword="[Keyword]" | -price=[Price])+
        if (t.size() < 2) return false;
        kv.clear();
        for (size_t i = 1; i < t.size(); ++i) {
            auto &arg = t[i];
            auto pos = arg.find('='); if (pos == string::npos) return false;
            string key = arg.substr(0, pos); string val = arg.substr(pos+1);
            if (val.size() >= 2 && val.front() == '"' && val.back() == '"') val = val.substr(1, val.size()-2);
            if (kv.count(key)) return false; // duplicate params illegal
            if (key != "-ISBN" && key != "-name" && key != "-author" && key != "-keyword" && key != "-price") return false;
            if (val.empty()) return false;
            kv[key] = val;
        }
        return !kv.empty();
    }

    bool cmd_modify(const vector<string>& t) {
        if (!requirePrivilege(3)) return false;
        if (!state.isLoggedIn()) return false;
        if (state.selectedISBN.back().empty()) return false;
        map<string,string> kv; if (!parseModifyArgs(t, kv)) return false;
        auto books = store::readAllBooks();
        size_t idx = books.size();
        for (size_t i = 0; i < books.size(); ++i) if (books[i].isbn == state.selectedISBN.back()) { idx = i; break; }
        if (idx == books.size()) return false;
        Book b = books[idx];
        // apply changes with validation
        if (kv.count("-ISBN")) {
            string newIsbn = kv["-ISBN"]; if (!strutil::isISBNValid(newIsbn)) return false;
            if (newIsbn == b.isbn) return false; // cannot change to original ISBN
            for (auto &x : books) if (x.isbn == newIsbn) return false; // existing
            b.isbn = newIsbn;
        }
        if (kv.count("-name")) { string v = kv["-name"]; if (!strutil::isBookNameOrAuthorValid(v)) return false; b.name = v; }
        if (kv.count("-author")) { string v = kv["-author"]; if (!strutil::isBookNameOrAuthorValid(v)) return false; b.author = v; }
        if (kv.count("-keyword")) {
            string v = kv["-keyword"]; if (!strutil::isKeywordValid(v)) return false;
            // check duplicate segments
            auto segs = strutil::split(v, '|');
            set<string> seen; for (auto &s : segs) { if (s.empty() || seen.count(s)) return false; seen.insert(s); }
            b.keywords = v;
        }
        if (kv.count("-price")) {
            long long cents = 0; if (!strutil::parseMoneyToCents(kv["-price"], cents)) return false; b.priceCents = cents;
        }
        books[idx] = b;
        bool ok = store::writeAllBooks(books);
        if (ok && kv.count("-ISBN")) state.selectedISBN.back() = b.isbn;
        return ok;
    }

    bool cmd_import(const vector<string>& t) {
        // {3} import [Quantity] [TotalCost]
        if (!requirePrivilege(3)) return false;
        if (!state.isLoggedIn()) return false;
        if (state.selectedISBN.back().empty()) return false;
        if (t.size() != 3) return false;
        long long qty = 0; if (!strutil::parseInt(t[1], qty) || qty <= 0) return false;
        long long cents = 0; if (!strutil::parseMoneyToCents(t[2], cents) || cents <= 0) return false;
        auto books = store::readAllBooks();
        for (auto &b : books) if (b.isbn == state.selectedISBN.back()) {
            b.stock += qty;
            if (!store::writeAllBooks(books)) return false;
            store::appendTx({TxType::IMPORT, cents});
            return true;
        }
        return false;
    }

    bool cmd_show_finance(const vector<string>& t, string &out) {
        // {7} show finance ([Count])?
        if (!requirePrivilege(7)) return false;
        if (t.size() < 2 || t[1] != string("finance")) return false;
        long long count = -1;
        if (t.size() == 3) { if (!strutil::parseInt(t[2], count) || count < 0) return false; }
        auto tx = store::readAllTx();
        if (count == 0) { out += "\n"; return true; }
        if (count > (long long)tx.size()) return false;
        long long income = 0, expense = 0;
        long long start = 0;
        if (count >= 0) start = (long long)tx.size() - count;
        for (long long i = start; i < (long long)tx.size(); ++i) {
            if (tx[i].type == TxType::BUY) income += tx[i].amountCents; else expense += tx[i].amountCents;
        }
        out += "+ " + strutil::centsToMoney(income) + " - " + strutil::centsToMoney(expense) + "\n";
        return true;
    }

    bool cmd_log(string &out) {
        // {7}
        if (!requirePrivilege(7)) return false;
        auto logs = store::readOpLog();
        for (auto &p : logs) {
            out += p.first + "\t" + p.second + "\n";
        }
        if (logs.empty()) out += "\n";
        return true;
    }

    bool cmd_report(const vector<string>& t, string &out) {
        // {7} report finance | report employee
        if (!requirePrivilege(7)) return false;
        if (t.size() != 2) return false;
        if (t[1] == string("finance")) {
            auto tx = store::readAllTx();
            long long income = 0, expense = 0;
            for (auto &x : tx) { if (x.type == TxType::BUY) income += x.amountCents; else expense += x.amountCents; }
            out += "Total\t+ " + strutil::centsToMoney(income) + "\t- " + strutil::centsToMoney(expense) + "\n";
            return true;
        } else if (t[1] == string("employee")) {
            auto logs = store::readOpLog();
            unordered_map<string, long long> cnt;
            for (auto &p : logs) cnt[p.first]++;
            vector<pair<string,long long>> v(cnt.begin(), cnt.end());
            sort(v.begin(), v.end());
            for (auto &e : v) { out += e.first + "\t" + to_string(e.second) + "\n"; }
            if (v.empty()) out += "\n";
            return true;
        }
        return false;
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    Engine engine;
    engine.run(cin, cout);
    return 0;
}
