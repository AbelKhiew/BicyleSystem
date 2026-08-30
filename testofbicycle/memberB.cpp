/*==================================================================
  memberB.cpp  -  Member B: Bike Inventory + Rental Booking

  what this file does:
  - admin: add / edit / delete / view bike types
  - renter: book a bike online (type, qty, start date, days,
    optional loyalty discount, pay -> get a reservation code).
    online bookings start "Pending"
  - admin: walk-in rental at the counter. starts "Active" and can
    take cash (works out the change)
  - shared bits: the invoice preview + the payment/receipt step
==================================================================*/
#include "memberB.h"
#include "memberA.h"
#include <iostream>
#include <iomanip>
#include <sstream>
using namespace std;

static void printBikeTypeTable(const vector<BikeType>& bikeTypes);   // defined lower down

/*==================================================================
  this method finds a bike type by id, or returns nullptr
==================================================================*/
BikeType* searchBikeTypeById(vector<BikeType>& bikeTypes, int id) {
    for (auto& b : bikeTypes) if (b.id == id) return &b;
    return nullptr;
}

/*==================================================================
  this method works out the deposit for a bike type:
  tyre + brake + chain repair costs + RM20 buffer
==================================================================*/
double calcDeposit(double tyreCost, double brakeCost, double chainCost) {
    return tyreCost + brakeCost + chainCost + 20.0;
}

/*==================================================================
  this method adds 2 starter bikes (City Cruiser, Mountain
  Explorer) the first time the program runs, so there is stock to
  demo with. it only runs when the inventory is empty
==================================================================*/
void seedDefaultBikeTypes() {
    vector<BikeType> bikeTypes = loadBikeTypes();
    if (!bikeTypes.empty()) return;

    BikeType cityBike;
    cityBike.id = 1;
    cityBike.type = "City Cruiser";
    cityBike.pricePerDay = 8.00;
    cityBike.tyreCost = 15.00;
    cityBike.brakeCost = 12.00;
    cityBike.chainCost = 10.00;
    cityBike.deposit = calcDeposit(cityBike.tyreCost, cityBike.brakeCost, cityBike.chainCost);
    cityBike.bookingQuantity = 15;
    cityBike.walkinQuantity = 8;
    cityBike.timesRented = 0;

    BikeType mtb;
    mtb.id = 2;
    mtb.type = "Mountain Explorer";
    mtb.pricePerDay = 15.00;
    mtb.tyreCost = 25.00;
    mtb.brakeCost = 20.00;
    mtb.chainCost = 18.00;
    mtb.deposit = calcDeposit(mtb.tyreCost, mtb.brakeCost, mtb.chainCost);
    mtb.bookingQuantity = 10;
    mtb.walkinQuantity = 6;
    mtb.timesRented = 0;

    bikeTypes.push_back(cityBike);
    bikeTypes.push_back(mtb);
    saveBikeTypes(bikeTypes);
}

/*==================================================================
  this method (admin) adds a new bike type. it asks for every
  field one at a time ('q' steps back), then works out the
  deposit and saves
==================================================================*/
void addBikeType() {
    uiHeader("ADD NEW BIKE TYPE");
    cout << "(type 'q' at any prompt to go back)\n\n";

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
                cout << "\nAdd bike type cancelled. No changes were saved.\n";
                pauseScreen();
                return;
            }
        }
        else {
            step++;
        }
    }

    // build + save the new bike type
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

    cout << "\nNew bike type added successfully.\n";
    cout << "  Assigned ID          : " << newType.id << "\n";
    cout << "  Calculated deposit   : RM" << fixed << setprecision(2) << newType.deposit << " per unit\n";
    pauseScreen();
}

