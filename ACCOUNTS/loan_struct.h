#ifndef LOAN_RECORD
#define LOAN_RECORD

struct Loan
{
    int ID;
    int account;
    float ammount;
    bool isapprove;//true:approve, false:reject
    bool isassign;//true:assign, false:not assign

};

#endif
