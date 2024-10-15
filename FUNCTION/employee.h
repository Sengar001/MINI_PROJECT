#ifndef EMPLOYEE_FUNCTIONS
#define EMPLOYEE_FUNCTIONS

#include<stdio.h>     
#include<unistd.h>    
#include<string.h>
#include<stdbool.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<stdlib.h>
#include<errno.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include"../EMPLOYEE/employee_struct.h"
#include"../CUSTOMER/customer_struct.h"
#include"../ACCOUNTS/transaction_struct.h"
#include"../ACCOUNTS/loan_struct.h"
#include"../ACCOUNTS/feedback_struct.h"
#define SALT "34"

struct Employee employee;
int semId;

bool login_employee(int connFD);
int add_customer(int connFD);
bool modify_customer_byemployee(int connFD);
bool get_transaction_details_byemployee(int connFD);
bool change_password_employee(int connFD);
bool activate_deactivate_account(int connFD);
bool assign_loan_to_employee(int connFD);
bool view_assign_loan_application(int connFD);
bool approved_reject_loan(int connFD);
bool review_feedback(int connFD);
bool lock_critical_section(struct sembuf *semOp);
bool unlock_critical_section(struct sembuf *sem_op);


bool employee_operation(int connFD,int role){
    if (login_employee(connFD)){
        int wBytes,rBytes;            
        char rBuffer[1000], wBuffer[1000];
        if(employee.ismanager==false && role==3){
            
            int semKey=ftok("./EMPLOYEE/employee.txt",employee.id); 
            union semun{
                int val; 
            } semSet;
            semId=semget(semKey,1,0);
            if(semId==-1){
                semId=semget(semKey,1,IPC_CREAT|0700);
                if (semId == -1){
                    perror("Error while creating semaphore!");
                    _exit(1);
                }
                semSet.val = 1; 
                int semctlStatus=semctl(semId,0,SETVAL,semSet);
                if(semctlStatus==-1){
                    perror("Error while initializing a binary sempahore!");
                    _exit(1);
                }
            }  
            struct sembuf semOp;
            lock_critical_section(&semOp);

            bzero(wBuffer,sizeof(wBuffer));
            strcpy(wBuffer,"Welcome employee!");
            while(1){
                strcat(wBuffer,"\n");
                strcat(wBuffer,"1. add new customer\n2. modify customer details\n3. approve/reject loan\n4. view assigned loan application\n5. view customer transaction\n6. change password\n7. logout");
                wBytes=write(connFD,wBuffer,strlen(wBuffer));
                if(wBytes==-1){
                    perror("Error while writing EMPLOYEE_MENU to client!");
                    return false;
                }
                bzero(wBuffer,sizeof(wBuffer));
                rBytes=read(connFD,rBuffer,sizeof(rBuffer));
                if(rBytes==-1){
                    perror("Error while reading client's choice for EMPLOYEE_MENU");
                    return false;
                }
                int choice=atoi(rBuffer);
                switch(choice){
                case 1:
                    add_customer(connFD);
                    break;
                case 2:
                    modify_customer_byemployee(connFD);
                    break;
                case 3:
                    approved_reject_loan(connFD);
                    break;
                case 4:
                    view_assign_loan_application(connFD);
                    break;
                case 5:
                    get_transaction_details_byemployee(connFD);
                    break;
                case 6:
                    change_password_employee(connFD);
                    break;
                default:
                    wBytes=write(connFD,"Logging out\n",strlen("Logging out\n"));
                    unlock_critical_section(&semOp);
                    return false;
                    break;
                }
            }
        }else if(employee.ismanager==true && role==2){
            int semKey=ftok("./EMPLOYEE/employee.txt",employee.id); 
            union semun{
                int val; 
            } semSet;
            semId=semget(semKey,1,0);
            if(semId==-1){
                semId=semget(semKey,1,IPC_CREAT|0700);
                if (semId == -1){
                    perror("Error while creating semaphore!");
                    _exit(1);
                }
                semSet.val = 1; 
                int semctlStatus=semctl(semId,0,SETVAL,semSet);
                if(semctlStatus==-1){
                    perror("Error while initializing a binary sempahore!");
                    _exit(1);
                }
            }
            struct sembuf semOp;
            lock_critical_section(&semOp);

            bzero(wBuffer,sizeof(wBuffer));
            strcpy(wBuffer,"Welcome manager!");
            while(1){
                strcat(wBuffer,"\n");
                strcat(wBuffer,"1. activate/deactivate customer account\n2. assign loan application to employee\n3. review custosmer feedback\n4. change password\n5. logout");
                wBytes=write(connFD,wBuffer,strlen(wBuffer));
                if(wBytes==-1){
                    perror("Error while writing MANAGER_MENU to client!");
                    return false;
                }
                bzero(wBuffer,sizeof(wBuffer));
                rBytes=read(connFD,rBuffer,sizeof(rBuffer));
                if(rBytes==-1){
                    perror("Error while reading client's choice for MANAGER_MENU");
                    return false;
                }
                int choice=atoi(rBuffer);
                switch(choice){
                case 1:
                    activate_deactivate_account(connFD);
                    break;
                case 2:
                    assign_loan_to_employee(connFD);
                    break;
                case 3:
                    review_feedback(connFD);
                    break;
                case 4:
                    change_password_employee(connFD);
                    break;
                default:
                    wBytes=write(connFD,"Logging out\n",strlen("Logging out\n"));
                    unlock_critical_section(&semOp);
                    return false;
                    break;
                }
            }
        }
    }
    else
    {
        return false;
    }
    return true;
}

