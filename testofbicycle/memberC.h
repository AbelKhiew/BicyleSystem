/* ============================================================
   memberC.h
   MEMBER C - Return Processing & Billing / Payment Module
   ============================================================ */
#ifndef MEMBERC_H
#define MEMBERC_H

#include "common.h"

void memberC_menu();

void activateReservation();     // admin: activate a Pending future booking via code
void processReturn();           // admin: return one or more units, per-unit damage
void viewReceiptMenu();

#endif