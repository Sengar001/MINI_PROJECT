#ifndef ADMIN_FUNCTIONS
#define ADMIN_FUNCTIONS

#include<stdio.h>     
#include<unistd.h>    
#include<string.h>
#include<stdbool.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<stdlib.h>
#include<errno.h>
#include"../ADMIN/admin_struct.h"
#include"../EMPLOYEE/employee_struct.h"
// #define ADMIN_LOGIN_ID "admin"
// #define ADMIN_PASSWORD "34Hr82oIlAE8I"
#define SALT "34"

bool login_admin(int connFD);
int add_employee(int connFD);
bool admin_operation(int connFD);
bool modify_employee(int connFD);
//bool modify_customer(int connFD);

bool admin_operation(int connFD){
    if (login_admin(connFD)){
        int wBytes,rBytes;            
        char rBuffer[1000], wBuffer[1000]; 
        bzero(wBuffer,sizeof(wBuffer));
        strcpy(wBuffer,"Welcome admin!");
        while(1){
            strcat(wBuffer,"\n");
            strcat(wBuffer,"1. add new bank employee\n2. modify customer/employee details\n3. manage user roles\n4. change password\n5. logout");
            wBytes=write(connFD,wBuffer,strlen(wBuffer));
            if(wBytes==-1){
                perror("Error while writing ADMIN_MENU to client!");
                return false;
            }
            bzero(wBuffer,sizeof(wBuffer));
            rBytes=read(connFD,rBuffer,sizeof(rBuffer));
            if(rBytes==-1){
                perror("Error while reading client's choice for ADMIN_MENU");
                return false;
            }
            int choice=atoi(rBuffer);
            switch(choice){
            case 1:
                add_employee(connFD);
                break;
            case 2:
                bzero(wBuffer,sizeof(wBuffer));
                sprintf(wBuffer,"1. modify customer details\n2. modify employee details");
                wBytes=write(connFD,wBuffer,strlen(wBuffer));
                if(wBytes==-1){
                    perror("Error while writing modification to client!");
                    return false;
                }
                bzero(rBuffer,sizeof(rBuffer));
                rBytes=read(connFD,rBuffer,sizeof(rBuffer));
                if(rBytes==-1){
                    perror("Error while reading client's choice for modification");
                    return false;
                }
                int ptr=atoi(rBuffer);
                if(ptr==1){
                    //modify_customer(connFD);
                }else{
                    modify_employee(connFD);
                }
                break;
            case 3:
                //get_Joint_account_details(connFD, NULL);
                break;
            case 4:
                //add_account(connFD);
                break;
            case 5:
                wBytes=write(connFD,"Logging out",strlen("Logging out"));
                return false;
                break;
            }
        }
    }
    else
    {
        // ADMIN LOGIN FAILED
        return false;
    }
    return true;
}

bool login_admin(int connFD){
    struct Admin admin;
    int rBytes,wBytes;            
    char rBuffer[1000], wBuffer[1000]; 
    char tBuffer[1000];
    int ID;
    bzero(rBuffer, sizeof(rBuffer));
    bzero(wBuffer, sizeof(wBuffer));
    
    strcpy(wBuffer,"Welcome\nEnter your credentials\n");
    strcat(wBuffer,"Enter your login ID");

    wBytes = write(connFD,wBuffer,strlen(wBuffer));
    if(wBytes == -1)
    {
        perror("Error in writing message to the client!");
        return false;
    }

    rBytes = read(connFD,rBuffer,sizeof(rBuffer));
    if (rBytes == -1)
    {
        perror("Error reading login ID from client!");
        return false;
    }
    bzero(tBuffer,sizeof(tBuffer));
    strcpy(tBuffer,rBuffer);
    strtok(tBuffer,"-");
    ID=atoi(strtok(NULL,"-"));
    int fileFD=open("./ADMIN/admin.txt",O_RDONLY);
    if(fileFD==-1){
        perror("error in opening in file");
        return false;
    }
    int offset=lseek(fileFD,ID*sizeof(struct Admin),SEEK_SET);

    // if(offset==0){
    //     wBytes=write(connFD,"wrong username",sizeof("wrong username"));
    //     return false;
    // }

    struct flock lock;
    lock.l_type=F_RDLCK;
    lock.l_whence=SEEK_SET;
    lock.l_start=ID*sizeof(struct Admin);
    lock.l_len=sizeof(struct Admin);
    lock.l_pid=getpid();

    int lock_check=fcntl(fileFD,F_SETLKW,&lock);
    if(lock_check==-1){
        perror("error in locking\n");
        return false;
    }
    int read_bytes=read(fileFD,&admin,sizeof(admin));
    lock.l_type=F_UNLCK;
    fcntl(fileFD,F_SETLK,&lock);

    bool userFound;
    if(strcmp(admin.username,rBuffer)==0)
        userFound=true;
    close(fileFD);
    if(userFound)
    {
        bzero(wBuffer,sizeof(wBuffer));
        wBytes=write(connFD,"Enter your password",strlen("Enter your password"));
        if(wBytes==-1)
        {
            perror("Error writing PASSWORD message to client!");
            return false;
        }

        bzero(rBuffer,sizeof(rBuffer));
        rBytes=read(connFD,rBuffer,sizeof(rBuffer));
        if(rBytes==-1)
        {
            perror("Error reading password from the client!");
            return false;
        }
        char hashing[1000];
        strcpy(hashing,crypt(rBuffer,SALT));
        if (strcmp(hashing,admin.password)==0)
            return true;

        bzero(wBuffer, sizeof(wBuffer));
        wBytes = write(connFD, "The password specified doesn't match!",strlen("The password specified doesn't match!"));
    }
    else
    {
        bzero(wBuffer, sizeof(wBuffer));
        wBytes = write(connFD,"The login ID specified doesn't exist!",strlen("The login ID specified doesn't exist!"));
    }

    return false;
}

