#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>

using namespace std;


int main () {
		int socket_desc, client_sock, c, read_size;
		struct sockaddr_in server, client;
		char client_message[4200];

		//Create socket
		socket_desc = socket(AF_INET, SOCK_STREAM, 0);
		if (socket_desc == -1) {
				printf("Could not create socket");
		}
		puts("Socket created");

		//Prepare the sockaddr_in struct
		server.sin_family = AF_INET;
		server.sin_addr.s_addr = INADDR_ANY;
		server.sin_port = htons(54002);

		//Bind
		if (bind(socket_desc, (struct sockaddr*)&server, sizeof(server)) < 0) {
				perror("bind failed. Error");
				return 1;
		}
		puts("bind done");
		while (true) {
				//Listen
				listen(socket_desc, 1);

				//Accept incoming connection
				puts("waiting for incoming connections...");
				c = sizeof(struct sockaddr_in);

				//Accept connection from an incoming client
				client_sock = accept(socket_desc, (struct sockaddr*)&client, (socklen_t*)&c);
				if (client_sock < 0) {
						perror("accept failed");
						return 1;
				}
				puts("Connection accepted");

				//Receive a message from client
				puts("Waiting for file transmittion to begin...\n");


				while ((read_size = recv(client_sock, client_message, 4097, 0)) > 0) {
						printf ("Read %d\n", read_size);
						//Send the message back
						printf("Received: %s \n", client_message);
						string msg = client_message;
						string reply;
						if (msg.find("<NAME>") != -1) {
								reply = "<OK>\0";
						}
						else if (msg.find("<LENGTH>") != -1) {
								reply = msg;
						}
						else if (msg.find("<FILE>") != -1) {
								printf("File transmittion began\n");
								reply = "<OK>\0";
								
								//
								send(client_sock, reply.c_str(), reply.size() + 1, 0);
								
								
								printf("sent reply\n");
								string data = "";
								printf("Waiting for packs...\n");
								
								
								while ((read_size = recv(client_sock, client_message, 4201, 0)) > 0) {
										
										printf("Received\n");
										msg = client_message;
										printf("%s \n", client_message);
										char buff[4096];
										memset(buff, '\0', 4096);
										data += (string)client_message;
										//
										reply = "<SIZE>" + strlen(client_message);
												
										//
										send(client_sock, reply.c_str(), reply.size() + 1, 0);
												
												
										puts("Waiting for packs...\n");
								}
						}

						//
						send(client_sock, reply.c_str(), reply.size() + 1, 0);
						
						
						memset(client_message, '\0', 4096);
				}

				if (read_size == 0) {
						puts("client disconnected");
						fflush(stdout);
				}
				else if (read_size == 1) {
						perror("recv failed");
				}
				else {
						puts("Waiting for file transmittion to begin...\n");
				}
		}
		int status = shutdown(socket_desc, SHUT_RDWR);
		if (!status)
				status = close(socket_desc);
		return 0;
}
