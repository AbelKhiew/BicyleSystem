/*==================================================================
  memberD.h  -  Member D: Reporting & Statistics (admin only)

  what this module does - 7 read-only reports built from the
  rentals / bike / member data:
  - today's takings, broken down by payment method
  - monthly takings in a month x payment-method grid (2D array)
  - which bike type has been rented the most
  - how much has been spent on repairs, by part
  - deposits lost to late returns
  - the top renters by total spending
  - what is still out: Active / Pending / Overdue rentals
==================================================================*/
#ifndef MEMBERD_H
#define MEMBERD_H

#include "common.h"

void memberD_menu();                    // the admin "reports" menu

void dailySummaryAndRevenueReport();    // today's bookings + money in / out
void monthlyRevenueReport();            // month x payment-method grid
void mostRentedTypeReport();            // bike type with the highest timesRented
void damageCostReport();                // repair spend split by tyre / brake / chain
void lateReturnReport();                // count + total of forfeited deposits
void topRentersReport();                // top 3 renters by spending
void overdueList();                     // rentals not yet returned

#endif
