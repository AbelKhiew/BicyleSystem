/* ============================================================
   memberB.cpp
   MEMBER B - Bicycle Inventory & Rental Booking Module

   Responsibilities:
   - Bike inventory by TYPE (quantity pools, price/day, damage-part
     costs, auto-calculated deposit) - admin add/edit/view
   - Renter self-service booking: choose quantity, choose date
     ('t' for today or a validated future date), invoice shown,
     payment via QR/Bank Transfer, reservation code generated.
     Today's bookings start "Active"; future bookings start
     "Pending" until admin activates them with the code.
   - Admin walk-in rental: separate quantity pool, optional link to
     an existing account (for points), payment via Cash/Card/QR
     Code - Cash collects the amount given and calculates change.
   ============================================================ */
#include "memberB.h"
#include "memberA.h"     // searchMemberByName() - link walk-in to an existing account
#include <iostream>
#include <iomanip>
using namespace std;

BikeType* searchBikeTypeById(vector<BikeType>& bikeTypes, int id) {
    for (auto& b : bikeTypes) if (b.id == id) return &b;
    return nullptr;
}

/* Deposit = tyre repair cost + brake repair cost + chain repair cost + RM20 buffer */
double calcDeposit(double tyreCost, double brakeCost, double chainCost) {
    return tyreCost + brakeCost + chainCost + 20.0;
}

/* ---------------- Admin: inventory management ---------------- */
void addBikeType() {
    printLine();
    cout << "ADD NEW BIKE TYPE\n";
    printLine();

    vector<BikeType> bikeTypes = loadBikeTypes();

    BikeType newType;
    newType.id = getNextBikeTypeId(bikeTypes);
    newType.type = getValidatedString("Enter bike type name (e.g. Mountain Bike): ");
    newType.pricePerDay = getValidatedDouble("Enter price per day (RM): ", 0.5, 1000.0);
    newType.tyreCost = getValidatedDouble("Enter tyre repair cost (RM): ", 0.0, 500.0);
    newType.brakeCost = getValidatedDouble("Enter brake repair cost (RM): ", 0.0, 500.0);
    newType.chainCost = getValidatedDouble("Enter chain repair cost (RM): ", 0.0, 500.0);
    newType.deposit = calcDeposit(newType.tyreCost, newType.brakeCost, newType.chainCost);
    newType.bookingQuantity = getValidatedInt("Enter quantity for online booking: ", 0, 1000);
    newType.walkinQuantity = getValidatedInt("Enter quantity for walk-in rental: ", 0, 1000);
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

    int id = getValidatedInt("Enter Bike Type ID to edit: ", 1, 999999);
    BikeType* b = searchBikeTypeById(bikeTypes, id);
    if (b == nullptr) {
        cout << "Bike type not found.\n";
        pauseScreen();
        return;
    }

    cout << "Re-enter each value (current shown in brackets):\n";
    b->type = getValidatedString("Type name [" + b->type + "]: ");
    b->pricePerDay = getValidatedDouble("Price per day (RM): ", 0.5, 1000.0);
    b->tyreCost = getValidatedDouble("Tyre repair cost (RM): ", 0.0, 500.0);
    b->brakeCost = getValidatedDouble("Brake repair cost (RM): ", 0.0, 500.0);
    b->chainCost = getValidatedDouble("Chain repair cost (RM): ", 0.0, 500.0);
    b->deposit = calcDeposit(b->tyreCost, b->brakeCost, b->chainCost);
    b->bookingQuantity = getValidatedInt("Quantity for online booking: ", 0, 1000);
    b->walkinQuantity = getValidatedInt("Quantity for walk-in rental: ", 0, 1000);

    saveBikeTypes(bikeTypes);
    cout << "\nUpdated! New deposit: RM" << fixed << setprecision(2) << b->deposit << "\n";
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

/* Renter-facing simplified availability view. */
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
    pauseScreen();
}

/* Shared payment collection + receipt printer.
   isWalkIn = true  -> Cash / Card / QR Code  (Cash asks amount given + shows change)
   isWalkIn = false -> QR Payment / Bank Transfer (renter self-service)          */
