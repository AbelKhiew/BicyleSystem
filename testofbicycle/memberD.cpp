/*==================================================================
  memberD.cpp  -  Member D: Reporting & Statistics (admin)

  what this file does - 7 read-only reports. none of them change
  any data, they just read the .json files and add things up:
  1 daily summary + revenue      5 late return report
  2 monthly revenue grid (2D)    6 top renters
  3 most rented bike type        7 active / pending / overdue list
  4 damage cost report
==================================================================*/
#include "memberD.h"
#include <iostream>
#include <iomanip>
#include <sstream>
using namespace std;

/*==================================================================
  a small helper record used only in this file - one row of
  "renter -> how much they spent, how many rentals"
==================================================================*/
struct RenterSummary {
    string name;
    double totalSpent = 0.0;
    int    rentalCount = 0;
};

// short month names for the monthly report rows
static const string MONTH_ABBR[12] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

/*==================================================================
  this method walks every FINISHED rental and adds up each
  renter's spending into a list of RenterSummary rows
==================================================================*/
static vector<RenterSummary> buildRenterSummaries(const vector<Rental>& rentals) {
    vector<RenterSummary> summaries;
    for (const auto& r : rentals) {
        if (r.status != "Return") continue;   // only count completed rentals

        // add to this renter's row if we have seen them before
        bool found = false;
        for (auto& s : summaries) {
            if (s.name == r.renterName) {
                s.totalSpent += r.totalPaid;
                s.rentalCount += 1;
                found = true;
                break;
            }
        }
        if (!found) {   // first time we see this renter - new row
            RenterSummary ns;
            ns.name = r.renterName;
            ns.totalSpent = r.totalPaid;
            ns.rentalCount = 1;
            summaries.push_back(ns);
        }
    }
    return summaries;
}

/*==================================================================
  this method reports today's activity: how many rentals started
  today, how many bikes are still out, money collected (split by
  payment method), deposit in vs deposit out, and the day's
  revenue (money in minus deposits given back)
==================================================================*/
void dailySummaryAndRevenueReport() {
    vector<Rental> rentals = loadRentals();
    string today = getCurrentDateString();

    int totalBookings = 0;
    int totalBicyclesNotReturned = 0;

    double totalCollectedMoney = 0.0;
    double totalDepositCollected = 0.0;
    double totalDepositReturned = 0.0;

    double methodTotals[NUM_PAYMENT_METHODS] = { 0 };   // one running total per method

    for (const auto& r : rentals) {
        // rentals that START today
        if (r.rentDate == today) {
            totalBookings++;
            totalCollectedMoney += r.totalPaid;
            totalDepositCollected += (r.depositPerUnit * r.quantity);

            // add this rental's payment to its method's total
            for (int i = 0; i < NUM_PAYMENT_METHODS; i++) {
                if (r.paymentMethod == paymentMethodNames[i]) {
                    methodTotals[i] += r.totalPaid;
                    break;
                }
            }

            for (const auto& u : r.units) {
                if (!u.isReturned) totalBicyclesNotReturned++;
            }
        }

        // deposits paid back today (the rental could have started any day)
        if (r.status == "Return" && r.returnDate == today) {
            for (const auto& u : r.units) {
                totalDepositReturned += u.depositRefunded;
            }
        }
    }

    double totalRevenue = totalCollectedMoney - totalDepositReturned;

    uiHeader("DAILY SUMMARY & REVENUE REPORT - " + today);
    cout << left << setw(30) << "Total Bookings Today:" << totalBookings << " reservation(s)\n";
    cout << left << setw(30) << "Bicycles Not Yet Returned:" << totalBicyclesNotReturned << " unit(s)\n\n";

    cout << fixed << setprecision(2);
    cout << left << setw(30) << "Total Collected Money:" << "RM" << totalCollectedMoney << "\n";
    cout << "Breakdown by Payment Method:\n";
    for (int i = 0; i < NUM_PAYMENT_METHODS; i++) {
        if (methodTotals[i] > 0) {
            cout << "  - " << left << setw(15) << paymentMethodNames[i]
                << ": RM" << methodTotals[i] << "\n";
        }
    }

    cout << "\n";
    cout << left << setw(30) << "Total Deposit Collected:" << "RM" << totalDepositCollected << "\n";
    cout << left << setw(30) << "Total Deposit Returned:" << "RM" << totalDepositReturned << "\n";
    printLine();
    cout << left << setw(30) << "TOTAL REVENUE:" << "RM" << totalRevenue << "\n";
    printLine();
    pauseScreen();
}

