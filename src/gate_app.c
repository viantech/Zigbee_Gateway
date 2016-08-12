/*
    C ECHO client example using sockets
*/
#include <stdlib.h>
#include <stdio.h> //printf
#include <stdint.h>
#include <string.h>    //strlen
#include <sys/socket.h>    //socket
#include <arpa/inet.h> //inet_addr
#include <sys/types.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include "stdbool.h"

//get keyboard mess
int readKeyboard(char *buffer);
void readFile(char *ID, int sock);
void *recvThread(void *sock);
volatile bool _wait_flag_ACK = 0;
char flag_stop_thread = 0;
char message[100] , server_reply[100] = {0};
FILE *df;

bool Contains(const char* MyChar, const char* Search)
{
	char *cp = (char *)MyChar;
	char *s1;
	char *s2 = (char *)Search;
	while (*cp)
	{
		s1 = cp;
        while ( *s1 && *s2 && !(*s1-*s2) )
                s1++, s2++;

        if (!*s2)
                return true;

        cp++;
	}
	return false;
}
void readFile(char *ID, int sock)
{
	/* FILE Handler*/
	FILE * file;
	size_t flen = 0;
	char *line = NULL;
	file = fopen ("/home/quangdo/Downloads/Intelhex/helllo.hex", "r");
	if (file == NULL) {
		perror ("hex file:");
		exit(EXIT_FAILURE);
	}
	/*AT command prefix*/
    char send_com[100] = {0};
    send_com[0] = 0x61; //'A';
    send_com[1] = 0x74; //'T';
    send_com[2] = 0x2B; //'+';
    send_com[3] = 0x75; //'U';
    send_com[4] = 0x63; //'C';
    send_com[5] = 0x61; //'A';
    send_com[6] = 0x73; //'S';
    send_com[7] = 0x74; //'T';
    send_com[8] = 0x3A; //':';
    strcat(ID, "=");
    strcat(send_com, ID); //at+ucast:ID=data

    while(getline(&line, &flen, file) != -1 ) /* read a line */
       {
       		strcat(line, "\r");
       		memcpy(&send_com[14], line, strlen(line));
       		send(sock, send_com, strlen(send_com), 0);
//       		if (send(sock, send_com, strlen(send_com), 0) < 0){
//       			perror("Send failed. Terminate sending file.");
//       			break;
//       		}
        	while (_wait_flag_ACK == 0);
        	_wait_flag_ACK = 0;
       		//printf("fu: %02x %02x %02x:ck\n", send_com[strlen(send_com) - 3], send_com[strlen(send_com) - 2], send_com[strlen(send_com)-1]);
       }
    fclose(file);
    if (line) free(line);

}
int main()
{
    int sock, nread = 0;
    char ip_addr[100] = {0};
    const char *pre_commandC = "AT+UCAST:000D6F000C469B5B=@";
    char CComand[100];
    struct sockaddr_in server;
    struct timeval tv;
    pthread_t recv_thread;
    void* thread_ret;

   memcpy(ip_addr, "192.168.204.247",15);
   memcpy(CComand, pre_commandC, strlen(pre_commandC));

    //Create socket
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1)
    {
        printf("Could not create socket");
    }
    printf("Socket created, sock identify: %d \nConnecting to server ...\n", sock);

   /* printf("input server addr: \n");
    fgets(ip_addr, sizeof(ip_addr), stdin);
    printf("input port: \n");
    scanf("%d", &port);*/
    server.sin_addr.s_addr = inet_addr("192.168.204.247");//convert string to long 4 bytes.

    server.sin_family = AF_INET;
    server.sin_port = htons(4096);

    //Connect to remote server
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect failed. Error");
        return 1;
    }

    puts("Connected\n");
    tv.tv_sec = 0;
    tv.tv_usec = 70000;
   //set time to receive
   if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval)) < 0)
	   perror("setsockopt failed \n");
    pthread_create(&recv_thread, NULL, recvThread, (void *)sock);
    //keep communicating with server

    time_t mytime;
    mytime = time(NULL);
    //create file and save log data
    //fi = fopen("log.txt", "a");
    df = fopen("log.txt", "w");
    if(df == NULL)
    {
    	perror ("log file");
    	exit(EXIT_FAILURE);
    }
    //write time start program:
