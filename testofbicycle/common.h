/*==================================================================
  common.h  -  shared stuff every module needs

  what this file does:
  - lists the data file names + fixed settings (constants)
  - defines the records (structs): Member, BikeType, Rental, ...
  - defines a tiny JSON value type (so we can save/load .json
    without any outside library)
  - declares the shared helper functions that live in common.cpp
    (file load/save, date checks, input validation, screen output)
==================================================================*/
#ifndef COMMON_H
#define COMMON_H

#include <string>
#include <vector>
#include <utility>
using namespace std;

// the 3 data files - one per kind of record
const string MEMBERS_FILE = "members.json";
const string BIKETYPES_FILE = "biketypes.json";
const string RENTALS_FILE = "rentals.json";

const int SHOP_CLOSE_HOUR = 22;      // shop closes 22:00 - used to decide if a return is late
const double LATE_PENALTY = 20.00;   // RM deducted from each unit's deposit on a late return
const int UI_WIDTH = 76;             // width of every centred title / divider line
const int MIN_RENTAL_DAYS = 1;       // shortest rental allowed
const int MAX_RENTAL_DAYS = 30;      // longest rental allowed

/*==================================================================
  our own tiny JSON value. it can hold null / bool / number /
  string / array / object, which is all the .json files need.
  keeps object keys in the order we add them
==================================================================*/
enum class JsonType { Null, Bool, Number, String, Array, Object };

struct JsonValue {
    JsonType type = JsonType::Null;
    bool boolVal = false;
    double numVal = 0.0;
    string strVal;
    vector<JsonValue> arrVal;
    vector<pair<string, JsonValue>> objVal;   // key order is kept

    // builders - make a value of one type
    static JsonValue makeObject();
    static JsonValue makeArray();
    static JsonValue makeString(const string& s);
    static JsonValue makeNumber(double n);
    static JsonValue makeBool(bool b);

    void set(const string& key, const JsonValue& v);   // object: add or replace a key
    void push(const JsonValue& v);                      // array: add a value to the end

    bool has(const string& key) const;                  // object: does this key exist?
    const JsonValue& get(const string& key) const;      // object: get a key (throws if missing)

    // safe getters - return 'def' if the key is not there
    string getStr(const string& key, const string& def = "") const;
    double getNum(const string& key, double def = 0.0) const;
    int getInt(const string& key, int def = 0) const;
    bool getBool(const string& key, bool def = false) const;

    string dump(int indent = 0) const;   // turn this value into pretty JSON text
};

JsonValue parseJson(const string& text);   // turn JSON text into a JsonValue

/*==================================================================
  the records (structs) the whole system passes around
==================================================================*/

// one renter account. 'name' is the unique login username
struct Member {
    int    id = 0;
    string name;
    string password;
    string phone;
    int    totalRentals = 0;
    int    points = 0;             // loyalty points
};

// one kind of bike. deposit is auto-worked-out = tyre+brake+chain repair + RM20
struct BikeType {
    int    id = 0;
    string type;
    double pricePerDay = 0.0;
    double tyreCost = 0.0;
    double brakeCost = 0.0;
    double chainCost = 0.0;
    double deposit = 0.0;
    int    bookingQuantity = 0;    // units for online booking
    int    walkinQuantity = 0;     // units for walk-in rental
    int    timesRented = 0;
};

// one physical bike inside a rental - tracked on its own so each
// bike can be returned / damaged / refunded separately
struct RentalUnit {
    int    unitIndex = 0;
    string condition = "-";        // "-" = not back yet, else "Good" / "Damaged"
    bool   tyreDamaged = false;
    bool   brakeDamaged = false;
    bool   chainDamaged = false;
    double damageCost = 0.0;
    double depositRefunded = 0.0;
    bool   returnedOnTime = true;
    bool   isReturned = false;
};

