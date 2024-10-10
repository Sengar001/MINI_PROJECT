#ifndef TRANSACTIONS
#define TRANSACTIONS

#include <time.h>

struct Transaction
{
    int transactionID; 
    int accountNumber;
    bool operation;//0:debit,1:credit
    float oldBalance;
    float newBalance;
    time_t transactionTime;
};

#endif