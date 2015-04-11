/*
 * tcpserver.c
 *
 *  Created on: Apr 6, 2015
 *      Author: evgenijavstein
 */

// socket server example, handles multiple clients using threads

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<pthread.h>
#include "tcpserver.h"
char * getRandomJoke(char* fname, char* lname,uint8_t fname_len, uint8_t lname_len);

int counter_concurrent_clients=0; /* global thread counter*/
pthread_mutex_t lock; /*global lock */

int main(int argc , char *argv[])
{
	pthread_t thread_id;/*thread id for each created child thread*/
	int server_socket , client_socket /*socket descriptors: client, server*/;


    server_socket=create_server_socket(8000);
    puts("Waiting for incoming connections...");

    //prepare mutex which will be locked by a thread incrementing/decrementing the counter_concurrent_clients
    if (pthread_mutex_init(&lock, NULL) != 0)
    {
         printf("\n mutex init failed\n");
         return 1;
    }

   //create thread for each incoming socket connection
   //accept only MAX_CONCURRENT_CLIENTS incoming sockets
   do{
	   if(counter_concurrent_clients<MAX_CONCURRENT_CLIENTS){

		   client_socket=accept_connection(server_socket);
		   printf("conncurrent clients:%d \n", counter_concurrent_clients);

		   thread_args *arg=(thread_args*)malloc(sizeof(thread_args));
		   arg->client_socket=client_socket;
		   arg->thread_no=counter_concurrent_clients;

		   //pass a socket to each handler of each thread
		   if( pthread_create( &thread_id , NULL ,  connection_handler , (void*) arg) < 0)
		   {
			   perror("could not create thread");
			   return 1;
		   }
		   //increment for each new accepted socket to client,
		   //let child thread decrement before  exit
		   increment_concurrent_clients();
		   puts("Handler assigned");
	   }

   }while(counter_concurrent_clients>0);

   //REACHED ONLY IF all clients disconnect
    pthread_mutex_destroy(&lock);
    close(server_socket);
    puts("server shut down");
    return 0;
}


/**
 * This will handle connection for each client off main thread
 * Tries to read  a packet of type JOKER_REQUEST_TYPE
 * Tries to read the name and send back a packet of type JOKER_RESPONSE_TYPE
 * @param arg  pointer to  threads_args
 * @see threada_args
 */
void *connection_handler(void * arg)
{
    //Get the socket descriptor
    int sock = ((thread_args*)arg)->client_socket;
    int no=((thread_args*)arg)->thread_no;
    //free struct, since values are copied
    free(arg);

    printf("thread no %d launched \n",no );

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
			//char joke[strlen(JOKE1) + reqHeader->len_first_name
				//	+ reqHeader->len_last_name];
			char * joke=getRandomJoke(fname, lname,reqHeader->len_first_name,reqHeader->len_last_name);
			//sprintf(joke, JOKE1, fname, lname);
			/*create packet big enough for header+ joke*/
			char response[(sizeof(response_header) + strlen(joke))];
			response_header resHeader;
			resHeader.type = JOKER_RESPONSE_TYPE;
			resHeader.len_joke = htonl(strlen(joke)); //make network byte order as client expects
			memcpy(response, (char *) &resHeader, sizeof(response_header));
			memcpy(response + sizeof(response_header), joke, strlen(joke));
			printf("%s\n", response);
			/*send packet*/
			//sleep(5);timeout test
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
      printf("thread no %d :termination", no);
    return 0;
}


/**
 * increments the counter_concurrent_clients global while
 * locking access for other threads
 */
void increment_concurrent_clients(){
	pthread_mutex_lock(&lock);
	counter_concurrent_clients++;
	pthread_mutex_unlock(&lock);
}

/**
 * decrements the counter_concurrent_clients global while
 * locking access for other threads
 */
void decrement_concurrent_clients(){
	pthread_mutex_lock(&lock);
	counter_concurrent_clients--;
	pthread_mutex_unlock(&lock);
}

char * getRandomJoke(char* fname, char* lname,uint8_t fname_len, uint8_t lname_len){

	const char *jokes[10];
		jokes[0] = JOKE0;
		jokes[1] = JOKE1;
		jokes[2] = JOKE2;
		jokes[3] = JOKE3;
		jokes[4] = JOKE4;
		jokes[5] = JOKE5;
		jokes[6] = JOKE6;
		jokes[7] = JOKE7;
		jokes[8] = JOKE8;
		jokes[9] = JOKE9;

	int n = rand() % 9 + 1;
	//int b=rand() % 1 + 1;

	char * c=(char *)malloc((strlen(jokes[n])+fname_len+lname_len));



	sprintf(c,jokes[n], fname, lname);

	return c;
}


