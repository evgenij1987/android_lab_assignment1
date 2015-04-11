/*
 * connection.c
 *
 *  Created on: Apr 11, 2015
 *  Author: evgenijavstein
 */
#include<sys/socket.h>
#include<arpa/inet.h>
#include<stdio.h>
#include<stdlib.h>

/**
 * Creates listening socket on port
 * @param port to bind to the socket
 * @return socket description of server socket
 */
int create_server_socket(int port){
	int server_socket;
	struct sockaddr_in server;
	int optval;
	    //Create socket
	    server_socket = socket(AF_INET , SOCK_STREAM , 0);
	    if (server_socket == -1)
	    {
	        printf("Could not create socket");
	    }
	    puts("Socket created");

	    //Prepare the sockaddr_in structure
	    server.sin_family = AF_INET;
	    server.sin_addr.s_addr = INADDR_ANY;
	    server.sin_port = htons(port);

	    // set SO_REUSEADDR on a socket to true (1):
	    optval = 1;
	    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
	    //Bind
	    if( bind(server_socket,(struct sockaddr *)&server , sizeof(server)) < 0)
	    {
	        //print the error message
	        perror("bind failed. Error");
	        return 1;
	    }
	    puts("bind done");

	    //Listen
	    listen(server_socket , 1);

	    //Accept and incoming connection

	    return server_socket;

}



/**
 * User listening socket server_socket to accept a new client socket
 * @param listening socket
 * @return accepted client socket
 */
int accept_connection(int server_socket){

	int client_socket;/* socket for client*/
	struct sockaddr_in client_adress;/* client adress*/
	int adress_len=sizeof(struct sockaddr_in);/* lenght of sockaddr_in structre*/

	client_socket=accept(server_socket,(struct sockaddr*)&client_adress,(socklen_t*)&adress_len);

	printf("Connection accepted : %s \n",inet_ntoa(client_adress.sin_addr));


	if (client_socket < 0)
	{
		perror("accept failed");
		return 1;
	}
    return client_socket;
}
