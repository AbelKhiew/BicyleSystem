/* ============================================================
   Implementation of the hand-written JSON engine, shared file
   I/O, date handling, and input validation utilities.
   TEAM COMPONENT - Shared data management
   ============================================================ */
#include "common.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cctype>
#include <ctime>
#include <limits>
#include <iomanip>
#include <stdexcept>
using namespace std;

/* =====================================================================
   JsonValue - construction helpers
   ===================================================================== */
JsonValue JsonValue::makeObject() { JsonValue v; v.type = JsonType::Object; return v; }
JsonValue JsonValue::makeArray() { JsonValue v; v.type = JsonType::Array;  return v; }
JsonValue JsonValue::makeString(const string& s) { JsonValue v; v.type = JsonType::String; v.strVal = s; return v; }
JsonValue JsonValue::makeNumber(double n) { JsonValue v; v.type = JsonType::Number; v.numVal = n; return v; }
JsonValue JsonValue::makeBool(bool b) { JsonValue v; v.type = JsonType::Bool; v.boolVal = b; return v; }

void JsonValue::set(const string& key, const JsonValue& v) {
    for (auto& kv : objVal) {
        if (kv.first == key) { kv.second = v; return; }
    }
    objVal.push_back({ key, v });
}

void JsonValue::push(const JsonValue& v) {
    arrVal.push_back(v);
}

bool JsonValue::has(const string& key) const {
    for (const auto& kv : objVal) if (kv.first == key) return true;
    return false;
}

const JsonValue& JsonValue::get(const string& key) const {
    for (const auto& kv : objVal) if (kv.first == key) return kv.second;
    throw runtime_error("JSON key not found: " + key);
}

string JsonValue::getStr(const string& key, const string& def) const {
    if (!has(key)) return def;
    return get(key).strVal;
}

double JsonValue::getNum(const string& key, double def) const {
    if (!has(key)) return def;
    return get(key).numVal;
}

int JsonValue::getInt(const string& key, int def) const {
    if (!has(key)) return def;
    return (int)get(key).numVal;
}

bool JsonValue::getBool(const string& key, bool def) const {
    if (!has(key)) return def;
    return get(key).boolVal;
}

/* ---------------- JSON serialization (pretty-printed) ---------------- */
static string jsonEscapeString(const string& s) {
    string out;
    for (char c : s) {
        if (c == '"' || c == '\\') { out += '\\'; out += c; }
        else if (c == '\n') { out += "\\n"; }
        else out += c;
    }
    return out;
}

static string numberToJsonString(double n) {
    /* Print whole numbers without a decimal point, otherwise 2 decimal places. */
    if (n == (long long)n) {
        return to_string((long long)n);
    }
    ostringstream oss;
    oss << fixed << setprecision(2) << n;
    return oss.str();
}

string JsonValue::dump(int indent) const {
    string pad(indent * 2, ' ');
    string padInner((indent + 1) * 2, ' ');

    switch (type) {
    case JsonType::Null:   return "null";
    case JsonType::Bool:   return boolVal ? "true" : "false";
    case JsonType::Number: return numberToJsonString(numVal);
    case JsonType::String: return "\"" + jsonEscapeString(strVal) + "\"";

    case JsonType::Array: {
        if (arrVal.empty()) return "[]";
        string out = "[\n";
        for (size_t i = 0; i < arrVal.size(); i++) {
            out += padInner + arrVal[i].dump(indent + 1);
            if (i + 1 < arrVal.size()) out += ",";
            out += "\n";
        }
        out += pad + "]";
        return out;
    }

    case JsonType::Object: {
        if (objVal.empty()) return "{}";
        string out = "{\n";
        for (size_t i = 0; i < objVal.size(); i++) {
            out += padInner + "\"" + jsonEscapeString(objVal[i].first) + "\": "
                + objVal[i].second.dump(indent + 1);
            if (i + 1 < objVal.size()) out += ",";
            out += "\n";
        }
        out += pad + "}";
        return out;
    }
    }
    return "null";
}

/* =====================================================================
   JSON parsing (simple recursive-descent parser)
   ===================================================================== */
