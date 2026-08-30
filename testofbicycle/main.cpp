/*==================================================================
  main.cpp  -  program start point + main menu / navigation

  what this file does:
  - shows the welcome screen
  - handles login: renter, new renter sign-up, or admin
  - shows the correct menu and calls into each member's module
  - shows the goodbye message when the user exits
==================================================================*/
#include "common.h"
#include "memberA.h"
#include "memberB.h"
#include "memberC.h"
#include "memberD.h"
#include <iostream>
#include <cstdlib>
#include <ctime>
using namespace std;

// the one fixed admin account (for the demo). renters live in members.json
const string ADMIN_ID = "a1";
const string ADMIN_PASSWORD = "a1";

/*==================================================================
  this method asks for the admin id + password and returns true
  only if both match the fixed values above
==================================================================*/
bool adminLogin() {
    string id, password;
    cout << "Administrator ID: ";
    getline(cin, id);
    cout << "Administrator Password: ";
    getline(cin, password);
    return (id == ADMIN_ID && password == ADMIN_PASSWORD);
}

/*==================================================================
  this method prints the welcome banner, shown once at start-up
==================================================================*/
void showIntro() {
    clearScreen();
    printLine();
    printCentered("WELCOME TO THE BICYCLE RENTAL MANAGEMENT SYSTEM");
    printLine();
    printCentered("An integrated platform for bicycle rentals, bookings,");
    printCentered("returns, billing and management reporting.");
    printLine();
}

/*==================================================================
  this method is the renter's menu. it keeps looping until the
  renter picks 0 (logout)
==================================================================*/
void renterMenu(int memberId, const string& renterName) {
    int choice;
    system("cls");
    do {
        uiHeader("RENTER MENU  (" + renterName + ")");
        printCentered("1. Book a Bicycle");
        printCentered("2. View My Rental History");
        printCentered("0. Logout");
        printLine();
        choice = getValidatedInt("Enter choice: ", 0, 2);

        // run the action the renter picked
        switch (choice) {
        case 1: bookBike(memberId, renterName); break;
        case 2: viewRentalHistory(renterName); break;
        case 0: cout << "Logging out...\n"; break;
        }
    } while (choice != 0);
}

/*==================================================================
  this method is the admin's menu. each option jumps into one
  member's module (A / B / C / D). it loops until 0 (logout)
==================================================================*/
void adminMenu() {
    int choice;
    do {
        uiHeader("ADMIN MENU");
        printCentered("1. Customer / Member Management");
        printCentered("2. Bicycle Inventory Management");
        printCentered("3. Walk-in Rental");
        printCentered("4. Activate Reservation");
        printCentered("5. Return Processing & Billing");
        printCentered("6. Reporting & Statistics");
        printCentered("0. Logout");
        printLine();
        choice = getValidatedInt("Enter choice: ", 0, 6);

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

/*==================================================================
  this method is where the program starts. it sets up the data
  files, shows the intro, then loops the login menu until the
  user picks 0 (exit)
==================================================================*/
int main() {
    srand((unsigned int)time(0));   // seed random numbers (used to make reservation codes)

    ensureDataFilesExist();   // create members / biketypes / rentals .json if missing
    seedDefaultMembers();     // add demo renter u1/u1 if there are no members yet
    seedDefaultBikeTypes();   // add 2 starter bikes if the inventory is empty

    int programChoice;
    bool firstLoginScreen = true;   // first pass: don't clear, so the intro stays visible
    do {
        printLine();
        cout << "LOGIN\n";
        printLine();
        programChoice = getValidatedInt("Enter choice: ", 0, 3);

        switch (programChoice) {
        case 1: {   // renter login - check name + password against members.json
            string name = getValidatedString("Username: ");
            string password = getValidatedString("Password: ");
            int memberId;
            if (renterLoginCheck(name, password, memberId)) {
                renterMenu(memberId, name);
            }
            else {
                cout << "\nLogin failed: the username or password is incorrect.\n";
                pauseScreen();
            }
            break;
        }
        case 2:                 // new renter sign-up
            registerMember();
            break;
        case 3: {
            if (adminLogin()) {
                adminMenu();
            }
            else {
                cout << "\nLogin failed: the administrator credentials are incorrect.\n";
                pauseScreen();
            }
            break;
        }
        case 0:
            break;              // leave the loop and end the program
        }
    } while (programChoice != 0);

    // goodbye screen
    clearScreen();
    printLine();
    printCentered("Thank you for using the Bicycle Rental Management System.");
    printCentered("The program has ended. Have a pleasant day.");
    printLine();

    return 0;
}
