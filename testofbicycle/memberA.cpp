/*==================================================================
  memberA.cpp  -  Member A: Customer / Member Management

  what this file does:
  - lets a new renter sign up (name must be unique, phone digits only)
  - checks a renter's login
  - admin tools: view / search / update / delete members
  - shows a renter's rental history
  - loyalty points: add points, spend points, and the tier picker
==================================================================*/
#include "memberA.h"
#include <iostream>
#include <iomanip>
#include <sstream>
using namespace std;

/*==================================================================
  this method adds one demo renter (u1 / u1) the first time the
  program runs, so there is an account to log in with straight away.
  it only runs when there are no members yet
==================================================================*/
void seedDefaultMembers() {
    vector<Member> members = loadMembers();
    if (!members.empty()) return;

    Member demo;
    demo.id = 1;
    demo.name = "u1";
    demo.password = "u1";
    demo.phone = "0123456789";
    demo.totalRentals = 0;
    demo.points = 0;

    members.push_back(demo);
    saveMembers(members);
}

/*==================================================================
  this method checks if a username is already taken
==================================================================*/
bool isNameTaken(const vector<Member>& members, const string& name) {
    for (const auto& m : members) {
        if (m.name == name) return true;
    }
    return false;
}

/*==================================================================
  this method finds a member by id, or returns nullptr
==================================================================*/
Member* searchMemberById(vector<Member>& members, int id) {
    for (auto& m : members) if (m.id == id) return &m;
    return nullptr;
}

/*==================================================================
  this method finds a member by name, or returns nullptr
==================================================================*/
Member* searchMemberByName(vector<Member>& members, const string& name) {
    for (auto& m : members) if (m.name == name) return &m;
    return nullptr;
}

/*==================================================================
  this method signs up a new renter. it asks for username /
  password / phone one at a time; 'q' steps back a question. the
  username is re-checked so two members can never share a name
==================================================================*/
void registerMember() {
    uiHeader("REGISTER NEW MEMBER");
    cout << "(type 'q' at any prompt to go back)\n\n";

    vector<Member> members = loadMembers();

    string name, password, phone;
    int step = 0;
    while (step < 3) {
        InputResult res = InputResult::OK;
        switch (step) {
        case 0:
            // keep asking for a username until it is free (or 'q')
            while (true) {
                res = stepString("Enter username: ", name);
                if (res == InputResult::BACK) break;
                if (isNameTaken(members, name)) {
                    cout << "This username is already registered. Please choose a different one (or 'q' to go back).\n";
                    continue;
                }
                break;
            }
            break;
        case 1:
            res = stepString("Enter password: ", password);
            break;
        case 2:
            res = stepPhone("Enter phone number (digits only, 7-15): ", phone);
            break;
        }

        // 'q' -> go back a step; if we back off step 0 the sign-up is cancelled
        if (res == InputResult::BACK) {
            step--;
            if (step < 0) {
                cout << "\nRegistration cancelled. No account was created.\n";
                pauseScreen();
                return;
            }
        }
        else {
            step++;
        }
    }

    // build the new member and save it
    Member newMember;
    newMember.id = getNextMemberId(members);
    newMember.name = name;
    newMember.password = password;
    newMember.phone = phone;
    newMember.totalRentals = 0;
    newMember.points = 0;

    members.push_back(newMember);
    saveMembers(members);

    cout << "\nRegistration completed successfully.\n";
    cout << "  You may now log in with the username: " << newMember.name << "\n";
    pauseScreen();
}

/*==================================================================
  this method checks a renter login. if name + password match, it
  returns true and puts that member's id into memberIdOut
==================================================================*/
bool renterLoginCheck(const string& name, const string& password, int& memberIdOut) {
    vector<Member> members = loadMembers();
    for (const auto& m : members) {
        if (m.name == name && m.password == password) {
            memberIdOut = m.id;
            return true;
        }
    }
    return false;
}

/*==================================================================
  this method (admin) prints a table of every member
==================================================================*/
void viewAllMembers() {
    vector<Member> members = loadMembers();
    uiHeader("ALL REGISTERED MEMBERS");

    if (members.empty()) {
        cout << "\nThere are no registered members in the system yet.\n";
        pauseScreen();
        return;
    }

    cout << left << setw(6) << "ID" << setw(18) << "Name" << setw(15)
        << "Phone" << setw(10) << "Rentals" << setw(8) << "Points" << "\n";
    printLine();
    for (const auto& m : members) {
        cout << left << setw(6) << m.id << setw(18) << fitStr(m.name, 18) << setw(15)
            << fitStr(m.phone, 15) << setw(10) << m.totalRentals << setw(8) << m.points << "\n";
    }
    printLine();
    pauseScreen();
}