namespace {

    struct JsonParser {
        const string& text;
        size_t pos = 0;

        explicit JsonParser(const string& t) : text(t) {}

        void skipWhitespace() {
            while (pos < text.size() && isspace((unsigned char)text[pos])) pos++;
        }

        char peek() { return pos < text.size() ? text[pos] : '\0'; }
        char advance() { return pos < text.size() ? text[pos++] : '\0'; }

        void expect(char c) {
            skipWhitespace();
            if (peek() != c) throw runtime_error(string("JSON parse error: expected '") + c + "'");
            advance();
        }

        JsonValue parseValue() {
            skipWhitespace();
            char c = peek();
            if (c == '{') return parseObject();
            if (c == '[') return parseArray();
            if (c == '"') return JsonValue::makeString(parseString());
            if (c == 't' || c == 'f') return parseBool();
            if (c == 'n') { pos += 4; return JsonValue(); }   // "null"
            return parseNumber();
        }

        JsonValue parseObject() {
            JsonValue obj = JsonValue::makeObject();
            expect('{');
            skipWhitespace();
            if (peek() == '}') { advance(); return obj; }
            while (true) {
                skipWhitespace();
                string key = parseString();
                expect(':');
                JsonValue val = parseValue();
                obj.set(key, val);
                skipWhitespace();
                if (peek() == ',') { advance(); continue; }
                break;
            }
            expect('}');
            return obj;
        }

        JsonValue parseArray() {
            JsonValue arr = JsonValue::makeArray();
            expect('[');
            skipWhitespace();
            if (peek() == ']') { advance(); return arr; }
            while (true) {
                JsonValue val = parseValue();
                arr.push(val);
                skipWhitespace();
                if (peek() == ',') { advance(); continue; }
                break;
            }
            expect(']');
            return arr;
        }

        string parseString() {
            skipWhitespace();
            expect('"');
            string out;
            while (pos < text.size() && text[pos] != '"') {
                char c = text[pos++];
                if (c == '\\' && pos < text.size()) {
                    char next = text[pos++];
                    if (next == 'n') out += '\n';
                    else out += next;
                }
                else {
                    out += c;
                }
            }
            if (pos < text.size()) pos++;   // closing quote
            return out;
        }

        JsonValue parseBool() {
            if (text.compare(pos, 4, "true") == 0) { pos += 4; return JsonValue::makeBool(true); }
            pos += 5;   // "false"
            return JsonValue::makeBool(false);
        }

        JsonValue parseNumber() {
            skipWhitespace();
            size_t start = pos;
            if (peek() == '-') advance();
            while (pos < text.size() && (isdigit((unsigned char)peek()) || peek() == '.')) advance();
            double val = stod(text.substr(start, pos - start));
            return JsonValue::makeNumber(val);
        }
    };

}   // anonymous namespace

JsonValue parseJson(const string& text) {
    if (text.find_first_not_of(" \t\r\n") == string::npos) {
        return JsonValue::makeArray();   // empty/whitespace-only file -> empty array
    }
    JsonParser parser(text);
    return parser.parseValue();
}

static string readFileText(const string& filename) {
    ifstream fin(filename);
    if (!fin.good()) return "";
    stringstream ss;
    ss << fin.rdbuf();
    return ss.str();
}

static void writeFileText(const string& filename, const string& content) {
    ofstream fout(filename);
    fout << content;
    fout.close();
}

void ensureDataFilesExist() {
    const string files[] = { MEMBERS_FILE, BIKETYPES_FILE, RENTALS_FILE };
    for (const auto& filename : files) {
        ifstream checkFile(filename);
        if (!checkFile.good()) {
            checkFile.close();
            writeFileText(filename, "[]");
            cout << "[System] Created new data file: " << filename << "\n";
        }
        else {
            checkFile.close();
        }
    }
}

/* =====================================================================
   Members - JSON array of objects
   ===================================================================== */
