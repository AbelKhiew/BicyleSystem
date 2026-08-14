/* ============================================================
   memberC.cpp
   MEMBER C - Return Processing & Billing / Payment Module

   Responsibilities:
   - Activate a "Pending" future-dated booking when the renter shows
     up and tells the admin the reservation code
   - Process the return of a rental: checks on-time (same day, before
     shop close); admin records EACH unit's condition individually
     (multiple damaged parts per unit allowed); refund computed per
     unit and summed
   - Restore returned units to the correct quantity pool
   - Award loyalty points once, when ALL units are back (linked
     accounts only), based on day price only (deposit excluded)
   ============================================================ */
#include "memberC.h"
#include "memberA.h"     // addPoints()
#include "memberB.h"     // searchBikeTypeById()
#include <iostream>
#include <iomanip>
using namespace std;

/* Admin activates a future-dated ("Pending") booking once the renter
   arrives and provides the reservation code. */
void activateReservation() {
    printLine();
    cout << "ACTIVATE RESERVATION\n";
    printLine();

    vector<Rental> rentals = loadRentals();
    string code = getValidatedString("Enter reservation code: ");

    Rental* found = nullptr;
    for (auto& r : rentals) {
        if (r.code == code) { found = &r; break; }
    }

    if (found == nullptr) {
        cout << "No reservation found with this code.\n";
    }
    else if (found->status == "Active") {
        cout << "This reservation is already active.\n";
    }
    else if (found->status == "Completed") {
        cout << "This reservation has already been completed.\n";
    }
    else {
        found->status = "Active";
        saveRentals(rentals);
        cout << "Reservation activated! " << found->renterName << " (" << found->quantity
            << "x bike) may now collect their bike(s).\n";
    }
    pauseScreen();
}

void processReturn() {
    printLine();
    cout << "PROCESS BIKE RETURN\n";
    printLine();

    vector<Rental> rentals = loadRentals();
    string code = getValidatedString("Enter reservation/reference code: ");

    Rental* rentalPtr = nullptr;
    for (auto& r : rentals) {
        if (r.code == code) { rentalPtr = &r; break; }
    }

    if (rentalPtr == nullptr) {
        cout << "No rental found with this code.\n";
        pauseScreen();
        return;
    }
    Rental& rental = *rentalPtr;

    if (rental.status == "Pending") {
        cout << "This reservation has not been activated yet. Please activate it first.\n";
        pauseScreen();
        return;
    }
    if (rental.status == "Completed") {
        cout << "This rental has already been fully returned.\n";
        pauseScreen();
        return;
    }

    vector<BikeType> bikeTypes = loadBikeTypes();
    BikeType* bikeType = searchBikeTypeById(bikeTypes, rental.bikeTypeId);
    if (bikeType == nullptr) {
        cout << "Associated bike type not found. Data error.\n";
        pauseScreen();
        return;
    }

    /* On-time check: must be the same day as rentDate, before shop close.
       Applied once for this whole return transaction (all units together). */
    string today = getCurrentDateString();
    int currentHour = getCurrentHour();
    bool onTime = (today == rental.rentDate) && (currentHour < SHOP_CLOSE_HOUR);

    if (!onTime) {
        cout << "\nNOTICE: This return is LATE (after " << SHOP_CLOSE_HOUR
            << ":00 on the rental date, or a different day).\n";
        cout << "Deposit will be FULLY FORFEITED for every unit, regardless of condition.\n\n";
    }

    int unitsReturnedNow = 0;
    double totalRefund = 0.0;

    cout << "Recording condition for each unit (" << rental.quantity << " total):\n";
    for (auto& unit : rental.units) {
        if (unit.isReturned) continue;   // already processed in a previous session

        cout << "\n--- Unit #" << unit.unitIndex << " ---\n";
        string cond = getValidatedString("Condition (Good/Damaged): ");
        unit.condition = cond;
        unit.returnedOnTime = onTime;

        double damageCost = 0.0;
        if (cond == "Damaged" || cond == "damaged") {
            string ans;
            ans = getValidatedString("  Tyre damaged? (y/n): ");
            unit.tyreDamaged = (ans == "y" || ans == "Y");
            if (unit.tyreDamaged) damageCost += bikeType->tyreCost;

            ans = getValidatedString("  Brake damaged? (y/n): ");
            unit.brakeDamaged = (ans == "y" || ans == "Y");
            if (unit.brakeDamaged) damageCost += bikeType->brakeCost;

            ans = getValidatedString("  Chain damaged? (y/n): ");
            unit.chainDamaged = (ans == "y" || ans == "Y");
            if (unit.chainDamaged) damageCost += bikeType->chainCost;
        }
        unit.damageCost = damageCost;

        double refund;
        if (!onTime) {
            refund = 0.0;
        }
        else if (cond == "Damaged" || cond == "damaged") {
            refund = rental.depositPerUnit - damageCost;
            if (refund < 0) refund = 0;
        }
        else {
            refund = rental.depositPerUnit;
        }
        unit.depositRefunded = refund;
        unit.isReturned = true;

        totalRefund += refund;
        unitsReturnedNow++;
    }

    if (unitsReturnedNow == 0) {
        cout << "\nAll units on this rental have already been returned.\n";
        pauseScreen();
        return;
    }

    /* Restore returned units to the correct quantity pool */
    if (rental.channel == "Booking") {
        bikeType->bookingQuantity += unitsReturnedNow;
    }
    else {
        bikeType->walkinQuantity += unitsReturnedNow;
    }
    saveBikeTypes(bikeTypes);

    /* Check if the whole rental is now complete */
    bool allReturned = true;
    for (const auto& u : rental.units) {
        if (!u.isReturned) { allReturned = false; break; }
    }
    if (allReturned) {
        rental.status = "Completed";
        rental.returnDate = today;

        if (rental.memberId != 0 && !rental.pointsAwarded) {
            int pts = rental.quantity * (int)rental.pricePerDay;   // RM1/day/unit = 1 point
            rental.pointsEarned = pts;
            rental.pointsAwarded = true;
            addPoints(rental.memberId, pts);
        }
    }

    saveRentals(rentals);

    /* Receipt for this return transaction */
    printLine();
    cout << "RETURN RECEIPT - Rental " << rental.code << " (" << bikeType->type << ")\n";
    printLine();
    cout << fixed << setprecision(2);
    cout << left << setw(24) << "Renter:" << rental.renterName << "\n";
    cout << left << setw(24) << "Returned On Time:" << (onTime ? "Yes" : "No (LATE)") << "\n";
    printLine();
    for (const auto& unit : rental.units) {
        if (unit.unitIndex == 0) continue;
        cout << "Unit #" << unit.unitIndex << " - " << unit.condition;
        if (unit.tyreDamaged || unit.brakeDamaged || unit.chainDamaged) {
            cout << " (";
            if (unit.tyreDamaged) cout << "Tyre ";
            if (unit.brakeDamaged) cout << "Brake ";
            if (unit.chainDamaged) cout << "Chain";
            cout << ")";
        }
        cout << " -> Refund: RM" << unit.depositRefunded << "\n";
    }
    printLine();
    cout << left << setw(24) << "Total Deposit Refunded:" << "RM" << totalRefund << "\n";
    cout << left << setw(24) << "Rental Status:" << rental.status << "\n";
    printLine();
    pauseScreen();
}

