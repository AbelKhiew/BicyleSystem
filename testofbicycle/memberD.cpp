/* ============================================================
   Reporting & Statistics Module (Admin)

   Responsibilities:
   - Revenue report, broken down by payment method
   - Most-rented bike TYPE report
   - Damage cost report (repair spend broken down by part)
   - Late return report (forfeited deposits from late returns)
   - Top renters report (top 3 by total spending)
   - Daily summary report (today's activity at a glance)
   - Active / Pending / Overdue rentals list

   ============================================================ */
#include "memberD.h"
#include <iostream>
#include <iomanip>
using namespace std;

/* Local structure used only for aggregating per-renter totals -
   demonstrates a struct built and populated entirely within this
   module, separate from the shared structures in common.h. */
struct RenterSummary {
    string name;
    double totalSpent = 0.0;
    int    rentalCount = 0;
};

static vector<RenterSummary> buildRenterSummaries(const vector<Rental>& rentals) {
    vector<RenterSummary> summaries;
    for (const auto& r : rentals) {
        if (r.status != "Return") continue;

        bool found = false;
        for (auto& s : summaries) {
            if (s.name == r.renterName) {
                s.totalSpent += r.totalPaid;
                s.rentalCount += 1;
                found = true;
                break;
            }
        }
        if (!found) {
            RenterSummary ns;
            ns.name = r.renterName;
            ns.totalSpent = r.totalPaid;
            ns.rentalCount = 1;
            summaries.push_back(ns);
        }
    }
    return summaries;
}

void dailySummaryAndRevenueReport() {
    vector<Rental> rentals = loadRentals();
    string today = getCurrentDateString();

    int totalBookings = 0;
    int totalBicyclesNotReturned = 0;

    double totalCollectedMoney = 0.0;
    double totalDepositCollected = 0.0;
    double totalDepositReturned = 0.0;

    const int NUM_METHODS = 5;
    string methodNames[NUM_METHODS] = { "QR Payment", "Bank Transfer", "Cash", "Card", "QR Code" };
    double methodTotals[NUM_METHODS] = { 0, 0, 0, 0, 0 };

    for (const auto& r : rentals) {
        if (r.rentDate == today) {
 
            totalBookings++;
            totalCollectedMoney += r.totalPaid;
            totalDepositCollected += (r.depositPerUnit * r.quantity);

            for (int i = 0; i < NUM_METHODS; i++) {
                if (r.paymentMethod == methodNames[i]) {
                    methodTotals[i] += r.totalPaid;
                    break;
                }
            }

            for (const auto& u : r.units) {
                if (!u.isReturned) {
                    totalBicyclesNotReturned++;
                }
            }
        }

        if (r.status == "Return" && r.returnDate == today) {
            for (const auto& u : r.units) {
                totalDepositReturned += u.depositRefunded;
            }
        }
    }

    double totalRevenue = totalCollectedMoney - totalDepositReturned;

    // --- Print the Report ---
    printLine();
    cout << "DAILY SUMMARY & REVENUE REPORT - " << today << "\n";
    printLine();

    cout << left << setw(30) << "Total Booking:" << totalBookings << " reservation(s)\n";
    cout << left << setw(30) << "Total Bicycle Not Return:" << totalBicyclesNotReturned << " unit(s)\n\n";

    cout << fixed << setprecision(2);
    cout << "Total Collected Money: RM" << totalCollectedMoney << "\n";
    cout << "Breakdown by Payment Method:\n";
    for (int i = 0; i < NUM_METHODS; i++) {
        if (methodTotals[i] > 0) {
            cout << "  - " << left << setw(15) << methodNames[i]
                << ": RM" << methodTotals[i] << "\n";
        }
    }

    cout << "\n";
    cout << left << setw(30) << "Total Deposit Collected:" << "RM" << totalDepositCollected << "\n";
    cout << left << setw(30) << "Total Deposit Return:" << "RM" << totalDepositReturned << "\n";
    printLine();
    cout << left << setw(30) << "TOTAL REVENUE:" << "RM" << totalRevenue << "\n";
    printLine();
    pauseScreen();
}

void mostRentedTypeReport() {
    vector<BikeType> bikeTypes = loadBikeTypes();

    if (bikeTypes.empty()) {
        cout << "No bike types found.\n";
        pauseScreen();
        return;
    }

    int maxRented = -1;
    size_t maxIndex = 0;
    for (size_t i = 0; i < bikeTypes.size(); i++) {
        if (bikeTypes[i].timesRented > maxRented) {
            maxRented = bikeTypes[i].timesRented;
            maxIndex = i;
        }
    }

    printLine();
    cout << "MOST RENTED BIKE TYPE\n";
    printLine();
    cout << "Type: " << bikeTypes[maxIndex].type << "\n";
    cout << "Total Units Rented (all time): " << bikeTypes[maxIndex].timesRented << "\n";
    printLine();
    pauseScreen();
}