vector<Member> loadMembers() {
    vector<Member> members;
    JsonValue arr = parseJson(readFileText(MEMBERS_FILE));
    for (const auto& obj : arr.arrVal) {
        Member m;
        m.id = obj.getInt("id");
        m.name = obj.getStr("name");
        m.password = obj.getStr("password");
        m.phone = obj.getStr("phone");
        m.totalRentals = obj.getInt("totalRentals");
        m.points = obj.getInt("points");
        members.push_back(m);
    }
    return members;
}

void saveMembers(const vector<Member>& members) {
    JsonValue arr = JsonValue::makeArray();
    for (const auto& m : members) {
        JsonValue obj = JsonValue::makeObject();
        obj.set("id", JsonValue::makeNumber(m.id));
        obj.set("name", JsonValue::makeString(m.name));
        obj.set("password", JsonValue::makeString(m.password));
        obj.set("phone", JsonValue::makeString(m.phone));
        obj.set("totalRentals", JsonValue::makeNumber(m.totalRentals));
        obj.set("points", JsonValue::makeNumber(m.points));
        arr.push(obj);
    }
    writeFileText(MEMBERS_FILE, arr.dump());
}

/* =====================================================================
   Bike Types - JSON array of objects
   ===================================================================== */
vector<BikeType> loadBikeTypes() {
    vector<BikeType> bikeTypes;
    JsonValue arr = parseJson(readFileText(BIKETYPES_FILE));
    for (const auto& obj : arr.arrVal) {
        BikeType b;
        b.id = obj.getInt("id");
        b.type = obj.getStr("type");
        b.pricePerDay = obj.getNum("pricePerDay");
        b.tyreCost = obj.getNum("tyreCost");
        b.brakeCost = obj.getNum("brakeCost");
        b.chainCost = obj.getNum("chainCost");
        b.deposit = obj.getNum("deposit");
        b.bookingQuantity = obj.getInt("bookingQuantity");
        b.walkinQuantity = obj.getInt("walkinQuantity");
        b.timesRented = obj.getInt("timesRented");
        bikeTypes.push_back(b);
    }
    return bikeTypes;
}

void saveBikeTypes(const vector<BikeType>& bikeTypes) {
    JsonValue arr = JsonValue::makeArray();
    for (const auto& b : bikeTypes) {
        JsonValue obj = JsonValue::makeObject();
        obj.set("id", JsonValue::makeNumber(b.id));
        obj.set("type", JsonValue::makeString(b.type));
        obj.set("pricePerDay", JsonValue::makeNumber(b.pricePerDay));
        obj.set("tyreCost", JsonValue::makeNumber(b.tyreCost));
        obj.set("brakeCost", JsonValue::makeNumber(b.brakeCost));
        obj.set("chainCost", JsonValue::makeNumber(b.chainCost));
        obj.set("deposit", JsonValue::makeNumber(b.deposit));
        obj.set("bookingQuantity", JsonValue::makeNumber(b.bookingQuantity));
        obj.set("walkinQuantity", JsonValue::makeNumber(b.walkinQuantity));
        obj.set("timesRented", JsonValue::makeNumber(b.timesRented));
        arr.push(obj);
    }
    writeFileText(BIKETYPES_FILE, arr.dump());
}

/* =====================================================================
   Rentals - JSON array of objects, each with a nested "units" array
   ===================================================================== */