/*==================================================================
  this method (admin) looks up one member by name and shows their
  details
==================================================================*/
void searchMemberMenu() {
    vector<Member> members = loadMembers();
    uiHeader("SEARCH MEMBER");

    string name;
    if (stepString("Enter renter name to search (or 'q' to cancel): ", name) == InputResult::BACK) {
        cout << "\nOperation cancelled.\n";
        pauseScreen();
        return;
    }

    Member* found = searchMemberByName(members, name);
    printLine();
    if (found == nullptr) {
        cout << "\nNo member record matches that name.\n";
    }
    else {
        cout << "ID            : " << found->id << "\n";
        cout << "Name          : " << found->name << "\n";
        cout << "Phone         : " << found->phone << "\n";
        cout << "Total Rentals : " << found->totalRentals << "\n";
        cout << "Loyalty Points: " << found->points << "\n";
    }
    printLine();
    pauseScreen();
}

/*==================================================================
  this method (admin) changes a member's name and phone. the new
  name is checked so it does not clash with someone else
==================================================================*/
void modifyMember() {
    vector<Member> members = loadMembers();
    uiHeader("UPDATE MEMBER");

    string name;
    if (stepString("Enter renter name to update (or 'q' to cancel): ", name) == InputResult::BACK) {
        cout << "\nOperation cancelled.\n";
        pauseScreen();
        return;
    }

    Member* found = searchMemberByName(members, name);
    if (found == nullptr) {
        cout << "\nNo member record matches that name.\n";
        pauseScreen();
        return;
    }

    cout << "Current Name: " << found->name << "\n";
    string newName;
    if (stepString("Enter new name (or 'q' to cancel): ", newName) == InputResult::BACK) {
        cout << "\nUpdate cancelled. No changes were saved.\n";
        pauseScreen();
        return;
    }
    if (newName != found->name && isNameTaken(members, newName)) {
        cout << "\nThat username is already used by another member. Update cancelled.\n";
        pauseScreen();
        return;
    }
    found->name = newName;

    cout << "Current Phone: " << found->phone << "\n";
    string newPhone;
    if (stepPhone("Enter new phone (digits only, 7-15, or 'q' to cancel): ", newPhone) == InputResult::BACK) {
        cout << "\nUpdate cancelled. No changes were saved.\n";
        pauseScreen();
        return;
    }
    found->phone = newPhone;

    saveMembers(members);
    cout << "\nMember record updated successfully.\n";
    pauseScreen();
}

/*==================================================================
  this method (admin) deletes a member. it rebuilds the list
  without the matching name, then saves
==================================================================*/
void deleteMember() {
    vector<Member> members = loadMembers();
    uiHeader("DELETE MEMBER");

    string name;
    if (stepString("Enter renter name to delete (or 'q' to cancel): ", name) == InputResult::BACK) {
        cout << "\nOperation cancelled.\n";
        pauseScreen();
        return;
    }

    bool foundFlag = false;
    vector<Member> updated;
    for (const auto& m : members) {
        if (m.name == name) { foundFlag = true; continue; }   // skip = drop this one
        updated.push_back(m);
    }

    if (!foundFlag) {
        cout << "\nNo member record matches that name.\n";
    }
    else {
        saveMembers(updated);
        cout << "\nMember record deleted successfully.\n";
    }
    pauseScreen();
}

/*==================================================================
  this method lists a renter's rentals. it collects the matching
  rows first so the table header only shows when there is data
==================================================================*/
void viewRentalHistory(const string& renterName) {
    vector<Rental> rentals = loadRentals();
    vector<BikeType> bikeTypes = loadBikeTypes();

    uiHeader("RENTAL HISTORY - " + renterName);

    vector<Rental> mine;
    for (const auto& r : rentals) {
        if (r.renterName == renterName) mine.push_back(r);
    }

    if (mine.empty()) {
        cout << "\nNo rental history was found for this renter.\n";
        pauseScreen();
        return;
    }

    const string indent = " ";
    cout << indent << left << setw(4) << "ID" << setw(26) << "Bike Type" << setw(4)
        << "Qty" << setw(5) << "Days" << setw(12) << "Date" << setw(8) << "Code"
        << setw(9) << "Total RM" << "Status" << "\n";
    cout << indent << string(74, '-') << "\n";

    for (const auto& r : mine) {
        // turn the bike type id into its name
        string bikeTypeName = "Unknown";
        for (const auto& b : bikeTypes) {
            if (b.id == r.bikeTypeId) { bikeTypeName = b.type; break; }
        }
        ostringstream total;
        total << fixed << setprecision(2) << r.totalPaid;

        cout << indent << left << setw(4) << r.rentalId << setw(26) << fitStr(bikeTypeName, 26)
            << setw(4) << r.quantity << setw(5) << r.rentalDays << setw(12) << r.rentDate
            << setw(8) << r.code << setw(9) << total.str() << r.status << "\n";
    }
    cout << indent << string(74, '-') << "\n";
    pauseScreen();
}