// one rental - can cover several units of the same bike type
struct Rental {
    int    rentalId = 0;
    string channel;                // "Booking" (renter) or "WalkIn" (admin)
    int    memberId = 0;           // 0 = no linked account
    string renterName;
    int    bikeTypeId = 0;
    int    quantity = 0;
    int    rentalDays = 1;         // how many days hired
    string rentDate;               // start date  YYYY-MM-DD
    string dueDate;                // last day it can be returned on time
    string code;                   // reservation / reference code
    double pricePerDay = 0.0;      // copied in at booking time (before discount)
    double depositPerUnit = 0.0;   // copied in at booking time
    int    discountApplied = 0;    // % off the price part (0 = none)
    int    discountPointsUsed = 0; // points spent to get that discount
    double totalPaid = 0.0;        // what was actually charged
    string paymentMethod;
    string status = "Pending";     // "Pending" -> "Active" -> "Return"
    int    pointsEarned = 0;
    bool   pointsAwarded = false;  // stops points being given twice
    string returnDate;             // filled in when status becomes "Return"
    vector<RentalUnit> units;
};

// loyalty tiers: spend this many points -> get this % off the rental price
const int NUM_DISCOUNT_TIERS = 5;
const int discountPointsReq[NUM_DISCOUNT_TIERS] = { 16, 32, 48, 64, 80 };
const int discountPercentTable[NUM_DISCOUNT_TIERS] = { 10, 20, 30, 40, 50 };

// the 3 bike parts we track for damage
const int NUM_DAMAGE_PARTS = 3;
const string damagePartNames[NUM_DAMAGE_PARTS] = { "Tyre", "Brake", "Chain" };

// the payment methods - same names used everywhere so reports add up
const int NUM_PAYMENT_METHODS = 4;
const string paymentMethodNames[NUM_PAYMENT_METHODS] = { "QR Pay", "Bank Transfer", "Cash", "Card" };

/*==================================================================
  file load / save helpers (defined in common.cpp)
==================================================================*/
void ensureDataFilesExist();                                // make the 3 .json files if missing

vector<Member> loadMembers();                               // read members.json into a vector
void saveMembers(const vector<Member>& members);            // write the vector back to members.json

vector<BikeType> loadBikeTypes();
void saveBikeTypes(const vector<BikeType>& bikeTypes);

vector<Rental> loadRentals();
void saveRentals(const vector<Rental>& rentals);

/*==================================================================
  small utility helpers (defined in common.cpp)
==================================================================*/
int getNextMemberId(const vector<Member>& members);         // biggest id + 1
int getNextBikeTypeId(const vector<BikeType>& bikeTypes);
int getNextRentalId(const vector<Rental>& rentals);
string generateReservationCode(const vector<Rental>& existingRentals);  // random 6-digit, no clash

int getCurrentHour();                                       // hour of the day, 0-23
string getCurrentDateString();                              // today as YYYY-MM-DD
bool isDateBeforeToday(const string& dateStr);              // is this date in the past?
bool isValidCalendarDate(int year, int month, int day);     // real date? (leap years checked)
string addDaysToDate(const string& dateStr, int days);      // date + N days -> YYYY-MM-DD

/*==================================================================
  input helpers that keep asking until the answer is valid
==================================================================*/
int getValidatedInt(const string& prompt, int minVal, int maxVal);
double getValidatedDouble(const string& prompt, double minVal, double maxVal);
string getValidatedString(const string& prompt);
string getValidatedDate(const string& prompt);             // 't' = today, or a YYYY-MM-DD date

/*==================================================================
  "wizard" input helpers - used by the multi-question flows
  (register, add bike, book bike...). the user can type 'q' at
  any question to step BACK one question instead of being stuck
==================================================================*/
enum class InputResult { OK, BACK };

InputResult stepString(const string& prompt, string& value);                     // must not be empty
InputResult stepInt(const string& prompt, int minVal, int maxVal, int& value);
InputResult stepDouble(const string& prompt, double minVal, double maxVal, double& value);
InputResult stepDate(const string& prompt, string& value);                       // 't' = today, or YYYY-MM-DD
InputResult stepConfirm(const string& prompt, bool& value);                      // y / n
InputResult stepPhone(const string& prompt, string& value);                      // digits only, 7-15 long

/*==================================================================
  screen output helpers - keep every screen looking the same
==================================================================*/
void clearScreen();                       // wipe the console
void printLine();                         // a full-width ---- divider
void printCentered(const string& text);   // print one line, centred
void uiHeader(const string& title);       // clear screen + centred title in a box
void pauseScreen();                       // "Press Enter to continue..."

// cut a long value so it fits a table column and ends with ".."
string fitStr(const string& text, int width);

#endif
