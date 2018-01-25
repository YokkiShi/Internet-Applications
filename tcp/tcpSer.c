#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>


#define ECHOMAX 255

int main(int argc,char *argv[]){
int sock;
int accSock;
int addrLen;
struct sockaddr_in servAddr;
struct sockaddr_in clntAddr;
char buffer[50];
char readBuf[100];
unsigned short servPort;
int recvMsgSize;
char *fileName;
int fd;
int i=0;
int j=0;


if(argc!=2)
{	printf("Usage :%s <PORTNUMBER>",argv[0]);
	exit(1);
}
	servPort=atoi(argv[1]);
	//socket()
	if((sock=socket(PF_INET,SOCK_STREAM,0))<0){
		printf("socket() failed.\n");}
	memset(&servAddr,0,sizeof(servAddr));
	servAddr.sin_family=AF_INET;
	servAddr.sin_addr.s_addr=htonl(INADDR_ANY);
	servAddr.sin_port=htons(servPort);
	//bind()
	if((bind(sock,(struct sockaddr *)&servAddr,sizeof(servAddr)))<0)
		{printf("bind() failed.\n");
			exit(1);	}
	//listen()
	if(listen(sock,10)<0){
		printf("listen() failed\n");
		exit(1);	}
	printf("ready to accept\n");	
	addrLen=sizeof(clntAddr);
	if((accSock=accept(sock,(struct sockaddr*)&clntAddr,&addrLen))<0){
		printf("accept() failed\n");}
	printf("ready to receive\n");

		while(1){	
   		
		if(recv(accSock,buffer,sizeof(buffer),0)<0){
			printf("receive() failed\n");
			exit(1);}
		fileName=buffer;
		//printf filename
		for(i;i<strlen(buffer);i++){
			printf("%c",buffer[i]);}
                printf("\n");
		if((fd=open(fileName,O_RDWR,0))==-1){
			printf("the file dosn't exist\n");}
		else{
	        i=100;
                
		while(read(fd,readBuf,100)>0)
			{ if(lseek(fd,i,SEEK_SET)==-1)
				{printf("lseek error\n");
				 exit(1);}			  
				 i=i+100;
			   if((send(accSock,readBuf,sizeof(readBuf),0))<0)
				{printf("send() faild\n");
                                  }
			for(j=0;j<sizeof(readBuf);j++){
				printf("%c",readBuf[j]);}
                           memset(readBuf,0,sizeof(readBuf)); 
                       
}break;
}
                            
			}
        close(accSock);
	close(fd);
	return(EXIT_SUCCESS);}