vector<Rental> loadRentals() {
    vector<Rental> rentals;
    JsonValue arr = parseJson(readFileText(RENTALS_FILE));
    for (const auto& obj : arr.arrVal) {
        Rental r;
        r.rentalId = obj.getInt("rentalId");
        r.channel = obj.getStr("channel");
        r.memberId = obj.getInt("memberId");
        r.renterName = obj.getStr("renterName");
        r.bikeTypeId = obj.getInt("bikeTypeId");
        r.quantity = obj.getInt("quantity");
        r.rentDate = obj.getStr("rentDate");
        r.code = obj.getStr("code");
        r.pricePerDay = obj.getNum("pricePerDay");
        r.depositPerUnit = obj.getNum("depositPerUnit");
        r.discountApplied = obj.getInt("discountApplied", 0);
        r.discountPointsUsed = obj.getInt("discountPointsUsed", 0);
        r.totalPaid = obj.getNum("totalPaid");
        r.paymentMethod = obj.getStr("paymentMethod");
        r.status = obj.getStr("status", "Active");
        r.pointsEarned = obj.getInt("pointsEarned");
        r.pointsAwarded = obj.getBool("pointsAwarded");
        r.returnDate = obj.getStr("returnDate", "");

        if (obj.has("units")) {
            for (const auto& u : obj.get("units").arrVal) {
                RentalUnit unit;
                unit.unitIndex = u.getInt("unitIndex");
                unit.condition = u.getStr("condition", "-");
                unit.tyreDamaged = u.getBool("tyreDamaged");
                unit.brakeDamaged = u.getBool("brakeDamaged");
                unit.chainDamaged = u.getBool("chainDamaged");
                unit.damageCost = u.getNum("damageCost");
                unit.depositRefunded = u.getNum("depositRefunded");
                unit.returnedOnTime = u.getBool("returnedOnTime", true);
                unit.isReturned = u.getBool("isReturned");
                r.units.push_back(unit);
            }
        }
        rentals.push_back(r);
    }
    return rentals;
}

void saveRentals(const vector<Rental>& rentals) {
    JsonValue arr = JsonValue::makeArray();
    for (const auto& r : rentals) {
        JsonValue obj = JsonValue::makeObject();
        obj.set("rentalId", JsonValue::makeNumber(r.rentalId));
        obj.set("channel", JsonValue::makeString(r.channel));
        obj.set("memberId", JsonValue::makeNumber(r.memberId));
        obj.set("renterName", JsonValue::makeString(r.renterName));
        obj.set("bikeTypeId", JsonValue::makeNumber(r.bikeTypeId));
        obj.set("quantity", JsonValue::makeNumber(r.quantity));
        obj.set("rentDate", JsonValue::makeString(r.rentDate));
        obj.set("code", JsonValue::makeString(r.code));
        obj.set("pricePerDay", JsonValue::makeNumber(r.pricePerDay));
        obj.set("depositPerUnit", JsonValue::makeNumber(r.depositPerUnit));
        obj.set("discountApplied", JsonValue::makeNumber(r.discountApplied));
        obj.set("discountPointsUsed", JsonValue::makeNumber(r.discountPointsUsed));
        obj.set("totalPaid", JsonValue::makeNumber(r.totalPaid));
        obj.set("paymentMethod", JsonValue::makeString(r.paymentMethod));
        obj.set("status", JsonValue::makeString(r.status));
        obj.set("pointsEarned", JsonValue::makeNumber(r.pointsEarned));
        obj.set("pointsAwarded", JsonValue::makeBool(r.pointsAwarded));
        obj.set("returnDate", JsonValue::makeString(r.returnDate));

        JsonValue unitsArr = JsonValue::makeArray();
        for (const auto& u : r.units) {
            JsonValue uo = JsonValue::makeObject();
            uo.set("unitIndex", JsonValue::makeNumber(u.unitIndex));
            uo.set("condition", JsonValue::makeString(u.condition));
            uo.set("tyreDamaged", JsonValue::makeBool(u.tyreDamaged));
            uo.set("brakeDamaged", JsonValue::makeBool(u.brakeDamaged));
            uo.set("chainDamaged", JsonValue::makeBool(u.chainDamaged));
            uo.set("damageCost", JsonValue::makeNumber(u.damageCost));
            uo.set("depositRefunded", JsonValue::makeNumber(u.depositRefunded));
            uo.set("returnedOnTime", JsonValue::makeBool(u.returnedOnTime));
            uo.set("isReturned", JsonValue::makeBool(u.isReturned));
            unitsArr.push(uo);
        }
        obj.set("units", unitsArr);

        arr.push(obj);
    }
    writeFileText(RENTALS_FILE, arr.dump());
}

// id generation utilitie
int getNextMemberId(const vector<Member>& members) {
    int maxId = 0;
    for (const auto& m : members) if (m.id > maxId) maxId = m.id;
    return maxId + 1;
}

