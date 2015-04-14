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
#include <netdb.h>

#define JOKER_REQUEST_TYPE 1
#define JOKER_RESPONSE_TYPE 2
#define MAX_LEN_NAME 20

size_t receiveJoke(int s);
int sendall(int s, char *buf);
struct sockaddr_in * lookUpAdress(char * domain, char * port);
int createConnection(char * domain, char * port);
typedef struct {
	uint8_t type;
	uint32_t len_joke;
}__attribute__ ((__packed__)) response_header;

typedef struct {
	uint8_t type;
	uint8_t len_first_name;
	uint8_t len_last_name;
}__attribute__ ((__packed__)) request_header;



int main(int argc, char * argv[]) {


	if (argc != 3) {
		fprintf(stderr, "Please provide hostname and port \n");
		return 1;
	}

	char fname[MAX_LEN_NAME];
	char lname[MAX_LEN_NAME];


	puts("Please provide your firstname:");
	scanf("%s", fname);
	int len_fname=strlen(fname);

	puts("Please provide your lastname: ");
	scanf("%s",  lname);
	int len_lname=strlen(lname);

	char string[len_fname+len_lname];
	sprintf(string,"%s%s", fname, lname);

	/** Create packet**/
	char buffer[((sizeof(request_header) + strlen(string)))]; // allocate buffer size needed for a packet
	request_header *reqHeader = (request_header*) buffer; //show to the beginning of buffer
	reqHeader->type = JOKER_REQUEST_TYPE;
	reqHeader->len_first_name = len_fname;
	reqHeader->len_last_name = len_lname;
	//show to payload where text begins
	char * payload = buffer + sizeof(request_header);
	//copy string to packet which is going to be sent
	strncpy(payload, string, strlen(string));


	/*connect to  the server*/
	int socket=createConnection(argv[1], argv[2]);

	/*send our packet*/
	sendall(socket, buffer );

	/*receive response spacket*/
	size_t lenght=receiveJoke(socket);

	printf("Joke length %d\n", lenght);
	close(socket);
	puts("socket closed \n");
	return 0;
}

size_t receiveJoke(int s){



	int received=0;
	int header_found=0;

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
		int total_bytes_received=0;
		char joke_buffer[len_joke];/*buffer for the whole joke*/
		joke_buffer[0]='\0';

		char part_buffer[len_joke];
		char * joke_part_buffer=part_buffer;
		int joke_part_received=0;
		int left_bytes_to_read=0;

		while(total_bytes_received<len_joke){
			left_bytes_to_read=len_joke-total_bytes_received;
			if((joke_part_received=recv(s, joke_part_buffer, left_bytes_to_read,0))<=0){
					perror("nothing received");
					return -1;
			}
			total_bytes_received+=joke_part_received;
			joke_part_buffer[joke_part_received]='\0';
			strcat(joke_buffer, joke_part_buffer);//append part of joke to the whole
			joke_part_buffer+=joke_part_received;//to not fill entire buffer, but just the letter which are left
		}
		joke_buffer[len_joke]='\0';

		//DISPLAY JOKE
		printf("joke:%s \n",joke_buffer);

	return len_joke;
}

int sendall(int s, char *buf)
{
    int total = 0;        // how many bytes we've sent
    int bytesleft = strlen(buf); // how many we have left to send
    int n;

    while(total < strlen(buf)) {
        n = send(s, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }



    return n==-1?-1:total; // return -1 on failure, 0 on success
}

int createConnection(char * domain, char * port){
	int s;


	struct addrinfo * adress=(struct addrinfo *)lookUpAdress(domain, port);

	/* Create socket*/
	if ((s = socket(adress->ai_family, adress->ai_socktype, 0)) < 0) {
		perror("socket");
		return 1;
	}




	/** Setting socket options */
		 struct timeval timeout;
		    timeout.tv_sec = 3;
		    timeout.tv_usec = 0;

	if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout,
			sizeof(timeout)) < 0)
		perror("setsockopt failed\n");


		/*Establishing tcp connection*/
		if (connect(s,  adress->ai_addr, adress->ai_addrlen)
				< 0) {
			perror("connect");
			return 1;
		}

		printf("connected to server\n");
		return s;
}



struct sockaddr_in * lookUpAdress(char * domain, char * port){

	struct addrinfo hints;
	struct addrinfo *servinfo;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;//IP/IPv6
	hints.ai_socktype = SOCK_STREAM;//TCP
	hints.ai_flags = AI_PASSIVE;
	int addrinfo_status;
	if((addrinfo_status=getaddrinfo(domain, port, &hints, &servinfo))!=0)
	{
			fprintf(stderr, "Error occured while calling getaddrinfo: %s.\n", gai_strerror(addrinfo_status));
			return -1;
	}
	return servinfo;
}

