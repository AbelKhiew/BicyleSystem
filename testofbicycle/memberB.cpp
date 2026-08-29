/* ============================================================
    Bicycle Inventory & Rental Booking Module

   Responsibilities:
   - Bike inventory by TYPE (quantity pools, price/day, damage-part
     costs, auto-calculated deposit) - admin add/edit/view
   - Renter self-service booking: choose quantity, choose date
     ('t' for today or a validated future date), invoice shown,
     payment via QR/Bank Transfer, reservation code generated.
     EVERY self-booking (today or future) starts "Pending" - it
     only becomes "Active" once the renter shows the admin the
     reservation code and admin activates it at the counter, which
     represents confirmed pickup of the bike(s).
   - Admin walk-in rental: separate quantity pool, optional link to
     an existing account (for points), payment via Cash/Card/QR
     Code - Cash collects the amount given and calculates change.
     Walk-in rentals start "Active" immediately, since admin is
     personally handing the bike(s) over at the counter.
   ============================================================ */
#include "memberB.h"
#include "memberA.h"
#include <iostream>
#include <iomanip>
using namespace std;

BikeType* searchBikeTypeById(vector<BikeType>& bikeTypes, int id) {
    for (auto& b : bikeTypes) if (b.id == id) return &b;
    return nullptr;
}

// Deposit = tyre repair cost + brake repair cost + chain repair cost + RM20 buffer
double calcDeposit(double tyreCost, double brakeCost, double chainCost) {
    return tyreCost + brakeCost + chainCost + 20.0;
}

//Admin: inventory management
void addBikeType() {
    printLine();
    cout << "ADD NEW BIKE TYPE (type 'q' at any prompt to go back)\n";
    printLine();

    vector<BikeType> bikeTypes = loadBikeTypes();

    string typeName;
    double pricePerDay = 0, tyreCost = 0, brakeCost = 0, chainCost = 0;
    int bookingQty = 0, walkinQty = 0;

    int step = 0;
    while (step < 7) {
        InputResult res = InputResult::OK;
        switch (step) {
        case 0: res = stepString("Enter bike type name (e.g. Mountain Bike): ", typeName); break;
        case 1: res = stepDouble("Enter price per day (RM): ", 0.5, 1000.0, pricePerDay); break;
        case 2: res = stepDouble("Enter tyre repair cost (RM): ", 0.0, 500.0, tyreCost); break;
        case 3: res = stepDouble("Enter brake repair cost (RM): ", 0.0, 500.0, brakeCost); break;
        case 4: res = stepDouble("Enter chain repair cost (RM): ", 0.0, 500.0, chainCost); break;
        case 5: res = stepInt("Enter quantity for online booking: ", 0, 1000, bookingQty); break;
        case 6: res = stepInt("Enter quantity for walk-in rental: ", 0, 1000, walkinQty); break;
        }

        if (res == InputResult::BACK) {
            step--;
            if (step < 0) {
                cout << "Cancelled adding bike type.\n";
                pauseScreen();
                return;
            }
        }
        else {
            step++;
        }
    }

    BikeType newType;
    newType.id = getNextBikeTypeId(bikeTypes);
    newType.type = typeName;
    newType.pricePerDay = pricePerDay;
    newType.tyreCost = tyreCost;
    newType.brakeCost = brakeCost;
    newType.chainCost = chainCost;
    newType.deposit = calcDeposit(tyreCost, brakeCost, chainCost);
    newType.bookingQuantity = bookingQty;
    newType.walkinQuantity = walkinQty;
    newType.timesRented = 0;

    bikeTypes.push_back(newType);
    saveBikeTypes(bikeTypes);

    cout << "\nBike type added! ID: " << newType.id
        << " | Auto-calculated deposit: RM" << fixed << setprecision(2) << newType.deposit << "\n";
    pauseScreen();
}

