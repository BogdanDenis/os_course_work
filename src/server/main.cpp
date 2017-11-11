#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>

using namespace std;

int socket_desc, client_sock;
struct sockaddr_in server, client;

void openServerSocket() {

		//Create socket
		socket_desc = socket(AF_INET, SOCK_STREAM, 0);
		if (socket_desc == -1) {
				printf("Could not create socket");
		}
		puts("Socket created");

		//Prepare the sockaddr_in struct
		server.sin_family = AF_INET;
		server.sin_addr.s_addr = INADDR_ANY;
		server.sin_port = htons(54000);

		//Bind
		if (bind(socket_desc, (struct sockaddr*)&server, sizeof(server)) < 0) {
				perror("bind failed. Error");
				exit(1);
		}
		puts("bind done");
}

void acceptClient() {
		puts("waiting for incoming connections...");
				int c = sizeof(struct sockaddr_in);

				//Accept connection from an incoming client
				client_sock = accept(socket_desc, (struct sockaddr*)&client, (socklen_t*)&c);
				if (client_sock < 0) {
						perror("accept failed");
						exit(1);
				}
				puts("Connection accepted");
}

void listenToSocket() {
		listen(socket_desc, 1);
				char client_message[4200];
				int read_size = 0;
				//Accept incoming connection
				acceptClient();		

				//Receive a message from client
				puts("Waiting for file transmittion to begin...\n");
				string fileName = "";
				while ((read_size = recv(client_sock, client_message, 4097, 0)) > 0) {
						printf ("Read %d\n", read_size);
						//Send the message back
						string msg = client_message;
						string reply;
						int len = 0;
						///   3   ///

						if (msg.find("<NAME>") != -1) {
								
								///   4   ///
								fileName = (string)msg.substr(6);
								printf("File name: %s\n", fileName.c_str());
								reply = "<OK>\0";
						}
						else if (msg.find("<LENGTH>") != -1) {
						
								///   8   ///
								len = atoi(msg.substr(8).c_str());
								printf("Length: %d\n", len);
								reply = msg;
						}
						else if (msg.find("<FILE>") != -1) {
								printf("File transmittion began\n");
								reply = "<OK>\0";
								

								///   12   ///

								send(client_sock, reply.c_str(), reply.size() + 1, 0);
								
								
								printf("sent reply\n");
								string data = "";
								printf("Waiting for packs...\n");
								
								///   13, 17   ///
								
								unsigned char binary_data[4096];
								memset(binary_data, '\0', 4096);

								int received = 0;
								FILE *file = fopen(fileName.c_str() , "wb");
								if (!file) {
								//		ferror(file);
										printf("Could not create or open file\n");
								}
								while ((read_size = recv(client_sock, binary_data, 4096, 0)) > 0) {
										printf("Received %d bytes\n", read_size);
										if ((string)((const char*)(binary_data)) == "<END>") {
												printf("END received\n");
												reply = "<OK>";
												send(client_sock, reply.c_str(), reply.size() + 1, 0);
												break;
										}
										else {
												//printf("Copying %d bytes of data\n", read_size - 1);
												//memcpy(&file_data[received], binary_data, read_size - 1);
												//printf("Copied %d bytes of data\n", read_size - 1);
												fwrite(binary_data, 1, read_size, file);
												received += read_size - 1;
												//printf("Received: %d\n", received);
										}
										
										//
										reply = "<RECEIVED>";
												
										
										///   16   ///

										send(client_sock, reply.c_str(), reply.size() + 1, 0);
								}
								printf("Received %d bytes of data in total\n", received);
								//FILE *file = fopen(fileName.c_str(), "bw");
								//fwrite(file_data, sizeof(unsigned char), len, file);
						}

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

int main () {
		int c, read_size;
		
		
		openServerSocket();

		while (true) {
				listenToSocket();
		}
		int status = shutdown(socket_desc, SHUT_RDWR);
		if (!status)
				status = close(socket_desc);
		return 0;
}
