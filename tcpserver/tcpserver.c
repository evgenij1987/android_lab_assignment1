/*
 * tcpserver.c
 *
 *  Created on: Apr 6, 2015
 *      Author: evgenijavstein
 */

// socket server example, handles multiple clients using threads
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdbool.h>

#include "tcpserver.h"
#include "connection.c"

#define OVER_EXICTED_HASH_LENGHT 10

size_t get_random_joke(char* fname, char* lname, uint8_t fname_len, uint8_t lname_len, char ** ppjoke);
char * randstring(size_t length);
int counter_concurrent_clients = 0; //global thread counter
pthread_mutex_t lock; //global lock

int main(int argc, char *argv[]) {
	pthread_t thread_id; //thread id for each created child thread
	int server_socket, client_socket; //socket descriptors: client, server

	server_socket = create_server_socket();
	if (server_socket == -1) {
		fprintf(stderr, "Error in socket setup.\n");
		return 1;
	}

	puts("Waiting for incoming connections...");

	//prepare mutex which will be locked by a thread incrementing/decrementing the counter_concurrent_clients
	if (pthread_mutex_init(&lock, NULL) != 0) {
		printf("\n mutex init failed\n");
		return 1;
	}

	//create thread for each incoming socket connection
	//accept only MAX_CONCURRENT_CLIENTS incoming sockets
	do {
		if (counter_concurrent_clients < MAX_CONCURRENT_CLIENTS) {
			client_socket = accept_connection(server_socket);
			if (client_socket == -1) {
				fprintf(stderr, "Could not accept connection from socket %d.\n", server_socket);
				continue;
			}
			printf("Concurrent clients: %d \n", counter_concurrent_clients);

			thread_args *arg = (thread_args*) malloc(sizeof(thread_args));
			arg->client_socket = client_socket;
			arg->thread_no = counter_concurrent_clients;

			//pass a socket to each handler of each thread
			if (pthread_create(&thread_id, NULL, connection_handler, (void*) arg) < 0) {
				perror("Could not create thread");
				return 1;
			}
			//increment for each new accepted socket to client,
			//let child thread decrement before  exit
			increment_concurrent_clients();
			puts("Handler assigned");
		}

	} while (true);

	pthread_mutex_destroy(&lock);
	close(server_socket);
	puts("Server shut down");
	return 0;
}

/**
 * This will handle connection for each client off main thread
 * Tries to read  a packet of type JOKER_REQUEST_TYPE
 * Tries to read the name and send back a packet of type JOKER_RESPONSE_TYPE
 * @param arg  pointer to  threads_args
 * @see threada_args
 */
