#ifndef CUSTOMER_FUNCTIONS
#define CUSTOMER_FUNCTIONS

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
#include"../CUSTOMER/customer_struct.h"
#include"../ACCOUNTS/transaction_struct.h"
#include"../ACCOUNTS/loan_struct.h"
#include"../ACCOUNTS/feedback_struct.h"
#define SALT "34"

struct Customer customer;
int semId;

bool login_customer(int connFD);
bool view_balance(int connFD);
bool deposit(int connFD);
bool withdrawl(int connFD);
bool fund_transfer(int connFD);
bool change_password(int connFD);
void transaction_customer(int *array,int ID);
int transaction_file(int account,float oldbalance,float newbalance,bool operation);
bool get_transaction_details(int connFD);
bool apply_for_loan(int connFD);
bool add_feedback(int connFD);
bool lock_critical_section(struct sembuf *semOp);
bool unlock_critical_section(struct sembuf *sem_op);


bool customer_operation(int connFD){
    if (login_customer(connFD)){
        int wBytes,rBytes;            
        char rBuffer[1000], wBuffer[1000]; 
        int semKey=ftok("./CUSTOMER/customer.txt",customer.account); 
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
        strcpy(wBuffer,"Welcome customer!");
        while(1){
            strcat(wBuffer,"\n");
            strcat(wBuffer,"1. view balance\n2. deposit money\n3. withdrawl money\n4. transfer fund\n5. apply for loan\n6. change password\n7. adding feedback\n8. view transaction history\n9. logout");
            wBytes=write(connFD,wBuffer,strlen(wBuffer));
            if(wBytes==-1){
                perror("Error while writing CUSTOMER_MENU to client!");
                return false;
            }
            bzero(wBuffer,sizeof(wBuffer));
            rBytes=read(connFD,rBuffer,sizeof(rBuffer));
            if(rBytes==-1){
                perror("Error while reading client's choice for CUSTOMER_MENU");
                return false;
            }
            int choice=atoi(rBuffer);
            switch(choice){
            case 1:
                view_balance(connFD);
                break;
            case 2:
                deposit(connFD);
                break;
            case 3:
                withdrawl(connFD);
                break;
            case 4:
                fund_transfer(connFD);
                break;
            case 5:
                apply_for_loan(connFD);
                break;
            case 6:
                change_password(connFD);
                break;
            case 7:
                add_feedback(connFD);
                break;
            case 8:
                get_transaction_details(connFD);
                break;
            default:
                wBytes=write(connFD,"Logging out\n",strlen("Logging out\n"));
                unlock_critical_section(&semOp);
                return false;
                break;
            }
        }
    }
    else
    {
        return false;
    }
    return true;
}

