/*
 * tcpserver.c
 *
 *  Created on: Apr 6, 2015
 *      Author: evgenijavstein
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#define JOKER_REQUEST_TYPE 1
#define JOKER_RESPONSE_TYPE 2


#define JOKE1 "%s %s starts his terminal immediatly after waking up."

typedef struct {
	uint8_t type;
	uint32_t len_joke;
}__attribute__ ((__packed__)) response_header;

typedef struct {
	uint8_t type;
	uint8_t len_first_name;
	uint8_t len_last_name;
}__attribute__ ((__packed__)) request_header;

int main(argc, argv)
{
        int i, s, fd, len_received, len_send;
        struct sockaddr_in my_addr;
        struct sockaddr_in remote_addr;
        int sin_size;
        char buf[BUFSIZ];

        memset(&my_addr, 0, sizeof(my_addr));
        my_addr.sin_family = AF_INET;
        my_addr.sin_addr.s_addr = INADDR_ANY;
        my_addr.sin_port = htons(8000);

        if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
                perror("socket");
                return 1;
        }

        if (bind(s, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0)
        {
                perror("bind");
                return 1;
        }

        listen(s, 5);
        sin_size = sizeof(struct sockaddr_in);
        if ((fd = accept(s, (struct sockaddr *)&remote_addr, &sin_size)) < 0)
        {
                perror("accept");
                return 1;
        }
        printf("accepted client %s\n", inet_ntoa(remote_addr.sin_addr));




        if((len_received = recv(fd, buf, BUFSIZ, 0))<0){
        	perror("receive");
        	return 1;

        }
        printf("%s\n", buf);
        request_header * reqHeader=(request_header *)buf;


        char * payload=buf+sizeof(request_header);
        if(reqHeader->type==JOKER_REQUEST_TYPE){
        	char fname[reqHeader->len_first_name];
        	char lname[reqHeader->len_last_name];
        	strncpy(fname, payload, reqHeader->len_first_name);
        	strncpy(lname, payload+sizeof(fname), reqHeader->len_last_name);


        	//get random joke
        	char joke[strlen(JOKE1)+reqHeader->len_first_name+reqHeader->len_last_name];
        	sprintf(joke, JOKE1, fname, lname);


        	/*create packet big enough for header+ joke*/
        	char response[(sizeof(response_header)+sizeof(joke))];
        	response_header resHeader;
        	resHeader.type=JOKER_RESPONSE_TYPE;
        	resHeader.len_joke=htonl(sizeof(joke));//make network byte order as client expects
        	memcpy(response, (char *)&resHeader, sizeof(response_header));
        	memcpy(response+sizeof(response_header), joke, sizeof(joke));

        	printf("%s\n", response);
        	/*send packet*/
        	if((len_send = send(fd, response, sizeof(response), 0))<0){
        	      perror("receive");
        	      return 1;
        	};
        }else{
        	 if((len_send = send(fd, "Malformed \n", 10, 0))<0){
        	       perror("receive");
        	       return 1;
        	 };
        }




        close(fd);
        close(s);
        return 0;
}


