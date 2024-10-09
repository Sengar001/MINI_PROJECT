#ifndef ACCOUNT_RECORD
#define ACCOUNT_RECORD

struct Account
{
    int accountNumber;                 
    int owner;                      
    bool active;                        
    long int balance;                   
    int transactions[10];               
};

#endif