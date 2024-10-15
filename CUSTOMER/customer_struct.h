#ifndef CUSTOMER_RECORD
#define CUSTOMER_RECORD

struct Customer
{
    char name[30];
    char gender;
    int age;
    char username[30];
    char password[30];
    int account;
    bool active;//true:active,false:not active
    float balance;
    int transaction[10];
    int loanID;
};

#endif