bool login_employee(int connFD){
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

    int fileFD=open("./EMPLOYEE/employee.txt",O_RDONLY);
    if(fileFD==-1){
        perror("error in opening in file");
        return false;
    }
    int offset=lseek(fileFD,ID*sizeof(struct Employee),SEEK_SET);

    if(offset==-1){
        wBytes=write(connFD,"wrong username",sizeof("wrong username"));
        return false;
    }

    struct flock lock;
    lock.l_type=F_RDLCK;
    lock.l_whence=SEEK_SET;
    lock.l_start=ID*sizeof(struct Employee);
    lock.l_len=sizeof(struct Employee);
    lock.l_pid=getpid();

    int lock_check=fcntl(fileFD,F_SETLKW,&lock);
    if(lock_check==-1){
        perror("error in locking\n");
        return false;
    }
    int read_bytes=read(fileFD,&employee,sizeof(employee));
    lock.l_type=F_UNLCK;
    fcntl(fileFD,F_SETLK,&lock);

    bool userFound=false;
    if(strcmp(employee.username,rBuffer)==0)
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
        if (strcmp(hashing,employee.password)==0)
            return true;

        bzero(wBuffer, sizeof(wBuffer));
        wBytes=write(connFD, "The password specified doesn't match!",strlen("The password specified doesn't match!"));
        read(connFD,rBuffer,sizeof(rBuffer));
        return false;
    }
    else
    {
        bzero(wBuffer, sizeof(wBuffer));
        wBytes=write(connFD,"The login ID specified doesn't exist!",strlen("The login ID specified doesn't exist!"));
        read(connFD,rBuffer,sizeof(rBuffer));
        return false;
    }

    return false;
}

int add_customer(int connFD){
    int rBytes,wBytes;
    char rBuffer[1000],wBuffer[1000];
    struct Customer new_customer,prev_customer;

    // name
    bzero(wBuffer,sizeof(wBuffer));
    sprintf(wBuffer,"Enter the details for the customer\nWhat is the customer's name?");
    wBytes=write(connFD,wBuffer,sizeof(wBuffer));
    if(wBytes==-1){
        perror("Error writing add customer details to client!");
        return false;
    }
    bzero(rBuffer,sizeof(rBuffer));
    rBytes=read(connFD,rBuffer,sizeof(rBuffer));
    if(rBytes==-1){
        perror("Error reading customer name response from client!");
        return false;
    }
    strcpy(new_customer.name,rBuffer);

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
    new_customer.gender=gender;

    // age
    bzero(wBuffer,sizeof(wBuffer));
    sprintf(wBuffer,"Enter age of customer");
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
    new_customer.age=age;
    
    //status
    new_customer.active=true;

    //loan
    new_customer.loanID=-1;

    //balance
    wBytes=write(connFD,"please enter balance!",strlen("please enter balance!"));
    if(wBytes==-1) {
        perror("Writing balance to client!");
        return false;
    }

    bzero(rBuffer,sizeof(rBuffer));
    rBytes=read(connFD,rBuffer,sizeof(rBuffer));
    if(rBytes==-1) {
        perror("reading balance!");
        return false;
    }

    float balance=atof(rBuffer);
    new_customer.balance=balance;

    //transaction
    for(int i=0;i<10;i++){
        new_customer.transaction[i]=-1;
    }

    int customerFD=open("./CUSTOMER/customer.txt",O_RDWR);
    if(customerFD==-1 && errno==ENOENT){
        new_customer.account=1;
    }else if(customerFD==-1){
        perror("error in opening file\n");
        return false;
    }else{
        int offset=lseek(customerFD,-sizeof(struct Customer),SEEK_END);
        if(offset==-1){
            perror("Error seeking to last customer record!");
            return false;
        }

        struct flock lock;
        lock.l_type=F_WRLCK;
        lock.l_whence=SEEK_SET;
        lock.l_start=offset;
        lock.l_len=sizeof(struct Customer);
        lock.l_pid=getpid();

        int lockingStatus=fcntl(customerFD,F_SETLKW,&lock);
        if(lockingStatus==-1){
            perror("Error obtaining read lock on customer record!");
            return false;
        }

        rBytes=read(customerFD,&prev_customer,sizeof(struct Customer));
        if(rBytes==-1){
            perror("Error while reading customer record from file!");
            return false;
        }

        lock.l_type = F_UNLCK;
        fcntl(customerFD,F_SETLK,&lock);
        close(customerFD);
        new_customer.account=prev_customer.account + 1;
    }
    //username 
    strcpy(new_customer.username,new_customer.name);
    strcat(new_customer.username,"-");
    sprintf(wBuffer,"%d",new_customer.account);
    strcat(new_customer.username,wBuffer);

    // password
    char hashing[1000];
    strcpy(hashing,crypt(new_customer.name,SALT));
    strcpy(new_customer.password,hashing);

    customerFD=open("./CUSTOMER/customer.txt",O_CREAT|O_APPEND|O_WRONLY,S_IRWXU);
    if(customerFD==-1){
        perror("Error while creating opening employee file!");
        return false;
    }
    
    struct flock lock;
    lock.l_type=F_WRLCK;
    lock.l_whence=SEEK_END;
    lock.l_start=0;
    lock.l_len=0;
    lock.l_pid=getpid();

    int lock_status=fcntl(customerFD,F_SETLKW,&lock);
    if(lock_status==-1){
        perror("Write lock on customer file");
        close(customerFD);
        return -1;
    }

    wBytes=write(customerFD,&new_customer,sizeof(new_customer));
    if(wBytes==-1){
        perror("Error while writing employee record to file!");
        return false;
    }

    close(customerFD);

    bzero(wBuffer,sizeof(wBuffer));
    sprintf(wBuffer,"The autogenerated username for the customer is : %s account number : %d\nThe autogenerated password for the customer is : %s",new_customer.username,new_customer.account,new_customer.name);
    wBytes=write(connFD,wBuffer,strlen(wBuffer));
    if(wBytes==-1){
        perror("Error sending employee loginID and password to the client!");
        return false;
    }
    read(connFD,rBuffer,sizeof(rBuffer));
    return new_customer.account;
}

