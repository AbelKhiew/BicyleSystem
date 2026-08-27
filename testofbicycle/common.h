/* ============================================================
   common.h
   Shared structures, constants, JSON value type, and shared
   function prototypes used across all team members' modules.
   TEAM COMPONENT - Shared data management
   ============================================================ */
#ifndef COMMON_H
#define COMMON_H

#include <string>
#include <vector>
#include <utility>
using namespace std;

/* ---------------- File names (JSON, one file per data category) ---------------- */
const string MEMBERS_FILE = "members.json";
const string BIKETYPES_FILE = "biketypes.json";
const string RENTALS_FILE = "rentals.json";

/* ---------------- Shop closing time (used for on-time return check) ---------------- */
const int SHOP_CLOSE_HOUR = 22;   // 10:00 PM, 24-hour format

/* =====================================================================
   Minimal hand-written JSON value type (no external library needed -
   uses only <fstream> for reading/writing raw text). Supports the
   subset of JSON this system needs: objects, arrays, strings, numbers,
   booleans, and null. Object keys preserve insertion order.
   ===================================================================== */
enum class JsonType { Null, Bool, Number, String, Array, Object };

struct JsonValue {
    JsonType type = JsonType::Null;
    bool boolVal = false;
    double numVal = 0.0;
    string strVal;
    vector<JsonValue> arrVal;
    vector<pair<string, JsonValue>> objVal;   // preserves key order

    static JsonValue makeObject();
    static JsonValue makeArray();
    static JsonValue makeString(const string& s);
    static JsonValue makeNumber(double n);
    static JsonValue makeBool(bool b);

    void set(const string& key, const JsonValue& v);   // object: add/replace a key
    void push(const JsonValue& v);                      // array: append a value

    bool has(const string& key) const;
    const JsonValue& get(const string& key) const;      // object: lookup (throws if missing)

    /* Convenience getters with defaults, used when reading records back */
    string getStr(const string& key, const string& def = "") const;
    double getNum(const string& key, double def = 0.0) const;
    int getInt(const string& key, int def = 0) const;
    bool getBool(const string& key, bool def = false) const;

    string dump(int indent = 0) const;   // serialize to pretty-printed JSON text
};

JsonValue parseJson(const string& text);   // parse JSON text into a JsonValue tree

/* ---------------- Structures (records) ---------------- */

/* Renter account. name is the unique login username. */
struct Member {
    int    id = 0;
    string name;
    string password;
    string phone;
    int    totalRentals = 0;
    int    points = 0;
};

/* One row per bike TYPE. deposit = per-unit deposit, auto-calculated
   as tyreCost + brakeCost + chainCost + RM20.
   bookingQuantity = units available for online renter booking
   walkinQuantity  = units available for admin walk-in rental        */
struct BikeType {
    int    id = 0;
    string type;
    double pricePerDay = 0.0;
    double tyreCost = 0.0;
    double brakeCost = 0.0;
    double chainCost = 0.0;
    double deposit = 0.0;
    int    bookingQuantity = 0;
    int    walkinQuantity = 0;
    int    timesRented = 0;
};

/* One physical unit within a multi-unit rental, tracked individually
   so damage/condition/refund can differ per bike returned.          */
struct RentalUnit {
    int    unitIndex = 0;
    string condition = "-";      // "-" pending, "Good", "Damaged"
    bool   tyreDamaged = false;
    bool   brakeDamaged = false;
    bool   chainDamaged = false;
    double damageCost = 0.0;
    double depositRefunded = 0.0;
    bool   returnedOnTime = true;
    bool   isReturned = false;
};

/* A rental transaction, possibly covering multiple units of the same
   bike type under one reservation code.                             */
struct Rental {
    int    rentalId = 0;
    string channel;               // "Booking" (renter) or "WalkIn" (admin)
    int    memberId = 0;          // 0 if no linked account
    string renterName;
    int    bikeTypeId = 0;
    int    quantity = 0;
    string rentDate;              // YYYY-MM-DD
    string code;                  // reservation / reference code
    double pricePerDay = 0.0;     // snapshot at time of booking (undiscounted rate)
    double depositPerUnit = 0.0;  // snapshot at time of booking
    int    discountApplied = 0;   // % discount redeemed against the price portion (0 if none)
    int    discountPointsUsed = 0; // loyalty points spent to redeem the discount
    double totalPaid = 0.0;       // actual amount charged (after discount, after cash rounding if Cash)
    string paymentMethod;
    string status = "Pending";    // "Pending", "Active", "Return"
    int    pointsEarned = 0;
    bool   pointsAwarded = false;
    string returnDate;            // set when the rental status becomes "Return" (YYYY-MM-DD)
    vector<RentalUnit> units;
};

/* ---------------- Loyalty discount tier table (points -> % off price portion) ---------------- */
const int NUM_DISCOUNT_TIERS = 5;
const int discountPointsReq[NUM_DISCOUNT_TIERS] = { 16, 32, 48, 64, 80 };
const int discountPercentTable[NUM_DISCOUNT_TIERS] = { 10, 20, 30, 40, 50 };

/* ---------------- Predefined damage part names (for display) ---------------- */
const int NUM_DAMAGE_PARTS = 3;
const string damagePartNames[NUM_DAMAGE_PARTS] = { "Tyre", "Brake", "Chain" };

/* ---------------- Shared file I/O function prototypes (common.cpp) ---------------- */
void ensureDataFilesExist();

vector<Member> loadMembers();
void saveMembers(const vector<Member>& members);

vector<BikeType> loadBikeTypes();
void saveBikeTypes(const vector<BikeType>& bikeTypes);

vector<Rental> loadRentals();
void saveRentals(const vector<Rental>& rentals);

/* ---------------- Shared utility function prototypes (common.cpp) ---------------- */
int getNextMemberId(const vector<Member>& members);
int getNextBikeTypeId(const vector<BikeType>& bikeTypes);
int getNextRentalId(const vector<Rental>& rentals);
string generateReservationCode();
int getCurrentHour();
string getCurrentDateString();               // today as YYYY-MM-DD
bool isDateBeforeToday(const string& dateStr);
bool isValidCalendarDate(int year, int month, int day);   // real day/month range + leap year check

int getValidatedInt(const string& prompt, int minVal, int maxVal);
double getValidatedDouble(const string& prompt, double minVal, double maxVal);
string getValidatedString(const string& prompt);
string getValidatedDate(const string& prompt);   // accepts 't'/'T' for today, or YYYY-MM-DD (validated + not before today)

/* =====================================================================
   Wizard-style step input - used by multi-field data-entry flows
   (add bike type, register, book bike, etc.) so the admin/renter can
   type 'q' at any prompt to step back to the previous field, instead
   of being stuck if they made a mistake earlier in the flow.
   ===================================================================== */
enum class InputResult { OK, BACK };

InputResult stepString(const string& prompt, string& value);                       // non-empty required
InputResult stepInt(const string& prompt, int minVal, int maxVal, int& value);
InputResult stepDouble(const string& prompt, double minVal, double maxVal, double& value);
InputResult stepDate(const string& prompt, string& value);                          // 't' for today, or YYYY-MM-DD
InputResult stepConfirm(const string& prompt, bool& value);                         // y/n

void pauseScreen();
void printLine();

#endif