// Breaks down total repair spend by part (tyre/brake/chain).
void damageCostReport() {
    vector<Rental> rentals = loadRentals();
    vector<BikeType> bikeTypes = loadBikeTypes();

    const int PARTS = 3;
    string partNames[PARTS] = { "Tyre", "Brake", "Chain" };
    double partTotals[PARTS] = { 0, 0, 0 };
    int partCounts[PARTS] = { 0, 0, 0 };

    for (const auto& r : rentals) {
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

    printLine();
    cout << "DAMAGE COST REPORT\n";
    printLine();
    cout << fixed << setprecision(2);
    double grandTotal = 0;
    for (int i = 0; i < PARTS; i++) {
        cout << left << setw(10) << partNames[i] << setw(6) << partCounts[i]
            << "incidents   RM" << partTotals[i] << "\n";
        grandTotal += partTotals[i];
    }
    printLine();
    cout << "Total Repair Cost Deducted: RM" << grandTotal << "\n";
    printLine();
    pauseScreen();
}

// Counts and totals deposits forfeited due to late returns
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

    printLine();
    cout << "LATE RETURN REPORT\n";
    printLine();
    cout << "Late Units Returned: " << lateUnitCount << "\n";
    cout << fixed << setprecision(2);
    cout << "Total Deposits Forfeited: RM" << forfeitedTotal << "\n";
    printLine();
    pauseScreen();
}

//Top 3 renters by total spending
void topRentersReport() {
    vector<Rental> rentals = loadRentals();
    vector<RenterSummary> summaries = buildRenterSummaries(rentals);

    printLine();
    cout << "TOP RENTERS BY SPENDING\n";
    printLine();

    if (summaries.empty()) {
        cout << "No completed rentals yet.\n";
        printLine();
        pauseScreen();
        return;
    }

    int topN = (int)summaries.size() < 3 ? (int)summaries.size() : 3;
    vector<bool> used(summaries.size(), false);

    for (int rank = 1; rank <= topN; rank++) {
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

        cout << rank << ". " << summaries[maxIdx].name << " - RM" << fixed << setprecision(2)
            << summaries[maxIdx].totalSpent << " (" << summaries[maxIdx].rentalCount << " rentals)\n";
    }
    printLine();
    pauseScreen();
}

/* Lists rentals not yet Returned. Flags Active rentals whose date has
   already passed as OVERDUE, and shows Pending (awaiting activation). */
void overdueList() {
    vector<Rental> rentals = loadRentals();
    vector<BikeType> bikeTypes = loadBikeTypes();
    string today = getCurrentDateString();
    bool any = false;

    printLine();
    cout << "ACTIVE / PENDING / OVERDUE RENTALS LIST\n";
    printLine();
    cout << left << setw(9) << "RentalID" << setw(12) << "Renter" << setw(14)
        << "Bike Type" << setw(5) << "Qty" << setw(12) << "Date" << setw(10) << "Status" << "\n";

    for (const auto& r : rentals) {
        if (r.status != "Return") {
            any = true;
            string bikeTypeName = "Unknown";
            for (const auto& b : bikeTypes) {
                if (b.id == r.bikeTypeId) { bikeTypeName = b.type; break; }
            }
            string displayStatus = r.status;
            if (r.status == "Active" && r.rentDate < today) displayStatus = "OVERDUE";

            cout << left << setw(9) << r.rentalId << setw(12) << r.renterName
                << setw(14) << bikeTypeName << setw(5) << r.quantity
                << setw(12) << r.rentDate << setw(10) << displayStatus << "\n";
        }
    }
    if (!any) cout << "No active, pending, or overdue rentals at the moment.\n";
    printLine();
    pauseScreen();
}

void memberD_menu() {
    int choice;
    do {
        printLine();
        cout << "REPORTING & STATISTICS (ADMIN)\n";
        printLine();
        cout << "1. Daily Summary & Revenue Report\n";
        cout << "2. Most Rented Bike Type\n";
        cout << "3. Damage Cost Report\n";
        cout << "4. Late Return Report\n";
        cout << "5. Top Renters Report\n";
        cout << "6. Active / Pending / Overdue Rentals List\n";
        cout << "0. Back to Main Menu\n";
        choice = getValidatedInt("Enter choice: ", 0, 6);

        switch (choice) {
        case 1: dailySummaryAndRevenueReport(); break;
        case 2: mostRentedTypeReport(); break;
        case 3: damageCostReport(); break;
        case 4: lateReturnReport(); break;
        case 5: topRentersReport(); break;
        case 6: overdueList(); break;
        case 0: cout << "Returning to main menu...\n"; break;
        }
    } while (choice != 0);
}