void editBikeType() {
    vector<BikeType> bikeTypes = loadBikeTypes();
    viewBikeTypes();

    int id;
    if (stepInt("Enter Bike Type ID to edit (or 'q' to cancel): ", 1, 999999, id) == InputResult::BACK) {
        cout << "Cancelled.\n";
        pauseScreen();
        return;
    }
    BikeType* b = searchBikeTypeById(bikeTypes, id);
    if (b == nullptr) {
        cout << "Bike type not found.\n";
        pauseScreen();
        return;
    }

    cout << "Re-enter each value (current shown in brackets). Type 'q' at any prompt to go back.\n";

    string typeName = b->type;
    double pricePerDay = b->pricePerDay, tyreCost = b->tyreCost, brakeCost = b->brakeCost, chainCost = b->chainCost;
    int bookingQty = b->bookingQuantity, walkinQty = b->walkinQuantity;

    int step = 0;
    while (step < 7) {
        InputResult res = InputResult::OK;
        switch (step) {
        case 0: res = stepString("Type name [" + b->type + "]: ", typeName); break;
        case 1: res = stepDouble("Price per day (RM): ", 0.5, 1000.0, pricePerDay); break;
        case 2: res = stepDouble("Tyre repair cost (RM): ", 0.0, 500.0, tyreCost); break;
        case 3: res = stepDouble("Brake repair cost (RM): ", 0.0, 500.0, brakeCost); break;
        case 4: res = stepDouble("Chain repair cost (RM): ", 0.0, 500.0, chainCost); break;
        case 5: res = stepInt("Quantity for online booking: ", 0, 1000, bookingQty); break;
        case 6: res = stepInt("Quantity for walk-in rental: ", 0, 1000, walkinQty); break;
        }

        if (res == InputResult::BACK) {
            step--;
            if (step < 0) {
                cout << "Edit cancelled - no changes saved.\n";
                pauseScreen();
                return;
            }
        }
        else {
            step++;
        }
    }

    b->type = typeName;
    b->pricePerDay = pricePerDay;
    b->tyreCost = tyreCost;
    b->brakeCost = brakeCost;
    b->chainCost = chainCost;
    b->deposit = calcDeposit(tyreCost, brakeCost, chainCost);
    b->bookingQuantity = bookingQty;
    b->walkinQuantity = walkinQty;

    saveBikeTypes(bikeTypes);
    cout << "\nUpdated! New deposit: RM" << fixed << setprecision(2) << b->deposit << "\n";
    pauseScreen();
}

//Deletes a bike type entirely. Blocked if any non-Return
void deleteBikeType() {
    vector<BikeType> bikeTypes = loadBikeTypes();
    viewBikeTypes();

    int id;
    if (stepInt("Enter Bike Type ID to delete (or 'q' to cancel): ", 1, 999999, id) == InputResult::BACK) {
        cout << "Cancelled.\n";
        pauseScreen();
        return;
    }

    BikeType* target = searchBikeTypeById(bikeTypes, id);
    if (target == nullptr) {
        cout << "Bike type not found.\n";
        pauseScreen();
        return;
    }

    vector<Rental> rentals = loadRentals();
    for (const auto& r : rentals) {
        if (r.bikeTypeId == id && r.status != "Return") {
            cout << "Cannot delete - Rental ID " << r.rentalId << " (" << r.status
                << ") still references this bike type. It must be returned first.\n";
            pauseScreen();
            return;
        }
    }

    bool confirm = false;
    InputResult res = stepConfirm("Delete '" + target->type + "'? This cannot be undone. (y/n): ", confirm);
    if (res == InputResult::BACK || !confirm) {
        cout << "Deletion cancelled.\n";
        pauseScreen();
        return;
    }

    vector<BikeType> updated;
    for (const auto& b : bikeTypes) {
        if (b.id != id) updated.push_back(b);
    }
    saveBikeTypes(updated);
    cout << "Bike type deleted successfully.\n";
    pauseScreen();
}

void viewBikeTypes() {
    vector<BikeType> bikeTypes = loadBikeTypes();
    printLine();
    cout << left << setw(4) << "ID" << setw(14) << "Type" << setw(9) << "Price/Day"
        << setw(9) << "Deposit" << setw(9) << "BookQty" << setw(9) << "WalkQty"
        << setw(8) << "Rented" << "\n";
    printLine();

    if (bikeTypes.empty()) {
        cout << "No bike types found.\n";
    }
    else {
        for (const auto& b : bikeTypes) {
            cout << left << setw(4) << b.id << setw(14) << b.type << setw(9)
                << fixed << setprecision(2) << b.pricePerDay << setw(9) << b.deposit
                << setw(9) << b.bookingQuantity << setw(9) << b.walkinQuantity
                << setw(8) << b.timesRented << "\n";
        }
    }
    printLine();
    pauseScreen();
}