bool modify_customer_byemployee(int connFD){
    int rBytes,wBytes;
    char rBuffer[1000],wBuffer[1000];
    struct Customer customer;
    bzero(rBuffer,sizeof(rBuffer));
    bzero(wBuffer,sizeof(wBuffer));
    sprintf(wBuffer,"enter customer account for details modification");
    wBytes=write(connFD,wBuffer,sizeof(wBuffer));
    if(wBytes==-1){
        perror("Error writing message to client!");
        return false;
    }
    rBytes=read(connFD,rBuffer,sizeof(rBuffer));
    if(rBytes==-1){
        perror("Error reading customer ID from client!");
        return false;
    }

    int ID=atoi(rBuffer)-1;
    int fileFD=open("./CUSTOMER/customer.txt",O_RDONLY);
    if(fileFD==-1){
        perror("error in opening in file");
        return false;
    }
    int offset=lseek(fileFD,ID*sizeof(struct Customer),SEEK_SET);

    if(offset==-1){
        wBytes=write(connFD,"wrong ID",sizeof("wrong ID"));
        return false;
    }

    struct flock lock;
    lock.l_type=F_RDLCK;
    lock.l_whence=SEEK_SET;
    lock.l_start=ID*sizeof(struct Customer);
    lock.l_len=sizeof(struct Customer);
    lock.l_pid=getpid(); 

    int lock_check=fcntl(fileFD,F_SETLKW,&lock);
    if(lock_check==-1){
        perror("error in locking\n");
        return false;
    }
    rBytes=read(fileFD,&customer,sizeof(customer)); 
    if(rBytes==-1){
        perror("error in reading from file!");
    }

    lock.l_type=F_UNLCK;
    fcntl(fileFD,F_SETLK,&lock);
    close(fileFD);

    bzero(wBuffer,sizeof(wBuffer));
    sprintf(wBuffer,"current details of customer:\nID : %d\nname : %s\nage : %d\ngender : %c\nusername : %s\npress YES for modification",customer.account,customer.name,customer.age,customer.gender,customer.username);
    write(connFD,wBuffer,sizeof(wBuffer));
    read(connFD,rBuffer,sizeof(rBuffer));
    int count=0;
    while(1){
        bzero(wBuffer,sizeof(wBuffer));
        strcpy(wBuffer,"enter details you want to modify\n1. name\n2. age\n3. gender\n4. no change");
        write(connFD,wBuffer,sizeof(wBuffer));
        bzero(rBuffer,sizeof(rBuffer));
        read(connFD,rBuffer,sizeof(rBuffer));
        int choice=atoi(rBuffer);
        switch(choice){
            case 1:
                write(connFD,"enter new name",sizeof("enter new name"));
                bzero(rBuffer,sizeof(rBuffer));
                read(connFD,rBuffer,sizeof(rBuffer));
                strcpy(customer.name,rBuffer);

                bzero(wBuffer,sizeof(wBuffer));
                strcpy(customer.username,customer.name);
                strcat(customer.username,"-");
                sprintf(wBuffer,"%d",customer.account);
                strcat(customer.username,wBuffer);
                break;
            case 2:
                write(connFD,"enter new age",sizeof("enter new age"));
                bzero(rBuffer,sizeof(rBuffer));
                read(connFD,rBuffer,sizeof(rBuffer));
                int age=atoi(rBuffer);
                customer.age=age;
                break;
            case 3:
                write(connFD,"enter gender\nMALE M\n FEMALE F\nOTHER O",sizeof("enter gender\nMALE M\n FEMALE F\nOTHER O"));
                bzero(rBuffer,sizeof(rBuffer));
                read(connFD,rBuffer,sizeof(rBuffer));
                char gen=rBuffer[0];
                customer.gender=gen;
                break;
            default:
                count++;
                break;
        }
        if(count)
            break;
    }

    fileFD=open("./CUSTOMER/customer.txt",O_WRONLY);
    if(fileFD==-1){
        perror("error in opening in file");
        return false;
    }
    offset=lseek(fileFD,ID*sizeof(struct Customer),SEEK_SET);

    if(offset==-1){
        wBytes=write(connFD,"wrong ID",sizeof("wrong ID"));
        return false;
    }

    lock.l_type=F_WRLCK;
    lock.l_whence=SEEK_SET;
    lock.l_start=ID*sizeof(struct Customer);
    lock.l_len=sizeof(struct Customer);
    lock.l_pid=getpid(); 

    lock_check=fcntl(fileFD,F_SETLKW,&lock);
    if(lock_check==-1){
        perror("error in locking\n");
        return false;
    }
    wBytes=write(fileFD,&customer,sizeof(customer)); 
    if(wBytes==-1){
        perror("error in writing to file!");
    }

    lock.l_type=F_UNLCK;
    fcntl(fileFD,F_SETLK,&lock);
    close(fileFD);

    bzero(wBuffer,sizeof(wBuffer));
    sprintf(wBuffer,"new details of customer:\nID : %d\nname : %s\nage : %d\ngender : %c\nusername : %s",customer.account,customer.name,customer.age,customer.gender,customer.username);
    write(connFD,wBuffer,sizeof(wBuffer));
    read(connFD,rBuffer,sizeof(rBuffer));

    return true;

}

