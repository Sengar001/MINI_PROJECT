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
#include"../CUSTOMER/customer_struct.h"

#define SALT "34"

bool login_admin(int connFD);
int add_employee(int connFD);
bool modify_employee(int connFD);
bool modify_customer(int connFD);
bool change_password_admin(int connFD);

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
                    modify_customer(connFD);
                }else{
                    modify_employee(connFD);
                }
                break;
            case 3:
                //get_Joint_account_details(connFD, NULL);
                break;
            case 4:
                change_password_admin(connFD);
                break;
            default:
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

    if(offset==-1){
        wBytes=write(connFD,"wrong username",sizeof("wrong username"));
        return false;
    }

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
        read(connFD,rBuffer,sizeof(rBuffer));
    }
    else
    {
        bzero(wBuffer, sizeof(wBuffer));
        wBytes = write(connFD,"The login ID specified doesn't exist!",strlen("The login ID specified doesn't exist!"));
        read(connFD,rBuffer,sizeof(rBuffer));
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

    //role
    write(connFD,"Enter role of Employee\nM:MANAGER\nE:EMPLOYEE",sizeof("Enter role of Employee\n0:MANAGER\n1:EMPLOYEE"));
    bzero(rBuffer,sizeof(rBuffer));
    read(connFD,rBuffer,sizeof(rBuffer));
    if(rBuffer[0]=='M')
        new_employee.ismanager=true;
    else
        new_employee.ismanager=false;

    //loan
    for(int i=0;i<10;i++){
        new_employee.loan[i]=-1;
    }

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
    int fileFD=open("./EMPLOYEE/employee.txt",O_RDONLY);
    if(fileFD==-1){
        perror("error in opening in file");
        return false;
    }
    int offset=lseek(fileFD,ID*sizeof(struct Employee),SEEK_SET);

    if(offset==-1){
        wBytes=write(connFD,"wrong ID",sizeof("wrong ID"));
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
    rBytes=read(fileFD,&employee,sizeof(employee)); 
    if(rBytes==-1){
        perror("error in reading from file!");
    }

    lock.l_type=F_UNLCK;
    fcntl(fileFD,F_SETLK,&lock);
    close(fileFD);

    bzero(wBuffer,sizeof(wBuffer));
    sprintf(wBuffer,"current details of employee:\nID : %d\nname : %s\nage : %d\ngender : %c\nusername : %s\npress YES for modification",employee.id,employee.name,employee.age,employee.gender,employee.username);
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
                strcpy(employee.name,rBuffer);

                bzero(wBuffer,sizeof(wBuffer));
                strcpy(employee.username,employee.name);
                strcat(employee.username,"-");
                sprintf(wBuffer,"%d",employee.id);
                strcat(employee.username,wBuffer);
                break;
            case 2:
                write(connFD,"enter new age",sizeof("enter new age"));
                bzero(rBuffer,sizeof(rBuffer));
                read(connFD,rBuffer,sizeof(rBuffer));
                int age=atoi(rBuffer);
                employee.age=age;
                break;
            case 3:
                write(connFD,"enter gender\nMALE M\n FEMALE F\nOTHER O",sizeof("enter gender\nMALE M\n FEMALE F\nOTHER O"));
                bzero(rBuffer,sizeof(rBuffer));
                read(connFD,rBuffer,sizeof(rBuffer));
                char gen=rBuffer[0];
                employee.gender=gen;
                break;
            default:
                count++;
                break;
        }
        if(count)
            break;
    }

    fileFD=open("./EMPLOYEE/employee.txt",O_WRONLY);
    if(fileFD==-1){
        perror("error in opening in file");
        return false;
    }
    offset=lseek(fileFD,ID*sizeof(struct Employee),SEEK_SET);

    if(offset==-1){
        wBytes=write(connFD,"wrong ID",sizeof("wrong ID"));
        return false;
    }

    lock.l_type=F_WRLCK;
    lock.l_whence=SEEK_SET;
    lock.l_start=ID*sizeof(struct Employee);
    lock.l_len=sizeof(struct Employee);
    lock.l_pid=getpid(); 

    lock_check=fcntl(fileFD,F_SETLKW,&lock);
    if(lock_check==-1){
        perror("error in locking\n");
        return false;
    }
    wBytes=write(fileFD,&employee,sizeof(employee)); 
    if(wBytes==-1){
        perror("error in writing to file!");
    }

    lock.l_type=F_UNLCK;
    fcntl(fileFD,F_SETLK,&lock);
    close(fileFD);

    bzero(wBuffer,sizeof(wBuffer));
    sprintf(wBuffer,"new details of employee:\nID : %d\nname : %s\nage : %d\ngender : %c\nusername : %s",employee.id,employee.name,employee.age,employee.gender,employee.username);
    write(connFD,wBuffer,sizeof(wBuffer));
    read(connFD,rBuffer,sizeof(rBuffer));

    return true;

}

bool modify_customer(int connFD){
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

bool change_password_admin(int connFD){
    int rBytes,wBytes;            
    char rBuffer[1000],wBuffer[1000];
    struct Admin admin;
    int ID=0;
    wBytes=write(connFD,"enter your old password!",sizeof("enter your old password!"));
    if(wBytes==-1){
        perror("error in writing to client\n");
        return false;
    }
    rBytes=read(connFD,rBuffer,sizeof(rBuffer));
    if(rBytes==-1){
        perror("error in reading from client\n");
    }

    int fileFD=open("./ADMIN/admin.txt",O_RDWR);
    if(fileFD==-1){
        perror("error in opening in file");
        return false;
    }

    int offset=lseek(fileFD,(ID)*sizeof(struct Admin),SEEK_SET);

    if(offset==-1){
        wBytes=write(connFD,"wrong username",sizeof("wrong username"));
        return false;
    }

    struct flock lock;
    lock.l_type=F_WRLCK;
    lock.l_whence=SEEK_SET;
    lock.l_start=(ID)*sizeof(struct Admin);
    lock.l_len=sizeof(struct Admin);
    lock.l_pid=getpid();

    int lock_check=fcntl(fileFD,F_SETLKW,&lock);
    if(lock_check==-1){
        perror("error in locking\n");
        return false;
    }
    int read_bytes=read(fileFD,&admin,sizeof(admin));

    char hashing[1000];
    strcpy(hashing,crypt(rBuffer,SALT));
    if(strcmp(hashing,admin.password)==0){
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
    strcpy(admin.password,hashing);

    offset=lseek(fileFD,(ID)*sizeof(struct Admin),SEEK_SET);

    if(offset==-1){
        wBytes=write(connFD,"wrong username",sizeof("wrong username"));
        return false;
    }
    wBytes=write(fileFD,&admin,sizeof(admin));
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