/*==================================================================
  this method shows takings as a grid: one row per month, one
  column per payment method, plus a total column and a total row.
  the grid is a real 2D array, filled with a nested loop
==================================================================*/
void monthlyRevenueReport() {
    vector<Rental> rentals = loadRentals();

    const int MONTHS = 12;
    double grid[MONTHS][NUM_PAYMENT_METHODS] = { { 0 } };   // <-- the 2D array

    // fill the grid: for each rental, add its total to grid[month][method]
    for (const auto& r : rentals) {
        if (r.rentDate.size() < 7) continue;
        int month = 0;
        try { month = stoi(r.rentDate.substr(5, 2)); }   // the "MM" part
        catch (...) { continue; }
        if (month < 1 || month > 12) continue;

        for (int j = 0; j < NUM_PAYMENT_METHODS; j++) {
            if (r.paymentMethod == paymentMethodNames[j]) {
                grid[month - 1][j] += r.totalPaid;
                break;
            }
        }
    }

    // column widths (used for the header, rows and divider so they line up)
    const int MON_W = 6;
    const int VAL_W = 14;
    const int TOT_W = 14;
    const int tableWidth = MON_W + NUM_PAYMENT_METHODS * VAL_W + TOT_W;

    uiHeader("MONTHLY REVENUE BREAKDOWN (BY PAYMENT METHOD)");
    cout << fixed << setprecision(2);

    cout << left << setw(MON_W) << "Month";
    for (int j = 0; j < NUM_PAYMENT_METHODS; j++) cout << right << setw(VAL_W) << paymentMethodNames[j];
    cout << right << setw(TOT_W) << "Row Total" << "\n";
    cout << string(tableWidth, '-') << "\n";

    double colTotal[NUM_PAYMENT_METHODS] = { 0 };
    double grandTotal = 0.0;
    bool anyData = false;

    // one printed row per month that actually has money in it
    for (int i = 0; i < MONTHS; i++) {
        double rowTotal = 0.0;
        for (int j = 0; j < NUM_PAYMENT_METHODS; j++) rowTotal += grid[i][j];
        if (rowTotal == 0.0) continue;

        anyData = true;
        cout << left << setw(MON_W) << MONTH_ABBR[i];
        for (int j = 0; j < NUM_PAYMENT_METHODS; j++) {
            cout << right << setw(VAL_W) << grid[i][j];
            colTotal[j] += grid[i][j];   // build the column totals as we go
        }
        cout << right << setw(TOT_W) << rowTotal << "\n";
        grandTotal += rowTotal;
    }

    if (!anyData) {
        cout << "No revenue has been recorded so far.\n";
        cout << string(tableWidth, '-') << "\n";
        pauseScreen();
        return;
    }

    // the total row
    cout << string(tableWidth, '-') << "\n";
    cout << left << setw(MON_W) << "TOTAL";
    for (int j = 0; j < NUM_PAYMENT_METHODS; j++) cout << right << setw(VAL_W) << colTotal[j];
    cout << right << setw(TOT_W) << grandTotal << "\n";
    cout << string(tableWidth, '-') << "\n";
    pauseScreen();
}

