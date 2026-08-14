/* ============================================================
   memberD.h
   MEMBER D - Reporting & Statistics Module (Admin)
   ============================================================ */
#ifndef MEMBERD_H
#define MEMBERD_H

#include "common.h"

void memberD_menu();

void revenueReport();          // total revenue + breakdown by payment method
void mostRentedTypeReport();
void damageCostReport();       // repair cost breakdown by part (tyre/brake/chain)
void lateReturnReport();       // count + total forfeited deposits from late returns
void topRentersReport();       // top 3 renters by total spending
void dailySummaryReport();     // today's bookings, walk-ins, returns, revenue
void overdueList();

#endif