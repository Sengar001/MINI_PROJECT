#include<sys/types.h>
#include<sys/socket.h>
#include<fcntl.h>
#include<netinet/in.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include"./FUNCTION/admin.h"
#include"./FUNCTION/employee.h"
#include"./FUNCTION/customer.h"

void connection(int fd){
    printf("client server connection successful\n");
    char rbuff[1000];
    int choice;
    int wrbytes=write(fd,"Select User for Login\n1.Admin\n2.Manager\n3.Employee\n4.Customer",strlen("Select User for Login\n1.Admin\n2.Manager\n3.Employee\n4.Customer"));
    if(wrbytes==-1){
        perror("error in sending message to client\n");
    }else{
        bzero(rbuff,sizeof(rbuff));
        int rbytes=read(fd,rbuff,sizeof(rbuff));
        if(rbytes==-1){
            perror("error while reading from client\n");
        }else if(rbytes==0){
            printf("no data sent by the client\n");
        }else{
            choice=atoi(rbuff);
            switch (choice)
            {
            case 1:
                admin_operation(fd);
                break;
            case 2:
                employee_operation(fd,2);
                break;
            case 3:
                employee_operation(fd,3);
                break;
            case 4:
                customer_operation(fd);
                break;
            default:
                break;
            }
        }
    }
    printf("connection break\n");
} 

int main(){
    struct sockaddr_in address,client;
    int fd1=socket(AF_INET,SOCK_STREAM,0);
    if(fd1==-1){
        perror("error while creating socket");
    }
    address.sin_addr.s_addr=htonl(INADDR_ANY);
    address.sin_family=AF_INET;
    address.sin_port=htons(8080);
    int status1=bind(fd1,(struct sockaddr *)&address,sizeof(address));
    if(status1==-1){
        perror("error while binding name to socket");
    }
    printf("Binding to socket was successful!\n");
    int status2=listen(fd1,2);
    if(status2==-1){
        perror("error while trying to listen for connections");
    }
    printf("now listening for connections on a socket\n");
    while (1){
        int client_size=(int)sizeof(client);
        int fd2=accept(fd1,(struct sockaddr *)&client,&client_size);
        if(fd2==-1){
            perror("error while accepting a connection");
	}else{
            if(fork()==0){
                connection(fd2);
                close(fd2);
            }
        }
    }
    close(fd1);
    return 0;
}