bool get_transaction_details_byemployee(int connFD){
    int rBytes, wBytes;                              
    char rBuffer[1000],wBuffer[10000],tBuffer[1000]; 
    struct Customer customer;
    write(connFD,"enter account number for which want to see transaction details!",sizeof("enter account number for which want to see transaction details!"));
    read(connFD,rBuffer,sizeof(rBuffer));
    int ID=atoi(rBuffer)-1;
    int fileFD=open("./CUSTOMER/customer.txt",O_RDONLY);
    if(fileFD==-1){
        perror("error in opening in file");
        return false;
    }
    int offset=lseek(fileFD,(ID)*sizeof(struct Customer),SEEK_SET);

    if(offset==-1){
        wBytes=write(connFD,"wrong username",sizeof("wrong username"));
        read(connFD,rBuffer,sizeof(rBuffer));
        return false;
    }

    struct flock lock;
    lock.l_type=F_RDLCK;
    lock.l_whence=SEEK_SET;
    lock.l_start=(ID)*sizeof(struct Customer);
    lock.l_len=sizeof(struct Customer);
    lock.l_pid=getpid();

    int lock_check=fcntl(fileFD,F_SETLKW,&lock);
    if(lock_check==-1){
        perror("error in locking\n");
        return false;
    }
    int read_bytes=read(fileFD,&customer,sizeof(customer));
    lock.l_type=F_UNLCK;
    fcntl(fileFD,F_SETLK,&lock);
    close(fileFD);
    
    int ptr=0;

    struct Transaction transaction;
    struct tm transactionTime;

    int transactionFD=open("./ACCOUNTS/transaction.txt",O_RDONLY);
    if (transactionFD==-1){
        perror("error in opening file!");
        return false;
    }
    while(ptr<10 && customer.transaction[ptr]!=-1){
        int offset=lseek(transactionFD,customer.transaction[ptr]*sizeof(struct Transaction),SEEK_SET);
        if(offset==-1){
            perror("Error while seeking to required transaction record!");
            return false;
        }
        struct flock lock;
        lock.l_type=F_RDLCK;
        lock.l_whence=SEEK_SET;
        lock.l_start=customer.transaction[ptr]*sizeof(struct Transaction);
        lock.l_len=sizeof(struct Transaction);
        lock.l_pid=getpid();

        int lockingStatus=fcntl(transactionFD,F_SETLKW,&lock);
        if(lockingStatus==-1){
            perror("error on taking read lock");
            return false;
        }

        rBytes=read(transactionFD,&transaction,sizeof(struct Transaction));
        if(rBytes==-1){
            perror("error reading transaction record from file!");
            return false;
        }

        lock.l_type=F_UNLCK;
        fcntl(transactionFD,F_SETLK,&lock);

        // transactionTime=*localtime(&(transaction.transactionTime));


        bzero(tBuffer,sizeof(tBuffer));
        sprintf(tBuffer,"Details of transaction %d - \n\tDate : %s\tOperation : %s \n\tPrevious Balance : %f \n\tNew Balance : %f",(ptr + 1),ctime(&transaction.transactionTime),(transaction.operation?"Deposit":"Withdrawl"),transaction.oldBalance,transaction.newBalance);

        strcat(wBuffer, tBuffer);
        ptr++;
    }

    close(transactionFD);
        
    if(strlen(wBuffer)==0){
        write(connFD,"No transactions were performed on this account by the customer!",strlen("No transactions were performed on this account by the customer!"));
        read(connFD,rBuffer,sizeof(rBuffer)); 
        return false;
    }
    else{
        write(connFD,wBuffer,strlen(wBuffer));
        read(connFD,rBuffer,sizeof(rBuffer)); 
        return true;
    }   
    
}

