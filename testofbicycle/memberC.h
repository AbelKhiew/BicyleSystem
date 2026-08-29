/*==================================================================
  memberC.h  -  Member C: Return Processing & Billing

  what this module does:
  - activate a "Pending" booking when the renter shows up with
    the reservation code (Pending -> Active)
  - process a return: record each bike's condition, work out the
    deposit refund (full / minus damage / nothing if late), put
    the bikes back in stock, and give loyalty points once every
    bike is back  (per-bike tracking is this module's EXTRA feature)
  - look up any rental record / receipt by its code
==================================================================*/
#ifndef MEMBERC_H
#define MEMBERC_H

#include "common.h"

void memberC_menu();          // the admin "returns & billing" menu

void activateReservation();   // Pending -> Active (renter gives the code)
void processReturn();         // the whole return + refund flow
void viewReceiptMenu();       // show one rental record by code

#endif
