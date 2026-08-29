/*==================================================================
  memberC.cpp  -  Member C: Return Processing & Billing

  what this file does:
  - activateReservation(): Pending booking -> Active when the
    renter shows the code at the counter
  - processReturn(): the big one. work out if it is late, ask the
    condition of each bike, work out each refund, put bikes back
    in stock, and (once every bike is back) give loyalty points
  - viewReceiptMenu(): look up any rental by its code
==================================================================*/
#include "memberC.h"
#include "memberA.h"
#include "memberB.h"
#include <iostream>
#include <iomanip>
using namespace std;

/*==================================================================
  this method turns a "Pending" booking into "Active". the renter
  gives the reservation code; if it is found and still Pending we
  flip it so they can collect the bikes
==================================================================*/
void activateReservation() {
    uiHeader("ACTIVATE RESERVATION");

    vector<Rental> rentals = loadRentals();
    string code;
    if (stepString("Enter reservation code (or 'q' to cancel): ", code) == InputResult::BACK) {
        cout << "\nOperation cancelled.\n";
        pauseScreen();
        return;
    }

    // find the rental with this code
    Rental* found = nullptr;
    for (auto& r : rentals) {
        if (r.code == code) { found = &r; break; }
    }

    if (found == nullptr) {
        cout << "\nNo reservation matches that code.\n";
    }
    else if (found->status == "Active") {
        cout << "\nThis reservation is already active.\n";
    }
    else if (found->status == "Return") {
        cout << "\nThis reservation has already been completed and returned.\n";
    }
    else {
        found->status = "Active";
        saveRentals(rentals);
        cout << "\nReservation " << found->code << " activated successfully.\n";
        cout << "  Renter               : " << found->renterName << "\n";
        cout << "  Units to collect     : " << found->quantity << "\n";
        cout << "  Return on or before  : " << found->dueDate << " (by "
            << SHOP_CLOSE_HOUR << ":00)\n";
    }
    pauseScreen();
}

/*==================================================================
  this method asks the admin about ONE bike being returned:
  Good or Damaged, and (if Damaged) which parts. 'q' steps back.
  returns false if the admin backed all the way out of this bike
==================================================================*/
static bool processUnitWizard(RentalUnit& unit) {
    string cond = unit.condition == "-" ? "" : unit.condition;
    bool tyre = unit.tyreDamaged, brake = unit.brakeDamaged, chain = unit.chainDamaged;

    int step = 0;
    while (step < 4) {
        // if it is "Good", skip the tyre/brake/chain questions
        if (step >= 1 && cond != "Damaged" && cond != "damaged") {
            break;
        }

        InputResult res = InputResult::OK;
        switch (step) {
        case 0:
            while (true) {
                res = stepString("  Condition (Good/Damaged): ", cond);
                if (res == InputResult::BACK) break;
                if (cond != "Good" && cond != "good" && cond != "Damaged" && cond != "damaged") {
                    cout << "  Please enter 'Good' or 'Damaged' (or 'q' to go back).\n";
                    continue;
                }
                break;
            }
            break;
        case 1: { bool v = tyre;  res = stepConfirm("  Tyre damaged? (y/n): ", v);  tyre = v;  break; }
        case 2: { bool v = brake; res = stepConfirm("  Brake damaged? (y/n): ", v); brake = v; break; }
        case 3: { bool v = chain; res = stepConfirm("  Chain damaged? (y/n): ", v); chain = v; break; }
        }

        if (res == InputResult::BACK) {
            step--;
            if (step < 0) return false;
        }
        else {
            step++;
        }
    }

    // tidy the capitalisation and copy the answers into the unit
    unit.condition = (cond == "good") ? "Good" : (cond == "damaged") ? "Damaged" : cond;
    unit.tyreDamaged = tyre;
    unit.brakeDamaged = brake;
    unit.chainDamaged = chain;
    return true;
}

