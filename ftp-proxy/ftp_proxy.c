#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <string.h>
#include <ctype.h>

#define TRUE 1
#define FALSE 0
#define BUFFSIZE 100
#define PORT 21
#define PORT2 2000


void getSockIp(int fd,char *ipStr,int buffsize)
{

    memset(ipStr,0,buffsize);
    struct sockaddr_in addr;
    socklen_t addrlen;
    addrlen = sizeof(addr);

    if(getsockname(fd,(struct sockaddr *)&addr,&addrlen) < 0){
        perror("getsockname() failed: ");
        exit(1);
    }

    inet_ntop(AF_INET,&addr.sin_addr,ipStr,addrlen);

    char *p = ipStr;
    while(*p){
        if(*p == '.') *p = ',';
        p++;
    }
}
int bindAndListenSocket(unsigned short port)
{
    int sockfd;
    struct sockaddr_in addr;

    sockfd = socket(AF_INET,SOCK_STREAM,0);
    if(sockfd < 0){
        perror("socket() failed: ");
        exit(1);
    }
    
    bzero(&addr,sizeof(addr));/*Zero out structure*/
    addr.sin_family = AF_INET;/*Internet addr family*/
    addr.sin_port = htons(port);/*port*/
    addr.sin_addr.s_addr = htonl(INADDR_ANY);/*ip address*/
   
	/*bind*/
    if(bind(sockfd,(struct sockaddr*)&addr,sizeof(addr)) < 0){
        perror("bind() failed: ");
        exit(1);
    }
	/*listen*/
    if(listen(sockfd,6) < 0){
        perror("listen() failed: ");
        exit(1);
    }
   
    return sockfd;
}
int acceptCmdSocket(int socket,struct sockaddr *addr,socklen_t *addrlen)
{
    int sockfd ;
    if((sockfd=accept(socket,addr,addrlen))< 1){
        perror("accept() failed:");
        exit(1);
    }

    return sockfd;
}
int connectToServer(char *ip,unsigned short port)
{
    int sockfd;
    struct sockaddr_in servaddr;

    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    inet_pton(AF_INET,ip,&servaddr.sin_addr);
        
    sockfd = socket(AF_INET,SOCK_STREAM,0);
    if(sockfd < 0){
    
        perror("socket() failed :");
        exit(1);
    }

    if(connect(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr)) < 0){
        perror("connect() failed :");
        exit(1);
    }

    return sockfd;
}
unsigned short getClientPort(char *content)
{
    unsigned short port,t;
    int count = 0;
    char *p = content;    

    while(count < 4){
        if(*(p++) == ','){
            count++;
        }
    }

    sscanf(p,"%hu",&port);
    while(*p != ',' && *p != '\r' && *p != ')') p++;
    if(*p == ','){
        p++;
        sscanf(p,"%hu",&t);
        port = port * 256 + t;
    }

    return port;
}
void getcontent(char *buff,char **content)
{
  
    char *p;
    p = strchr(buff,' ');
  
    if(!p){
        *content = NULL;
    }else{
        *p = 0;
        *content = p + 1;
    }


    
    
}
unsigned short getSockPort(int sockfd)
{
    struct sockaddr_in addr;
    socklen_t addrlen;
    addrlen = sizeof(addr);

    if(getsockname(sockfd,(struct sockaddr *)&addr,&addrlen) < 0){
        perror("getsockname() failed: ");
        exit(1);
    }

    return ntohs(addr.sin_port);
}
int connectToClient(struct sockaddr_in servaddr)
{
    int fd;

    struct sockaddr_in cliaddr;
    bzero(&cliaddr,sizeof(cliaddr));
    cliaddr.sin_family = AF_INET;
    cliaddr.sin_addr.s_addr = htonl(INADDR_ANY);    
  

    fd = socket(AF_INET,SOCK_STREAM,0);
    if(fd < 0){
        perror("socket() failed :");
        exit(1);
    }

    if(bind(fd,(struct sockaddr *)&cliaddr,sizeof(cliaddr) ) < 0){
        perror("bind() failed :");
        exit(1);
    }

    servaddr.sin_family = AF_INET;
    if(connect(fd,(struct sockaddr *)&servaddr,sizeof(servaddr)) < 0){
        perror("connect() failed :");
        exit(1);
    }

    return fd;
}
int main(int argc, const char *argv[])
{
    fd_set master_set, working_set;
    struct timeval timeout;
    int proxy_cmd_socket    = 0;
    int accept_cmd_socket   = 0;
    int connect_cmd_socket  = 0;
    int proxy_data_socket   = 0;
    int accept_data_socket  = 0;
    int connect_data_socket = 0;
    int selectResult = 0;
    int select_sd = 10;
    char serverIp[BUFFSIZE] = "192.168.56.1";
    char fileName[BUFFSIZE];
    char proxyIpforActive[BUFFSIZE];
    char proxyIpforPassive[BUFFSIZE];
    FD_ZERO(&master_set);
    bzero(&timeout, sizeof(timeout));
    int i;
    int pasv_mode = 1;
    int retr;
    int fdNew;
    int filefd;
    int cache=0;
    unsigned short proxy_data_port;
    unsigned short data_port;
    
    
    FD_ZERO(&master_set);   //clear the master_set
    bzero(&timeout, sizeof(timeout));
    socklen_t clilen;
    struct sockaddr_in cliaddr;
    proxy_cmd_socket = bindAndListenSocket(PORT);  //The first socket created in the proxy
    FD_SET(proxy_cmd_socket, &master_set);  // put the first socket into the master set
    while (TRUE) {
        FD_ZERO(&working_set); //clear the working set
        memcpy(&working_set, &master_set, sizeof(master_set)); //copy the master_set to working set
        timeout.tv_sec = 1000;    //the timeout fime for select()
        timeout.tv_usec = 0;    //ms
        
       
        selectResult = select(select_sd, &working_set, NULL, NULL, &timeout);
        if (selectResult < 0) {
            perror("select() failed\n");
            exit(1);
        }
        
        // timeout
        if (selectResult == 0) {
            printf("select() timed out.\n");
            continue;
        }
        for(i=0;i<select_sd;i++){
        
            if (FD_ISSET(i, &working_set)) {
                if (i == proxy_cmd_socket) {
					accept_cmd_socket = acceptCmdSocket(proxy_cmd_socket,NULL,NULL);
                    connect_cmd_socket = connectToServer(serverIp,PORT);
                    
                    getSockIp(connect_cmd_socket,proxyIpforActive,BUFFSIZE);
                    getSockIp(accept_cmd_socket,proxyIpforPassive,BUFFSIZE); 
                    
                    FD_SET(accept_cmd_socket, &master_set);
                    FD_SET(connect_cmd_socket, &master_set);
				}
                
                if (i == accept_cmd_socket) {
               
                    char buff[BUFFSIZE] = {0};
                    char copy[BUFFSIZE] = {0};
                    char send_buff[BUFFSIZE];
                
                	char readBuf[BUFFSIZE];
                	memset(readBuf,0,sizeof(readBuf));
                   
                    
                    if (read(i, buff, BUFFSIZE) == 0) 
					{
                        close(i);
                        close(connect_cmd_socket);
                        printf("client closed\n");
                        
                        
                        FD_CLR(i, &master_set);
                        FD_CLR(connect_cmd_socket, &master_set);

                    } 
					else 
					{
                        printf("Command received from client : %s\n",buff);
                        char *content;
                        strcpy(copy,buff);
                        getcontent(copy,&content);
       	
                       //RETR
                        if(memcmp(buff,"RETR",4) == 0)
                        {
                            retr=1;
                            memset(fileName,0,sizeof(fileName));
                            
                            int j;
                            
                            for(j=0;j<strlen(buff)-7;j++)
                            {
                                fileName[j]=buff[j+5];	
                            }
                            if((filefd=open(fileName,O_RDONLY))<0)
							{
								printf("File doesn't exist in the cache\n");
								//If the file does not exit, creat a new file with the same name
								if((fdNew=creat(fileName,0644))<0)
								{
									printf("creat failed");
								}
                        		write(connect_cmd_socket, buff, strlen(buff));
							}
						
							else
							{
								printf("File exist in the cache.\n We can download from the cache.\n");
                                if(pasv_mode==0)
								{
									//build the connection to ftp client
									close(proxy_data_socket);
									FD_CLR(proxy_data_socket,&master_set);
									proxy_data_socket=0;
									connect_data_socket=connectToServer(serverIp,data_port);
									FD_SET(connect_data_socket, &master_set);
									//write the file to client
									while((read(filefd,readBuf,BUFFSIZE))>0)
          							{ 
              							write(connect_data_socket,readBuf,sizeof(readBuf));
                    					memset(readBuf,0,sizeof(readBuf));
									}
		   							close(filefd);
									close(connect_data_socket);
									FD_CLR(connect_data_socket,&master_set);
									connect_data_socket=0;
									memset(send_buff,0,sizeof(send_buff));
									sprintf(send_buff,"150 Opening data channel for file download from server of \"/%s\"\r\n",fileName);
									printf("%s\n",send_buff);
									write(accept_cmd_socket,send_buff,strlen(send_buff));//proxy inform the client to start data transfer
									memset(send_buff,0,sizeof(send_buff));
									sprintf(send_buff,"226 Successfully transferred \"/%s\"\r\n",fileName);
									printf("%s\n",send_buff);
									write(accept_cmd_socket,send_buff,strlen(send_buff));//proxy inform the client the finish of data transfer
			
								}	
								//passive mode
								else 
								{
									cache=1;
								}
							}
						}
                        //PORT
                        
                       else if(memcmp(buff,"PORT",4) == 0)
					   {
                           
                            proxy_data_socket = bindAndListenSocket(0);
                        	proxy_data_port = getSockPort(proxy_data_socket);
                            FD_SET(proxy_data_socket, &master_set);
                            pasv_mode = 0;
  	                        data_port = getClientPort(content);
                            bzero(buff,BUFFSIZE);
                            sprintf(buff,"PORT %s,%d,%d\r\n",proxyIpforActive,proxy_data_port / 256,proxy_data_port % 256);
                            printf("command sent to server : %s\n",buff);
                       		write(connect_cmd_socket, buff, strlen(buff));
                        }
                        
                    	else
						{
                    		printf("command sent to server : %s\n",buff);
                       		write(connect_cmd_socket, buff, strlen(buff));	
                    	}
                       
                    }
				}
                
                if (i == connect_cmd_socket) {        
                  
                    char buff[BUFFSIZE] = {0};
                    if(read(i,buff,BUFFSIZE) == 0){
                        close(i);
                        close(accept_cmd_socket);
                        FD_CLR(i,&master_set);
                        FD_CLR(accept_cmd_socket,&master_set);
					}
					printf("Reply received from server : %s\n",buff);
                    
                  //PASV
                  
                  if(memcmp(buff,"227",3) == 0){
                  		char *content;
                        proxy_data_socket = bindAndListenSocket(0); //
                        proxy_data_port = getSockPort(proxy_data_socket);
                        FD_SET(proxy_data_socket, &master_set);//
                        content = buff + 27;
						data_port = getClientPort(content);
                        bzero(content,BUFFSIZE - 27);
                        sprintf(content,"%s,%d,%d).\r\n",proxyIpforPassive,proxy_data_port / 256,proxy_data_port % 256);
                    }
                      printf("reply sent to client : %s\n",buff);

                    write(accept_cmd_socket,buff,strlen(buff));
                }
                
                
                if (i == proxy_data_socket) {
					if(cache==1)
					{
            			cache=0;
            			if((accept_data_socket=acceptCmdSocket(proxy_data_socket,NULL,NULL))<0)
						{
                    		printf("data accept() failed\n");
						}
            			printf("Data accepted successfully\n");
                        
                		char readBuf[BUFFSIZE];
                		char send_buff[BUFFSIZE];
                		memset(readBuf,0,sizeof(readBuf));
						while((read(filefd,readBuf,BUFFSIZE))>0)
          				{ 
              				write(accept_data_socket,readBuf,sizeof(readBuf));
                    		memset(readBuf,0,sizeof(readBuf));
  						}
						close(filefd);
						close(proxy_data_socket);
						close(accept_data_socket);
						FD_CLR(proxy_data_socket,&master_set);
						FD_CLR(accept_data_socket,&master_set);
						accept_data_socket=0;
						proxy_data_socket=0;
						memset(send_buff,0,sizeof(send_buff));
						sprintf(send_buff,"150 Opening data channel for file download from server of \"/%s\"\r\n",fileName);
						write(accept_cmd_socket,send_buff,strlen(send_buff));
						memset(send_buff,0,sizeof(send_buff));
						sprintf(send_buff,"226 Successfully transferred \"/%s\"\r\n",fileName);
						write(accept_cmd_socket,send_buff,strlen(send_buff));
            	   }
            	    else
					{
            	
			
						if(pasv_mode)
						{            //clinet connect
                        	accept_data_socket = acceptCmdSocket(proxy_data_socket,NULL,NULL);        //client connection with proxy
                        	connect_data_socket = connectToServer(serverIp,data_port);        //proxy connection with server
                    	}
                    	else
						{
                       		accept_data_socket = acceptCmdSocket(proxy_data_socket,NULL,NULL);        //proxy connection with server
                        	clilen = sizeof(cliaddr);
                        	if(getpeername(accept_cmd_socket,(struct sockaddr *)&cliaddr,&clilen) < 0)
							{
                            	perror("getpeername() failed: ");
                        	}
                        	cliaddr.sin_port = htons(data_port);
                        	connect_data_socket = connectToClient(cliaddr);        //client connection with proxy
                        } 

                    	FD_SET(accept_data_socket, &master_set);
                    	FD_SET(connect_data_socket, &master_set);
                    	printf("Data connection established\n");
                  	}
                }
                
                if (i == accept_data_socket) {

                    int n;
                    char buff[BUFFSIZE] = {0};
                    if((n = read(accept_data_socket,buff,BUFFSIZE)) == 0){
                        close(accept_data_socket);
                        close(connect_data_socket);
                        close(proxy_data_socket);
                        FD_CLR(proxy_data_socket,&master_set);
                        FD_CLR(accept_data_socket, &master_set);
                        FD_CLR(connect_data_socket, &master_set);
                        proxy_data_socket   = 0;
                        accept_data_socket  = 0;
                        connect_data_socket = 0;
                    }
                    else
                    {
                        if(pasv_mode==0 && retr==1) //download files in active mode
                        {
                            write(fdNew,buff,sizeof(buff));
                            if(write(connect_data_socket,buff,BUFFSIZE)!=BUFFSIZE)
                            {
                                printf("write failed\n");
                            }
                        }
                        else
                        {
                            if(write(connect_data_socket,buff,BUFFSIZE)!=BUFFSIZE)
                            {
                                printf("write failed\n");
                            }
                        }
                    }
            

                    
                }
                
                if (i == connect_data_socket) {
                    int n;
                    char buff[BUFFSIZE] = {0};
                    if((n = read(connect_data_socket,buff,BUFFSIZE)) == 0){
                        close(accept_data_socket);
                        close(connect_data_socket);
                        close(proxy_data_socket);
                        FD_CLR(proxy_data_socket,&master_set);
                        FD_CLR(accept_data_socket, &master_set);
                        FD_CLR(connect_data_socket, &master_set);
                        proxy_data_socket   = 0;
                        accept_data_socket  = 0;
                        connect_data_socket = 0;
                    }
                    else
                    {
                        if(pasv_mode==1 && retr==1) //download the file in passive mode
                        {
                            if(write(fdNew,buff,sizeof(buff))<0)
                            {
                                printf("Fail to write content into file \n");
                            }
                            if(write(accept_data_socket,buff,BUFFSIZE)!=BUFFSIZE)
                            {
                                printf("write failed\n");
                            }
                        }
                        else
                        {
                            if(write(accept_data_socket,buff,BUFFSIZE)!=BUFFSIZE)
                            {
                                printf("write failed\n");
                            }
                        }
                    }
                    
                
                }
            }
        }
    }
 
    return 0;
}