/*==================================================================
  this method finds the bike type with the highest timesRented
  and shows it (or a message if nothing has been rented yet)
==================================================================*/
void mostRentedTypeReport() {
    vector<BikeType> bikeTypes = loadBikeTypes();
    uiHeader("MOST RENTED BIKE TYPE");

    if (bikeTypes.empty()) {
        cout << "\nThere are no bike types in the system yet.\n";
        pauseScreen();
        return;
    }

    // walk the list, keeping the one with the biggest timesRented
    int maxRented = -1;
    size_t maxIndex = 0;
    for (size_t i = 0; i < bikeTypes.size(); i++) {
        if (bikeTypes[i].timesRented > maxRented) {
            maxRented = bikeTypes[i].timesRented;
            maxIndex = i;
        }
    }

    if (maxRented <= 0) {
        cout << "\nNo bicycles have been rented out yet.\n";
        printLine();
        pauseScreen();
        return;
    }

    cout << "\n  Most rented bike type : " << bikeTypes[maxIndex].type << "\n";
    cout << "  Total units rented    : " << bikeTypes[maxIndex].timesRented << " (all time)\n";
    printLine();
    pauseScreen();
}

/*==================================================================
  this method adds up repair spending, split by part. it looks at
  every returned bike and, for each damaged part, adds that
  part's repair cost and counts one incident
==================================================================*/
void damageCostReport() {
    vector<Rental> rentals = loadRentals();
    vector<BikeType> bikeTypes = loadBikeTypes();

    double partTotals[NUM_DAMAGE_PARTS] = { 0 };   // [tyre, brake, chain]
    int    partCounts[NUM_DAMAGE_PARTS] = { 0 };

    for (const auto& r : rentals) {
        // find this rental's bike type (for the part costs)
        BikeType matchedType;
        bool foundType = false;
        for (const auto& b : bikeTypes) {
            if (b.id == r.bikeTypeId) { matchedType = b; foundType = true; break; }
        }
        if (!foundType) continue;

        for (const auto& u : r.units) {
            if (!u.isReturned) continue;
            if (u.tyreDamaged) { partTotals[0] += matchedType.tyreCost;  partCounts[0]++; }
            if (u.brakeDamaged) { partTotals[1] += matchedType.brakeCost; partCounts[1]++; }
            if (u.chainDamaged) { partTotals[2] += matchedType.chainCost; partCounts[2]++; }
        }
    }

    uiHeader("DAMAGE COST REPORT");
    cout << fixed << setprecision(2);
    double grandTotal = 0;
    for (int i = 0; i < NUM_DAMAGE_PARTS; i++) {
        cout << "  " << left << setw(10) << damagePartNames[i] << setw(6) << partCounts[i]
            << "incident(s)    RM" << partTotals[i] << "\n";
        grandTotal += partTotals[i];
    }
    printLine();
    cout << "  Total Repair Cost Deducted: RM" << grandTotal << "\n";
    printLine();
    pauseScreen();
}

/*==================================================================
  this method counts bikes that came back late and adds up the
  deposits that were forfeited because of it
==================================================================*/
void lateReturnReport() {
    vector<Rental> rentals = loadRentals();

    int lateUnitCount = 0;
    double forfeitedTotal = 0;

    for (const auto& r : rentals) {
        for (const auto& u : r.units) {
            if (u.isReturned && !u.returnedOnTime) {
                lateUnitCount++;
                forfeitedTotal += r.depositPerUnit;
            }
        }
    }

    uiHeader("LATE RETURN REPORT");
    cout << "  Late Units Returned      : " << lateUnitCount << "\n";
    cout << fixed << setprecision(2);
    cout << "  Total Deposits Forfeited : RM" << forfeitedTotal << "\n";
    printLine();
    pauseScreen();
}