//   fprintf(fi, "%s", ctime(&mytime));
    fprintf(df, "%s", ctime(&mytime));
    fflush(df);
	while (1) {
		// printf("Enter message : ");
		nread = readKeyboard(message);
		if (nread > 0) {
			printf("comm: %s", message);
			if (strstr(message, "quit") != NULL)
			{
				flag_stop_thread = 1;
				break;
			}
			//Send some data
			if(strstr(message, "at") != NULL)
			{
				message[strlen(message) - 1] = 0x0D;
				if (send(sock, message, strlen(message), 0) < 0) {
					perror("Send failed");
					return 1;
				}

			}

			if(strstr(message, "sendto:"))
			{
				char remote_ID[4];
//				strtok(message,":");
//				remote_ID = strtok(NULL, ":");
				memmove(remote_ID, message + 7, 4);
				readFile(remote_ID, sock);
			}
			if(strstr(message, "C"))
			{
				message[strlen(message) - 1] = '\0';
				strcat(message, "\r");
				//strcat(CComand,pre_commandC);
				memcpy(&CComand[strlen(pre_commandC)], message, strlen(message));
//				strcat(CComand,message);

				//printf("\nlenh la: %s %s\n",message,CComand);
				if (send(sock, CComand, strlen(CComand) + 1, 0) < 0) {
					perror("Send failed");
					return 1;
				}
			}
			nread = 0;
		}

	}
	pthread_join(recv_thread, &thread_ret);
	printf("close socket %d \n", sock);
    close(sock);
//	fclose(fi);
	fclose(df);
    return 0;
}

int readKeyboard(char *buffer)
{
	//char buffer[128];
	int result, nread = 0;
	fd_set inputs, testfds;
	struct timeval timeout;
	FD_ZERO (&inputs);
	FD_SET(0, &inputs);
	testfds = inputs;
	timeout.tv_sec = 5;
	timeout.tv_usec = 500000;
	result = select(FD_SETSIZE, &testfds, (fd_set*)0, (fd_set*)0, &timeout);

	switch(result)
	{
		case 0:
			//printf("timeout \n");
			break;
		case -1:
			perror("select error");
			return -1;
			break;
		default:
			if(FD_ISSET(0, &testfds))
			{
				ioctl(0, FIONREAD, &nread);
				if(nread == 0)
				{
					printf("keyboard done \n");
					return -1;
				}
				nread = read(0, buffer, nread);
				buffer[nread] = 0;
				//printf("read %d from keyboard: %s \n", nread, buffer);
			}
			break;
	}
	return nread;
}


void *recvThread(void *sock)
{
	int nrev;// count_CRLF = 0, timeout = 0;
	struct timeval begin, stop;
    static char flag_count_timer = 0;//, flag_cont_rev = 0;
//    char rev_bcast[200] = {0};
    char *buffptr = NULL, *findCR = NULL;

	while (1)
	{
		bzero(server_reply, sizeof(server_reply)); // clear buffer;
		buffptr = server_reply;
//		timeout = 0;
		while ((nrev = recv((int *) sock, buffptr, server_reply + sizeof(server_reply) - buffptr - 1, 0)) > 0)
		{
		  buffptr += nrev;
		  if(strlen(server_reply) > 4)
		  {
			  findCR = strchr(&server_reply[2], '\r');
			  if(findCR != NULL)
			  {
			  	 server_reply[strlen(server_reply)] = '\0';
			  	 break;
			  }
		  }
		  if(flag_stop_thread == 1) //quit program
		  {
			  printf("ok quit, wait data here: %d  %s\n", strlen(server_reply), server_reply);
			  break;
		  }
		}

/*		if(nrev == -1)
		{
			perror("receive err");
		}*/
//		nrev = recv((int *) sock, server_reply, 200,  MSG_DONTWAIT);

		if (nrev > 0 && strlen(server_reply)) { //has data
			//printf("Server reply %d: \n", strlen(server_reply));

			printf("%s",server_reply);
			if (Contains(server_reply, "UCAST")) {

				if (flag_count_timer == 0) {
					//puts("start receive: \n");
					gettimeofday(&begin, NULL);
					flag_count_timer = 1;
				}

				if ((flag_count_timer == 1)
						&& (strstr(server_reply, "done") != NULL)) {
					gettimeofday(&stop, NULL);
					double timetaken = (double) (stop.tv_usec - begin.tv_usec)
							/ 1000000 + (double) (stop.tv_sec - begin.tv_sec);
					printf("time is: %f\n", timetaken);
					flag_count_timer = 0;
				}
				fprintf(df, "%s", server_reply); //write to draft file
//				fflush(df);
			}
			else if(Contains(server_reply, "ACK"))
		      	  _wait_flag_ACK = 1;

			nrev = -1;
			//bzero(server_reply, sizeof(server_reply));
		/*	if(strlen(rev_bcast) && (flag_cont_rev == 0))
			{
				fprintf(d, "%s", rev_bcast); //write to draft file
				fflush(d);
				printf("rev_bcast: %s \n", rev_bcast);
				bzero(rev_bcast, sizeof(rev_bcast));
				count_CRLF++;
			}*/
		} else {
			if (nrev == 0) //determine server has closed socket
				break; // to close socket
		}
		if (flag_stop_thread == 1)
		{
			printf("close thread \n");
			pthread_exit(NULL);
		}
	}
	return NULL;
}
