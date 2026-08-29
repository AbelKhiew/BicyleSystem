/* ============================================================
   Customer / Member Management Module

   Responsibilities:
   - Renter self-registration (name/password/phone, name unique)
   - Renter login verification (name + password)
   - Admin-side view / search / update / delete / rental history
   - Loyalty points earning and redemption support
   ============================================================ */
#include "memberA.h"
#include <iostream>
#include <iomanip>
using namespace std;

bool isNameTaken(const vector<Member>& members, const string& name) {
    for (const auto& m : members) {
        if (m.name == name) return true;
    }
    return false;
}

Member* searchMemberById(vector<Member>& members, int id) {
    for (auto& m : members) if (m.id == id) return &m;
    return nullptr;
}

Member* searchMemberByName(vector<Member>& members, const string& name) {
    for (auto& m : members) if (m.name == name) return &m;
    return nullptr;
}

void registerMember() {
    printLine();
    cout << "REGISTER NEW MEMBER (type 'q' at any prompt to go back)\n";
    printLine();

    vector<Member> members = loadMembers();

    string name, password, phone;
    int step = 0;
    while (step < 3) {
        InputResult res = InputResult::OK;
        switch (step) {
        case 0:
            while (true) {
                res = stepString("Enter username: ", name);
                if (res == InputResult::BACK) break;
                if (isNameTaken(members, name)) {
                    cout << "This name is already taken. Please choose another (or 'q' to go back).\n";
                    continue;
                }
                break;
            }
            break;
        case 1:
            res = stepString("Enter password: ", password);
            break;
        case 2:
            res = stepString("Enter phone number: ", phone);
            break;
        }

        if (res == InputResult::BACK) {
            step--;
            if (step < 0) {
                cout << "Registration cancelled.\n";
                pauseScreen();
                return;
            }
        }
        else {
            step++;
        }
    }

    Member newMember;
    newMember.id = getNextMemberId(members);
    newMember.name = name;
    newMember.password = password;
    newMember.phone = phone;
    newMember.totalRentals = 0;
    newMember.points = 0;

    members.push_back(newMember);
    saveMembers(members);

    cout << "\nRegistration successful! You can now log in as: " << newMember.name << "\n";
    pauseScreen();
}

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

void viewAllMembers() {
    vector<Member> members = loadMembers();
    printLine();
    cout << left << setw(6) << "ID" << setw(18) << "Name" << setw(15)
        << "Phone" << setw(10) << "Rentals" << setw(8) << "Points" << "\n";
    printLine();

    if (members.empty()) {
        cout << "No members found.\n";
    }
    else {
        for (const auto& m : members) {
            cout << left << setw(6) << m.id << setw(18) << m.name << setw(15)
                << m.phone << setw(10) << m.totalRentals << setw(8) << m.points << "\n";
        }
    }
    printLine();
    pauseScreen();
}

void searchMemberMenu() {
    vector<Member> members = loadMembers();
    string name;
    if (stepString("Enter renter name to search (or 'q' to cancel): ", name) == InputResult::BACK) {
        cout << "Cancelled.\n";
        pauseScreen();
        return;
    }

    Member* found = searchMemberByName(members, name);
    printLine();
    if (found == nullptr) {
        cout << "Member not found.\n";
    }
    else {
        cout << "ID: " << found->id << "\n";
        cout << "Name: " << found->name << "\n";
        cout << "Phone: " << found->phone << "\n";
        cout << "Total Rentals: " << found->totalRentals << "\n";
        cout << "Loyalty Points: " << found->points << "\n";
    }
    printLine();
    pauseScreen();
}

void modifyMember() {
    vector<Member> members = loadMembers();
    string name;
    if (stepString("Enter renter name to update (or 'q' to cancel): ", name) == InputResult::BACK) {
        cout << "Cancelled.\n";
        pauseScreen();
        return;
    }

    Member* found = searchMemberByName(members, name);
    if (found == nullptr) {
        cout << "Member not found.\n";
        pauseScreen();
        return;
    }

    cout << "Current Name: " << found->name << "\n";
    string newName;
    if (stepString("Enter new name (or 'q' to cancel): ", newName) == InputResult::BACK) {
        cout << "Update cancelled.\n";
        pauseScreen();
        return;
    }
    found->name = newName;

    cout << "Current Phone: " << found->phone << "\n";
    string newPhone;
    if (stepString("Enter new phone (or 'q' to cancel): ", newPhone) == InputResult::BACK) {
        cout << "Update cancelled.\n";
        pauseScreen();
        return;
    }
    found->phone = newPhone;

    saveMembers(members);
    cout << "Member updated successfully.\n";
    pauseScreen();
}

