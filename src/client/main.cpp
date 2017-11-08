#include <stdio.h>
#include <string.h>
#include <string>

using namespace std;

#ifdef _WIN32
		#ifndef _WIN32_WINNT
				#define _WIN32_WINNT 0x0501 /*Windows XP*/
		#endif
		#include <winsock2.h>
		#include <WS2tcpip.h>

		#pragma comment(lib, "ws2_32.lib")

		typedef SOCKET int
#else
		#include <sys/socket.h>
		#include <arpa/inet.h>
		#include <netdb.h>
		#include <unistd.h>
#endif

bool sockCreated(int sock) {
		#ifdef _WIN32
				return sock != INVALID_SOCKET;
		#else
				return sock != -1;
		#endif
}

int initSock() {
		#ifdef _WIN32
				WSADATA wsa_data;
				return WSAStartup(MAKEWORD(2,2), &wsa_data);
		#else
				return 0;
		#endif
}

int quitSock() {
		#ifdef _WIN32
				return WSACleanup();
		#else
				return 0;
		#endif
}

int sockClose(int sock) {
		int status = 0;
		#ifdef _WIN32
				status = shutdown(sock,	SD_BOTH);
				if (!status)
						status = closesocket(sock);
		#else
				status = shutdown(sock, SHUT_RDWR);
				if (!status)
						status = close(sock);
		#endif
		return status;
}

#define PACK_SIZE 4096

int sock;

bool sendData(const char* message, int len) {
		if (len < PACK_SIZE) {
				if (send(sock, message, len, 0) < 0) {
						puts("Send failed");
						return 1;
				}
		}
}

bool sendData(const unsigned char* message, int len) {
		if (len < PACK_SIZE) {
				if (send(sock, message, len, 0) < 0) {
						printf("Send failed\n");
						return 0;
				}
		}
}

string receiveReply() {
		char reply[PACK_SIZE];
		memset(reply, '\0', PACK_SIZE);

		if (recv(sock, reply, PACK_SIZE, 0) < 0) {
				printf("recv failed");
				return "";
		}
		return (string)(reply);
}


unsigned char* getFileData(string filePath, unsigned long &fileLen) {
		FILE *file = fopen(filePath.c_str(), "rb");
		fseek(file, 0 , SEEK_END);
		fileLen = ftell(file);
		unsigned char *file_data;
		rewind(file);
		file_data = new unsigned char[fileLen * sizeof(char)];
		if (!file_data) {
				printf("Memory error");
				return NULL;
		}
		fread(file_data, fileLen, 1, file);
		fclose(file);
		return file_data;
}

int sendFile() {
		char tmp[1024];
		string filePath;
		printf("Input file path: ");
		scanf("%s", tmp);
		filePath = tmp;
		
		unsigned long fileLen;
		unsigned char* file_data = getFileData(filePath, fileLen);

		string fileName = "<NAME>" + filePath.substr(filePath.find_last_of("/") + 1);
		
		///   1   ///

		sendData(fileName.c_str(), fileName.size() + 1);
		
		///   2   ///
		
		string reply = receiveReply(); //should be <OK>
		
		if (reply == "<OK>") {
				printf("File name successfully sent\n");

				string fileLength = "<LENGTH>" + to_string(fileLen);

				///   6   ///

				sendData(fileLength.c_str(), fileLength.size() + 1);
				
				///   7   ///

				reply = receiveReply(); // should be <LENGTH>[length]

				if (reply == fileLength) {
						printf("File length successfully sent\n");
						
						string msg = "<FILE>";
						
						///   10   ///
						
						sendData(msg.c_str(), msg.size() + 1);
						
						///   11   ///

						reply = receiveReply(); //should be <OK>
						
						
						if (reply == "<OK>") {
								printf("File transmittion began\n");

								int bytes_sent = 0;
								int len = (int)(fileLen) * sizeof(char);
								printf("File length: %d bytes\n", len);

								while (bytes_sent != len) {
										char pack[PACK_SIZE];
										memset(pack, '\0', PACK_SIZE);
										int currently_sent = 0;
										if (len - bytes_sent >= PACK_SIZE - 1) {
												printf("Packing %d bytes of data\n", PACK_SIZE - 1);
												memcpy(pack, file_data + bytes_sent, PACK_SIZE - 1);
												bytes_sent += PACK_SIZE - 1;
												currently_sent = PACK_SIZE - 1;
										}
										else {
												printf("Packing %d bytes of data\n", len - bytes_sent);
												memcpy(pack, file_data + bytes_sent, len - bytes_sent);
												currently_sent = len - bytes_sent;
												bytes_sent += (len - bytes_sent);
										}
										pack[currently_sent] = '\0';
										
										printf("Sending %d bytes of data\n", currently_sent);
										
										///   14   ///

										sendData(pack, currently_sent);
										///   15   ///

										reply = receiveReply(); // should be <SIZE>[currently_sent]

										if (reply == "<SIZE>" + to_string(currently_sent)) {
												printf("Pack [%d - %d] successfully sent\n", bytes_sent - currently_sent, bytes_sent);
										}
										else {
												printf ("Pack [%d - %d] was not sent\n", bytes_sent - currently_sent, bytes_sent);
										}
								}
								printf("File was sent. Waiting for confirmation\n");
								msg = "<END>\0";
								
								
								///   18   ///

								sendData(msg.c_str(), msg.size() + 1);
								
								///   19   ///
								
								reply = receiveReply(); //should be <OK>
								
								
								if (reply == "<OK>") {
										puts("Whole file was successfully sent\n");
								}
						}
						
						return 0;
				}
		}
}

int main () {
		initSock();
		struct sockaddr_in server;
		char message[PACK_SIZE], reply[PACK_SIZE];

		//Create socket
		sock = socket(AF_INET, SOCK_STREAM, 0);
		if (!sockCreated(sock)) {
				printf("Could not create socket!");
		}
		puts("Socket created");
		server.sin_addr.s_addr = inet_addr("127.0.0.1");
		server.sin_family = AF_INET;
		server.sin_port = htons(54000);

		//Connect to remote server
		if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
				perror("connection failed. Error");
				return 1;
		}

		puts("Connected");

		//Communicate with server
		while (true) {
				/*printf("Enter message: ");
				memset(message, '\0', 4096);
				scanf("%s", message);

				if (send(sock, message, strlen(message), 0) < 0) {
						puts("Send failed");
						return 1;
				}

				memset(reply, '\0', 4096);

				if (recv(sock, reply, 4096, 0) < 0) {
						puts("recv failed");
						break;
				}

				puts("Server reply: ");
				puts(reply);*/
				sendFile();
		}

		sockClose(sock);
		quitSock();

		return 0;
}
