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
#include"../CUSTOMER/customer_struct.h"
#define SALT "34"
struct Customer customer;

bool login_customer(int connFD);
bool view_balance(int connFD);
bool deposit(int connFD);
bool withdrawl(int connFD);
bool fund_transfer(int connFD);
bool change_password(int connFD);


bool customer_operation(int connFD){
    if (login_customer(connFD)){
        int wBytes,rBytes;            
        char rBuffer[1000], wBuffer[1000]; 
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
                break;
            case 6:
                change_password(connFD);
                break;
            case 7:
                break;
            case 8:
                break;
            case 9:
                wBytes=write(connFD,"Logging out",strlen("Logging out"));
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

    bool userFound;
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

    float new_balance=customer.balance-ammount;
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

    float new_balance=customer.balance-ammount;
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

    ID=account_number-1;
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
    rBytes=read(fileFD,&customer,sizeof(customer));

    new_balance=customer.balance+ammount;
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
    }

    bzero(hashing,sizeof(hashing));
    strcpy(hashing,crypt(rBuffer,SALT));
    strcpy(customer.password,hashing);

    offset=lseek(fileFD,(ID)*sizeof(struct Customer),SEEK_SET);
    //

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

#endif