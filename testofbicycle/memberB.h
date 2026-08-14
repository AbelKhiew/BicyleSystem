/* ============================================================
   memberB.h
   MEMBER B - Bicycle Inventory & Rental Booking Module
   ============================================================ */
#ifndef MEMBERB_H
#define MEMBERB_H

#include "common.h"

void memberB_menu();          // admin: inventory management only

/* Inventory management (admin) */
void addBikeType();
void editBikeType();
void viewBikeTypes();
BikeType* searchBikeTypeById(vector<BikeType>& bikeTypes, int id);
double calcDeposit(double tyreCost, double brakeCost, double chainCost);

/* Renter-facing booking flow (single flow: today or future date, quantity input) */
void viewAvailability();
void bookBike(int memberId, const string& renterName);

/* Admin walk-in rental (separate quantity pool, cash/card/QR payment) */
void walkInRental();

/* Shared payment helper: handles cash-with-change, prints final receipt */
void collectPaymentAndReceipt(const string& renterName, const string& bikeTypeName,
    int quantity, const string& rentDate, double totalDue,
    const string& code, bool isWalkIn, string& paymentMethodOut);

#endif