void deleteMember() {
    vector<Member> members = loadMembers();
    string name;
    if (stepString("Enter renter name to delete (or 'q' to cancel): ", name) == InputResult::BACK) {
        cout << "Cancelled.\n";
        pauseScreen();
        return;
    }

    bool foundFlag = false;
    vector<Member> updated;
    for (const auto& m : members) {
        if (m.name == name) { foundFlag = true; continue; }
        updated.push_back(m);
    }

    if (!foundFlag) {
        cout << "Member not found.\n";
    }
    else {
        saveMembers(updated);
        cout << "Member deleted successfully.\n";
    }
    pauseScreen();
}

void viewRentalHistory(const string& renterName) {
    vector<Rental> rentals = loadRentals();
    vector<BikeType> bikeTypes = loadBikeTypes();

    printLine();
    cout << "RENTAL HISTORY FOR MEMBER: " << renterName << "\n";
    printLine();
    cout << left << setw(9) << "RentalID" << setw(14) << "Bike Type" << setw(5)
        << "Qty" << setw(12) << "Date" << setw(9) << "Code" << setw(11)
        << "Total(RM)" << setw(11) << "Status" << "\n";

    bool any = false;
    for (const auto& r : rentals) {
        if (r.renterName == renterName) {
            any = true;
            string bikeTypeName = "Unknown";
            for (const auto& b : bikeTypes) {
                if (b.id == r.bikeTypeId) { bikeTypeName = b.type; break; }
            }
            cout << left << setw(9) << r.rentalId << setw(14) << bikeTypeName << setw(5)
                << r.quantity << setw(12) << r.rentDate << setw(9) << r.code << setw(11)
                << fixed << setprecision(2) << r.totalPaid << setw(11) << r.status << "\n";
        }
    }
    if (!any) cout << "No rental history found.\n";
    printLine();
    pauseScreen();
}

// after payment - RM1 spent on the bike price = 1 point
void addPoints(int memberId, int amount) {
    vector<Member> members = loadMembers();
    Member* found = searchMemberById(members, memberId);
    if (found != nullptr) {
        found->points += amount;
        found->totalRentals += 1;
        saveMembers(members);
    }
}

bool deductPoints(int memberId, int amount) {
    vector<Member> members = loadMembers();
    Member* found = searchMemberById(members, memberId);
    if (found == nullptr) return false;
    if (found->points < amount) return false;

    found->points -= amount;
    saveMembers(members);
    return true;
}

int applyLoyaltyDiscount(int memberPoints, int& pointsUsedOut) {
    pointsUsedOut = 0;

    while (true) {
        bool wantDiscount;
        InputResult r = stepConfirm("Redeem loyalty points for a discount? (y/n): ", wantDiscount);
        if (r == InputResult::BACK) return 0;
        if (!wantDiscount) return 0;

        printLine();
        cout << "LOYALTY DISCOUNT TIERS (Your points: " << memberPoints << ")\n";
        printLine();
        for (int i = 0; i < NUM_DISCOUNT_TIERS; i++) {
            cout << (i + 1) << ". " << discountPointsReq[i] << " points -> "
                << discountPercentTable[i] << "% off rental price\n";
        }
        cout << "0. Skip discount\n";
        printLine();

        int tierChoice;
        InputResult r2 = stepInt("Select tier: ", 0, NUM_DISCOUNT_TIERS, tierChoice);
        if (r2 == InputResult::BACK) continue;   // re-ask "redeem?" from scratch
        if (tierChoice == 0) return 0;

        int reqPoints = discountPointsReq[tierChoice - 1];
        if (reqPoints > memberPoints) {
            cout << "Insufficient points for this tier (" << reqPoints << " needed, you have "
                << memberPoints << "). Please choose again.\n";
            continue;
        }

        else
            cout << "Selected discount applied";

        pointsUsedOut = reqPoints;
        return discountPercentTable[tierChoice - 1];
    }
}

void memberA_menu() {
    int choice;
    do {
        printLine();
        cout << "CUSTOMER / MEMBER MANAGEMENT (ADMIN VIEW)\n";
        printLine();
        cout << "1. View All Members\n";
        cout << "2. Search Member\n";
        cout << "3. Update Member\n";
        cout << "4. Delete Member\n";
        cout << "5. View Rental History (by name)\n";
        cout << "0. Back to Main Menu\n";
        choice = getValidatedInt("Enter choice: ", 0, 5);

        switch (choice) {
        case 1: viewAllMembers(); break;
        case 2: searchMemberMenu(); break;
        case 3: modifyMember(); break;
        case 4: deleteMember(); break;
        case 5: {
            string name;
            if (stepString("Enter renter name (or 'q' to cancel): ", name) == InputResult::BACK) {
                cout << "Cancelled.\n";
                pauseScreen();
                break;
            }
            viewRentalHistory(name);
            break;
        }
        case 0: cout << "Returning to main menu...\n"; break;
        }
    } while (choice != 0);
}