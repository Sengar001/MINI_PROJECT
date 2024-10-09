#ifndef TRANSACTIONS
#define TRANSACTIONS

#include <time.h>

struct Transaction
{
    int transactionID; 
    int accountNumber;
    int operation;//0:deposit,1:withdrawl,2:transfer 
    long int oldBalance;
    long int newBalance;
    time_t transactionTime;
};

#endif