/*==================================================================
  this method (admin) edits a bike type. shows the list first,
  then re-asks every field (current value shown in [brackets]).
  the deposit is re-worked-out from the new repair costs
==================================================================*/
void editBikeType() {
    vector<BikeType> bikeTypes = loadBikeTypes();
    uiHeader("EDIT BIKE TYPE");
    printBikeTypeTable(bikeTypes);
    cout << "\n";

    int id;
    if (stepInt("Enter Bike Type ID to edit (or 'q' to cancel): ", 1, 999999, id) == InputResult::BACK) {
        cout << "\nOperation cancelled.\n";
        pauseScreen();
        return;
    }
    BikeType* b = searchBikeTypeById(bikeTypes, id);
    if (b == nullptr) {
        cout << "\nNo bike type matches that ID.\n";
        pauseScreen();
        return;
    }

    cout << "Re-enter each value (current shown in brackets). Type 'q' at any prompt to go back.\n";

    // start from the current values, then overwrite as the admin types
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
                cout << "\nEdit cancelled. No changes were saved.\n";
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
    cout << "\nBike type updated successfully. New deposit: RM"
        << fixed << setprecision(2) << b->deposit << " per unit\n";
    pauseScreen();
}

/*==================================================================
  this method (admin) deletes a bike type. it is blocked if any
  rental that is not "Return" still uses this bike type - those
  must be returned first
==================================================================*/
void deleteBikeType() {
    vector<BikeType> bikeTypes = loadBikeTypes();
    uiHeader("DELETE BIKE TYPE");
    printBikeTypeTable(bikeTypes);
    cout << "\n";

    int id;
    if (stepInt("Enter Bike Type ID to delete (or 'q' to cancel): ", 1, 999999, id) == InputResult::BACK) {
        cout << "\nOperation cancelled.\n";
        pauseScreen();
        return;
    }

    BikeType* target = searchBikeTypeById(bikeTypes, id);
    if (target == nullptr) {
        cout << "\nNo bike type matches that ID.\n";
        pauseScreen();
        return;
    }

    // stop the delete if a live rental still points at this bike type
    vector<Rental> rentals = loadRentals();
    for (const auto& r : rentals) {
        if (r.bikeTypeId == id && r.status != "Return") {
            cout << "\nThis bike type cannot be deleted. Rental ID " << r.rentalId << " ("
                << r.status << ") is still linked to it and must be returned first.\n";
            pauseScreen();
            return;
        }
    }

    bool confirm = false;
    InputResult res = stepConfirm("Delete '" + target->type + "'? This cannot be undone. (y/n): ", confirm);
    if (res == InputResult::BACK || !confirm) {
        cout << "\nDeletion cancelled. The bike type was not removed.\n";
        pauseScreen();
        return;
    }

    // rebuild the list without this id
    vector<BikeType> updated;
    for (const auto& b : bikeTypes) {
        if (b.id != id) updated.push_back(b);
    }
    saveBikeTypes(updated);
    cout << "\nBike type deleted successfully.\n";
    pauseScreen();
}

/*==================================================================
  this method prints just the bike-type table (no clear, no
  pause) so add/edit/delete and the menu option can all reuse it
==================================================================*/
static void printBikeTypeTable(const vector<BikeType>& bikeTypes) {
    if (bikeTypes.empty()) {
        cout << "\nThere are no bike types in the system yet.\n";
        return;
    }

    cout << left << setw(4) << "ID" << setw(26) << "Type" << setw(10) << "Price/Day"
        << setw(10) << "Deposit" << setw(9) << "BookQty" << setw(9) << "WalkQty"
        << setw(7) << "Rented" << "\n";
    printLine();
    cout << fixed << setprecision(2);
    for (const auto& b : bikeTypes) {
        cout << left << setw(4) << b.id << setw(26) << fitStr(b.type, 26) << setw(10)
            << b.pricePerDay << setw(10) << b.deposit
            << setw(9) << b.bookingQuantity << setw(9) << b.walkinQuantity
            << setw(7) << b.timesRented << "\n";
    }
    printLine();
}

/*==================================================================
  this method is the admin menu option "View All Bike Types"
==================================================================*/
void viewBikeTypes() {
    uiHeader("ALL BIKE TYPES");
    printBikeTypeTable(loadBikeTypes());
    pauseScreen();
}

/*==================================================================
  this method shows the renter which bikes can be booked and how
  many are free. no screen clear because it is printed inside the
  booking flow
==================================================================*/
void viewAvailability() {
    vector<BikeType> bikeTypes = loadBikeTypes();
    printLine();
    printCentered("BIKE AVAILABILITY");
    printLine();

    if (bikeTypes.empty()) {
        cout << "No bicycles are currently available for booking.\n";
        printLine();
        return;
    }

    cout << left << setw(4) << "ID" << setw(28) << "Type" << setw(12) << "Price/Day"
        << setw(12) << "Deposit" << setw(10) << "Available" << "\n";
    cout << fixed << setprecision(2);
    for (const auto& b : bikeTypes) {
        cout << left << setw(4) << b.id << setw(28) << fitStr(b.type, 28) << setw(12)
            << b.pricePerDay << setw(12) << b.deposit
            << setw(10) << b.bookingQuantity << "\n";
    }
    printLine();
}