void *connection_handler(void * arg) {
	//info from client
	int recv_bytes_for_header;
	int recv_bytes_for_first_name;
	int recv_bytes_for_last_name;
	request_header client_header;
	uint8_t len_of_first_name = 0;
	uint8_t len_of_last_name = 0;
	char *first_name_buffer;
	char *last_name_buffer;

	//info from server
	int response_struct_size;
	response_header *response_msg;

	//Get the socket descriptor
	int socketfd = ((thread_args*) arg)->client_socket;
	int thread_number = ((thread_args*) arg)->thread_no;
	//free struct, since values are copied
	free(arg);

	printf("Thread no %d launched \n", thread_number);

	recv_bytes_for_header = recv(socketfd, (char *) &client_header, sizeof(request_header), 0);

	int request_header_size = sizeof(request_header);
	if (recv_bytes_for_header < request_header_size || client_header.type != JOKER_REQUEST_TYPE) {
		close(socketfd);
		fprintf(stderr, "A client did not respond with the proper header.\n");
	}

	len_of_first_name = client_header.len_first_name;
	len_of_last_name = client_header.len_last_name;

	first_name_buffer = (char *) malloc(len_of_first_name);
	last_name_buffer = (char *) malloc(len_of_last_name);
	recv_bytes_for_first_name = recv(socketfd, first_name_buffer, len_of_first_name, 0);
	first_name_buffer[len_of_first_name] = '\0';
	recv_bytes_for_last_name = recv(socketfd, last_name_buffer, len_of_last_name, 0);
	last_name_buffer[len_of_last_name] = '\0';

	if (recv_bytes_for_first_name < len_of_first_name || recv_bytes_for_last_name < len_of_last_name) {
		close(socketfd);
		fprintf(stderr, "The first and last names were not fully received from the client.\n");
	}

	printf("Client data received correctly. \n");
	printf("%s \n", first_name_buffer);
	printf("%s \n", last_name_buffer);

	//send a joke to the client
	char joke[] = "Always and forever %s %s googles with regexps.";
	char *joke_with_junk;
	size_t pure_joke_len = get_random_joke(first_name_buffer, last_name_buffer, len_of_first_name, len_of_last_name,
			&joke_with_junk);

	free(first_name_buffer);
	free(last_name_buffer);
	printf("Sent to the client: %s\n", joke_with_junk);
	uint32_t joke_length = strlen(joke_with_junk);

	response_struct_size = sizeof(response_header);
	int buffer_size = response_struct_size + joke_length;
	char *buffer = (char *) malloc(buffer_size);
	response_msg = (response_header *) buffer;
	response_msg->type = JOKER_RESPONSE_TYPE;
	response_msg->len_joke = htonl(pure_joke_len);

	char *payload = buffer + response_struct_size;
	strncpy(payload, joke_with_junk, joke_length);

	int bytes_sent = send(socketfd, buffer, buffer_size, 0);
	free(buffer);
	free(joke_with_junk);
	if (bytes_sent == -1) {
		printf("Error while sending joke");
	}

	decrement_concurrent_clients();
	printf("Thread no %d :termination", thread_number);
	return 0;
}

/**
 * increments the counter_concurrent_clients global while
 * locking access for other threads
 */
void increment_concurrent_clients() {
	pthread_mutex_lock(&lock);
	counter_concurrent_clients++;
	pthread_mutex_unlock(&lock);
}

/**
 * decrements the counter_concurrent_clients global while
 * locking access for other threads
 */
void decrement_concurrent_clients() {
	pthread_mutex_lock(&lock);
	counter_concurrent_clients--;
	pthread_mutex_unlock(&lock);
}
/**
 * Chooses one from 10 jokes randomly.
 * Joke is allocated  ppjoke pointer will be pointed to it.
 * Random character are appended to the joke string. But function returns
 * the lenght of the pure joke
 * @param fname
 * @param lname
 * @param fname_len
 * @param lname_len
 * @param ppjoke pointer to a pointer pointing to NULL so far
 * @return pure joke lenght without random characters
 */
size_t get_random_joke(char* fname, char* lname, uint8_t fname_len, uint8_t lname_len, char ** ppjoke) {

	const char *jokes[10] = { JOKE0, JOKE1, JOKE2, JOKE3, JOKE4, JOKE5, JOKE6, JOKE7, JOKE8, JOKE9 };

	int n = rand() % 9 + 1;
	srand(time(NULL));
	int b = rand() % 2;

	size_t overexcitement_hash_len;

	if (b == 0) {
		overexcitement_hash_len = 0;
	} else {
		overexcitement_hash_len = OVER_EXICTED_HASH_LENGHT;

	}
	//let second pointer, which is pointing to NULL point now to newly allocated memory
	*ppjoke = (char *) malloc((strlen(jokes[n]) + fname_len + lname_len + overexcitement_hash_len));

	//now inhabit newly allocated memory to which second pointer is pointing
	sprintf((char *) *ppjoke, jokes[n], fname, lname);

	//joke length without junk
	size_t joke_len = strlen(*ppjoke);

	//append junk randomly
	if (overexcitement_hash_len > 0) {
		char *hash = randstring(overexcitement_hash_len);
		strcat(*ppjoke, hash);
		free(hash);
	}
	return joke_len;
}

char * randstring(size_t length) {

	static char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789,.-#'?!";
	char *randomString = NULL;

	if (length) {
		randomString = malloc(sizeof(char) * (length + 1));

		if (randomString) {
			int n;
			for (n = 0; n < length; n++) {
				int key = rand() % (int) (sizeof(charset) - 1);
				randomString[n] = charset[key];
			}

			randomString[length] = '\0';
		}
	}

	return randomString;
}

