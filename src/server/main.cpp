#include <stdio.h>
#include <string.h>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <direct.h>
#pragma comment(lib, "ws2_32.lib")


using namespace std;

int socket_desc, client_sock;
struct sockaddr_in server, client;

string backupFolderPath = "D:\\BackupServer\\";

int initSock() {
	WSADATA wsa_data;
	return WSAStartup(MAKEWORD(2, 2), &wsa_data);
}

int quitSock() {
	return WSACleanup();
}

int sockClose(int sock) {
	int status = 0;
	status = shutdown(sock, SD_BOTH);
	if (!status)
		status = closesocket(sock);
	return status;
}

void openServerSocket() {

	//Create socket
	socket_desc = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_desc == INVALID_SOCKET) {
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

void assembleFile(string fileName, string file_data, unsigned long fileLen, string nullIndexesChar, int nullIndexesSize) {
	
	//string => int[]
	int *nullIndexes = new int[nullIndexesSize];
	string num = "";
	int numbersConverted = 0;
	for (int i = 0; i < nullIndexesSize; i++) {
		if (nullIndexesChar[i] != '/')
			num += nullIndexesChar[i];
		else {
			int numInt = atoi(num.c_str());
			nullIndexes[numbersConverted++] = numInt;
			num = "";
		}
	}

	FILE *file = fopen((backupFolderPath + fileName).c_str(), "wb");
	if (!file) {
		printf("Could not open file for writing!\n");
		return;
	}
	int nullsPassed = 0;

	//замена 0, которых не было в файле, на \0
	for (int i = 0; i < fileLen; i++) {
		unsigned char c = file_data[i];
		if (c == '0') {
			int ind = nullIndexes[nullsPassed++];
			if (i == ind)
				c = '\0';
			else
				nullsPassed--;
		}
		fwrite((const char*)&c, 1, 1, file);
	}
	fclose(file);
}

void endFileTransmission() {
	
}

bool receivePack(string &file_data_string, int &nulls) {
	int read_size = 0;
	unsigned char binary_data[4096];
	memset(binary_data, '\0', 4096);
	while ((read_size = recv(client_sock, (char *)binary_data, 4096, 0)) > 0) {
		printf("Received %d bytes\n", read_size);
		string msg = string((char*)binary_data, read_size);
		if (msg.find("<NULLS>") != -1) {
			printf("Received <NULLS>\n");
			nulls = atoi(msg.substr(7).c_str());
			string reply = "<OK>";
			send(client_sock, reply.c_str(), reply.size() + 1, 0);
			return true;
		}
		file_data_string += msg;
		string reply = "<RECEIVED>";
		send(client_sock, reply.c_str(), reply.size() + 1, 0);
		memset(binary_data, '\0', 4096);
	}
	return false;
}

bool receivedNullIndexes(string &null_indexes_string) {
	int read_size = 0;
	char data[4096];
	memset(data, '\0', 4096);
	while ((read_size = recv(client_sock, data, 4096, 0)) > 0) {
		printf("Received %d bytes\n", read_size);
		string msg = string(data, read_size);
		if (msg.find("<END>") != -1) {
			printf("Received <END>\n");
			return true;
		}
		null_indexes_string += msg;
		memset(data, '\0', 4096);
	}
	return false;
}

void receiveFile(string &file_data, string &nullIndexesChar) {
	string reply = "<OK>";
	send(client_sock, reply.c_str(), reply.size() + 1, 0);

	int nulls = 0;
	while (!receivePack(file_data, nulls)) { ; }
	while (!receivedNullIndexes(nullIndexesChar)) { ; }
}

void listenToSocket() {
	listen(socket_desc, 1);
	char client_message[4200];
	int read_size = 0;
	//Accept incoming connection
	acceptClient();

	//Receive a message from client
	string fileName = "";
	memset(client_message, '\0', 4200);
	int len = 0;
	while ((read_size = recv(client_sock, client_message, 4097, 0)) > 0) {
		printf("Read %d\n", read_size);
		//Send the message back
		string msg = client_message;
		string reply;

		if (msg.find("<NAME>") != -1) {
			fileName = (string)msg.substr(6);
			printf("File name: %s\n", fileName.c_str());
			reply = "<OK>\0";
		}
		else if (msg.find("<LENGTH>") != -1) {
			len = atoi(msg.substr(8).c_str());
			printf("Length: %d\n", len);
			reply = msg;
		}
		else if (msg.find("<FILE>") != -1) {
			printf("File transmittion began\n");
			//unsigned char* file_data = new unsigned char[len];
			//char *nullIndexesChar;
			string file_data = "";
			string nullIndexesChar = "";
			receiveFile(file_data, nullIndexesChar);
			if (len != file_data.size())
				printf("File data length and length in header don't correspond\n");
			else
				assembleFile(fileName, file_data, (unsigned long)len, nullIndexesChar, nullIndexesChar.size());
			continue;
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

int main() {
	initSock();
	int c, read_size;
	if (_mkdir(backupFolderPath.c_str())) {
		if (errno != EEXIST) {
			printf("Could not create directory %s, backup files will be saved to C:/\n", backupFolderPath.c_str());
			backupFolderPath = "C:/";
			perror("mkdir");
		}
	}
	openServerSocket();
	while (true) {
		listenToSocket();
	}
	sockClose(socket_desc);
	quitSock();
	return 0;
}