bool change_password_employee(int connFD){
    int rBytes,wBytes;            
    char rBuffer[1000],wBuffer[1000];
    int ID=employee.id;
    wBytes=write(connFD,"enter your old password!",sizeof("enter your old password!"));
    if(wBytes==-1){
        perror("error in writing to client\n");
        return false;
    }
    rBytes=read(connFD,rBuffer,sizeof(rBuffer));
    if(rBytes==-1){
        perror("error in reading from client\n");
    }

    int fileFD=open("./EMPLOYEE/employee.txt",O_RDWR);
    if(fileFD==-1){
        perror("error in opening in file");
        return false;
    }

    int offset=lseek(fileFD,(ID)*sizeof(struct Employee),SEEK_SET);

    if(offset==-1){
        wBytes=write(connFD,"wrong username",sizeof("wrong username"));
        return false;
    }

    struct flock lock;
    lock.l_type=F_WRLCK;
    lock.l_whence=SEEK_SET;
    lock.l_start=(ID)*sizeof(struct Employee);
    lock.l_len=sizeof(struct Employee);
    lock.l_pid=getpid();

    int lock_check=fcntl(fileFD,F_SETLKW,&lock);
    if(lock_check==-1){
        perror("error in locking\n");
        return false;
    }
    int read_bytes=read(fileFD,&employee,sizeof(employee));

    char hashing[1000];
    strcpy(hashing,crypt(rBuffer,SALT));
    if(strcmp(hashing,employee.password)==0){
        wBytes=write(connFD,"enter new password!",sizeof("enter new password!"));
        if(wBytes==-1){
            perror("error in writing to client\n");
        }
        bzero(rBuffer,sizeof(rBuffer));
        rBytes=read(connFD,rBuffer,sizeof(rBuffer));
        if(rBytes==-1){
            perror("error in reading from client\n");
        }
    }
    else{
        perror("wrong password\n");
        close(fileFD);
        return false;
    }

    bzero(hashing,sizeof(hashing));
    strcpy(hashing,crypt(rBuffer,SALT));
    strcpy(employee.password,hashing);

    offset=lseek(fileFD,(ID)*sizeof(struct Employee),SEEK_SET);

    if(offset==-1){
        wBytes=write(connFD,"wrong username",sizeof("wrong username"));
        return false;
    }
    wBytes=write(fileFD,&employee,sizeof(employee));
    if(wBytes==-1){
        perror("error in writing to file\n");
    }
    lock.l_type=F_UNLCK;
    fcntl(fileFD,F_SETLK,&lock);
    close(fileFD);
    wBytes=write(connFD,"password updated!",sizeof("password updated!"));
    if(wBytes==-1){
        perror("error in writing to client\n");
    }
    read(connFD,rBuffer,sizeof(rBuffer));
    return false;
}

bool activate_deactivate_account(int connFD){
    int rBytes,wBytes;            
    char rBuffer[1000],wBuffer[1000];
    struct Customer customer;
    write(connFD,"enter account number want to activate/deactivate!",sizeof("enter account number want to activate/deactivate!"));
    read(connFD,rBuffer,sizeof(rBuffer));
    int ID=atoi(rBuffer)-1;
    int fileFD=open("./CUSTOMER/customer.txt",O_RDWR);
    if(fileFD==-1){
        perror("error in opening in file");
        return false;
    }
    int offset=lseek(fileFD,(ID)*sizeof(struct Customer),SEEK_SET);

    if(offset==-1){
        wBytes=write(connFD,"wrong username",sizeof("wrong username"));
        return false;
    }

    struct flock lock;
    lock.l_type=F_WRLCK;
    lock.l_whence=SEEK_SET;
    lock.l_start=(ID)*sizeof(struct Customer);
    lock.l_len=sizeof(struct Customer);
    lock.l_pid=getpid();

    int lock_check=fcntl(fileFD,F_SETLKW,&lock);
    if(lock_check==-1){
        perror("error in locking\n");
        return false;
    }
    int read_bytes=read(fileFD,&customer,sizeof(customer));
    bzero(wBuffer,sizeof(wBuffer));
    sprintf(wBuffer,"currently account is %s",(customer.active?"ACTIVE":"DEACTIVE"));
    if(customer.active==true){
        strcat(wBuffer,"\n");
        strcat(wBuffer,"1. Deactivate\n2. No change");
        write(connFD,wBuffer,sizeof(wBuffer));
        bzero(rBuffer,sizeof(rBuffer));
        read(connFD,rBuffer,sizeof(rBuffer));
        int choice=atoi(rBuffer);
        switch(choice){
            case 1:
                customer.active=false;
                break;
            default:
                break;
        }
    }else{
        strcat(wBuffer,"\n");
        strcat(wBuffer,"1. Activate\n2. No change");
        write(connFD,wBuffer,sizeof(wBuffer));
        bzero(rBuffer,sizeof(rBuffer));
        read(connFD,rBuffer,sizeof(rBuffer));
        int choice=atoi(rBuffer);
        switch(choice){
            case 1:
                customer.active=true;
                break;
            default:
                break;
        }
    }

    offset=lseek(fileFD,(ID)*sizeof(struct Customer),SEEK_SET);

    if(offset==-1){
        wBytes=write(connFD,"wrong username",sizeof("wrong username"));
        return false;
    }

    lock.l_type=F_WRLCK;
    lock.l_whence=SEEK_SET;
    lock.l_start=(ID)*sizeof(struct Customer);
    lock.l_len=sizeof(struct Customer);
    lock.l_pid=getpid();

    lock_check=fcntl(fileFD,F_SETLKW,&lock);
    if(lock_check==-1){
        perror("error in locking\n");
        return false;
    }
    write(fileFD,&customer,sizeof(customer));
    lock.l_type=F_UNLCK;
    fcntl(fileFD,F_SETLK,&lock);
    close(fileFD);

    return false;
}