/*==================================================================
  this method prints one right-aligned "RM ....." money line -
  keeps the invoice / receipt columns lined up
==================================================================*/
static void invoiceMoneyRow(const string& label, double amount, const string& note = "") {
    cout << "  " << left << setw(22) << label << "RM " << right << setw(10)
        << fixed << setprecision(2) << amount;
    if (!note.empty()) cout << "   " << note;
    cout << "\n" << left;
}

/*==================================================================
  this method prints the invoice the renter/admin sees BEFORE
  paying: bike, dates, the price maths, discount, deposit, total
==================================================================*/
void printInvoicePreview(const string& renterName, const string& bikeTypeName, int quantity,
    int days, double pricePerDay, const string& rentDate, const string& dueDate,
    double subtotalPrice, double depositPerUnit, double depositTotal,
    int discountPercent, double discountAmount, double total, bool isWalkIn, double tyreCost, double brakeCost, double chainCost) {

    // 1. HEADER
    cout << "\n\n";
    printLine();
    printCentered("RENTAL INVOICE");
    printLine();

    if (isWalkIn) {
        cout << "  " << left << setw(22) << "Renter Name:" << renterName << "\n";
    }
    cout << "  " << left << setw(22) << "Bike Type:" << bikeTypeName << "\n";
    cout << "  " << left << setw(22) << "Quantity:" << quantity << " unit(s)\n";

    // walk-in rentals always start today, so mark the date "(today)"
    cout << "  " << left << setw(22) << "Rental Start Date:" << rentDate;
    if (isWalkIn) cout << " (today)";
    cout << "\n";

    cout << "  " << left << setw(22) << "Rental Duration:" << days << " day(s)\n";
    cout << "  " << left << setw(22) << "Return By (Due Date):" << dueDate << "\n";
    printLine();

    // little "(RM8 x 2 unit x 3 day)" note next to the subtotal
    ostringstream rateNote;
    rateNote << fixed << setprecision(2) << "(RM" << pricePerDay << " x " << quantity
        << " unit x " << days << " day)";

    invoiceMoneyRow("Rate per day:", pricePerDay);
    invoiceMoneyRow("Rental Subtotal:", subtotalPrice, rateNote.str());

    invoiceMoneyRow("\nDeposit per unit:", depositPerUnit, "(refundable if returned on time & undamaged)");
    cout << "  " << left << setw(23) << "- Tyre Coverage:" << "RM " << tyreCost << "\n";
    cout << "  " << left << setw(23) << "- Brake Coverage:" << "RM " << brakeCost << "\n";
    cout << "  " << left << setw(23) << "- Chain Coverage:" << "RM " << chainCost << "\n";
    cout << "  " << left << setw(23) << "- Base Fee:" << "RM 20.00\n";
    cout << "\n";

    if (discountPercent > 0) {
        ostringstream discNote;
        discNote << "(" << discountPercent << "% loyalty discount)";
        invoiceMoneyRow("Discount:", -discountAmount, discNote.str());
    }
    invoiceMoneyRow("Deposit Subtotal:", depositTotal);
    printLine();
    invoiceMoneyRow("TOTAL TO PAY:", total);
    printLine();
}