/*==================================================================
  this method processes a bike return from start to finish:
  1. find the rental by code, check it is "Active"
  2. decide if it is late  (later than the due date, or on the
     due date after the shop closes)
  3. ask the condition of every bike not yet back
  4. work out each refund:
       late            -> RM0
       damaged         -> deposit minus the damaged parts' cost
       good & on time  -> full deposit
  5. put the bikes back in stock
  6. if every bike is now back: mark "Return", and give loyalty
     points once (linked accounts only)
  7. print the return receipt
==================================================================*/
void processReturn() {
    uiHeader("PROCESS BIKE RETURN");

    vector<Rental> rentals = loadRentals();
    string code;
    if (stepString("Enter reservation/reference code (or 'q' to cancel): ", code) == InputResult::BACK) {
        cout << "\nOperation cancelled.\n";
        pauseScreen();
        return;
    }

    // 1) find the rental
    Rental* rentalPtr = nullptr;
    for (auto& r : rentals) {
        if (r.code == code) { rentalPtr = &r; break; }
    }

    if (rentalPtr == nullptr) {
        cout << "\nNo rental record matches that code.\n";
        pauseScreen();
        return;
    }
    Rental& rental = *rentalPtr;

    // must be "Active" - not still Pending, not already Returned
    if (rental.status == "Pending") {
        cout << "\nThis reservation has not been activated yet. Please activate it before processing a return.\n";
        pauseScreen();
        return;
    }
    if (rental.status == "Return") {
        cout << "\nThis rental has already been fully returned.\n";
        pauseScreen();
        return;
    }

    // need the bike type for its repair costs + to put stock back
    vector<BikeType> bikeTypes = loadBikeTypes();
    BikeType* bikeType = searchBikeTypeById(bikeTypes, rental.bikeTypeId);
    if (bikeType == nullptr) {
        cout << "\nError: the bike type linked to this rental could not be found in the inventory.\n";
        pauseScreen();
        return;
    }

    // 2) late?  late = after the due date, OR on the due date after closing time
    string today = getCurrentDateString();
    int currentHour = getCurrentHour();
    string dueRef = rental.dueDate.empty() ? rental.rentDate : rental.dueDate;

    bool late = (today > dueRef) || (today == dueRef && currentHour >= SHOP_CLOSE_HOUR);
    bool onTime = !late;

    if (late) {
        cout << "\nNotice: this return is LATE (after " << SHOP_CLOSE_HOUR
            << ":00 on the due date " << dueRef << ", or on a later day).\n";
        cout << "In line with the rental policy, the deposit is fully forfeited for\n";
        cout << "every unit on this rental, regardless of its condition.\n\n";
    }

    // which bikes are still out?
    vector<int> pendingIndices;
    for (size_t i = 0; i < rental.units.size(); i++) {
        if (!rental.units[i].isReturned) pendingIndices.push_back((int)i);
    }

    if (pendingIndices.empty()) {
        cout << "\nEvery unit on this rental has already been returned.\n";
        pauseScreen();
        return;
    }

    // 3) ask the condition of each bike still out. going back past the
    //    first one cancels the whole return
    cout << "Please record the condition of each unit (" << pendingIndices.size()
        << " remaining). Type 'q' at any prompt to go back.\n";

    size_t unitPos = 0;
    while (unitPos < pendingIndices.size()) {
        RentalUnit& unit = rental.units[pendingIndices[unitPos]];
        cout << "\n--- Unit #" << unit.unitIndex << " ---\n";

        bool ok = processUnitWizard(unit);
        if (!ok) {
            if (unitPos == 0) {
                cout << "\nReturn processing cancelled. No changes were saved.\n";
                pauseScreen();
                return;
            }
            unitPos--;   // step back to the previous bike
            continue;
        }
        unitPos++;
    }

    // 4) work out each refund and add them up
    int unitsReturnedNow = 0;
    double totalRefund = 0.0;

    for (int idx : pendingIndices) {
        RentalUnit& unit = rental.units[idx];

        // add up the cost of the damaged parts
        double damageCost = 0.0;
        if (unit.condition == "Damaged") {
            if (unit.tyreDamaged)  damageCost += bikeType->tyreCost;
            if (unit.brakeDamaged) damageCost += bikeType->brakeCost;
            if (unit.chainDamaged) damageCost += bikeType->chainCost;
        }
        unit.damageCost = damageCost;
        unit.returnedOnTime = onTime;

        double refund;
        if (!onTime) {
            refund = 0.0;                                   // late -> nothing back
        }
        else if (unit.condition == "Damaged") {
            refund = rental.depositPerUnit - damageCost;    // damaged -> deposit minus repairs
            if (refund < 0) refund = 0;
        }
        else {
            refund = rental.depositPerUnit;                 // good + on time -> full deposit
        }
        unit.depositRefunded = refund;
        unit.isReturned = true;

        totalRefund += refund;
        unitsReturnedNow++;
    }

    // 5) put the returned bikes back in the right stock pool
    if (rental.channel == "Booking") {
        bikeType->bookingQuantity += unitsReturnedNow;
    }
    else {
        bikeType->walkinQuantity += unitsReturnedNow;
    }
    saveBikeTypes(bikeTypes);

    // 6) if every bike is back, close the rental and give points once
    bool allReturned = true;
    for (const auto& u : rental.units) {
        if (!u.isReturned) { allReturned = false; break; }
    }
    if (allReturned) {
        rental.status = "Return";
        rental.returnDate = today;

        if (rental.memberId != 0 && !rental.pointsAwarded) {
            // RM1 of rental price (qty x price/day x days) = 1 point; deposit does not count
            int pts = (int)(rental.quantity * rental.pricePerDay * rental.rentalDays);
            rental.pointsEarned = pts;
            rental.pointsAwarded = true;
            addPoints(rental.memberId, pts);
        }
    }

    saveRentals(rentals);

    // 7) print the return receipt
    printLine();
    printCentered("RETURN RECEIPT - " + rental.code + "  (" + bikeType->type + ")");
    printLine();
    cout << fixed << setprecision(2);
    cout << "  " << left << setw(24) << "Renter:" << rental.renterName << "\n";
    cout << "  " << left << setw(24) << "Due Date:" << dueRef << "\n";
    cout << "  " << left << setw(24) << "Returned On Time:" << (onTime ? "Yes" : "No (LATE)") << "\n";
    printLine();
    for (const auto& unit : rental.units) {
        if (unit.unitIndex == 0) continue;
        cout << "  Unit #" << unit.unitIndex << " - " << unit.condition;
        if (unit.tyreDamaged || unit.brakeDamaged || unit.chainDamaged) {
            cout << " (";
            if (unit.tyreDamaged) cout << "Tyre ";
            if (unit.brakeDamaged) cout << "Brake ";
            if (unit.chainDamaged) cout << "Chain";
            cout << ")";
        }
        cout << "  ->  Refund: RM" << unit.depositRefunded << "\n";
    }
    printLine();
    cout << "  " << left << setw(24) << "Total Deposit Refunded:" << "RM" << totalRefund << "\n";
    cout << "  " << left << setw(24) << "Rental Status:" << rental.status << "\n";
    printLine();
    pauseScreen();
}

