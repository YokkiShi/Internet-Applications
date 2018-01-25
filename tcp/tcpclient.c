#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <fcntl.h>
#define ECHOMAX 255

int main(int argc,char *argv[])
{	
	int sock;
	int fd;
	struct sockaddr_in servAddr;
	unsigned short servPort;
	char *servIP;
        char *ptr;
	char *filename;
	char buffer[ECHOMAX+1];
	int fileNameLen;
	int respStringLen=100;
	struct in_addr addr;
	struct hostent *hptr;
	char **pptr;
	char str[32];
	int recDes;
	int i,j;
	ptr=argv[1];//IP adrress or domain name 
	filename=argv[3];
	servPort=atoi(argv[2]);
        
	
	if(inet_aton(ptr,&addr)==0){
		hptr=gethostbyname(ptr);
		pptr=hptr->h_addr_list;
		servIP=inet_ntop(hptr->h_addrtype,*pptr,str,sizeof(str));
	}
	else{
		servIP=ptr;
	}
	if((sock=socket(PF_INET,SOCK_STREAM,0))<0)   //socket of client
		printf("socket() failed.\n");
	memset(&servAddr,0,sizeof(servAddr));
	servAddr.sin_family=AF_INET;
	servAddr.sin_addr.s_addr=inet_addr(servIP);
	servAddr.sin_port=htons(servPort);
        //connect
	if(connect(sock,(struct sockaddr *)&servAddr,sizeof(servAddr))<0){
		printf("connection failed");
        }
	//send
	fileNameLen=strlen(filename);
	if(send(sock,filename,fileNameLen,0)!=fileNameLen){
		printf("send() send a different number of bytes than expected.\n");
}
	printf("Sending message to server\n");
         
	//create a new file
	if((fd=open("readLi.txt",O_CREAT | O_RDWR| O_APPEND,0))<0){
		printf("create error\n");
		exit(1);}
	memset(buffer,0,sizeof(buffer));
	while(respStringLen>=100)
	{
	memset(buffer,0,sizeof(buffer));	
	if((respStringLen=recv(sock,buffer,ECHOMAX,0))<0){
		printf("receive failed\n");}
	buffer[respStringLen]='\0';
	if(write(fd,buffer,respStringLen)!=respStringLen){
		printf("buffer write error");
		exit(1);}}
        close(fd);
        close(sock);
	 
	
}