/*==================================================================
  this method takes the payment and prints the receipt.
  - walk-in: Cash / Card / QR Pay (Cash asks for the amount given
    and works out the change; keeps asking if it is too little)
  - online: QR Pay / Bank Transfer
  returns false if the user typed 'q' to back out
==================================================================*/
bool collectPaymentAndReceipt(const string& renterName, const string& bikeTypeName,
    int quantity, const string& rentDate, double totalDue,
    const string& code, bool isWalkIn, string& paymentMethodOut) {
    string paymentMethod;
    double cashGiven = 0.0, change = 0.0;

    if (isWalkIn) {
        int choice;
        cout << "\nSelect Payment Method:\n  1. Cash\n  2. Card\n  3. QR Pay\n";
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
            paymentMethod = "QR Pay";
        }
    }
    else {
        int choice;
        cout << "\nSelect Payment Method:\n  1. QR Pay\n  2. Bank Transfer\n";
        if (stepInt("Enter choice (or 'q' to cancel): ", 1, 2, choice) == InputResult::BACK) return false;
        if (choice == 1) paymentMethod = "QR Pay";
        else             paymentMethod = "Bank Transfer";
    }

    paymentMethodOut = paymentMethod;

    // print the receipt
    printLine();
    printCentered("PAYMENT RECEIPT");
    printLine();
    cout << fixed << setprecision(2);
    cout << "  " << left << setw(22) << "Renter Name:" << renterName << "\n";
    cout << "  " << left << setw(22) << "Bike Type:" << bikeTypeName << "\n";
    cout << "  " << left << setw(22) << "Quantity:" << quantity << " unit(s)\n";
    cout << "  " << left << setw(22) << "Rental Start Date:" << rentDate << "\n";
    invoiceMoneyRow("Total Paid:", totalDue);
    cout << "  " << left << setw(22) << "Payment Method:" << paymentMethod << "\n";
    if (paymentMethod == "Cash") {
        invoiceMoneyRow("Cash Given:", cashGiven);
        invoiceMoneyRow("Change Returned:", change);
    }
    cout << "  " << left << setw(22) << "Reference Code:" << code << "\n";
    printLine();
    return true;
}