// Renter-facing simplified availability view
void viewAvailability() {
    vector<BikeType> bikeTypes = loadBikeTypes();
    printLine();
    cout << "BIKE AVAILABILITY\n";
    printLine();
    cout << left << setw(4) << "ID" << setw(16) << "Type" << setw(10) << "Price/Day"
        << setw(10) << "Deposit" << setw(10) << "Available" << "\n";

    if (bikeTypes.empty()) {
        cout << "No bikes available.\n";
    }
    else {
        for (const auto& b : bikeTypes) {
            cout << left << setw(4) << b.id << setw(16) << b.type << setw(10)
                << fixed << setprecision(2) << b.pricePerDay << setw(10) << b.deposit
                << setw(10) << b.bookingQuantity << "\n";
        }
    }
    printLine();
}

void printInvoicePreview(const string& renterName, const string& bikeTypeName, int quantity,
    double pricePerDay, const string& rentDate, double subtotalPrice,
    double depositTotal, int discountPercent, double discountAmount,
    double total, bool isWalkIn, double tyreCost, double brakeCost, double chainCost) {

    // 1. HEADER
    cout << "\n\n";
    printLine();
    cout << "                    INVOICE PREVIEW\n";
    printLine();
    cout << fixed << setprecision(2);

    // 2. RENTAL DETAILS
    if (isWalkIn) {
        cout << left << setw(25) << "Renter Name:" << renterName << "\n";
    }
    cout << left << setw(25) << "Rental Date:" << rentDate << (isWalkIn ? " (today)\n" : "\n");
    cout << left << setw(25) << "Bike Type:" << bikeTypeName << "\n";
    cout << left << setw(25) << "Quantity:" << quantity << " unit(s)\n";
    printLine();

    // 3. ITEMISED COSTS
    double depositPerUnit = (quantity > 0) ? (depositTotal / quantity) : 0.0;

    cout << left << setw(25) << "Price (per day):" << "RM " << pricePerDay << "\n";
    cout << left << setw(25) << "Deposit (per unit):" << "RM " << depositPerUnit << "\n";

    // Deposit Breakdown (Indented for readability)
    cout << "  " << left << setw(23) << "- Tyre Coverage:" << "RM " << tyreCost << "\n";
    cout << "  " << left << setw(23) << "- Brake Coverage:" << "RM " << brakeCost << "\n";
    cout << "  " << left << setw(23) << "- Chain Coverage:" << "RM " << chainCost << "\n";
    cout << "  " << left << setw(23) << "- Base Fee:" << "RM 20.00\n";
    cout << "\n";

    cout << left << setw(25) << "Rental Subtotal:" << "RM " << subtotalPrice << "\n";
    if (discountPercent > 0) {
        cout << left << setw(25) << ("Discount (" + to_string(discountPercent) + "%):")
            << "-RM " << discountAmount << "\n";
    }
    cout << left << setw(25) << "Deposit Total:" << "RM " << depositTotal << "\n";
    printLine();

    // 4. FINAL TOTAL
    cout << left << setw(25) << "TOTAL TO PAY:" << "RM " << total << "\n";
    printLine();

    // 5. FOOTER NOTE
    cout << "* Note: Deposit is fully refundable upon on-time return\n";
    cout << "        without any damage to the bicycle.\n";
    printLine();
}