void viewReceiptMenu() {
    vector<Rental> rentals = loadRentals();
    vector<BikeType> bikeTypes = loadBikeTypes();
    string code = getValidatedString("Enter reservation/reference code: ");

    for (const auto& r : rentals) {
        if (r.code == code) {
            string bikeTypeName = "Unknown";
            for (const auto& b : bikeTypes) {
                if (b.id == r.bikeTypeId) { bikeTypeName = b.type; break; }
            }
            printLine();
            cout << "RENTAL RECORD - " << r.code << "\n";
            printLine();
            cout << fixed << setprecision(2);
            cout << left << setw(22) << "Renter:" << r.renterName << "\n";
            cout << left << setw(22) << "Bike Type:" << bikeTypeName << "\n";
            cout << left << setw(22) << "Quantity:" << r.quantity << "\n";
            cout << left << setw(22) << "Date:" << r.rentDate << "\n";
            cout << left << setw(22) << "Total Paid:" << "RM" << r.totalPaid << "\n";
            cout << left << setw(22) << "Payment Method:" << r.paymentMethod << "\n";
            cout << left << setw(22) << "Status:" << r.status << "\n";
            printLine();
            for (const auto& unit : r.units) {
                cout << "Unit #" << unit.unitIndex << ": " << unit.condition
                    << " | Refund: RM" << unit.depositRefunded
                    << " | Returned: " << (unit.isReturned ? "Yes" : "No") << "\n";
            }
            printLine();
            pauseScreen();
            return;
        }
    }
    cout << "No rental found with this code.\n";
    pauseScreen();
}

void memberC_menu() {
    int choice;
    do {
        printLine();
        cout << "RETURN PROCESSING & BILLING (ADMIN)\n";
        printLine();
        cout << "1. Activate Reservation (future bookings)\n";
        cout << "2. Process Bike Return\n";
        cout << "3. View Rental Record / Receipt\n";
        cout << "0. Back to Main Menu\n";
        choice = getValidatedInt("Enter choice: ", 0, 3);

        switch (choice) {
        case 1: activateReservation(); break;
        case 2: processReturn(); break;
        case 3: viewReceiptMenu(); break;
        case 0: cout << "Returning to main menu...\n"; break;
        }
    } while (choice != 0);
}