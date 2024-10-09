#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<sys/socket.h>
#include<netinet/ip.h>
int main(){
	int fd=socket(AF_INET,SOCK_STREAM,0);
	if(fd==-1){
		perror("error in creating socket");
	}
	struct sockaddr_in address;
	address.sin_addr.s_addr=htonl(INADDR_ANY);
    address.sin_family=AF_INET;
    address.sin_port=htons(8080);
	int status=connect(fd,(struct sockaddr*)&address,sizeof(address));
	if(status==-1){
		perror("error in connecting to server");
	}
	printf("client server connection successfully\n");
    while(1){
        char rbuff[1000],wbuff[1000];
        memset(rbuff,0,sizeof(rbuff));
        memset(wbuff,0,sizeof(wbuff));
        int readbyte=read(fd,rbuff,sizeof(rbuff));
        if(readbyte==-1){
            perror("error in reading from server\n");
        }else if(readbyte==0){
            printf("nothing send from server closing connection\n");
            break;
        }else{
            memset(wbuff,0,sizeof(wbuff));
            printf("%s\n",rbuff);
            scanf("%[^\n]%*c",wbuff);
            int write_bytes=write(fd,wbuff,sizeof(wbuff));
            if (write_bytes == -1) {
                perror("Write to client socket\n");
                break;
            }
            
        }
    }
	close(fd);
	return 0;
}