int add_employee(int connFD){
    int rBytes,wBytes;
    char rBuffer[1000],wBuffer[1000];
    struct Employee new_employee,prev_emplyee;

    int employeeFD=open("./EMPLOYEE/employee.txt",O_RDONLY);
    if(employeeFD==-1 && errno==ENOENT){
        new_employee.id=0;
    }else if(employeeFD==-1){
        perror("error in opening file\n");
        return -1;
    }else{
        int offset=lseek(employeeFD,-sizeof(struct Employee),SEEK_END);
        if(offset==-1){
            perror("Error seeking to last employee record!");
            return -1;
        }

        struct flock lock;
        lock.l_type=F_RDLCK;
        lock.l_whence=SEEK_SET;
        lock.l_start=offset;
        lock.l_len=sizeof(struct Employee);
        lock.l_pid=getpid();

        int lockingStatus=fcntl(employeeFD,F_SETLKW,&lock);
        if(lockingStatus==-1){
            perror("Error obtaining read lock on employee record!");
            return false;
        }

        rBytes=read(employeeFD,&prev_emplyee,sizeof(struct Employee));
        if(rBytes==-1){
            perror("Error while reading employee record from file!");
            return false;
        }

        lock.l_type = F_UNLCK;
        fcntl(employeeFD,F_SETLK,&lock);
        close(employeeFD);
        new_employee.id=prev_emplyee.id + 1;
    }
    // name
    bzero(wBuffer,sizeof(wBuffer));
    sprintf(wBuffer,"Enter the details for the employee\nWhat is the employee's name?");
    wBytes=write(connFD,wBuffer,sizeof(wBuffer));
    if(wBytes==-1){
        perror("Error writing add employee details to client!");
        return false;
    }
    bzero(rBuffer,sizeof(rBuffer));
    rBytes=read(connFD,rBuffer,sizeof(rBuffer));
    if(rBytes==-1){
        perror("Error reading employee name response from client!");
        return false;
    }
    strcpy(new_employee.name,rBuffer);

    //username 
    strcpy(new_employee.username,new_employee.name);
    strcat(new_employee.username,"-");
    sprintf(wBuffer,"%d",new_employee.id);
    strcat(new_employee.username,wBuffer);

    // gender
    bzero(wBuffer,sizeof(wBuffer));
    sprintf(wBuffer,"Enter gender\n1. MALE M\n2. FEMALE F\n3. OTHER O");
    wBytes=write(connFD,wBuffer,sizeof(wBuffer));
    if(wBytes==-1){
        perror("Error gender details to client!");
        return false;
    }
    bzero(rBuffer,sizeof(rBuffer));
    rBytes=read(connFD,rBuffer,sizeof(rBuffer));
    if(rBytes==-1){
        perror("Error reading gender from client!");
        return false;
    }
    char gender=rBuffer[0];
    new_employee.gender=gender;

    // age
    bzero(wBuffer,sizeof(wBuffer));
    sprintf(wBuffer,"Enter age of employee");
    wBytes=write(connFD,wBuffer,sizeof(wBuffer));
    if(wBytes==-1){
        perror("Error age details to client!");
        return false;
    }
    bzero(rBuffer,sizeof(rBuffer));
    rBytes=read(connFD,rBuffer,sizeof(rBuffer));
    if(rBytes==-1){
        perror("Error reading age from client!");
        return false;
    }
    int age=atoi(rBuffer);
    new_employee.age=age;

    // password
    char hashing[1000];
    strcpy(hashing,crypt(new_employee.name,SALT));
    strcpy(new_employee.password,hashing);

    employeeFD=open("./EMPLOYEE/employee.txt",O_CREAT|O_APPEND|O_WRONLY,S_IRWXU);
    if(employeeFD==-1){
        perror("Error while creating opening employee file!");
        return false;
    }
    wBytes=write(employeeFD,&new_employee,sizeof(new_employee));
    if(wBytes==-1){
        perror("Error while writing employee record to file!");
        return false;
    }

    close(employeeFD);

    bzero(wBuffer,sizeof(wBuffer));
    sprintf(wBuffer,"The autogenerated username and id for the employee is : %s ID : %d\nThe autogenerated password for the employee is : %s",new_employee.username,new_employee.id,new_employee.name);
    wBytes=write(connFD,wBuffer,strlen(wBuffer));
    if(wBytes==-1){
        perror("Error sending employee loginID and password to the client!");
        return false;
    }
    read(connFD,rBuffer,sizeof(rBuffer));
    return new_employee.id;
}