bool collectPaymentAndReceipt(const string& renterName, const string& bikeTypeName,
    int quantity, const string& rentDate, double totalDue,
    const string& code, bool isWalkIn, string& paymentMethodOut) {
    string paymentMethod;
    double cashGiven = 0.0, change = 0.0;

    if (isWalkIn) {
        int choice;
        cout << "\nSelect Payment Method:\n1. Cash\n2. Card\n3. QR Code\n";
        if (stepInt("Enter choice (or 'q' to cancel): ", 1, 3, choice) == InputResult::BACK) return false;

        if (choice == 1) {
            paymentMethod = "Cash";
            while (true) {
                if (stepDouble("Enter cash received from renter (RM) (or 'q' to cancel): ",
                    0.0, 1000000.0, cashGiven) == InputResult::BACK) return false;
                if (cashGiven < totalDue) {
                    cout << "Insufficient cash - amount given is less than the total due (RM"
                        << fixed << setprecision(2) << totalDue << "). Please re-enter.\n";
                    continue;
                }
                change = cashGiven - totalDue;
                break;
            }
        }
        else if (choice == 2) {
            paymentMethod = "Card";
        }
        else {
            paymentMethod = "QR Code";
        }
    }
    else {
        int choice;
        cout << "\nSelect Payment Method:\n1. QR Payment\n2. Bank Transfer\n";
        if (stepInt("Enter choice (or 'q' to cancel): ", 1, 2, choice) == InputResult::BACK) return false;
        paymentMethod = (choice == 1) ? "QR Payment" : "Bank Transfer";
    }

    paymentMethodOut = paymentMethod;

    printLine();
    cout << "PAYMENT RECEIPT\n";
    printLine();
    cout << fixed << setprecision(2);
    cout << left << setw(22) << "Renter Name:" << renterName << "\n";
    cout << left << setw(22) << "Bike Type:" << bikeTypeName << "\n";
    cout << left << setw(22) << "Quantity:" << quantity << "\n";
    cout << left << setw(22) << "Rental Date:" << rentDate << "\n";
    cout << left << setw(22) << "Total Paid:" << "RM" << totalDue << "\n";
    cout << left << setw(22) << "Payment Method:" << paymentMethod << "\n";
    if (paymentMethod == "Cash") {
        cout << left << setw(22) << "Cash Given:" << "RM" << cashGiven << "\n";
        cout << left << setw(22) << "Change Returned:" << "RM" << change << "\n";
    }
    cout << left << setw(22) << "Reference Code:" << code << "\n";
    printLine();
    return true;
}

//Renter self-service booking
void bookBike(int memberId, const string& renterName) {
    printLine();
    cout << "BOOK A BICYCLE (type 'q' at any prompt to go back)\n";
    printLine();

    vector<BikeType> bikeTypes = loadBikeTypes();
    viewAvailability();

    vector<Member> members = loadMembers();
    Member* member = searchMemberById(members, memberId);
    int memberPoints = (member != nullptr) ? member->points : 0;

    int typeId = 0, quantity = 0;
    string rentDate;
    bool confirm = false;
    BikeType* bikeType = nullptr;
    int discountPercent = 0, pointsUsed = 0;
    double subtotalPrice = 0.0, depositTotal = 0.0, discountAmount = 0.0, total = 0.0;

    int step = 0;
    while (step < 5) {
        InputResult res = InputResult::OK;
        switch (step) {
        case 0:
            while (true) {
                res = stepInt("Enter Bike Type ID to book: ", 1, 999999, typeId);
                if (res == InputResult::BACK) break;
                bikeType = searchBikeTypeById(bikeTypes, typeId);
                if (bikeType == nullptr) {
                    cout << "Bike type not found. Please try again (or 'q' to go back).\n";
                    continue;
                }
                if (bikeType->bookingQuantity <= 0) {
                    cout << "Sorry, no units of this bike type are available. Choose another (or 'q' to go back).\n";
                    continue;
                }
                break;
            }
            break;
        case 1:
            res = stepInt("Enter quantity to rent (1-" + to_string(bikeType->bookingQuantity) + "): ",
                1, bikeType->bookingQuantity, quantity);
            break;
        case 2:
            res = stepDate("Enter rental date (YYYY-MM-DD, or 't' for today): ", rentDate);
            break;
        case 3:
            discountPercent = applyLoyaltyDiscount(memberPoints, pointsUsed);
            res = InputResult::OK;
            break;
        case 4: {
            subtotalPrice = quantity * bikeType->pricePerDay;
            depositTotal = quantity * bikeType->deposit;
            discountAmount = subtotalPrice * discountPercent / 100.0;
            total = (subtotalPrice - discountAmount) + depositTotal;

            printInvoicePreview("", bikeType->type, quantity, bikeType->pricePerDay,
                rentDate, subtotalPrice, depositTotal, discountPercent,
                discountAmount, total, false, bikeType->tyreCost,
                bikeType->brakeCost, bikeType->chainCost);

            res = stepConfirm("Confirm booking? (y/n): ", confirm);
            break;
        }
        }

        if (res == InputResult::BACK) {
            step--;
            if (step < 0) {
                cout << "Booking cancelled.\n";
                pauseScreen();
                return;
            }
        }
        else {
            step++;
        }
    }

    if (!confirm) {
        cout << "Booking cancelled.\n";
        pauseScreen();
        return;
    }

    string code = generateReservationCode();
    string paymentMethod;
    if (!collectPaymentAndReceipt(renterName, bikeType->type, quantity, rentDate, total, code, false, paymentMethod)) {
        cout << "Payment cancelled - booking was not created.\n";
        pauseScreen();
        return;
    }

    if (pointsUsed > 0) {
        deductPoints(memberId, pointsUsed);
    }

    bikeType->bookingQuantity -= quantity;
    bikeType->timesRented += quantity;
    saveBikeTypes(bikeTypes);

    //save data to json
    vector<Rental> rentals = loadRentals();
    Rental newRental;
    newRental.rentalId = getNextRentalId(rentals);
    newRental.channel = "Booking";
    newRental.memberId = memberId;
    newRental.renterName = renterName;
    newRental.bikeTypeId = typeId;
    newRental.quantity = quantity;
    newRental.rentDate = rentDate;
    newRental.code = code;
    newRental.pricePerDay = bikeType->pricePerDay;
    newRental.depositPerUnit = bikeType->deposit;
    newRental.discountApplied = discountPercent;
    newRental.discountPointsUsed = pointsUsed;
    newRental.totalPaid = total;
    newRental.paymentMethod = paymentMethod;
    newRental.status = "Pending";

    for (int i = 1; i <= quantity; i++) {
        RentalUnit unit;
        unit.unitIndex = i;
        newRental.units.push_back(unit);
    }

    rentals.push_back(newRental);
    saveRentals(rentals);

    cout << "\nYour reservation code is: " << code << "\n";
    cout << "Show this code to the admin at the counter to activate your booking\n";
    cout << "before you can collect the bike(s).\n";
    cout << "Please return before " << SHOP_CLOSE_HOUR << ":00 on " << rentDate
        << " to receive your deposit refund.\n";
    pauseScreen();
}