bool assign_loan_to_employee(int connFD){
    int rBytes,wBytes;            
    char rBuffer[1000],wBuffer[1000],tBuffer[1000];
    struct Loan loan;
    int loanFD=open("./ACCOUNTS/loan.txt",O_RDWR);
    if(loanFD==-1){
        perror("error in opening file");
        return false;
    }
    struct flock lock;
    lock.l_type=F_RDLCK;
    lock.l_whence=SEEK_SET;
    lock.l_pid=getpid();

    int lock_status=fcntl(loanFD,F_SETLKW,&lock);
    if(lock_status==-1){
        perror("Write lock\n");
        close(loanFD);
        return false;
    }


    bzero(wBuffer,sizeof(wBuffer));
    while(1){
        rBytes=read(loanFD,&loan,sizeof(struct Loan));
        if(rBytes==0){
            break;
        }
        if(rBytes==-1){
            perror("Error while reading loan record from file!");
            return false;
        }
        bzero(tBuffer,sizeof(tBuffer));
        sprintf(tBuffer,"ID : %d \nAccount : %d \nAmmount : %f\nStatus: %s\n",loan.ID,loan.account,loan.ammount,(loan.isapprove?"Approved":"Not Approved"));
        strcat(wBuffer,tBuffer);
        
    }
    strcat(wBuffer,"\n");
    strcat(wBuffer,"enter loan ID!");
    write(connFD,wBuffer,sizeof(wBuffer));
    bzero(rBuffer,sizeof(rBuffer));
    read(connFD,rBuffer,sizeof(rBuffer));
    int ID=atoi(rBuffer);

    int offset=lseek(loanFD,(ID-1)*sizeof(struct Loan),SEEK_SET);
    if(offset==-1){
        wBytes=write(connFD,"wrong ID",sizeof("wrong ID"));
        read(connFD,rBuffer,sizeof(rBuffer));
        return false;
    }
    lock.l_type=F_WRLCK;
    lock.l_whence=SEEK_SET;
    lock.l_start=(ID-1)*sizeof(struct Loan);
    lock.l_len=sizeof(struct Loan);
    lock.l_pid=getpid();

    int lock_check=fcntl(loanFD,F_SETLKW,&lock);
    if(lock_check==-1){
        perror("error in locking\n");
        return false;
    }
    rBytes=read(loanFD,&loan,sizeof(loan));
    if(rBytes==-1){
        perror("error in reading file\n");
        return false;
    }
    if(loan.isassign==true){
        write(connFD,"loan already assigned!",sizeof("loan already assigned!"));
        read(connFD,rBuffer,sizeof(rBuffer));
        close(loanFD);
        return false;
    }

    int fileFD=open("./EMPLOYEE/employee.txt",O_RDWR);
    if(fileFD==-1){
        perror("error in opening file");
        return false;
    }
    write(connFD,"enter employee id you want to assign loan!",sizeof("enter employee id you want to assign loan!"));
    bzero(rBuffer,sizeof(rBuffer));
    read(connFD,rBuffer,sizeof(rBuffer));

    while(1){
    struct Employee temp_employee;
    int employee_ID=atoi(rBuffer);
    int offset=lseek(fileFD,(employee_ID)*sizeof(struct Employee),SEEK_SET);
    if(offset==-1){
        wBytes=write(connFD,"wrong ID",sizeof("wrong ID"));
        read(connFD,rBuffer,sizeof(rBuffer));
        return false;
    }
    lock.l_type=F_WRLCK;
    lock.l_whence=SEEK_SET;
    lock.l_start=(employee_ID)*sizeof(struct Employee);
    lock.l_len=sizeof(struct Employee);
    lock.l_pid=getpid();

    lock_check=fcntl(fileFD,F_SETLKW,&lock);
    if(lock_check==-1){
        perror("error in locking\n");
        return false;
    }
    rBytes=read(fileFD,&temp_employee,sizeof(temp_employee));
    if(rBytes==-1){
        perror("error in reading file");
        return false;
    }
    if(temp_employee.ismanager==true){
        write(connFD,"can't assign loan employee is a manager!",sizeof("can't assign loan employee is a manager!"));
        read(connFD,rBuffer,sizeof(rBuffer));
        close(fileFD);
        return false;
    }
    int ptr=0;
    while(temp_employee.loan[ptr]!=-1 && ptr<10){
        ptr++;
    }
    if(ptr>=10){
        bzero(wBuffer,sizeof(wBuffer));
        strcpy(wBuffer,"can't assign loan already too many loan assign to employee");
        strcat(wBuffer,"\n");
        strcat(wBuffer,"enter new employee ID!");
        write(connFD,wBuffer,sizeof(wBuffer));
        bzero(rBuffer,sizeof(rBuffer));
        read(connFD,rBuffer,sizeof(rBuffer));
        continue;
    }else{
        temp_employee.loan[ptr]=ID;
        loan.isassign=true;
    }
    offset=lseek(fileFD,(employee_ID)*sizeof(struct Employee),SEEK_SET);
    write(fileFD,&temp_employee,sizeof(temp_employee));
    lock.l_type=F_UNLCK;
    fcntl(fileFD,F_SETLK,&lock);
    break;
    }
    close(fileFD);

    lseek(loanFD,(ID-1)*sizeof(struct Loan),SEEK_SET);
    wBytes=write(loanFD,&loan,sizeof(loan));
    if(wBytes==-1){
        perror("error in writeing to file\n");
        return false;
    }
    lock.l_type=F_UNLCK;
    fcntl(loanFD,F_SETLK,&lock);
    close(loanFD);
    write(connFD,"loan assign successfully",sizeof("loan assign successfully"));
    read(connFD,rBuffer,sizeof(rBuffer));
    return false;
}

