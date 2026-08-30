/*==================================================================
  common.cpp  -  the shared "engine" behind common.h

  what this file does:
  - a tiny JSON reader/writer (so .json files work with no library)
  - load / save for members, bike types and rentals
  - id makers + the reservation-code maker
  - date helpers (today, +N days, "is this a real date?")
  - input helpers that keep asking until the answer is valid
  - screen helpers (clear, divider, centred title, pause)
==================================================================*/
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
#include <exception>
using namespace std;

/*==================================================================
  builders - each makes an empty JsonValue of one type
==================================================================*/
JsonValue JsonValue::makeObject() { JsonValue v; v.type = JsonType::Object; return v; }
JsonValue JsonValue::makeArray() { JsonValue v; v.type = JsonType::Array;  return v; }
JsonValue JsonValue::makeString(const string& s) { JsonValue v; v.type = JsonType::String; v.strVal = s; return v; }
JsonValue JsonValue::makeNumber(double n) { JsonValue v; v.type = JsonType::Number; v.numVal = n; return v; }
JsonValue JsonValue::makeBool(bool b) { JsonValue v; v.type = JsonType::Bool; v.boolVal = b; return v; }

/*==================================================================
  this method adds a key to an object, or replaces it if the key
  is already there
==================================================================*/
void JsonValue::set(const string& key, const JsonValue& v) {
    for (auto& kv : objVal) {
        if (kv.first == key) { kv.second = v; return; }
    }
    objVal.push_back({ key, v });
}

/*==================================================================
  this method adds one value to the end of an array
==================================================================*/
void JsonValue::push(const JsonValue& v) {
    arrVal.push_back(v);
}

/*==================================================================
  this method checks whether an object has a given key
==================================================================*/
bool JsonValue::has(const string& key) const {
    for (const auto& kv : objVal) if (kv.first == key) return true;
    return false;
}

/*==================================================================
  this method returns the value for a key, or throws if it is
  missing (callers usually use the safe getStr/getInt/... below)
==================================================================*/
const JsonValue& JsonValue::get(const string& key) const {
    for (const auto& kv : objVal) if (kv.first == key) return kv.second;
    throw runtime_error("JSON key not found: " + key);
}

// these 4 return the value for a key, or 'def' if the key is missing
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

/*==================================================================
  this method escapes " and \ and newlines so a string is safe
  to put inside JSON text
==================================================================*/
static string jsonEscapeString(const string& s) {
    string out;
    for (char c : s) {
        if (c == '"' || c == '\\') { out += '\\'; out += c; }
        else if (c == '\n') { out += "\\n"; }
        else out += c;
    }
    return out;
}

/*==================================================================
  this method turns a number into text - whole numbers get no
  decimal point, others get 2 decimal places
==================================================================*/
static string numberToJsonString(double n) {
    if (n == (long long)n) {
        return to_string((long long)n);
    }
    ostringstream oss;
    oss << fixed << setprecision(2) << n;
    return oss.str();
}

