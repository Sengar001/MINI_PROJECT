#ifndef EMPLOYEE_RECORD
#define EMPLOYEE_RECORD

struct Employee {
    int id;
    char name[30];
    char gender;
    int age;
    char username[30];
    char password[30];
    bool ismanager;//true:manager,false:employee
    int loan[10];
};

#endif