/*==================================================================
  this method looks up one rental by its code and prints the full
  record: renter, bike, dates, money, status, and each bike's
  condition / refund
==================================================================*/
void viewReceiptMenu() {
    vector<Rental> rentals = loadRentals();
    vector<BikeType> bikeTypes = loadBikeTypes();
    uiHeader("VIEW RENTAL RECORD / RECEIPT");

    string code;
    if (stepString("Enter reservation/reference code (or 'q' to cancel): ", code) == InputResult::BACK) {
        cout << "\nOperation cancelled.\n";
        pauseScreen();
        return;
    }

    for (const auto& r : rentals) {
        if (r.code == code) {
            // turn the bike type id into its name
            string bikeTypeName = "Unknown";
            for (const auto& b : bikeTypes) {
                if (b.id == r.bikeTypeId) { bikeTypeName = b.type; break; }
            }
            printLine();
            printCentered("RENTAL RECORD - " + r.code);
            printLine();
            cout << fixed << setprecision(2);
            cout << "  " << left << setw(22) << "Renter:" << r.renterName << "\n";
            cout << "  " << left << setw(22) << "Bike Type:" << bikeTypeName << "\n";
            cout << "  " << left << setw(22) << "Quantity:" << r.quantity << " unit(s)\n";
            cout << "  " << left << setw(22) << "Start Date:" << r.rentDate << "\n";
            cout << "  " << left << setw(22) << "Duration:" << r.rentalDays << " day(s)\n";
            cout << "  " << left << setw(22) << "Due Date:" << r.dueDate << "\n";
            cout << "  " << left << setw(22) << "Total Paid:" << "RM" << r.totalPaid << "\n";
            cout << "  " << left << setw(22) << "Payment Method:" << r.paymentMethod << "\n";
            cout << "  " << left << setw(22) << "Status:" << r.status << "\n";
            if (r.status == "Return") {
                cout << "  " << left << setw(22) << "Return Date:" << r.returnDate << "\n";
            }
            printLine();
            for (const auto& unit : r.units) {
                cout << "  Unit #" << unit.unitIndex << ": " << unit.condition
                    << "  | Refund: RM" << unit.depositRefunded
                    << "  | Returned: " << (unit.isReturned ? "Yes" : "No") << "\n";
            }
            printLine();
            pauseScreen();
            return;
        }
    }
    cout << "\nNo rental record matches that code.\n";
    pauseScreen();
}

/*==================================================================
  this method is the admin "returns & billing" menu. loops until 0
==================================================================*/
void memberC_menu() {
    int choice;
    do {
        uiHeader("RETURN PROCESSING & BILLING  (ADMIN)");
        printCentered("1. Process Bike Return");
        printCentered("2. View Rental Record / Receipt");
        printCentered("0. Back to Main Menu");
        printLine();
        choice = getValidatedInt("Enter choice: ", 0, 2);

        switch (choice) {
        case 1: processReturn(); break;
        case 2: viewReceiptMenu(); break;
        case 0: cout << "\nReturning to the main menu...\n"; break;
        }
    } while (choice != 0);
}