/*==================================================================
  this method turns a JsonValue into pretty (indented) JSON text.
  it calls itself for nested arrays / objects
==================================================================*/
string JsonValue::dump(int indent) const {
    string pad(indent * 2, ' ');
    string padInner((indent + 1) * 2, ' ');

    switch (type) {
    case JsonType::Null:   return "null";
    case JsonType::Bool:
        if (boolVal) return "true";
        return "false";
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

/*==================================================================
  JsonParser  -  walks through JSON text one character at a time
  and builds a JsonValue tree. one small helper method per JSON
  piece (object / array / string / bool / number)
==================================================================*/
namespace {

    struct JsonParser {
        const string& text;
        size_t pos = 0;

        explicit JsonParser(const string& t) : text(t) {}

        // skip spaces / tabs / newlines
        void skipWhitespace() {
            while (pos < text.size() && isspace((unsigned char)text[pos])) pos++;
        }

        // look at the current character without moving on
        char peek() {
            if (pos < text.size()) return text[pos];
            return '\0';
        }
        // take the current character and move to the next one
        char advance() {
            if (pos < text.size()) return text[pos++];
            return '\0';
        }

        // demand a specific char here, or throw
        void expect(char c) {
            skipWhitespace();
            if (peek() != c) throw runtime_error(string("JSON parse error: expected '") + c + "'");
            advance();
        }

        // read whatever value comes next, based on the first char
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
                if (c == '\\' && pos < text.size()) {   // handle \" \\ \n
                    char next = text[pos++];
                    if (next == 'n') out += '\n';
                    else out += next;
                }
                else {
                    out += c;
                }
            }
            if (pos < text.size()) pos++;   // skip the closing quote
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

}   // end anonymous namespace

/*==================================================================
  this method is the way the rest of the program parses JSON. an
  empty / blank file is treated as an empty array
==================================================================*/
JsonValue parseJson(const string& text) {
    if (text.find_first_not_of(" \t\r\n") == string::npos) {
        return JsonValue::makeArray();
    }
    JsonParser parser(text);
    return parser.parseValue();
}

/*==================================================================
  this method reads a whole text file into one string ("" if the
  file is missing)
==================================================================*/
static string readFileText(const string& filename) {
    ifstream fin(filename);
    if (!fin.good()) return "";
    stringstream ss;
    ss << fin.rdbuf();
    return ss.str();
}

/*==================================================================
  this method writes a string to a file (overwrites it)
==================================================================*/
static void writeFileText(const string& filename, const string& content) {
    ofstream fout(filename);
    fout << content;
    fout.close();
}

/*==================================================================
  this method makes sure the 3 data files exist - if one is
  missing it creates it holding an empty array "[]"
==================================================================*/
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

/*==================================================================
  this method reads members.json into a vector<Member>. if the
  file is broken it warns and returns an empty list (no crash)
==================================================================*/
vector<Member> loadMembers() {
    vector<Member> members;
    try {
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
    }
    catch (const exception& e) {
        cout << "[Warning] Could not read " << MEMBERS_FILE << " (" << e.what()
            << "). Continuing with no member records loaded.\n";
        members.clear();
    }
    return members;
}

/*==================================================================
  this method writes the whole member list back to members.json
==================================================================*/
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

/*==================================================================
  this method reads biketypes.json into a vector<BikeType>
  (empty list + warning if the file is broken)
==================================================================*/
vector<BikeType> loadBikeTypes() {
    vector<BikeType> bikeTypes;
    try {
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
    }
    catch (const exception& e) {
        cout << "[Warning] Could not read " << BIKETYPES_FILE << " (" << e.what()
            << "). Continuing with no bike types loaded.\n";
        bikeTypes.clear();
    }
    return bikeTypes;
}

/*==================================================================
  this method writes the whole bike-type list back to biketypes.json
==================================================================*/
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

/*==================================================================
  this method reads rentals.json into a vector<Rental>. each
  rental also has a nested "units" array which is read here too
==================================================================*/
vector<Rental> loadRentals() {
    vector<Rental> rentals;
    try {
        JsonValue arr = parseJson(readFileText(RENTALS_FILE));
        for (const auto& obj : arr.arrVal) {
            Rental r;
            r.rentalId = obj.getInt("rentalId");
            r.channel = obj.getStr("channel");
            r.memberId = obj.getInt("memberId");
            r.renterName = obj.getStr("renterName");
            r.bikeTypeId = obj.getInt("bikeTypeId");
            r.quantity = obj.getInt("quantity");
            r.rentalDays = obj.getInt("rentalDays", 1);
            r.rentDate = obj.getStr("rentDate");
            r.dueDate = obj.getStr("dueDate", r.rentDate);   // old records had no dueDate
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

            // read the per-bike unit rows
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
    }
    catch (const exception& e) {
        cout << "[Warning] Could not read " << RENTALS_FILE << " (" << e.what()
            << "). Continuing with no rental records loaded.\n";
        rentals.clear();
    }
    return rentals;
}

/*==================================================================
  this method writes the whole rental list (with its units) back
  to rentals.json
==================================================================*/
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
        obj.set("rentalDays", JsonValue::makeNumber(r.rentalDays));
        obj.set("rentDate", JsonValue::makeString(r.rentDate));
        obj.set("dueDate", JsonValue::makeString(r.dueDate));
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

/*==================================================================
  these 3 methods return "biggest existing id + 1" so a new
  record always gets a fresh, unused id
==================================================================*/
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

/*==================================================================
  this method makes a random 6-digit code and keeps re-rolling
  until it does not match any existing rental's code
==================================================================*/
string generateReservationCode(const vector<Rental>& existingRentals) {
    while (true) {
        int code = 100000 + (rand() % 900000);
        string candidate = to_string(code);

        bool taken = false;
        for (const auto& r : existingRentals) {
            if (r.code == candidate) { taken = true; break; }
        }
        if (!taken) return candidate;
    }
}

// month names, used only for the "February 2027 has 28 days" message
static const string MONTH_NAMES[12] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};

/*==================================================================
  this method returns how many days a given month has, and
  handles leap-year February
==================================================================*/
static int daysInMonthFor(int year, int month) {
    int dim[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    bool isLeap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    if (isLeap) dim[1] = 29;
    return dim[month - 1];
}

/*==================================================================
  this method returns the current hour (0-23) from the PC clock -
  used to check if a same-day return is before the shop closes
==================================================================*/
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

/*==================================================================
  this method returns today's date as "YYYY-MM-DD"
==================================================================*/
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

/*==================================================================
  this method tells whether a "YYYY-MM-DD" date is before today
  (string compare works because the format is fixed-width)
==================================================================*/
bool isDateBeforeToday(const string& dateStr) {
    return dateStr < getCurrentDateString();
}

/*==================================================================
  this method checks a year/month/day is a real calendar date
==================================================================*/
bool isValidCalendarDate(int year, int month, int day) {
    if (year < 2000 || year > 2100) return false;
    if (month < 1 || month > 12) return false;
    if (day < 1) return false;
    return day <= daysInMonthFor(year, month);
}

/*==================================================================
  this method adds N days to a "YYYY-MM-DD" date and returns the
  new date, rolling over the end of a month / year
==================================================================*/
string addDaysToDate(const string& dateStr, int days) {
    int year = stoi(dateStr.substr(0, 4));
    int month = stoi(dateStr.substr(5, 2));
    int day = stoi(dateStr.substr(8, 2));

    day += days;
    while (day > daysInMonthFor(year, month)) {
        day -= daysInMonthFor(year, month);
        month++;
        if (month > 12) { month = 1; year++; }
    }

    // pad month and day to 2 digits
    string mm = to_string(month);
    if (month < 10) mm = "0" + mm;
    string dd = to_string(day);
    if (day < 10) dd = "0" + dd;
    return to_string(year) + "-" + mm + "-" + dd;
}

/*==================================================================
  this method checks a string is all digits (and not empty)
==================================================================*/
static bool isAllDigits(const string& s) {
    if (s.empty()) return false;
    for (char c : s) if (!isdigit((unsigned char)c)) return false;
    return true;
}

/*==================================================================
  this method reads a date the user typed. accepts 't' = today,
  or YYYY-MM-DD (month/day may be 1 or 2 digits, year must be 4).
  it checks year, then month, then day, and the first thing that
  is wrong sets a short 'errorOut' message. returns true + the
  tidied "YYYY-MM-DD" in 'normalized' on success
==================================================================*/
static bool tryParseDate(const string& raw, string& normalized, string& errorOut) {
    // trim spaces off both ends
    size_t a = raw.find_first_not_of(" \t");
    size_t b = raw.find_last_not_of(" \t");
    if (a == string::npos) { errorOut = "Please enter a date."; return false; }
    string s = raw.substr(a, b - a + 1);

    if (s == "t" || s == "T") {
        normalized = getCurrentDateString();
        return true;
    }

    // split on '-' into year / month / day
    vector<string> parts;
    string current;
    for (char c : s) {
        if (c == '-') { parts.push_back(current); current.clear(); }
        else current.push_back(c);
    }
    parts.push_back(current);

    if (parts.size() != 3) {
        errorOut = "Invalid date. Please enter it as YYYY-MM-DD.";
        return false;
    }

    int currentYear = stoi(getCurrentDateString().substr(0, 4));

    // 1) year - exactly 4 digits, not in the past, not too far ahead
    if (!isAllDigits(parts[0]) || parts[0].size() != 4) {
        errorOut = "Invalid year. The year must be written in full (4 digits).";
        return false;
    }
    int year = stoi(parts[0]);
    if (year < currentYear) {
        errorOut = "Invalid year. The year cannot be earlier than " + to_string(currentYear) + ".";
        return false;
    }
    if (year > currentYear + 5) {
        errorOut = "Invalid year. The year is too far in the future.";
        return false;
    }

    // 2) month - 1 to 12
    if (!isAllDigits(parts[1]) || parts[1].size() > 2) {
        errorOut = "Invalid month. The month must be a number from 1 to 12.";
        return false;
    }
    int month = stoi(parts[1]);
    if (month < 1 || month > 12) {
        errorOut = "Invalid month. The month must be between 1 and 12.";
        return false;
    }

    // 3) day - 1 to however many days that month has
    if (!isAllDigits(parts[2]) || parts[2].size() > 2) {
        errorOut = "Invalid day. The day must be a number.";
        return false;
    }
    int day = stoi(parts[2]);
    int dim = daysInMonthFor(year, month);
    if (day < 1 || day > dim) {
        errorOut = "Invalid day. " + MONTH_NAMES[month - 1] + " " + to_string(year)
            + " has " + to_string(dim) + " days.";
        return false;
    }

    // pad month/day to 2 digits and hand back "YYYY-MM-DD"
    string mm = to_string(month);
    if (month < 10) mm = "0" + mm;
    string dd = to_string(day);
    if (day < 10) dd = "0" + dd;
    normalized = to_string(year) + "-" + mm + "-" + dd;
    return true;
}

/*==================================================================
  this method asks for a whole number and keeps asking until it
  is between minVal and maxVal
==================================================================*/
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

/*==================================================================
  this method asks for a decimal number and keeps asking until it
  is between minVal and maxVal
==================================================================*/
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

/*==================================================================
  this method asks for text and keeps asking until it is not empty
==================================================================*/
string getValidatedString(const string& prompt) {
    string value;
    while (true) {
        cout << prompt;
        getline(cin, value);
        if (cin.eof() && value.empty()) { cout << "\nNo more input. Exiting program.\n"; exit(0); }
        if (!value.empty()) return value;
        cout << "Input cannot be empty. Please try again.\n";
    }
}

/*==================================================================
  this method asks for a date and keeps asking until it is valid
  AND not before today
==================================================================*/
string getValidatedDate(const string& prompt) {
    while (true) {
        string value = getValidatedString(prompt);

        string normalized, errorMsg;
        if (!tryParseDate(value, normalized, errorMsg)) {
            cout << errorMsg << "\n";
            continue;
        }
        if (isDateBeforeToday(normalized)) {
            cout << "Invalid date. The date cannot be earlier than today (" << getCurrentDateString() << ").\n";
            continue;
        }
        return normalized;
    }
}

/*==================================================================
  this method checks if the user typed 'q' (used by every step*
  helper to mean "go back one question")
==================================================================*/
static bool isQuit(const string& input) {
    return input == "q" || input == "Q";
}

/*==================================================================
  this method asks for one line of text. returns BACK if the user
  types 'q', otherwise keeps asking until the text is not empty
==================================================================*/
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

/*==================================================================
  this method asks for a whole number in a range. 'q' = BACK
==================================================================*/
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
            if (idx != input.size()) throw invalid_argument("trailing characters");   // e.g. "12abc"
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

/*==================================================================
  this method asks for a decimal number in a range. 'q' = BACK
==================================================================*/
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

/*==================================================================
  this method asks for a date ('t' = today). 'q' = BACK. uses the
  same tryParseDate checks as getValidatedDate
==================================================================*/
InputResult stepDate(const string& prompt, string& value) {
    while (true) {
        cout << prompt;
        string input;
        getline(cin, input);
        if (cin.eof() && input.empty()) { cout << "\nNo more input. Exiting program.\n"; exit(0); }
        if (isQuit(input)) return InputResult::BACK;

        string normalized, errorMsg;
        if (!tryParseDate(input, normalized, errorMsg)) {
            cout << errorMsg << " (or 'q' to go back)\n";
            continue;
        }
        if (isDateBeforeToday(normalized)) {
            cout << "Invalid date. The date cannot be earlier than today (" << getCurrentDateString()
                << "). (or 'q' to go back)\n";
            continue;
        }
        value = normalized;
        return InputResult::OK;
    }
}

/*==================================================================
  this method asks a yes/no question. 'q' = BACK
==================================================================*/
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

/*==================================================================
  this method asks for a phone number: digits only (no spaces,
  '-' or letters), 7 to 15 long. 'q' = BACK
==================================================================*/
InputResult stepPhone(const string& prompt, string& value) {
    while (true) {
        cout << prompt;
        string input;
        getline(cin, input);
        if (cin.eof() && input.empty()) { cout << "\nNo more input. Exiting program.\n"; exit(0); }
        if (isQuit(input)) return InputResult::BACK;

        if (input.empty()) {
            cout << "Phone number cannot be empty. Please try again (or 'q' to go back).\n";
            continue;
        }

        bool digitsOnly = true;
        for (char c : input) {
            if (!isdigit((unsigned char)c)) { digitsOnly = false; break; }
        }
        if (!digitsOnly) {
            cout << "Phone number must be digits only - no spaces, '-' or letters (or 'q' to go back).\n";
            continue;
        }
        if (input.size() < 7 || input.size() > 15) {
            cout << "Phone number must be between 7 and 15 digits (or 'q' to go back).\n";
            continue;
        }

        value = input;
        return InputResult::OK;
    }
}

/*==================================================================
  this method clears the console (Windows "cls", other OS "clear")
==================================================================*/
void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

/*==================================================================
  this method prints one full-width "-----" divider line
==================================================================*/
void printLine() {
    cout << string(UI_WIDTH, '-') << "\n";
}

/*==================================================================
  this method prints one line of text centred in UI_WIDTH
==================================================================*/
void printCentered(const string& text) {
    int padding = (UI_WIDTH - (int)text.size()) / 2;
    if (padding < 0) padding = 0;
    cout << string(padding, ' ') << text << "\n";
}

/*==================================================================
  this method starts a new screen: clear, then the title inside
  a line box
==================================================================*/
void uiHeader(const string& title) {
    clearScreen();
    printLine();
    printCentered(title);
    printLine();
}

/*==================================================================
  this method waits for the user to press Enter before moving on
==================================================================*/
void pauseScreen() {
    cout << "\n";
    printCentered("Press Enter to continue...");
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

/*==================================================================
  this method cuts a long value so it fits a table column - a
  value that is too long ends with ".." instead of spilling over
==================================================================*/
string fitStr(const string& text, int width) {
    if (width < 4) return text.substr(0, width);
    if ((int)text.size() <= width - 1) return text;   // short enough, leave a gap after it
    return text.substr(0, width - 3) + "..";
}
