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


size_t receiveJoke(int s);

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


	/** Setting socket options
	struct timeval tv;
	tv.tv_sec = 3;

	if((success_setting_options=setsockopt(s, SOL_SOCKET, SO_RCVTIMEO,(struct timeval *)&tv,sizeof(struct timeval)))<0){
		perror("setsocketopt");
		return 1;
	}
	**/


	/*Establishing tcp connection*/
	if (connect(s, (struct sockaddr *) &remote_addr, sizeof(struct sockaddr))
			< 0) {
		perror("connect");
		return 1;
	}

	printf("connected to server\n");


	char *string = "BenLim"; //received from input
	//printf("size of message to be sent: %1u \n", sizeof(string));
	//printf("size of request header: %1u \n", sizeof(request_header));
	/*Creating packet*/
	char buffer[((sizeof(request_header) + strlen(string)))]; // allocate buffer size needed for a packet
	//printf("size of final packet: %1u \n", sizeof(buffer));

	request_header *reqHeader = (request_header*) buffer; //show to the beginning of buffer
	reqHeader->type = JOKER_REQUEST_TYPE;
	reqHeader->len_first_name = 3;
	reqHeader->len_last_name = 3;
	//show to payload where text begins
	char * payload = buffer + sizeof(request_header);
	//copy string to packet which is going to be sent
	strncpy(payload, string, strlen(string));
	//printf("payload to be sent %s \n", payload);
	//printf("packet size to be sent: %1u \n", sizeof(buffer));
	if ((len_sent = send(s, buffer, sizeof(buffer), 0)) < 0) {
		perror("write");
		return 1;
	};






	size_t lenght=receiveJoke(s);

	printf("Joke length %d\n", lenght);
	close(s);
	puts("socket closed \n");
	return 0;
}

size_t receiveJoke(int s){


	int total_bytes_received=0;
	int received=0;
	int header_found=0;
	char tmark;
	char  header_buffer[5];
	response_header * res_header;

		/** find header first, may be there is some junk prepended before header */
		do{
			if((received=recv(s, header_buffer, 1,0))<=0){
				perror("nothing received");
				return -1;
			}
			if(header_buffer[0]==JOKER_RESPONSE_TYPE){
				if((received=recv(s, header_buffer+1, sizeof(response_header)-1,0))<=0){
					perror("nothing received");
					return -1;
				}

				header_found=1;
				res_header=(response_header*)header_buffer;

			}

		}while(!header_found);


		/* Now get the joke, with appropriate lenght received from header */
		uint32_t len_joke=ntohl(res_header->len_joke);
		char joke_buffer[len_joke];/*buffer for the whole joke*/
		char joke_part_buffer[len_joke];
		int joke_part_received=0;

		while(total_bytes_received<len_joke){
			if((joke_part_received=recv(s, joke_part_buffer, len_joke,0))<=0){
					perror("nothing received");
					return -1;
			}
			total_bytes_received+=joke_part_received;
			joke_part_buffer[joke_part_received]='\0';
			strcat(joke_buffer, joke_part_buffer);//append part of joke to the whole
		}
		joke_buffer[len_joke]='\0';

		//DISPLAY JOKE
		printf("joke:%s \n",joke_buffer);

	return len_joke;
}

