#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

#include <unistd.h>


#define PORT 2080
#define DEST_ADDR "255.255.255.255"
 
int main(int argc, char *argv[])
{
	int sockfd;
	int broadcast=1;
	struct sockaddr_in sendaddr;
	struct sockaddr_in recvaddr;
	int numbytes;
	char snd_buf[2048];
	char recv_buf[2048];
	ssize_t recv_size = 0;
	ssize_t snd_size = 0;
	
	int seq = 0;
	char * sid = NULL;
	
	if(argc != 3){
		printf("Usage: %s seq sid\n", argv[0]);
		return -1;
	}	
	
	seq = atoi(argv[1]);
	sid = argv[2];
	printf("seq: %d\n", seq);
	printf("sid: %s\n", sid);
	
	if((sockfd = socket(PF_INET,SOCK_DGRAM,0)) == -1)
	{
		perror("sockfd");
		exit(1);
	}
	
	if((setsockopt(sockfd,SOL_SOCKET,SO_BROADCAST,
				&broadcast,sizeof broadcast)) == -1)
	{
		perror("setsockopt - SO_SOCKET ");
		exit(1);
	}
	        	
	sendaddr.sin_family = AF_INET;
	sendaddr.sin_port = htons(PORT);
	sendaddr.sin_addr.s_addr = INADDR_ANY;
	memset(sendaddr.sin_zero,'\0',sizeof sendaddr.sin_zero);
	
	if(bind(sockfd, (struct sockaddr*) &sendaddr, sizeof sendaddr) == -1)
	{
		perror("bind");
		exit(1);
	}
	
	recvaddr.sin_family = AF_INET;
	recvaddr.sin_port = htons(PORT);
	recvaddr.sin_addr.s_addr = inet_addr(DEST_ADDR);
	memset(recvaddr.sin_zero,'\0',sizeof recvaddr.sin_zero);
	
	
	int msg_size = sprintf(snd_buf, "REQ:%d#SID:%s", seq, sid);
	
	
	printf("send id broadcast start\n");
	
	while((numbytes = sendto(sockfd, snd_buf, msg_size , 0,
        (struct sockaddr *)&recvaddr, sizeof recvaddr)) != -1) 
   {
   		printf("Sent a broadcast packet, wait for reply\n");
   		sleep(1);
		recv_size = recvfrom(sockfd, recv_buf, sizeof(recv_buf), MSG_DONTWAIT, NULL, NULL);
		
		if(recv_size > 4 && recv_buf[0] == 'R' && recv_buf[1] == 'S' && recv_buf[2] == 'P'){
			printf("Got response size:%d\n", recv_size);
			break;
		}
		
   }
	
	close(sockfd);
	
	printf("Exiting\n");
	
	return 0;
}