bool view_assign_loan_application(int connFD){           
    char rBuffer[1000],wBuffer[1000];
    char tBuffer[1000];
    struct Loan loan;
    int loanFD=open("./ACCOUNTS/loan.txt",O_RDWR);
    if(loanFD==-1){
        perror("error in opening file");
        return false;
    }
    bzero(wBuffer,sizeof(wBuffer));
    struct flock lock;
    for(int i=0;i<10;i++){
        if(employee.loan[i]!=-1){
            int ID=employee.loan[i];
            lseek(loanFD,(ID-1)*sizeof(struct Loan),SEEK_SET);
            lock.l_type=F_RDLCK;
            lock.l_whence=SEEK_SET;
            lock.l_start=(ID-1)*sizeof(struct Loan);
            lock.l_len=sizeof(struct Loan);
            lock.l_pid=getpid();

            int lock_check=fcntl(loanFD,F_SETLKW,&lock);
            if(lock_check==-1){
                perror("error in locking\n");
                return false;
            }
            int read_bytes=read(loanFD,&loan,sizeof(loan));
            if(read_bytes==-1){
                perror("error in reading file\n");
                return false;
            }
            bzero(tBuffer,sizeof(tBuffer));
            sprintf(tBuffer,"Assigned loan details: \nLoan ID : %d Account : %d Ammount : %f\n",loan.ID,loan.account,loan.ammount);
            strcat(wBuffer,tBuffer);
        }   
    }
    lock.l_type=F_UNLCK;
    fcntl(loanFD,F_SETLK,&lock);
    close(loanFD);

    if(strlen(wBuffer)==0){
        write(connFD,"No loan assigned!",strlen("No loan assigned!"));
        read(connFD,rBuffer,sizeof(rBuffer)); 
        return false;
    }
    else{
        write(connFD,wBuffer,strlen(wBuffer));
        read(connFD,rBuffer,sizeof(rBuffer)); 
        return true;
    }   

}