/*==================================================================
  this method is the renter's whole booking flow. steps (0-5):
  0 bike id -> 1 quantity -> 2 start date -> 3 number of days ->
  4 loyalty discount -> 5 show invoice + confirm. 'q' steps back.
  after confirm: pay, cut stock, save a new "Pending" rental with
  one unit row per bike
==================================================================*/
void bookBike(int memberId, const string& renterName) {
    uiHeader("BOOK A BICYCLE");
    cout << "(type 'q' at any prompt to go back)\n";

    vector<BikeType> bikeTypes = loadBikeTypes();
    viewAvailability();

    // how many points this renter has (for the discount step)
    vector<Member> members = loadMembers();
    Member* member = searchMemberById(members, memberId);
    int memberPoints = 0;
    if (member != nullptr) memberPoints = member->points;

    int typeId = 0, quantity = 0, days = 1;
    string rentDate, dueDate;
    bool confirm = false;
    BikeType* bikeType = nullptr;
    int discountPercent = 0, pointsUsed = 0;
    double subtotalPrice = 0.0, depositTotal = 0.0, discountAmount = 0.0, total = 0.0;

    int step = 0;
    while (step < 6) {
        InputResult res = InputResult::OK;
        switch (step) {
        case 0:
            // keep asking for a bike id until it exists AND has stock
            while (true) {
                res = stepInt("Enter Bike Type ID to book: ", 1, 999999, typeId);
                if (res == InputResult::BACK) break;
                bikeType = searchBikeTypeById(bikeTypes, typeId);
                if (bikeType == nullptr) {
                    cout << "No bike type matches that ID. Please try again (or 'q' to go back).\n";
                    continue;
                }
                if (bikeType->bookingQuantity <= 0) {
                    cout << "This bike type is fully booked. Please choose another (or 'q' to go back).\n";
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
            res = stepDate("Enter rental start date (YYYY-MM-DD, or 't' for today): ", rentDate);
            break;
        case 3:
            res = stepInt("Enter number of rental days (" + to_string(MIN_RENTAL_DAYS) + "-"
                + to_string(MAX_RENTAL_DAYS) + "): ", MIN_RENTAL_DAYS, MAX_RENTAL_DAYS, days);
            break;
        case 4:
            discountPercent = applyLoyaltyDiscount(memberPoints, pointsUsed);
            res = InputResult::OK;
            break;
        case 5: {
            // work out the money and show the invoice
            dueDate = addDaysToDate(rentDate, days - 1);
            subtotalPrice = quantity * bikeType->pricePerDay * days;
            depositTotal = quantity * bikeType->deposit;
            discountAmount = subtotalPrice * discountPercent / 100.0;
            total = (subtotalPrice - discountAmount) + depositTotal;

            printInvoicePreview("", bikeType->type, quantity, days, bikeType->pricePerDay,
                rentDate, dueDate, subtotalPrice, bikeType->deposit, depositTotal,
                discountPercent, discountAmount, total, false, bikeType->tyreCost, bikeType->brakeCost, bikeType->chainCost);

            res = stepConfirm("Confirm booking? (y/n): ", confirm);
            break;
        }
        }

        if (res == InputResult::BACK) {
            step--;
            if (step < 0) {
                cout << "\nBooking cancelled. No reservation was created.\n";
                pauseScreen();
                return;
            }
        }
        else {
            step++;
        }
    }

    if (!confirm) {
        cout << "\nBooking cancelled. No reservation was created.\n";
        pauseScreen();
        return;
    }

    // make a unique code, then take payment
    vector<Rental> rentals = loadRentals();
    string code = generateReservationCode(rentals);
    string paymentMethod;
    if (!collectPaymentAndReceipt(renterName, bikeType->type, quantity, rentDate, total, code, false, paymentMethod)) {
        cout << "\nPayment cancelled. No reservation was created.\n";
        pauseScreen();
        return;
    }

    // spend the loyalty points (if any were used)
    if (pointsUsed > 0) {
        deductPoints(memberId, pointsUsed);
    }

    // take the bikes out of the booking stock
    bikeType->bookingQuantity -= quantity;
    bikeType->timesRented += quantity;
    saveBikeTypes(bikeTypes);

    // build + save the new rental (one unit row per bike). starts "Pending"
    Rental newRental;
    newRental.rentalId = getNextRentalId(rentals);
    newRental.channel = "Booking";
    newRental.memberId = memberId;
    newRental.renterName = renterName;
    newRental.bikeTypeId = typeId;
    newRental.quantity = quantity;
    newRental.rentalDays = days;
    newRental.rentDate = rentDate;
    newRental.dueDate = dueDate;
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

    cout << "\nBooking confirmed.\n";
    cout << "  Reservation code     : " << code << "\n";
    cout << "  Please present this code to the counter staff to activate the\n";
    cout << "  reservation before collecting the bicycle(s).\n";
    cout << "  Return the bicycle(s) by " << SHOP_CLOSE_HOUR << ":00 on " << dueDate
        << " to be eligible for a deposit refund.\n";
    pauseScreen();
}

/*==================================================================
  this method is the admin's walk-in rental. first it asks if the
  customer has an account (to link points), then bike id ->
  quantity -> days -> (discount, only if linked). it shows the
  invoice, takes payment, cuts the walk-in stock, and saves a
  new rental that starts "Active" straight away
==================================================================*/
void walkInRental() {
    uiHeader("WALK-IN RENTAL  (ADMIN)");
    cout << "(type 'q' at any prompt to go back)\n\n";

    int linkedMemberId = 0;
    int linkedMemberPoints = 0;
    string renterName;
    bool hasAccount = false;

    // --- work out who the renter is (linked account or a guest name) ---
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
                    // no such account -> carry on as a guest
                    cout << "No account matches that name. Proceeding as a guest (loyalty points will not apply).\n";
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
                cout << "\nWalk-in rental cancelled. No record was created.\n";
                pauseScreen();
                return;
            }
        }
        else {
            nameStep++;
        }
    }

    // show the walk-in stock
    vector<BikeType> bikeTypes = loadBikeTypes();
    printLine();
    cout << left << setw(4) << "ID" << setw(28) << "Type" << setw(12) << "Price/Day"
        << setw(12) << "Deposit" << setw(10) << "WalkinQty" << "\n";
    cout << fixed << setprecision(2);
    for (const auto& b : bikeTypes) {
        cout << left << setw(4) << b.id << setw(28) << fitStr(b.type, 28) << setw(12)
            << b.pricePerDay << setw(12) << b.deposit
            << setw(10) << b.walkinQuantity << "\n";
    }
    printLine();

    int typeId = 0, quantity = 0, days = 1;
    int discountPercent = 0, pointsUsed = 0;
    BikeType* bikeType = nullptr;

    // the discount step (3) only exists when an account is linked
    int step = 0;
    int totalSteps = 3;
    if (linkedMemberId != 0) totalSteps = 4;
    while (step < totalSteps) {
        InputResult res = InputResult::OK;
        switch (step) {
        case 0:
            while (true) {
                res = stepInt("Enter Bike Type ID: ", 1, 999999, typeId);
                if (res == InputResult::BACK) break;
                bikeType = searchBikeTypeById(bikeTypes, typeId);
                if (bikeType == nullptr) {
                    cout << "No bike type matches that ID. Please try again (or 'q' to go back).\n";
                    continue;
                }
                if (bikeType->walkinQuantity <= 0) {
                    cout << "No walk-in units of this bike type are available. Please choose another (or 'q' to go back).\n";
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
            res = stepInt("Enter number of rental days (" + to_string(MIN_RENTAL_DAYS) + "-"
                + to_string(MAX_RENTAL_DAYS) + "): ", MIN_RENTAL_DAYS, MAX_RENTAL_DAYS, days);
            break;
        case 3:
            discountPercent = applyLoyaltyDiscount(linkedMemberPoints, pointsUsed);
            res = InputResult::OK;
            break;
        }

        if (res == InputResult::BACK) {
            step--;
            if (step < 0) {
                cout << "\nWalk-in rental cancelled. No record was created.\n";
                pauseScreen();
                return;
            }
        }
        else {
            step++;
        }
    }

    // walk-in always starts today
    string rentDate = getCurrentDateString();
    string dueDate = addDaysToDate(rentDate, days - 1);
    double subtotalPrice = quantity * bikeType->pricePerDay * days;
    double depositTotal = quantity * bikeType->deposit;
    double discountAmount = subtotalPrice * discountPercent / 100.0;
    double total = (subtotalPrice - discountAmount) + depositTotal;

    printInvoicePreview("", bikeType->type, quantity, days, bikeType->pricePerDay,
        rentDate, dueDate, subtotalPrice, bikeType->deposit, depositTotal,
        discountPercent, discountAmount, total, false, bikeType->tyreCost, bikeType->brakeCost, bikeType->chainCost);

    vector<Rental> rentals = loadRentals();
    string code = generateReservationCode(rentals);
    string paymentMethod;
    if (!collectPaymentAndReceipt(renterName, bikeType->type, quantity, rentDate, total, code, true, paymentMethod)) {
        cout << "\nPayment cancelled. The walk-in rental was not created.\n";
        pauseScreen();
        return;
    }

    if (pointsUsed > 0) {
        deductPoints(linkedMemberId, pointsUsed);
    }

    bikeType->walkinQuantity -= quantity;
    bikeType->timesRented += quantity;
    saveBikeTypes(bikeTypes);

    // build + save the rental. walk-in starts "Active" (bike handed over now)
    Rental newRental;
    newRental.rentalId = getNextRentalId(rentals);
    newRental.channel = "WalkIn";
    newRental.memberId = linkedMemberId;
    newRental.renterName = renterName;
    newRental.bikeTypeId = typeId;
    newRental.quantity = quantity;
    newRental.rentalDays = days;
    newRental.rentDate = rentDate;
    newRental.dueDate = dueDate;
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

    cout << "\nWalk-in rental completed successfully.\n";
    cout << "  Reference code       : " << code << "\n";
    cout << "  The bicycle(s) must be returned by " << SHOP_CLOSE_HOUR << ":00 on " << dueDate << ".\n";
    pauseScreen();
}

/*==================================================================
  this method is the admin "inventory" menu. loops until 0
==================================================================*/
void memberB_menu() {
    int choice;
    do {
        uiHeader("BICYCLE INVENTORY MANAGEMENT  (ADMIN)");
        printCentered("1. Add New Bike Type");
        printCentered("2. Edit Bike Type");
        printCentered("3. Delete Bike Type");
        printCentered("4. View All Bike Types");
        printCentered("0. Back to Main Menu");
        printLine();
        choice = getValidatedInt("Enter choice: ", 0, 4);

        switch (choice) {
        case 1: addBikeType(); break;
        case 2: editBikeType(); break;
        case 3: deleteBikeType(); break;
        case 4: viewBikeTypes(); break;
        case 0: cout << "\nReturning to the main menu...\n"; break;
        }
    } while (choice != 0);
}
