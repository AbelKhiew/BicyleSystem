/* ============================================================
   main.cpp
   BICYCLE RENTAL MANAGEMENT SYSTEM

   TEAM COMPONENT:
   - Introduction screen
   - Login: Renter (name + password) / Admin (hardcoded id=a, pass=a)
   - Main menu navigation branching to each member's module
   - Program termination message
   ============================================================ */
#include "common.h"
#include "memberA.h"
#include "memberB.h"
#include "memberC.h"
#include "memberD.h"
#include <iostream>
#include <cstdlib>
#include <ctime>
using namespace std;

/* Hardcoded admin credentials: ID = a, Password = a */
bool adminLogin() {
    string id, password;
    cout << "Admin ID: ";
    getline(cin, id);
    cout << "Admin Password: ";
    getline(cin, password);
    return (id == "a" && password == "a");
}

void showIntro() {
    printLine();
    cout << "     WELCOME TO THE BICYCLE RENTAL MANAGEMENT SYSTEM\n";
    printLine();
    cout << "Rent a bike, manage bookings, and track your rides with ease.\n";
    printLine();
}

/* Renter menu: booking/reservation self-service only. */
void renterMenu(int memberId, const string& renterName) {
    int choice;
    do {
        printLine();
        cout << "RENTER MENU  (" << renterName << ")\n";
        printLine();
        cout << "1. View Bike Availability\n";
        cout << "2. Book a Bike\n";
        cout << "3. View My Rental History\n";
        cout << "0. Logout\n";
        choice = getValidatedInt("Enter choice: ", 0, 3);

        switch (choice) {
        case 1: viewAvailability(); break;
        case 2: bookBike(memberId, renterName); break;
        case 3: viewRentalHistory(renterName); break;
        case 0: cout << "Logging out...\n"; break;
        }
    } while (choice != 0);
}

/* Admin menu: Walk-in Rental has its own top-level entry (not nested
   inside Inventory Management). Admin has NO access to the renter's
   self-service booking/reservation flow. */
void adminMenu() {
    int choice;
    do {
        printLine();
        cout << "ADMIN MENU\n";
        printLine();
        cout << "1. Customer / Member Management\n";
        cout << "2. Bicycle Inventory Management\n";
        cout << "3. Walk-in Rental\n";
        cout << "4. Activate Reservation (future bookings)\n";
        cout << "5. Return Processing & Billing\n";
        cout << "6. Reporting & Statistics\n";
        cout << "0. Logout\n";
        choice = getValidatedInt("Enter choice: ", 0, 5);

        switch (choice) {
        case 1: memberA_menu(); break;
        case 2: memberB_menu(); break;
        case 3: walkInRental(); break;
        case 4: activateReservation(); break;
        case 5: memberC_menu(); break;
        case 6: memberD_menu(); break;
        case 0: cout << "Logging out...\n"; break;
        }
    } while (choice != 0);
}

int main() {
    srand((unsigned int)time(0));
    ensureDataFilesExist();

    showIntro();

    int programChoice;
    do {
        printLine();
        cout << "LOGIN\n";
        printLine();
        cout << "1. Login as Renter\n";
        cout << "2. Register as New Renter\n";
        cout << "3. Login as Admin\n";
        cout << "0. Exit Program\n";
        programChoice = getValidatedInt("Enter choice: ", 0, 3);

        switch (programChoice) {
        case 1: {
            string name = getValidatedString("Username: ");
            string password = getValidatedString("Password: ");
            int memberId;
            if (renterLoginCheck(name, password, memberId)) {
                renterMenu(memberId, name);
            }
            else {
                cout << "Invalid username or password.\n";
                pauseScreen();
            }
            break;
        }
        case 2:
            registerMember();
            break;
        case 3: {
            if (adminLogin()) {
                adminMenu();
            }
            else {
                cout << "Invalid admin credentials.\n";
                pauseScreen();
            }
            break;
        }
        case 0:
            break;
        }
    } while (programChoice != 0);

    printLine();
    cout << "   Thank you for using the Bicycle Rental Management System.\n";
    cout << "                     Have a safe ride! Goodbye.\n";
    printLine();

    return 0;
}