bool approved_reject_loan(int connFD){
    view_assign_loan_application(connFD);
    char rBuffer[1000],wBuffer[1000];
    write(connFD,"enter loan ID!",sizeof("enter loan ID!"));
    read(connFD,rBuffer,sizeof(rBuffer));
    int ID=atoi(rBuffer);
    struct Loan loan;
    int loanFD=open("./ACCOUNTS/loan.txt",O_RDWR);
    if(loanFD==-1){
        perror("error in opening file");
        return false;
    }
    int offset=lseek(loanFD,(ID-1)*sizeof(struct Loan),SEEK_SET);
    if(offset==-1){
        write(connFD,"wrong ID",sizeof("wrong ID"));
        read(connFD,rBuffer,sizeof(rBuffer));
        return false;
    }
    struct flock lock;
    lock.l_type=F_WRLCK;
    lock.l_whence=SEEK_SET;
    lock.l_start=(ID-1)*sizeof(struct Loan);
    lock.l_len=sizeof(struct Loan);
    lock.l_pid=getpid();

    int lock_check=fcntl(loanFD,F_SETLKW,&lock);
    if(lock_check==-1){
        perror("error in locking\n");
        return false;
    }
    int rBytes=read(loanFD,&loan,sizeof(loan));
    if(rBytes==-1){
        perror("error in reading file\n");
        return false;
    }
    bzero(wBuffer,sizeof(wBuffer));
    sprintf(wBuffer,"1. APPROVE LOAN\n2. REJECT LOAN");
    write(connFD,wBuffer,sizeof(wBuffer));
    bzero(rBuffer,sizeof(rBuffer));
    read(connFD,rBuffer,sizeof(rBuffer));
    int choice=atoi(rBuffer);
    switch(choice){
        case 1:
            loan.isapprove=true;
            break;
        case 2:
            loan.isapprove=false;
            break;
    }
    int wBytes=write(loanFD,&loan,sizeof(loan));
    if(wBytes==-1){
        perror("error in writing file\n");
        return false;
    }
    lock.l_type=F_UNLCK;
    fcntl(loanFD,F_SETLK,&lock);
    close(loanFD);

    int fileFD=open("./EMPLOYEE/employee.txt",O_RDWR);
    if(fileFD==-1){
        perror("error in opening in file");
        return false;
    }
    int employee_ID=employee.id;
    offset=lseek(fileFD,(employee_ID)*sizeof(struct Employee),SEEK_SET);

    if(offset==-1){
        wBytes=write(connFD,"wrong username",sizeof("wrong username"));
        return false;
    }

    lock.l_type=F_WRLCK;
    lock.l_whence=SEEK_SET;
    lock.l_start=((employee_ID))*sizeof(struct Employee);
    lock.l_len=sizeof(struct Employee);
    lock.l_pid=getpid();

    lock_check=fcntl(fileFD,F_SETLKW,&lock);
    if(lock_check==-1){
        perror("error in locking\n");
        return false;
    }
    for(int i=0;i<10;i++){
        if(employee.loan[i]==ID){
            employee.loan[i]=-1;
            break;
        }
    }
    wBytes=write(fileFD,&employee,sizeof(employee));
    if(wBytes==-1){
        perror("error in writing to file");
        return false;
    }
    lock.l_type=F_UNLCK;
    fcntl(fileFD,F_SETLK,&lock);
    close(fileFD);
    return false;
}

bool review_feedback(int connFD){
    int rBytes,wBytes;
    char rBuffer[1000],wBuffer[1000],tBuffer[1000];
    struct Feedback feedback;
    int feedbackFD=open("./ACCOUNTS/feedback.txt",O_RDWR);
    if(feedbackFD==-1){
        perror("error in opening file");
        return false;
    }
    int ptr=0;
    bzero(wBuffer,sizeof(wBuffer));
    while(1){
        rBytes=read(feedbackFD,&feedback,sizeof(struct Feedback));
        if(rBytes==0){
            break;
        }
        if(rBytes==-1){
            perror("Error while reading feedback record from file!");
            return false;
        }
        if(feedback.isreview==false){
            ptr++;
            bzero(tBuffer,sizeof(tBuffer));
            sprintf(tBuffer,"ID : %d \nFeedback : %s \nStatus : %s\n",feedback.ID,feedback.buffer,(feedback.isreview?"Reviewed":"Not Reviewed"));
            strcat(wBuffer,tBuffer);
        }
    }
    if(ptr==0){
        write(connFD,"no pending feedback",sizeof("no pending feedback"));
        read(connFD,rBuffer,sizeof(rBuffer));
        return false;
    }
    strcat(wBuffer,"\n");
    strcat(wBuffer,"enter feedback ID to review");
    write(connFD,wBuffer,sizeof(wBuffer));
    bzero(rBuffer,sizeof(rBuffer));
    read(connFD,rBuffer,sizeof(rBuffer));
    int temp=atoi(rBuffer);
    int offset=lseek(feedbackFD,temp*sizeof(struct Feedback),SEEK_SET);
    if(offset==-1){
        perror("wrong ID!");
        return false;
    }
    read(feedbackFD,&feedback,sizeof(struct Feedback));
    feedback.isreview=true;
    offset=lseek(feedbackFD,temp*sizeof(struct Feedback),SEEK_SET);
    if(offset==-1){
        perror("wrong ID!");
        return false;
    }
    write(feedbackFD,&feedback,sizeof(feedback));
    close(feedbackFD);
    return false;   
}

bool lock_critical_section(struct sembuf *semOp){
    semOp->sem_flg=SEM_UNDO;
    semOp->sem_op=-1;
    semOp->sem_num=0;
    int semopStatus=semop(semId,semOp,1);
    if(semopStatus == -1){
        perror("Error while locking critical section");
        return false;
    }
    return true;
}

bool unlock_critical_section(struct sembuf *semOp){
    semOp->sem_op=1;
    int semopStatus=semop(semId,semOp,1);
    if(semopStatus == -1){
        perror("Error while operating on semaphore!");
        _exit(1);
    }
    return true;
}



#endif
