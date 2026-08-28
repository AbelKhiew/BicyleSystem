/* ============================================================
   Return Processing & Billing / Payment Module
   ============================================================ */
#ifndef MEMBERC_H
#define MEMBERC_H

#include "common.h"

void memberC_menu();

void processReturn();
void printReceipt(const Rental& rental, const string& bikeTypeName);
void activateReservation();

#endif