bool login_customer(int connFD){
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
    int fileFD=open("./CUSTOMER/customer.txt",O_RDONLY);
    if(fileFD==-1){
        perror("error in opening in file");
        return false;
    }
    int offset=lseek(fileFD,(ID-1)*sizeof(struct Customer),SEEK_SET);

    if(offset==-1){
        wBytes=write(connFD,"wrong username",sizeof("wrong username"));
        read(connFD,rBuffer,sizeof(rBuffer));
        return false;
    }

    struct flock lock;
    lock.l_type=F_RDLCK;
    lock.l_whence=SEEK_SET;
    lock.l_start=(ID-1)*sizeof(struct Customer);
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

    if(customer.active==false){
        write(connFD,"You'r account has been deactivated!",sizeof("You'r account has been deactivated!"));
        read(connFD,rBuffer,sizeof(rBuffer));
        return false;
    }

    bool userFound=false;
    if(strcmp(customer.username,rBuffer)==0)
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
        if (strcmp(hashing,customer.password)==0)
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

bool view_balance(int connFD){
    int rBytes,wBytes;            
    char rBuffer[1000],wBuffer[1000];
    int ID=customer.account-1;
    int fileFD=open("./CUSTOMER/customer.txt",O_RDONLY);
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

    sprintf(wBuffer,"account balance : %f",customer.balance);
    wBytes=write(connFD,wBuffer,sizeof(wBuffer));
    if(wBytes==-1){
        perror("error in writing to client!");
    }
    read(connFD,rBuffer,sizeof(rBuffer));
    return false;

}

bool deposit(int connFD){
    int rBytes,wBytes;            
    char rBuffer[1000],wBuffer[1000];
    int ID=customer.account-1;
    wBytes=write(connFD,"enter ammount to be deposit",sizeof("enter ammount to be deposit"));
    if(wBytes==-1){
        perror("error in writing to client\n");
        return false;
    }
    rBytes=read(connFD,rBuffer,sizeof(rBuffer));
    if(rBytes==-1){
        perror("error in reading from client\n");
    }

    float ammount=atof(rBuffer);

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

    int transactionID=transaction_file(customer.account,customer.balance,customer.balance+ammount,1);
    transaction_customer(customer.transaction,transactionID);

    float new_balance=customer.balance+ammount;
    customer.balance=new_balance;
    offset=lseek(fileFD,(ID)*sizeof(struct Customer),SEEK_SET);

    if(offset==-1){
        wBytes=write(connFD,"wrong username",sizeof("wrong username"));
        return false;
    }
    wBytes=write(fileFD,&customer,sizeof(customer));
    if(wBytes==-1){
        perror("error in writing to file\n");
    }
    lock.l_type=F_UNLCK;
    fcntl(fileFD,F_SETLK,&lock);
    close(fileFD);
    view_balance(connFD);
    return false;
}

bool withdrawl(int connFD){
    int rBytes,wBytes;            
    char rBuffer[1000],wBuffer[1000];
    int ID=customer.account-1;
    wBytes=write(connFD,"enter ammount to be withdrawl",sizeof("enter ammount to be withdrawl"));
    if(wBytes==-1){
        perror("error in writing to client\n");
        return false;
    }
    rBytes=read(connFD,rBuffer,sizeof(rBuffer));
    if(rBytes==-1){
        perror("error in reading from client\n");
    }

    float ammount=atof(rBuffer);

    int fileFD=open("./CUSTOMER/customer.txt",O_RDWR);
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

    int transactionID=transaction_file(customer.account,customer.balance,customer.balance-ammount,0);
    transaction_customer(customer.transaction,transactionID);    

    float new_balance=customer.balance-ammount;
    if(new_balance<=0){
        write(connFD,"Insufficient Balanace can't withdrawl!",sizeof("Insufficient Balanace can't withdrawl!"));
        read(connFD,rBuffer,sizeof(rBuffer));
        return false;
    }
    customer.balance=new_balance;
    offset=lseek(fileFD,(ID)*sizeof(struct Customer),SEEK_SET);

    if(offset==-1){
        wBytes=write(connFD,"wrong username",sizeof("wrong username"));
        return false;
    }
    wBytes=write(fileFD,&customer,sizeof(customer));
    if(wBytes==-1){
        perror("error in writing to file\n");
    }
    lock.l_type=F_UNLCK;
    fcntl(fileFD,F_SETLK,&lock);
    close(fileFD);
    view_balance(connFD);
    return false;
}

bool fund_transfer(int connFD){
    int rBytes,wBytes;            
    char rBuffer[1000],wBuffer[1000];
    int ID=customer.account-1;
    wBytes=write(connFD,"enter ammount to be transfer",sizeof("enter ammount to be trasfer"));
    if(wBytes==-1){
        perror("error in writing to client\n");
        return false;
    }
    rBytes=read(connFD,rBuffer,sizeof(rBuffer));
    if(rBytes==-1){
        perror("error in reading from client\n");
    }

    float ammount=atof(rBuffer);

    bzero(rBuffer,sizeof(rBuffer));

    wBytes=write(connFD,"enter account number for fund transfer",sizeof("enter account number for fund transfer"));
    if(wBytes==-1){
        perror("error in writing to client\n");
        return false;
    }
    rBytes=read(connFD,rBuffer,sizeof(rBuffer));
    if(rBytes==-1){
        perror("error in reading from client\n");
    }
    int account_number=atoi(rBuffer);

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
    rBytes=read(fileFD,&customer,sizeof(customer));

    int transactionID=transaction_file(customer.account,customer.balance,customer.balance-ammount,0);
    transaction_customer(customer.transaction,transactionID);

    float new_balance=customer.balance-ammount;
    if(new_balance<=0){
        write(connFD,"Insufficient Balanace can't withdrawl!",sizeof("Insufficient Balanace can't withdrawl!"));
        read(connFD,rBuffer,sizeof(rBuffer));
        return false;
    }
    customer.balance=new_balance;
    offset=lseek(fileFD,(ID)*sizeof(struct Customer),SEEK_SET);

    if(offset==-1){
        wBytes=write(connFD,"wrong username",sizeof("wrong username"));
        return false;
    }
    wBytes=write(fileFD,&customer,sizeof(customer));
    if(wBytes==-1){
        perror("error in writing to file\n");
    }
    lock.l_type=F_UNLCK;
    fcntl(fileFD,F_SETLK,&lock);
    close(fileFD);

    ID=account_number-1;
    struct Customer temp_customer;
    fileFD=open("./CUSTOMER/customer.txt",O_RDWR);
    if(fileFD==-1){
        perror("error in opening in file");
        return false;
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
    rBytes=read(fileFD,&temp_customer,sizeof(temp_customer));

    transactionID=transaction_file(temp_customer.account,temp_customer.balance,temp_customer.balance+ammount,1);
    transaction_customer(temp_customer.transaction,transactionID);

    new_balance=temp_customer.balance+ammount;
    temp_customer.balance=new_balance;
    offset=lseek(fileFD,(ID)*sizeof(struct Customer),SEEK_SET);

    if(offset==-1){
        wBytes=write(connFD,"wrong username",sizeof("wrong username"));
        return false;
    }
    wBytes=write(fileFD,&temp_customer,sizeof(temp_customer));
    if(wBytes==-1){
        perror("error in writing to file\n");
    }
    lock.l_type=F_UNLCK;
    fcntl(fileFD,F_SETLK,&lock);
    close(fileFD);

    view_balance(connFD);

    return false;
}

bool change_password(int connFD){
    int rBytes,wBytes;            
    char rBuffer[1000],wBuffer[1000];
    int ID=customer.account-1;
    wBytes=write(connFD,"enter your old password!",sizeof("enter your old password!"));
    if(wBytes==-1){
        perror("error in writing to client\n");
        return false;
    }
    rBytes=read(connFD,rBuffer,sizeof(rBuffer));
    if(rBytes==-1){
        perror("error in reading from client\n");
    }

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

    char hashing[1000];
    strcpy(hashing,crypt(rBuffer,SALT));
    if(strcmp(hashing,customer.password)==0){
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
    strcpy(customer.password,hashing);

    offset=lseek(fileFD,(ID)*sizeof(struct Customer),SEEK_SET);

    if(offset==-1){
        wBytes=write(connFD,"wrong username",sizeof("wrong username"));
        return false;
    }
    wBytes=write(fileFD,&customer,sizeof(customer));
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

void transaction_customer(int *array,int ID){
    int ptr=0;
    while(array[ptr]!=-1 && ptr<10){
        ptr++;
    }
    if(ptr>=10){
        for(int i=1;i<10;i++){
            array[i-1]=array[i];
        }
        array[ptr-1]=ID;
    }else{
        array[ptr]=ID;
    }
}

int transaction_file(int account,float oldbalance,float newbalance,bool operation){
    struct Transaction new_transaction;
    new_transaction.accountNumber=account;
    new_transaction.oldBalance=oldbalance;
    new_transaction.newBalance=newbalance;
    new_transaction.operation=operation;
    new_transaction.transactionTime=time(NULL);
    int rBytes,wBytes;
    int transactionFD=open("./ACCOUNTS/transaction.txt",O_CREAT|O_APPEND|O_RDWR,S_IRWXU);
    int offset=lseek(transactionFD,-sizeof(struct Transaction),SEEK_END);
    if(offset>=0){
        struct Transaction prev_transaction;
        rBytes=read(transactionFD,&prev_transaction,sizeof(struct Transaction));
        new_transaction.transactionID=prev_transaction.transactionID+1;
    }else{
        new_transaction.transactionID=0;
    }
    wBytes=write(transactionFD,&new_transaction,sizeof(struct Transaction));

    return new_transaction.transactionID;
}

bool get_transaction_details(int connFD){
    int rBytes, wBytes;                              
    char rBuffer[1000],wBuffer[10000],tBuffer[1000]; 
    
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

bool apply_for_loan(int connFD){
    int rBytes,wBytes;            
    char rBuffer[1000],wBuffer[1000];
    if(customer.loanID!=-1){
        write(connFD,"you have already applied for loan!",sizeof("you have already applied for loan!"));
        read(connFD,rBuffer,sizeof(rBuffer));
        return false;
    }
    struct Loan new_loan,prev_loan;

    new_loan.account=customer.account;
    write(connFD,"enter loan ammount",sizeof("enter loan ammount"));
    bzero(rBuffer,sizeof(rBuffer));
    read(connFD,rBuffer,sizeof(rBuffer));
    new_loan.ammount=atoi(rBuffer);
    new_loan.isapprove=false;
    new_loan.isassign=false;

    int loanFD=open("./ACCOUNTS/loan.txt",O_RDONLY);
    if(loanFD==-1 && errno==ENOENT){
        new_loan.ID=1;
    }else if(loanFD==-1){
        perror("error in opening file\n");
        return false;
    }else{
        int offset=lseek(loanFD,-sizeof(struct Loan),SEEK_END);
        if(offset==-1){
            perror("Error seeking to last loan record!");
            return false;
        }

        struct flock lock;
        lock.l_type=F_RDLCK;
        lock.l_whence=SEEK_SET;
        lock.l_start=offset;
        lock.l_len=sizeof(struct Loan);
        lock.l_pid=getpid();

        int lockingStatus=fcntl(loanFD,F_SETLKW,&lock);
        if(lockingStatus==-1){
            perror("Error obtaining read lock on loan record!");
            return false;
        }

        rBytes=read(loanFD,&prev_loan,sizeof(struct Loan));
        if(rBytes==-1){
        perror("Error while reading loan record from file!");
        return false;
        }

        lock.l_type = F_UNLCK;
        fcntl(loanFD,F_SETLK,&lock);
        close(loanFD);
        new_loan.ID=prev_loan.ID+1;
    }

    loanFD=open("./ACCOUNTS/loan.txt",O_CREAT|O_APPEND|O_WRONLY,S_IRWXU);
    if(loanFD==-1){
        perror("Error while creating opening loan file!");
        return false;
    }

    struct flock lock;
    lock.l_type=F_WRLCK;
    lock.l_whence=SEEK_END;
    lock.l_start=0;
    lock.l_len=0;
    lock.l_pid=getpid();

    int lock_status=fcntl(loanFD,F_SETLKW,&lock);
    if(lock_status==-1){
        perror("Write lock on loan file");
        close(loanFD);
        return -1;
    }

    wBytes=write(loanFD,&new_loan,sizeof(struct Loan));
    if(wBytes==-1){
        perror("Error while reading loan record from file!");
        return false;
    }

    close(loanFD);

    int fileFD=open("./CUSTOMER/customer.txt",O_WRONLY);
    if(fileFD==-1){
        perror("error in opening in file");
        return false;
    }

    int offset=lseek(fileFD,(customer.account-1)*sizeof(struct Customer),SEEK_SET);

    if(offset==-1){
        wBytes=write(connFD,"wrong username",sizeof("wrong username"));
        read(connFD,rBuffer,sizeof(rBuffer));
        return false;
    }

    lock.l_type=F_WRLCK;
    lock.l_whence=SEEK_SET;
    lock.l_start=(customer.account-1)*sizeof(struct Customer);
    lock.l_len=sizeof(struct Customer);
    lock.l_pid=getpid();

    int lock_check=fcntl(fileFD,F_SETLKW,&lock);
    if(lock_check==-1){
        perror("error in locking\n");
        return false;
    }
    customer.loanID=new_loan.ID;
    write(fileFD,&customer,sizeof(customer));
    lock.l_type=F_UNLCK;
    fcntl(fileFD,F_SETLK,&lock);
    close(fileFD);
    write(connFD,"you have successfully applied for loan!",sizeof("you have successfully applied for loan!"));
    read(connFD,rBuffer,sizeof(rBuffer));

    return false;

}

bool add_feedback(int connFD){
    int rBytes,wBytes;
    char rBuffer[1000],wBuffer[1000];
    struct Feedback new_feedback,prev_feedback;

    bzero(rBuffer,sizeof(rBuffer));
    write(connFD,"write you feedback!",sizeof("write you feedback!"));
    read(connFD,rBuffer,sizeof(rBuffer));
    strcpy(new_feedback.buffer,rBuffer);

    new_feedback.isreview=false;

    int feedbackFD=open("./ACCOUNTS/feedback.txt",O_RDONLY);
    if(feedbackFD==-1 && errno==ENOENT){
        new_feedback.ID=0;
    }else if(feedbackFD==-1){
        perror("error in opening file\n");
        return false;
    }else{
        int offset=lseek(feedbackFD,-sizeof(struct Feedback),SEEK_END);
        if(offset==-1){
            perror("Error seeking to last feedback record!");
            return false;
        }

        struct flock lock;
        lock.l_type=F_RDLCK;
        lock.l_whence=SEEK_SET;
        lock.l_start=offset;
        lock.l_len=sizeof(struct Feedback);
        lock.l_pid=getpid();

        int lockingStatus=fcntl(feedbackFD,F_SETLKW,&lock);
        if(lockingStatus==-1){
            perror("Error obtaining read lock on feedback record!");
            return false;
        }

        rBytes=read(feedbackFD,&prev_feedback,sizeof(struct Feedback));
        if(rBytes==-1){
            perror("Error while reading feedback record from file!");
            return false;
        }

        lock.l_type = F_UNLCK;
        fcntl(feedbackFD,F_SETLK,&lock);
        close(feedbackFD);
        new_feedback.ID=prev_feedback.ID+1;
    }
    
    feedbackFD=open("./ACCOUNTS/feedback.txt",O_CREAT|O_APPEND|O_WRONLY,S_IRWXU);
    if(feedbackFD==-1){
        perror("Error while creating opening feedback file!");
        return false;
    }

    struct flock lock;
    lock.l_type=F_WRLCK;
    lock.l_whence=SEEK_END;
    lock.l_start=0;
    lock.l_len=0;
    lock.l_pid=getpid();

    int lock_status=fcntl(feedbackFD,F_SETLKW,&lock);
    if(lock_status==-1){
        perror("Write lock on feedback file");
        close(feedbackFD);
        return -1;
    }

    wBytes=write(feedbackFD,&new_feedback,sizeof(new_feedback));
    if(wBytes==-1){
        perror("Error while writing feedback record to file!");
        return false;
    }

    close(feedbackFD);
    return false;

}


#endif