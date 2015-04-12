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
	char recvbuffer[BUFSIZ];

	memset(&remote_addr, 0, sizeof(remote_addr));
	remote_addr.sin_family = AF_INET;
	remote_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	//remote_addr.sin_addr.s_addr = inet_addr("137.226.59.41");
	remote_addr.sin_port = htons(8000);
	//remote_addr.sin_port = htons(2345);
	if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		return 1;
	}


	/** Setting socket options**/
	struct timeval tv;
	tv.tv_sec = 120;  /* 3 Secs Timeout */

	if((success_setting_options=setsockopt(s, SOL_SOCKET, SO_RCVTIMEO,(struct timeval *)&tv,sizeof(struct timeval)))<0){
		perror("setsocketopt");
		return 1;
	}



	/*Establishing tcp connection*/
	if (connect(s, (struct sockaddr *) &remote_addr, sizeof(struct sockaddr))
			< 0) {
		perror("connect");
		return 1;
	}

	printf("connected to server\n");


	char *string="BenLim";//received from input
	printf("size of message to be sent: %1u \n", sizeof(string));
	printf("size of request header: %1u \n", sizeof(request_header));

	/*Creating packet*/
	char buffer[((sizeof(request_header)+sizeof(string)))];// allocate buffer size needed for a packet
	printf("size of  final packet: %1u \n", sizeof(buffer));

	request_header *reqHeader=( request_header*)buffer;//show to the beginning of buffer
	reqHeader->type=JOKER_REQUEST_TYPE;
	reqHeader->len_first_name=3;
	reqHeader->len_last_name=3;



	//show to payload where text begins
	char * payload=buffer+sizeof(request_header);


	//copy string to packet which is going to be sent
	strncpy(payload, string, strlen(string));

	printf("payload to be sent %s \n", payload);
	printf("packet size to be sent: %1u \n", sizeof(buffer));
	if((len_sent = send(s, buffer, sizeof(buffer), 0))<0){
		perror("write");
		return 1;
	};
	//receive a buffer, take BUFSIZE as size so far
	if((len_received = recv(s, recvbuffer, BUFSIZ, 0))<0){
		perror("read");
		return 1;
	}
	printf("received response packet size: %d \n", sizeof(recvbuffer));
	printf("len_received %1u \n", sizeof(recvbuffer));
	//we read allways buf from socket buffer until end of message-> recv in loop
	//we have to detect different messages by the header and display them




	response_header * resHeader=(response_header * )recvbuffer;
	char * message=recvbuffer+sizeof(response_header);
	if(resHeader->type==JOKER_RESPONSE_TYPE){
		printf("expected joke size: %d \n",ntohl(resHeader->len_joke));
		printf("joke size & junk: %d\n", strlen(message));
		printf("joke with junk: %s\n", message);

		uint32_t len_joke=ntohl(resHeader->len_joke);
		char joke_part[len_joke];
		//if server send to more than just the joke copy the joke part to new string
		if(len_joke<=strlen(message)){



			strncpy(joke_part, message, len_joke);
			joke_part[len_joke+1]='\0';//mark end of new string

			printf("pure joke: %s\n", joke_part);

		}else{
			//TODO what if not all data received yet?
		}


	}



	close(s);
	printf("  socket closed: %s\n", recvbuffer);
	return 0;
}