int getNextBikeTypeId(const vector<BikeType>& bikeTypes) {
    int maxId = 0;
    for (const auto& b : bikeTypes) if (b.id > maxId) maxId = b.id;
    return maxId + 1;
}

int getNextRentalId(const vector<Rental>& rentals) {
    int maxId = 0;
    for (const auto& r : rentals) if (r.rentalId > maxId) maxId = r.rentalId;
    return maxId + 1;
}

string generateReservationCode() {
    int code = 100000 + (rand() % 900000);
    return to_string(code);
}

int getCurrentHour() {
    time_t now = time(0);
    tm ltm{};
#ifdef _WIN32
    localtime_s(&ltm, &now);
#else
    localtime_r(&now, &ltm);
#endif
    return ltm.tm_hour;
}

string getCurrentDateString() {
    time_t now = time(0);
    tm ltm{};
#ifdef _WIN32
    localtime_s(&ltm, &now);
#else
    localtime_r(&now, &ltm);
#endif
    stringstream ss;
    ss << (ltm.tm_year + 1900) << "-"
        << setw(2) << setfill('0') << (ltm.tm_mon + 1) << "-"
        << setw(2) << setfill('0') << ltm.tm_mday;
    return ss.str();
}

bool isDateBeforeToday(const string& dateStr) {
    return dateStr < getCurrentDateString();
}

bool isValidCalendarDate(int year, int month, int day) {
    if (year < 2000 || year > 2100) return false;
    if (month < 1 || month > 12) return false;
    if (day < 1) return false;

    int daysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    bool isLeap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    if (isLeap) daysInMonth[1] = 29;

    return day <= daysInMonth[month - 1];
}

// Input validation utilities
int getValidatedInt(const string& prompt, int minVal, int maxVal) {
    int value;
    while (true) {
        cout << prompt;
        if (cin >> value && value >= minVal && value <= maxVal) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return value;
        }
        if (cin.eof()) { cout << "\nNo more input. Exiting program.\n"; exit(0); }
        cout << "Invalid input. Please enter a number between "
            << minVal << " and " << maxVal << ".\n";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
}

double getValidatedDouble(const string& prompt, double minVal, double maxVal) {
    double value;
    while (true) {
        cout << prompt;
        if (cin >> value && value >= minVal && value <= maxVal) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return value;
        }
        if (cin.eof()) { cout << "\nNo more input. Exiting program.\n"; exit(0); }
        cout << "Invalid input. Please enter a value between "
            << minVal << " and " << maxVal << ".\n";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
}

string getValidatedString(const string& prompt) {
    string value;
    while (true) {
        cout << prompt;
        getline(cin, value);
        if (!value.empty()) return value;
        cout << "Input cannot be empty. Please try again.\n";
    }
}

// Accepts 't'/'T' as shorthand for today, or a full YYYY-MM-DD date
string getValidatedDate(const string& prompt) {
    while (true) {
        string value = getValidatedString(prompt);

        if (value == "t" || value == "T") {
            return getCurrentDateString();
        }

        bool structureOk = (value.size() == 10 && value[4] == '-' && value[7] == '-');
        if (structureOk) {
            for (size_t i = 0; i < value.size() && structureOk; i++) {
                if (i == 4 || i == 7) continue;
                if (!isdigit((unsigned char)value[i])) structureOk = false;
            }
        }
        if (!structureOk) {
            cout << "Invalid format. Use YYYY-MM-DD (e.g. 2026-08-15), or 't' for today.\n";
            continue;
        }

        int year = stoi(value.substr(0, 4));
        int month = stoi(value.substr(5, 2));
        int day = stoi(value.substr(8, 2));

        if (!isValidCalendarDate(year, month, day)) {
            cout << "Invalid date - check the month (01-12) and day are within range.\n";
            continue;
        }
        if (isDateBeforeToday(value)) {
            cout << "Date cannot be before today (" << getCurrentDateString() << "). Please try again.\n";
            continue;
        }
        return value;
    }
}


static bool isQuit(const string& input) {
    return input == "q" || input == "Q";
}

