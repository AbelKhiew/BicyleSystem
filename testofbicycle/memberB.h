/*==================================================================
  memberB.h  -  Member B: Bike Inventory + Rental Booking

  what this module does:
  - admin: add / edit / delete / view bike types (each type has a
    price, repair costs, an auto-worked-out deposit, and 2 stock
    pools: one for online booking, one for walk-in)
  - renter: book a bike online - pick type, quantity, start date,
    number of days, optional loyalty discount, pay, get a code.
    the booking starts "Pending" until the counter activates it
  - admin: walk-in rental - same idea but done at the counter, so
    it starts "Active" right away and can take cash (with change)
==================================================================*/
#ifndef MEMBERB_H
#define MEMBERB_H

#include "common.h"

void memberB_menu();          // the admin "inventory" menu

void seedDefaultBikeTypes();  // add 2 starter bikes if the inventory is empty

// admin inventory tools
void addBikeType();
void editBikeType();
void deleteBikeType();
void viewBikeTypes();
BikeType* searchBikeTypeById(vector<BikeType>& bikeTypes, int id);          // find one, or nullptr
double calcDeposit(double tyreCost, double brakeCost, double chainCost);    // = tyre+brake+chain+20

// renter booking
void viewAvailability();                                    // list bikes that can be booked
void bookBike(int memberId, const string& renterName);      // the whole booking flow

// admin walk-in rental
void walkInRental();

// shared payment step: pick a method, take cash + give change, print the receipt.
// returns false if the user backed out with 'q'
bool collectPaymentAndReceipt(const string& renterName, const string& bikeTypeName,
    int quantity, const string& rentDate, double totalDue,
    const string& code, bool isWalkIn, string& paymentMethodOut);

#endif
