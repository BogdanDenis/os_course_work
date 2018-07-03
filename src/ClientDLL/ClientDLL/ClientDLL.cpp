// ClientDLL.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

#include <stdio.h>
#include <string.h>
#include <string>
#include <winsock2.h>
#include <WS2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

#define DLLExport __declspec(dllexport)
#define FuncExport extern "C" DLLExport
#define PACK_SIZE 4096

using std::string;
using std::to_string;

FuncExport int initSock() {
	WSADATA wsa_data;
	return WSAStartup(MAKEWORD(2, 2), &wsa_data);
}

FuncExport int quitSock() {
	return WSACleanup();
}

FuncExport int sockClose(SOCKET sock) {
	int status = shutdown(sock, SD_BOTH);
	if (!status)
		status = closesocket(sock);
	return status;
}

FuncExport bool openSocket(SOCKET &sock, const char*ip, int port) {
	struct sockaddr_in server;
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) {
		printf("Could not create socket!");
		return 0;
	}
	server.sin_addr.s_addr = inet_addr(ip);
	server.sin_family = AF_INET;
	server.sin_port = htons(port);

	if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
		perror("connection failed. Error");
		return 0;
	}
	return 1;
}

FuncExport bool sendData(SOCKET sock, const char* message) {
	if (send(sock, message, strlen(message), 0) < 0) {
		puts("Send failed");
		return 0;
	}
	return 1;
}

FuncExport string receiveReply(SOCKET sock) {
	char reply[PACK_SIZE];
	memset(reply, '\0', PACK_SIZE);

	if (recv(sock, reply, PACK_SIZE, 0) < 0) {
		printf("recv failed");
		return "";
	}
	return (string)(reply);
}

FuncExport int sendFile(SOCKET sock, const char* fileName, unsigned char* file_data, char *nullIndexes, unsigned long fileLen) {
	string msg = "<NAME>";
	msg += (string)fileName;
	sendData(sock, msg.c_str());
	string reply = receiveReply(sock); //should be <OK>

	if (reply == "<OK>") {
		printf("File name successfully sent\n");
		string fileLength = "<LENGTH>" + to_string(fileLen);

		sendData(sock, fileLength.c_str());
		reply = receiveReply(sock); // should be <LENGTH>[length]

		if (reply == fileLength) {
			printf("File length successfully sent\n");
			string msg = "<FILE>";

			sendData(sock, msg.c_str());
			reply = receiveReply(sock); //should be <OK>

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

					sendData(sock, pack);
					reply = receiveReply(sock); // should be <RECEIVED>	

					if (reply == "<RECEIVED>") {
						printf("Pack [%d - %d] successfully sent\n", bytes_sent - currently_sent, bytes_sent);
					}
					else {
						printf("Pack [%d - %d] was not sent\n", bytes_sent - currently_sent, bytes_sent);
					}
				}
				printf("File data was sent. Waiting for confirmation\n");
				msg = "<NULLS>" + to_string(strlen(nullIndexes));

				sendData(sock, msg.c_str());
				reply = receiveReply(sock); //should be <OK>

				if (reply == "<OK>") {
					puts("Whole file was successfully sent\n");
					bytes_sent = 0;
					while (bytes_sent != strlen(nullIndexes)) {
						char pack[PACK_SIZE];
						memset(pack, '\0', PACK_SIZE);
						int currently_sent = 0;
						if (strlen(nullIndexes) - bytes_sent >= PACK_SIZE - 1) {
							printf("Packing %d bytes of data\n", PACK_SIZE - 1);
							memcpy(pack, nullIndexes + bytes_sent, PACK_SIZE - 1);
							bytes_sent += PACK_SIZE - 1;
							currently_sent = PACK_SIZE - 1;
						}
						else {
							printf("Packing %d bytes of data\n", strlen(nullIndexes) - bytes_sent);
							memcpy(pack, nullIndexes + bytes_sent, strlen(nullIndexes) - bytes_sent);
							currently_sent = strlen(nullIndexes) - bytes_sent;
							bytes_sent += (strlen(nullIndexes) - bytes_sent);
						}
						pack[currently_sent] = '\0';

						printf("Sending %d bytes of data\n", currently_sent);

						sendData(sock, pack);
						reply = "";// receiveReply(sock); // should be <RECEIVED>

						if (reply == "<RECEIVED>") {
							printf("Pack of NULL [%d - %d] successfully sent\n", bytes_sent - currently_sent, bytes_sent);
						}
						else {
							printf("Pack of NULL [%d - %d] was not sent\n", bytes_sent - currently_sent, bytes_sent);
						}
					}
					sendData(sock, (const char*)"<END>");
					return 0;
				}
			}
			return 3;
		}
		return 2;
	}
	return 1;
}