void collectPaymentAndReceipt(const string& renterName, const string& bikeTypeName,
    int quantity, const string& rentDate, double totalDue,
    const string& code, bool isWalkIn, string& paymentMethodOut) {
    string paymentMethod;
    double cashGiven = 0.0, change = 0.0;

    if (isWalkIn) {
        cout << "\nSelect Payment Method:\n1. Cash\n2. Card\n3. QR Code\n";
        int choice = getValidatedInt("Enter choice: ", 1, 3);

        if (choice == 1) {
            paymentMethod = "Cash";
            while (true) {
                cashGiven = getValidatedDouble("Enter cash received from renter (RM): ", 0.0, 1000000.0);
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
        cout << "\nSelect Payment Method:\n1. QR Payment\n2. Bank Transfer\n";
        int choice = getValidatedInt("Enter choice: ", 1, 2);
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
}

/* ---------------- Renter self-service booking (single flow) ---------------- */
void bookBike(int memberId, const string& renterName) {
    printLine();
    cout << "BOOK A BICYCLE\n";
    printLine();

    vector<BikeType> bikeTypes = loadBikeTypes();
    viewAvailability();

    int typeId = getValidatedInt("Enter Bike Type ID to book: ", 1, 999999);
    BikeType* bikeType = searchBikeTypeById(bikeTypes, typeId);

    if (bikeType == nullptr) {
        cout << "Bike type not found.\n";
        pauseScreen();
        return;
    }
    if (bikeType->bookingQuantity <= 0) {
        cout << "Sorry, no units of this bike type are currently available for booking.\n";
        pauseScreen();
        return;
    }

    int quantity = getValidatedInt(
        "Enter quantity to rent (1-" + to_string(bikeType->bookingQuantity) + "): ",
        1, bikeType->bookingQuantity);

    string rentDate = getValidatedDate("Enter rental date (YYYY-MM-DD, or 't' for today): ");

    double total = quantity * (bikeType->pricePerDay + bikeType->deposit);

    printLine();
    cout << "INVOICE PREVIEW\n";
    printLine();
    cout << fixed << setprecision(2);
    cout << left << setw(22) << "Bike Type:" << bikeType->type << "\n";
    cout << left << setw(22) << "Quantity:" << quantity << "\n";
    cout << left << setw(22) << "Rental Date:" << rentDate << "\n";
    cout << left << setw(22) << "Price/day x Qty:" << "RM" << (bikeType->pricePerDay * quantity) << "\n";
    cout << left << setw(22) << "Deposit x Qty:" << "RM" << (bikeType->deposit * quantity) << "\n";
    cout << left << setw(22) << "TOTAL TO PAY:" << "RM" << total << "\n";
    printLine();

    string confirm = getValidatedString("Confirm booking? (y/n): ");
    if (confirm != "y" && confirm != "Y") {
        cout << "Booking cancelled.\n";
        pauseScreen();
        return;
    }

    string code = generateReservationCode();
    string paymentMethod;
    collectPaymentAndReceipt(renterName, bikeType->type, quantity, rentDate, total, code, false, paymentMethod);

    /* Deduct from booking pool, increment stats */
    bikeType->bookingQuantity -= quantity;
    bikeType->timesRented += quantity;
    saveBikeTypes(bikeTypes);

    /* Today's bookings are usable immediately; future bookings stay
       Pending until the renter shows the code and admin activates it. */
    string status = (rentDate == getCurrentDateString()) ? "Active" : "Pending";

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
    newRental.totalPaid = total;
    newRental.paymentMethod = paymentMethod;
    newRental.status = status;

    for (int i = 1; i <= quantity; i++) {
        RentalUnit unit;
        unit.unitIndex = i;
        newRental.units.push_back(unit);
    }

    rentals.push_back(newRental);
    saveRentals(rentals);

    cout << "\nYour reservation code is: " << code << "\n";
    if (status == "Pending") {
        cout << "This booking is for a FUTURE date. Show this code to the admin\n";
        cout << "on arrival so it can be activated before you take the bike(s).\n";
    }
    cout << "Please return before " << SHOP_CLOSE_HOUR << ":00 on " << rentDate
        << " to receive your deposit refund.\n";
    pauseScreen();
}

/* ---------------- Admin walk-in rental (separate quantity pool) ---------------- */
void walkInRental() {
    printLine();
    cout << "WALK-IN RENTAL (ADMIN)\n";
    printLine();

    int linkedMemberId = 0;
    string renterName;

    string hasAccount = getValidatedString("Does the customer have an account? (y/n): ");
    if (hasAccount == "y" || hasAccount == "Y") {
        vector<Member> members = loadMembers();
        string accountName = getValidatedString("Enter the customer's account name: ");
        Member* found = searchMemberByName(members, accountName);

        if (found == nullptr) {
            cout << "No account found with that name. Proceeding as guest (no loyalty points).\n";
            renterName = getValidatedString("Enter renter's name: ");
        }
        else {
            linkedMemberId = found->id;
            renterName = found->name;
            cout << "Account linked: " << renterName << " (Points balance: " << found->points << ")\n";
        }
    }
    else {
        renterName = getValidatedString("Enter renter's name: ");
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

    int typeId = getValidatedInt("Enter Bike Type ID: ", 1, 999999);
    BikeType* bikeType = searchBikeTypeById(bikeTypes, typeId);

    if (bikeType == nullptr) {
        cout << "Bike type not found.\n";
        pauseScreen();
        return;
    }
    if (bikeType->walkinQuantity <= 0) {
        cout << "Sorry, no units of this bike type are currently available for walk-in rental.\n";
        pauseScreen();
        return;
    }

    int quantity = getValidatedInt(
        "Enter quantity to rent (1-" + to_string(bikeType->walkinQuantity) + "): ",
        1, bikeType->walkinQuantity);

    string rentDate = getCurrentDateString();   // walk-in is always for today
    double total = quantity * (bikeType->pricePerDay + bikeType->deposit);

    printLine();
    cout << "INVOICE PREVIEW\n";
    printLine();
    cout << fixed << setprecision(2);
    cout << left << setw(22) << "Renter Name:" << renterName << "\n";
    cout << left << setw(22) << "Bike Type:" << bikeType->type << "\n";
    cout << left << setw(22) << "Quantity:" << quantity << "\n";
    cout << left << setw(22) << "Rental Date:" << rentDate << " (today)\n";
    cout << left << setw(22) << "Price/day x Qty:" << "RM" << (bikeType->pricePerDay * quantity) << "\n";
    cout << left << setw(22) << "Deposit x Qty:" << "RM" << (bikeType->deposit * quantity) << "\n";
    cout << left << setw(22) << "TOTAL TO PAY:" << "RM" << total << "\n";
    printLine();

    string code = generateReservationCode();
    string paymentMethod;
    collectPaymentAndReceipt(renterName, bikeType->type, quantity, rentDate, total, code, true, paymentMethod);

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
    newRental.totalPaid = total;
    newRental.paymentMethod = paymentMethod;
    newRental.status = "Active";   // walk-in is picked up immediately, no activation needed

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

/* Admin menu: inventory management ONLY (booking is renter self-service,
   walk-in rental has its own top-level admin menu entry - see main.cpp). */
void memberB_menu() {
    int choice;
    do {
        printLine();
        cout << "BICYCLE INVENTORY MANAGEMENT (ADMIN)\n";
        printLine();
        cout << "1. Add New Bike Type\n";
        cout << "2. Edit Bike Type\n";
        cout << "3. View All Bike Types\n";
        cout << "0. Back to Main Menu\n";
        choice = getValidatedInt("Enter choice: ", 0, 3);

        switch (choice) {
        case 1: addBikeType(); break;
        case 2: editBikeType(); break;
        case 3: viewBikeTypes(); break;
        case 0: cout << "Returning to main menu...\n"; break;
        }
    } while (choice != 0);
}