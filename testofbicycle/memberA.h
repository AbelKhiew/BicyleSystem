/*==================================================================
  memberA.h  -  Member A: Customer / Member Management

  what this module does:
  - renter sign-up + login checking
  - admin side: view / search / update / delete members
  - view any renter's rental history (looked up by name)
  - loyalty points: add / spend, and the discount picker
    (points earning + redeeming is this module's EXTRA feature,
    shared with the billing module)
==================================================================*/
#ifndef MEMBERA_H
#define MEMBERA_H

#include "common.h"

void memberA_menu();          // the admin "member management" menu

void seedDefaultMembers();    // add demo renter u1/u1 if there are no members

// small search helpers used across the module (and by other modules)
bool isNameTaken(const vector<Member>& members, const string& name);
Member* searchMemberById(vector<Member>& members, int id);
Member* searchMemberByName(vector<Member>& members, const string& name);

void registerMember();                                                        // new renter sign-up
bool renterLoginCheck(const string& name, const string& password, int& memberIdOut);  // login check

void viewAllMembers();                        // admin: list every member
void searchMemberMenu();                      // admin: find one member by name
void modifyMember();                          // admin: change a member's name / phone
void deleteMember();                          // admin: remove a member
void viewRentalHistory(const string& renterName);   // list a renter's rentals

// called by the billing module when a rental is returned / when points are spent
void addPoints(int memberId, int amount);
bool deductPoints(int memberId, int amount);

// asks the renter if they want to spend points for a discount; returns the % off
int applyLoyaltyDiscount(int memberPoints, int& pointsUsedOut);

#endif