/*==================================================================
  this method shows the top 3 renters by total spending. it
  builds the per-renter totals, then picks the biggest 3 by
  scanning the list once per rank
==================================================================*/
void topRentersReport() {
    vector<Rental> rentals = loadRentals();
    vector<RenterSummary> summaries = buildRenterSummaries(rentals);

    uiHeader("TOP RENTERS BY SPENDING");

    if (summaries.empty()) {
        cout << "\nThere are no completed rentals to report on yet.\n";
        pauseScreen();
        return;
    }

    int topN = (int)summaries.size() < 3 ? (int)summaries.size() : 3;
    vector<bool> used(summaries.size(), false);   // which rows we have already printed

    for (int rank = 1; rank <= topN; rank++) {
        // find the highest unused row
        int maxIdx = -1;
        double maxVal = -1;
        for (size_t i = 0; i < summaries.size(); i++) {
            if (!used[i] && summaries[i].totalSpent > maxVal) {
                maxVal = summaries[i].totalSpent;
                maxIdx = (int)i;
            }
        }
        if (maxIdx == -1) break;
        used[maxIdx] = true;

        cout << "  " << rank << ". " << left << setw(20) << fitStr(summaries[maxIdx].name, 20)
            << "RM " << fixed << setprecision(2) << summaries[maxIdx].totalSpent
            << "  (" << summaries[maxIdx].rentalCount << " rental(s))\n";
    }
    printLine();
    pauseScreen();
}

/*==================================================================
  this method lists every rental that is not "Return" yet:
  - Pending  = booked online, waiting for counter activation
  - Active   = bike collected, still out, not late
  - OVERDUE  = Active but the due date has already passed
==================================================================*/
void overdueList() {
    vector<Rental> rentals = loadRentals();
    vector<BikeType> bikeTypes = loadBikeTypes();
    string today = getCurrentDateString();

    uiHeader("ACTIVE / PENDING / OVERDUE RENTALS LIST");

    // collect the open rentals first so the header only shows if there are any
    vector<Rental> openRentals;
    for (const auto& r : rentals) {
        if (r.status != "Return") openRentals.push_back(r);
    }

    if (openRentals.empty()) {
        cout << "\nThere are currently no active, pending, or overdue rentals.\n";
        pauseScreen();
        return;
    }

    const string indent = " ";
    cout << indent << left << setw(4) << "ID" << setw(14) << "Renter" << setw(26)
        << "Bike Type" << setw(5) << "Qty" << setw(12) << "Due Date" << setw(9) << "Status" << "\n";
    cout << indent << string(74, '-') << "\n";

    for (const auto& r : openRentals) {
        string bikeTypeName = "Unknown";
        for (const auto& b : bikeTypes) {
            if (b.id == r.bikeTypeId) { bikeTypeName = b.type; break; }
        }
        string dueRef = r.dueDate.empty() ? r.rentDate : r.dueDate;

        // an Active rental past its due date is shown as OVERDUE
        string displayStatus = r.status;
        if (r.status == "Active" && dueRef < today) displayStatus = "OVERDUE";

        cout << indent << left << setw(4) << r.rentalId << setw(14) << fitStr(r.renterName, 14)
            << setw(26) << fitStr(bikeTypeName, 26) << setw(5) << r.quantity
            << setw(12) << dueRef << setw(9) << displayStatus << "\n";
    }
    cout << indent << string(74, '-') << "\n";
    pauseScreen();
}

/*==================================================================
  this method is the admin "reports" menu. loops until 0
==================================================================*/
void memberD_menu() {
    int choice;
    do {
        uiHeader("REPORTING & STATISTICS  (ADMIN)");
        printCentered("1. Daily Summary & Revenue Report");
        printCentered("2. Monthly Revenue Breakdown (by Payment Method)");
        printCentered("3. Most Rented Bike Type");
        printCentered("4. Damage Cost Report");
        printCentered("5. Late Return Report");
        printCentered("6. Top Renters Report");
        printCentered("7. Active / Pending / Overdue Rentals List");
        printCentered("0. Back to Main Menu");
        printLine();
        choice = getValidatedInt("Enter choice: ", 0, 7);

        switch (choice) {
        case 1: dailySummaryAndRevenueReport(); break;
        case 2: monthlyRevenueReport(); break;
        case 3: mostRentedTypeReport(); break;
        case 4: damageCostReport(); break;
        case 5: lateReturnReport(); break;
        case 6: topRentersReport(); break;
        case 7: overdueList(); break;
        case 0: cout << "\nReturning to the main menu...\n"; break;
        }
    } while (choice != 0);
}
