/*
 * tcpserver.c
 *
 *  Created on: Apr 6, 2015
 *      Author: evgenijavstein
 */

// socket server example, handles multiple clients using threads

#include<stdio.h>
#include<string.h>    //strlen
#include<stdlib.h>    //strlen
#include<sys/socket.h>
#include<arpa/inet.h> //inet_addr
#include<unistd.h>    //write
#include<pthread.h> //for threading , link with lpthread

#define MAX_CONCURRENT_CLIENTS 2
//the thread function

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

void *connection_handler(void *);

int counter_concurrent_clients=0;
pthread_mutex_t lock;

int main(int argc , char *argv[])
{
    int listening_socket , client_socket , c , *new_sock;
    struct sockaddr_in server , client;
    int optval;
    //Create socket
    listening_socket = socket(AF_INET , SOCK_STREAM , 0);
    if (listening_socket == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");

    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(8000);

    // set SO_REUSEADDR on a socket to true (1):
    optval = 1;
    setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
    //Bind
    if( bind(listening_socket,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        perror("bind failed. Error");
        return 1;
    }
    puts("bind done");

    //Listen
    listen(listening_socket , 1);

    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);

    //prepare mutex which will be locked by a thread incrementing/decrementing the counter_concurrent_clients
    if (pthread_mutex_init(&lock, NULL) != 0)
    {
         printf("\n mutex init failed\n");
         return 1;
    }


   //create thread for each incoming socket connection
   //accept only MAX_CONCURRENT_CLIENTS incomming sockets

   do{
	   if(counter_concurrent_clients<MAX_CONCURRENT_CLIENTS){

		   client_socket=accept(listening_socket,(struct sockaddr*)&client,(socklen_t*)&c);
		   //increment for each new accepted socket to client,
		   //decrement before  exit of thread
		   increment_concurrent_clients();
		   printf("Connection accepted : %s concurrent clients: %d",inet_ntoa(client.sin_addr), counter_concurrent_clients);

		   if (client_socket < 0)
		   {
			   perror("accept failed");
			   return 1;
		   }
		   pthread_t sniffer_thread;

		   new_sock = malloc(1);
		   *new_sock = client_socket;

		   //pass a socket to each handler of each thread
		   if( pthread_create( &sniffer_thread , NULL ,  connection_handler , (void*) new_sock) < 0)
		   {
			   perror("could not create thread");
			   return 1;
		   }

		   puts("Handler assigned");
	   }

   }while(counter_concurrent_clients>0);

    pthread_mutex_destroy(&lock);
    close(listening_socket);
    return 0;
}

void increment_concurrent_clients(){
	pthread_mutex_lock(&lock);
	counter_concurrent_clients++;
	pthread_mutex_unlock(&lock);


}
void decrement_concurrent_clients(){
	pthread_mutex_lock(&lock);
	counter_concurrent_clients--;
	pthread_mutex_unlock(&lock);

}

/*
  This will handle connection for each client:
  - recieves a packet of type JOKER_REQUEST_TYPE
  - reads the name and send back a packet of type JOKER_RESPONSE_TYPE

  */
void *connection_handler(void *socket_desc)
{
    //Get the socket descriptor
    int sock = *(int*)socket_desc;
    int len_send;

      char buf[BUFSIZ];

      while((len_send=recv(sock,buf,BUFSIZ,0))>0)
      {

		printf("%s\n", buf);

		request_header * reqHeader = (request_header *) buf;
		char * payload = buf + sizeof(request_header);

		/** if a packet of type JOKER_REQUEST_TYPE arrives send a joke back**/
		if (reqHeader->type == JOKER_REQUEST_TYPE) {

			char *fname = malloc(reqHeader->len_first_name);
			char *lname = malloc(reqHeader->len_last_name);
			strncpy(fname, payload, reqHeader->len_first_name);
			strncpy(lname, payload + strlen(fname), reqHeader->len_last_name);
			//get random joke
			char joke[strlen(JOKE1) + reqHeader->len_first_name
					+ reqHeader->len_last_name];
			sprintf(joke, JOKE1, fname, lname);
			/*create packet big enough for header+ joke*/
			char response[(sizeof(response_header) + sizeof(joke))];
			response_header resHeader;
			resHeader.type = JOKER_RESPONSE_TYPE;
			resHeader.len_joke = htonl(sizeof(joke)); //make network byte order as client expects
			memcpy(response, (char *) &resHeader, sizeof(response_header));
			memcpy(response + sizeof(response_header), joke, sizeof(joke));
			printf("%s\n", response);
			/*send packet*/
			if ((len_send = send(sock, response, sizeof(response), 0)) < 0) {
				perror("receive");
				return 0;
			};
			printf("packet sent %s", response);
		/*else respond that the request is malformed*/
		} else {

			//if ((len_send = send(fd, "Malformed \n", 10, 0)) < 0) {
			if ((len_send = send(sock, buf, 10, 0)) < 0) {
				perror("receive");
				return 0;
			};
		}

      }
      close(sock);

      if(len_send==0)
      {
        puts("Client Disconnected");
      }
      else
      {
        perror("recv failed");
      }
      //this way new sockets can be accepted
      decrement_concurrent_clients();
    return 0;
}
