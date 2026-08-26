/* ============================================================
   memberA.h
   MEMBER A - Customer / Member Management Module
   ============================================================ */
#ifndef MEMBERA_H
#define MEMBERA_H

#include "common.h"

void memberA_menu();               // admin-facing view/search/history menu

bool isNameTaken(const vector<Member>& members, const string& name);
Member* searchMemberById(vector<Member>& members, int id);
Member* searchMemberByName(vector<Member>& members, const string& name);

void registerMember();             // self-service: name, password, phone (name must be unique)
bool renterLoginCheck(const string& name, const string& password, int& memberIdOut);

void viewAllMembers();
void searchMemberMenu();
void updateMember();
void deleteMember();
void viewRentalHistory(const string& renterName);   // shown by name, per requirement

/* Called by billing module (cross-module function) */
void addPoints(int memberId, int amount);
bool deductPoints(int memberId, int amount);

#endif