bool modify_employee(int connFD){
    int rBytes,wBytes;
    char rBuffer[1000],wBuffer[1000];
    struct Employee employee;
    bzero(rBuffer,sizeof(rBuffer));
    bzero(wBuffer,sizeof(wBuffer));
    sprintf(wBuffer,"enter employee ID for details modification");
    wBytes=write(connFD,wBuffer,sizeof(wBuffer));
    if(wBytes==-1){
        perror("Error writing message to client!");
        return false;
    }
    rBytes=read(connFD,rBuffer,sizeof(rBuffer));
    if(rBytes==-1){
        perror("Error reading employee ID from client!");
        return false;
    }

    int ID=atoi(rBuffer);
    
    int fileFD=open("./EMPLOYEE/employee.txt",O_RDWR);
    if(fileFD==-1) {
        perror("Open employee file");
        return false;
    }

    int offset=lseek(fileFD,ID*sizeof(struct Employee),0);
    if(offset==-1) {
        wBytes=write(connFD,"envalid employee ID", strlen("envalid employee ID"));
        close(fileFD);
        return false;
    }

    struct flock lock;
    lock.l_type=F_RDLCK;
    lock.l_whence=ID*sizeof(struct Employee);
    lock.l_start=0;
    lock.l_len=sizeof(struct Employee);
    lock.l_pid=getpid();

    int lock_status=fcntl(fileFD,F_SETLKW,&lock);
    if(lock_status==-1){
        perror("Read lock\n");
        return false;
    }

    rBytes=read(fileFD,&employee,sizeof(struct Employee));
    if(rBytes==-1){
        perror("Reading file\n");
    }
    lock.l_type=F_UNLCK;
    fcntl(fileFD,F_SETLK,&lock);
    close(fileFD);
    write(connFD,"current details of the employee!\n",strlen("current details of the employee!\n"));
    bzero(wBuffer,sizeof(wBuffer));
    sprintf(wBuffer,"employee ID : %d\nname : %s\ngender : %c\nage : %d",employee.id,employee.name,employee.gender,employee.age);
    strcat(wBuffer,"\n");
    strcat(wBuffer,"enter new name!");
    wBytes=write(connFD,wBuffer,sizeof(wBuffer));
    if(wBytes==-1){
        perror("Error writing message to client!");
        return false;
    }  
    rBytes=read(connFD,rBuffer,sizeof(rBuffer));
    strcpy(employee.name,rBuffer);
    strcpy(employee.username,employee.name);
    strcat(employee.username,"-");
    sprintf(wBuffer,"%d",employee.id);
    strcat(employee.username,wBuffer);

           
    fileFD=open("./EMPLOYEE/employee.txt",O_RDWR);
    if(fileFD==-1) {
        perror("Open employee file");
        return false;
    }

    offset=lseek(fileFD,ID*sizeof(struct Employee),0);
    if(offset==-1) {
        perror("error seeking in file\n");
        close(fileFD);
        return false;
    }

    lock.l_type=F_WRLCK;
    lock.l_whence=ID*sizeof(struct Employee);
    lock.l_start=0;
    lock.l_len=sizeof(struct Employee);
    lock.l_pid=getpid();

    lock_status=fcntl(fileFD,F_SETLKW,&lock);
    if(lock_status==-1){
        perror("write lock\n");
        return false;
    }

    wBytes=write(fileFD,&employee,sizeof(struct Employee));
    if(wBytes==-1){
        perror("error in writing to the file\n");
    }
    lock.l_type=F_UNLCK;
    fcntl(fileFD,F_SETLK,&lock);
    write(connFD,"new details of the employee!\n",strlen("new details of the employee!\n"));
    bzero(wBuffer,sizeof(wBuffer));
    sprintf(wBuffer,"employee ID : %d\nname : %s\ngender : %c\nage : %d\nusername : %s",employee.id,employee.name,employee.gender,employee.age,employee.username);
    wBytes=write(connFD,wBuffer,sizeof(wBuffer));
    if(wBytes==-1){
        perror("Error writing message to client!");
        return false;
    }
    close(fileFD);
    read(connFD,rBuffer,sizeof(rBuffer));
    return false;

}

#endif