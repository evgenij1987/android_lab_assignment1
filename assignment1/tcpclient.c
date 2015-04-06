/*
 * tcpclient.c
 *
 *  Created on: Apr 2, 2015
 *      Author: evgenijavstein
 */

/*
 *
 * My TCP client for the server running on laboratory.comsys.rwth-aachen.de:2345
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h> /* memset */

#define JOKER_REQUEST_TYPE 1
#define JOKER_RESPONSE_TYPE 2

typedef struct {
	uint8_t type;
	uint32_t len_joke;
}__attribute__ ((__packed__)) response_header;

typedef struct {
	uint8_t type;
	uint8_t len_first_name;
	uint8_t len_last_name;
}__attribute__ ((__packed__)) request_header;



int main( argc, argv) {
	int s, len_sent, len_received, success_setting_options;
	struct sockaddr_in remote_addr;
	char buf[BUFSIZ];

	memset(&remote_addr, 0, sizeof(remote_addr));
	remote_addr.sin_family = AF_INET;
	//remote_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	remote_addr.sin_addr.s_addr = inet_addr("137.226.59.41");
	//remote_addr.sin_port = htons(8000);
	remote_addr.sin_port = htons(2345);
	if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		return 1;
	}


	//after creation of socket set socket option
	struct timeval tv;

	tv.tv_sec = 3;  /* 3 Secs Timeout */

	if((success_setting_options=setsockopt(s, SOL_SOCKET, SO_RCVTIMEO,(struct timeval *)&tv,sizeof(struct timeval)))<0){
		perror("setsocketopt");
		return 1;
	}


	if (connect(s, (struct sockaddr *) &remote_addr, sizeof(struct sockaddr))
			< 0) {
		perror("connect");
		return 1;
	}

	printf("connected to server\n");

	char *string="BenLim";//received from input
	printf("size of message to be sent: %1u \n", sizeof(string));
	printf("size of request header: %1u \n", sizeof(request_header));
	char buffer[((sizeof(request_header)+sizeof(string)))];// allocate buffer size needed for a packet
	printf("size of  final packet: %1u \n", sizeof(buffer));

	request_header *rHeader=( request_header*)buffer;//show to the beginning of buffer
	rHeader->type=0x01;
	rHeader->len_first_name=0x03;
	rHeader->len_last_name=0x03;



	//show to payload where text begins
	char * payload=buffer+sizeof(request_header);

	////fill buffer with character received from user in string
	int i=0;
	int string_lenght=strlen(string);
	for(i=0;i<string_lenght;i++){
		*payload=*string;//writy to memory the pointer points at
		string++;//increment the pointer to get all leters from string to buffer
		payload++;
	}
	printf("packet size to be sent: %1u \n", sizeof(buffer));
	if((len_sent = send(s, buffer, sizeof(buffer), 0))<0){
		perror("write");
		return 1;
	};
	//receive a buffer, take BUFSIZE as size so far
	if((len_received = recv(s, buf, BUFSIZ, 0))<0){
		perror("read");
		return 1;
	}
	printf("received response packet size: %d \n", sizeof(buf));
	printf("len_received %1u \n", sizeof(buf));
	//we read allways buf from socket buffer until end of message-> recv in loop
	//we have to detect different messages by the header and display them


	//buf[len_received] = '\0';

	response_header * resHeader=(response_header * )buf;
	char * message=buf+sizeof(response_header);
	if(resHeader->type==2){
		printf("expected joke size: %d \n",ntohl(resHeader->len_joke));
		printf("joke size & junk: %d\n", strlen(message));
		printf("joke with junk: %s\n", message);

		//if server send to much as expected message
		if(resHeader->len_joke>=strlen(message)){
			uint32_t len_joke=ntohl(resHeader->len_joke);
			char joke_part[len_joke];
			//memset(joke_part, '\0', sizeof(joke_part));
			strncpy(joke_part, message, len_joke);
			joke_part[len_joke+1]='\0';//mark end of new string
			//memcpy(joke_part,&message[0], resHeader->len_joke-1);//copy only the characters expected lenght
			printf("pure joke: %s\n", joke_part);

		}

	}



	close(s);
	printf("  socket closed: %s\n", buf);
	return 0;
}