InputResult stepString(const string& prompt, string& value) {
    while (true) {
        cout << prompt;
        string input;
        getline(cin, input);
        if (cin.eof() && input.empty()) { cout << "\nNo more input. Exiting program.\n"; exit(0); }
        if (isQuit(input)) return InputResult::BACK;
        if (input.empty()) {
            cout << "Input cannot be empty. Please try again (or 'q' to go back).\n";
            continue;
        }
        value = input;
        return InputResult::OK;
    }
}

InputResult stepInt(const string& prompt, int minVal, int maxVal, int& value) {
    while (true) {
        cout << prompt;
        string input;
        getline(cin, input);
        if (cin.eof() && input.empty()) { cout << "\nNo more input. Exiting program.\n"; exit(0); }
        if (isQuit(input)) return InputResult::BACK;
        try {
            size_t idx;
            int v = stoi(input, &idx);
            if (idx != input.size()) throw invalid_argument("trailing characters");
            if (v < minVal || v > maxVal) {
                cout << "Please enter a number between " << minVal << " and " << maxVal
                    << " (or 'q' to go back).\n";
                continue;
            }
            value = v;
            return InputResult::OK;
        }
        catch (...) {
            cout << "Invalid number. Please try again (or 'q' to go back).\n";
        }
    }
}

InputResult stepDouble(const string& prompt, double minVal, double maxVal, double& value) {
    while (true) {
        cout << prompt;
        string input;
        getline(cin, input);
        if (cin.eof() && input.empty()) { cout << "\nNo more input. Exiting program.\n"; exit(0); }
        if (isQuit(input)) return InputResult::BACK;
        try {
            size_t idx;
            double v = stod(input, &idx);
            if (idx != input.size()) throw invalid_argument("trailing characters");
            if (v < minVal || v > maxVal) {
                cout << "Please enter a value between " << minVal << " and " << maxVal
                    << " (or 'q' to go back).\n";
                continue;
            }
            value = v;
            return InputResult::OK;
        }
        catch (...) {
            cout << "Invalid number. Please try again (or 'q' to go back).\n";
        }
    }
}

InputResult stepDate(const string& prompt, string& value) {
    while (true) {
        cout << prompt;
        string input;
        getline(cin, input);
        if (cin.eof() && input.empty()) { cout << "\nNo more input. Exiting program.\n"; exit(0); }
        if (isQuit(input)) return InputResult::BACK;

        if (input == "t" || input == "T") {
            value = getCurrentDateString();
            return InputResult::OK;
        }

        bool structureOk = (input.size() == 10 && input[4] == '-' && input[7] == '-');
        if (structureOk) {
            for (size_t i = 0; i < input.size() && structureOk; i++) {
                if (i == 4 || i == 7) continue;
                if (!isdigit((unsigned char)input[i])) structureOk = false;
            }
        }
        if (!structureOk) {
            cout << "Invalid format. Use YYYY-MM-DD, 't' for today, or 'q' to go back.\n";
            continue;
        }

        int year = stoi(input.substr(0, 4));
        int month = stoi(input.substr(5, 2));
        int day = stoi(input.substr(8, 2));

        if (!isValidCalendarDate(year, month, day)) {
            cout << "Invalid date - check the month (01-12) and day are within range (or 'q' to go back).\n";
            continue;
        }
        if (isDateBeforeToday(input)) {
            cout << "Date cannot be before today (" << getCurrentDateString()
                << "). Please try again (or 'q' to go back).\n";
            continue;
        }
        value = input;
        return InputResult::OK;
    }
}

InputResult stepConfirm(const string& prompt, bool& value) {
    while (true) {
        cout << prompt;
        string input;
        getline(cin, input);
        if (cin.eof() && input.empty()) { cout << "\nNo more input. Exiting program.\n"; exit(0); }
        if (isQuit(input)) return InputResult::BACK;
        if (input == "y" || input == "Y") { value = true; return InputResult::OK; }
        if (input == "n" || input == "N") { value = false; return InputResult::OK; }
        cout << "Please enter y or n (or 'q' to go back).\n";
    }
}

void pauseScreen() {
    cout << "\nPress Enter to continue...";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

void printLine() {
    cout << "--------------------------------------------------------------\n";
}