//Admin walk-in rental
void walkInRental() {
    printLine();
    cout << "WALK-IN RENTAL (ADMIN) - type 'q' at any prompt to go back\n";
    printLine();

    int linkedMemberId = 0;
    int linkedMemberPoints = 0;
    string renterName;
    bool hasAccount = false;

    int nameStep = 0;
    while (nameStep < 2) {
        InputResult res = InputResult::OK;
        switch (nameStep) {
        case 0:
            res = stepConfirm("Does the customer have an account? (y/n): ", hasAccount);
            break;
        case 1: {
            if (hasAccount) {
                vector<Member> members = loadMembers();
                string accountName;
                res = stepString("Enter the customer's account name: ", accountName);
                if (res == InputResult::BACK) break;

                Member* found = searchMemberByName(members, accountName);
                if (found == nullptr) {
                    cout << "No account found with that name. Proceeding as guest (no loyalty points).\n";
                    linkedMemberId = 0;
                    string guestName;
                    InputResult r2 = stepString("Enter renter's name: ", guestName);
                    if (r2 == InputResult::BACK) {
                        res = InputResult::BACK;
                        break;
                    }
                    renterName = guestName;
                }
                else {
                    linkedMemberId = found->id;
                    linkedMemberPoints = found->points;
                    renterName = found->name;
                    cout << "Account linked: " << renterName << " (Points balance: " << found->points << ")\n";
                }
            }
            else {
                res = stepString("Enter renter's name: ", renterName);
            }
            break;
        }
        }

        if (res == InputResult::BACK) {
            nameStep--;
            if (nameStep < 0) {
                cout << "Walk-in rental cancelled.\n";
                pauseScreen();
                return;
            }
        }
        else {
            nameStep++;
        }
    }

    vector<BikeType> bikeTypes = loadBikeTypes();
    printLine();
    cout << left << setw(4) << "ID" << setw(16) << "Type" << setw(10) << "Price/Day"
        << setw(10) << "Deposit" << setw(10) << "WalkinQty" << "\n";
    for (const auto& b : bikeTypes) {
        cout << left << setw(4) << b.id << setw(16) << b.type << setw(10)
            << fixed << setprecision(2) << b.pricePerDay << setw(10) << b.deposit
            << setw(10) << b.walkinQuantity << "\n";
    }
    printLine();

    int typeId = 0, quantity = 0;
    int discountPercent = 0, pointsUsed = 0;
    BikeType* bikeType = nullptr;

    int step = 0;
    int totalSteps = (linkedMemberId != 0) ? 3 : 2;
    while (step < totalSteps) {
        InputResult res = InputResult::OK;
        switch (step) {
        case 0:
            while (true) {
                res = stepInt("Enter Bike Type ID: ", 1, 999999, typeId);
                if (res == InputResult::BACK) break;
                bikeType = searchBikeTypeById(bikeTypes, typeId);
                if (bikeType == nullptr) {
                    cout << "Bike type not found. Please try again (or 'q' to go back).\n";
                    continue;
                }
                if (bikeType->walkinQuantity <= 0) {
                    cout << "Sorry, no units of this bike type are available for walk-in. Choose another (or 'q' to go back).\n";
                    continue;
                }
                break;
            }
            break;
        case 1:
            res = stepInt("Enter quantity to rent (1-" + to_string(bikeType->walkinQuantity) + "): ",
                1, bikeType->walkinQuantity, quantity);
            break;
        case 2:
            discountPercent = applyLoyaltyDiscount(linkedMemberPoints, pointsUsed);
            res = InputResult::OK;
            break;
        }

        if (res == InputResult::BACK) {
            step--;
            if (step < 0) {
                cout << "Walk-in rental cancelled.\n";
                pauseScreen();
                return;
            }
        }
        else {
            step++;
        }
    }

    string rentDate = getCurrentDateString();
    double subtotalPrice = quantity * bikeType->pricePerDay;
    double depositTotal = quantity * bikeType->deposit;
    double discountAmount = subtotalPrice * discountPercent / 100.0;
    double total = (subtotalPrice - discountAmount) + depositTotal;

    printInvoicePreview("", bikeType->type, quantity, bikeType->pricePerDay,
        rentDate, subtotalPrice, depositTotal, discountPercent,
        discountAmount, total, false, bikeType->tyreCost,
        bikeType->brakeCost, bikeType->chainCost);

    string code = generateReservationCode();
    string paymentMethod;
    if (!collectPaymentAndReceipt(renterName, bikeType->type, quantity, rentDate, total, code, true, paymentMethod)) {
        cout << "Payment cancelled - walk-in rental was not created.\n";
        pauseScreen();
        return;
    }

    if (pointsUsed > 0) {
        deductPoints(linkedMemberId, pointsUsed);
    }

    bikeType->walkinQuantity -= quantity;
    bikeType->timesRented += quantity;
    saveBikeTypes(bikeTypes);

    vector<Rental> rentals = loadRentals();
    Rental newRental;
    newRental.rentalId = getNextRentalId(rentals);
    newRental.channel = "WalkIn";
    newRental.memberId = linkedMemberId;
    newRental.renterName = renterName;
    newRental.bikeTypeId = typeId;
    newRental.quantity = quantity;
    newRental.rentDate = rentDate;
    newRental.code = code;
    newRental.pricePerDay = bikeType->pricePerDay;
    newRental.depositPerUnit = bikeType->deposit;
    newRental.discountApplied = discountPercent;
    newRental.discountPointsUsed = pointsUsed;
    newRental.totalPaid = total;
    newRental.paymentMethod = paymentMethod;
    newRental.status = "Active";

    for (int i = 1; i <= quantity; i++) {
        RentalUnit unit;
        unit.unitIndex = i;
        newRental.units.push_back(unit);
    }

    rentals.push_back(newRental);
    saveRentals(rentals);

    cout << "\nWalk-in rental recorded! Reference Code: " << code << "\n";
    cout << "Bike(s) must be returned before " << SHOP_CLOSE_HOUR << ":00 today.\n";
    pauseScreen();
}

void memberB_menu() {
    int choice;
    do {
        printLine();
        cout << "BICYCLE INVENTORY MANAGEMENT (ADMIN)\n";
        printLine();
        cout << "1. Add New Bike Type\n";
        cout << "2. Edit Bike Type\n";
        cout << "3. Delete Bike Type\n";
        cout << "4. View All Bike Types\n";
        cout << "0. Back to Main Menu\n";
        choice = getValidatedInt("Enter choice: ", 0, 4);

        switch (choice) {
        case 1: addBikeType(); break;
        case 2: editBikeType(); break;
        case 3: deleteBikeType(); break;
        case 4: viewBikeTypes(); break;
        case 0: cout << "Returning to main menu...\n"; break;
        }
    } while (choice != 0);
}