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

int sock;

bool sendData(const char* message, int len) {
		if (len < 4096) {
				if (send(sock, message, len, 0) < 0) {
						puts("Send failed");
						return 1;
				}
		}
}

string receiveReply() {
		char reply[4096];
		memset(reply, '\0', 4096);

		if (recv(sock, reply, 4096, 0) < 0) {
				printf("recv failed");
				return "";
		}
		return (string)(reply);
}

int sendFile() {
		char tmp[1024];
		string filePath;
		printf("Input file path: ");
		scanf("%s", tmp);
		filePath = tmp;

		FILE *file = fopen(filePath.c_str(), "rb");
		fseek(file, 0 , SEEK_END);
		unsigned long fileLen = ftell(file);
		char *file_data;
		rewind(file);
		file_data = new char[fileLen * sizeof(char)];
		if (!file_data) {
				printf("Memory error");
				return 1;
		}
		int num_read = 0;
		char s;
		while ((num_read = fread(&s, 1, 1, file))) {
				strncat(file_data, &s, 1);
		}
		fclose(file);
		


		string fileName = "<NAME>" + filePath.substr(filePath.find_last_of("/") + 1);
		//Send file name
		sendData(fileName.c_str(), fileName.size() + 1);
		string reply = receiveReply(); //should be <OK>
		printf("Reply: %s\n", reply.c_str());
		if (reply == "<OK>") {
				printf("File name successfully sent\n");

				string fileLength = "<LENGTH>" + to_string(fileLen);
				//Send file length
				sendData(fileLength.c_str(), fileLength.size() + 1);
				reply = receiveReply(); // should be <LENGTH>[length]
				printf("Data: %s, reply: %s\n", fileLength.c_str(), reply.c_str());
				if (reply == fileLength) {
						printf("File length successfully sent\n");
						
						string msg = "<FILE>";
						
						//
						sendData(msg.c_str(), msg.size() + 1);
						
						
						printf("Listening...\n");
						
						//
						reply = receiveReply(); //should be <OK>
						
						
						if (reply == "<OK>") {
								printf("File transmittion began\n");
								int bytes_sent = 0;
								int len = (int)(fileLen) * sizeof(char);
								
								while (bytes_sent != len) {
										printf("%d bytes sent\n", bytes_sent);
										char pack[4096];
										memset(pack, '\0', 4096);
										int currently_sent = 0;
										if (len - bytes_sent >= 4095) {
												strncpy(pack, file_data + bytes_sent, 4095);
												bytes_sent += 4095;
												currently_sent = 4095;
										}
										else {
												strncpy(pack, file_data + bytes_sent, len - bytes_sent);
												bytes_sent += (len - bytes_sent);
												currently_sent = len - bytes_sent;
										}
										
										//
										sendData("ASDADSAD\0", 4096);
										
										//
										reply = receiveReply(); // should be <SIZE>[currently_sent]

										if (reply == "<SIZE>" + to_string(currently_sent)) {
												printf("Pack [%d - %d] successfully sent", bytes_sent - currently_sent, bytes_sent);
										}
										else {
												printf ("Pack [%d - %d] was not sent", bytes_sent - currently_sent, bytes_sent);
										}
								}
								msg = "<END>";
								
								//
								sendData(msg.c_str(), msg.size() + 1);
								
								//
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
		char message[4096], reply[4096];

		//Create socket
		sock = socket(AF_INET, SOCK_STREAM, 0);
		if (!sockCreated(sock)) {
				printf("Could not create socket!");
		}
		puts("Socket created");
		server.sin_addr.s_addr = inet_addr("127.0.0.1");
		server.sin_family = AF_INET;
		server.sin_port = htons(54002);

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