/*==================================================================
  this method gives a member points and bumps their rental count.
  called by the billing module once a rental is fully returned
  (RM1 of rental price = 1 point)
==================================================================*/
void addPoints(int memberId, int amount) {
    vector<Member> members = loadMembers();
    Member* found = searchMemberById(members, memberId);
    if (found != nullptr) {
        found->points += amount;
        found->totalRentals += 1;
        saveMembers(members);
    }
}

/*==================================================================
  this method spends a member's points. returns false (and changes
  nothing) if the member is missing or does not have enough
==================================================================*/
bool deductPoints(int memberId, int amount) {
    vector<Member> members = loadMembers();
    Member* found = searchMemberById(members, memberId);
    if (found == nullptr) return false;
    if (found->points < amount) return false;

    found->points -= amount;
    saveMembers(members);
    return true;
}

/*==================================================================
  this method asks the renter (during booking) if they want to
  spend points for a discount. it shows the tiers, checks they
  have enough, and returns the % off (0 = no discount).
  pointsUsedOut = how many points to spend
==================================================================*/
int applyLoyaltyDiscount(int memberPoints, int& pointsUsedOut) {
    pointsUsedOut = 0;

    while (true) {
        bool wantDiscount;
        InputResult r = stepConfirm("Redeem loyalty points for a discount? (y/n): ", wantDiscount);
        if (r == InputResult::BACK) return 0;
        if (!wantDiscount) return 0;

        // show the tier table
        printLine();
        printCentered("LOYALTY DISCOUNT TIERS  (Your points: " + to_string(memberPoints) + ")");
        printLine();
        for (int i = 0; i < NUM_DISCOUNT_TIERS; i++) {
            cout << "  " << (i + 1) << ". " << discountPointsReq[i] << " points  ->  "
                << discountPercentTable[i] << "% off rental price\n";
        }
        cout << "  0. Skip discount\n";
        printLine();

        int tierChoice;
        InputResult r2 = stepInt("Select tier: ", 0, NUM_DISCOUNT_TIERS, tierChoice);
        if (r2 == InputResult::BACK) continue;   // back to the "redeem?" question
        if (tierChoice == 0) return 0;

        // check the member can afford this tier
        int reqPoints = discountPointsReq[tierChoice - 1];
        if (reqPoints > memberPoints) {
            cout << "You do not have enough points for this tier (" << reqPoints
                << " required, current balance " << memberPoints << "). Please choose another option.\n";
            continue;
        }

        pointsUsedOut = reqPoints;
        return discountPercentTable[tierChoice - 1];
    }
}

/*==================================================================
  this method is the admin "member management" menu. loops until 0
==================================================================*/
void memberA_menu() {
    int choice;
    do {
        uiHeader("CUSTOMER / MEMBER MANAGEMENT  (ADMIN)");
        printCentered("1. View All Members");
        printCentered("2. Search Member");
        printCentered("3. Update Member");
        printCentered("4. Delete Member");
        printCentered("5. View Rental History (by name)");
        printCentered("0. Back to Main Menu");
        printLine();
        choice = getValidatedInt("Enter choice: ", 0, 5);

        switch (choice) {
        case 1: viewAllMembers(); break;
        case 2: searchMemberMenu(); break;
        case 3: modifyMember(); break;
        case 4: deleteMember(); break;
        case 5: {
            uiHeader("VIEW RENTAL HISTORY");
            string name;
            if (stepString("Enter renter name (or 'q' to cancel): ", name) == InputResult::BACK) {
                cout << "\nOperation cancelled.\n";
                pauseScreen();
                break;
            }
            viewRentalHistory(name);
            break;
        }
        case 0: cout << "\nReturning to the main menu...\n"; break;
        }
    } while (choice != 0);
}
