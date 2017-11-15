// DataBackup.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "DataBackup.h"

#include <string.h>
#include <string>
#include <winsock2.h>
#include <WS2tcpip.h>

#include <vector>

#include <commdlg.h>

#pragma comment(lib, "Ws2_32.lib")

#define PACK_SIZE 4096

using std::string;
using std::to_string;
using std::vector;

#define MAX_LOADSTRING 100

#define EDIT_IP 1001
#define EDIT_PORT 1002
#define EDIT_DAYS 1003
#define EDIT_HOURS 1004
#define EDIT_MINUTES 1005
#define EDIT_SECONDS 1006
#define BTN_CONNECT 1007
#define BTN_DISCONNECT 1008
#define BTN_SELECT_FILE 1009
#define BTN_SEND_FILE 1010
#define BTN_SET_PERIOD 1011
#define LABEL_IP 1012
#define LABEL_PORT 1013
#define LABEL_DAYS 1014
#define LABEL_HOURS 1015
#define LABEL_MINUTES 1016
#define LABEL_SECONDS 1017

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

HMODULE ClientDll;
int(*initSock)(void);
int(*quitSock)(void);
int(*sockClose)(SOCKET);
bool(*openSock)(SOCKET*, const char*, int);
bool(*sendData)(SOCKET, const char*);
string(*receiveReply)(SOCKET);
int(*sendFile)(SOCKET, const char*, unsigned char*, char*, unsigned long);

SOCKET sock;

OPENFILENAME ofn;
string fileName;
unsigned char* file_data;
char *nullIndexesChar;
vector<string> directories = vector<string>();

unsigned char *GetFileData(string &fileName) {
	char path[1024];
	memset(path, '\0', 1024);
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;
	ofn.lpstrFile = path;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(path);
	ofn.lpstrFilter = "All\0*.*\0Text\0*.TXT\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	GetOpenFileName(&ofn);
	string filePath = path;
	if (!filePath.size())
		return NULL;
	int pos = filePath.find_last_of("\\");
	if (pos == -1)
		pos = filePath.find_last_of("/");
	fileName = filePath.substr(pos + 1);

	FILE *file = fopen(path, "rb");
	if (!file) {
		MessageBox(NULL, "Could not open the file", "", MB_OK);
	}
	fseek(file, 0, SEEK_END);
	unsigned long fileLen = 0;
	fileLen = ftell(file);
	rewind(file);

	unsigned char* file_data = new unsigned char[fileLen * sizeof(char) + 1];
	memset(file_data, '\0', fileLen + 1);
	if (!file_data) {
		MessageBox(NULL, "Memory error when allocating space for file data", "", MB_OK);
		return NULL;
	}

	int size = 100;
	int *nullIndexes = new int[size];
	memset(nullIndexes, 0, size);
	int nullsFound = 0;
	for (int i = 0; i < fileLen; i++) {
		unsigned char c;
		c = fgetc(file);
		if (c == '\0') {
			c = '0';
			if (nullsFound == size) {
				int *temp = new int[size * 2];
				memset(temp, 0, size * 2);
				memcpy(temp, nullIndexes, size * sizeof(int));
				size *= 2;
				//delete nullIndexes;
				nullIndexes = temp;
			}
			nullIndexes[nullsFound++] = i;
		}
		memcpy(file_data + i, (const void*)&c, 1);
	}
	fclose(file);

	int *nullIndexesShort = new int[nullsFound];
	memset(nullIndexesShort, 0, nullsFound);
	memcpy(nullIndexesShort, nullIndexes, nullsFound * sizeof(int));
	//delete nullIndexes;
	nullIndexes = nullIndexesShort;

	size = nullsFound;
	char *indexes = new char[size];
	memset(indexes, '\0', size);
	int symbolsCopied = 0;
	for (int i = 0; i < nullsFound; i++) {
		int index = nullIndexes[i];
		char buff[50];
		memset(buff, '\0', 50);
		itoa(index, buff, 10);
		int len = strlen(buff) + 1;
		if (symbolsCopied + len >= size) {
			char *temp = new char[size * 2];
			memset(temp, '\0', size * 2);
			strncpy(temp, indexes, symbolsCopied);
			size *= 2;
			//delete indexes;
			indexes = temp;
		}
		strncpy(indexes + symbolsCopied, buff, len - 1);
		symbolsCopied += len - 1;
		if (i != nullsFound - 1) {
			strncpy(indexes + symbolsCopied, "/", 1);
			symbolsCopied++;
		}
	}

	char *indexesShort = new char[symbolsCopied + 1];
	memset(indexesShort, '\0', symbolsCopied + 1);
	strncpy(indexesShort, indexes, symbolsCopied);
	indexesShort[symbolsCopied + 1] = '\0';
	//delete indexes;	somehow crashes
	nullIndexesChar = indexesShort;
	sendFile(sock, fileName.c_str(), file_data, nullIndexesChar, fileLen);

	//delete nullIndexes;

	return file_data;
}

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

string GetIpFromWindow() {
	HWND IP_EDIT = GetDlgItem(GetActiveWindow(), EDIT_IP);
	char text[20];
	GetWindowText(IP_EDIT, text, sizeof(text));
	return (string)text;
}

string GetPortFromWindow() {
	HWND PORT_EDIT = GetDlgItem(GetActiveWindow(), EDIT_PORT);
	char text[20];
	GetWindowText(PORT_EDIT, text, sizeof(text));
	return (string)text;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: Place code here.

	// Initialize global strings
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_DATABACKUP, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_DATABACKUP));

	MSG msg;

	ClientDll = ::LoadLibrary("ClientDll.dll");
	if (!ClientDll) {
		MessageBox(NULL, "Could not load ClientDll.dll library", "", MB_OK);
	}
	
	initSock = (int(*)(void)) ::GetProcAddress(ClientDll, "initSock");
	if (!initSock) {
		MessageBox(NULL, "Could not load initSock function", "", MB_OK);
	}
	quitSock = (int(*)(void)) ::GetProcAddress(ClientDll, "quitSock");
	if (!quitSock) {
		MessageBox(NULL, "Could not load quitSock function", "", MB_OK);
	}
	sockClose = (int(*)(SOCKET)) ::GetProcAddress(ClientDll, "sockClose");
	if (!sockClose) {
		MessageBox(NULL, "Could not load sockClose function", "", MB_OK);
	}
	openSock = (bool(*)(SOCKET*, const char*, int)) ::GetProcAddress(ClientDll, "openSocket");
	if (!openSock) {
		MessageBox(NULL, "Could not load openSock function", "", MB_OK);
	}
	sendData = (bool(*)(SOCKET, const char*)) ::GetProcAddress(ClientDll, "sendData");
	if (!sendData) {
		MessageBox(NULL, "Could not load sendData function", "", MB_OK);
	}
	receiveReply = (string(*)(SOCKET)) ::GetProcAddress(ClientDll, "receiveReply");
	if (!receiveReply) {
		MessageBox(NULL, "Could not load receiveReply function", "", MB_OK);
	}
	sendFile = (int(*)(SOCKET, const char*, unsigned char*, char*, unsigned long)) ::GetProcAddress(ClientDll, "sendFile");
	if (!sendFile) {
		MessageBox(NULL, "Could not load sendFile function", "", MB_OK);
	}

	initSock();

	// Main message loop:
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DATABACKUP));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_DATABACKUP);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // Store instance handle in our global variable

	HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, 500, 300, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		return FALSE;
	}

	HWND label_ip = CreateWindowEx(NULL, "STATIC", NULL, WS_CHILD | WS_VISIBLE, 50, 50, 20, 20, hWnd, (HMENU)LABEL_IP, GetModuleHandle(NULL), NULL);
	CreateWindowEx(NULL, "EDIT", NULL, WS_BORDER | WS_CHILD | WS_VISIBLE, 80, 50, 100, 20, hWnd, (HMENU)EDIT_IP, GetModuleHandle(NULL), NULL);
	HWND label_port = CreateWindowEx(NULL, "STATIC", NULL, WS_CHILD | WS_VISIBLE, 190, 50, 30, 20, hWnd, (HMENU)LABEL_PORT, GetModuleHandle(NULL), NULL);
	CreateWindowEx(NULL, "EDIT", NULL, WS_BORDER | WS_CHILD | WS_VISIBLE, 230, 50, 50, 20, hWnd, (HMENU)EDIT_PORT, GetModuleHandle(NULL), NULL);
	HWND btn_connect = CreateWindowEx(NULL, "BUTTON", NULL, WS_BORDER | WS_CHILD | WS_VISIBLE, 290, 50, 80, 20, hWnd, (HMENU)BTN_CONNECT, GetModuleHandle(NULL), NULL);
	HWND btn_disconnect = CreateWindowEx(NULL, "BUTTON", NULL, WS_BORDER | WS_CHILD | WS_VISIBLE, 380, 50, 90, 20, hWnd, (HMENU)BTN_DISCONNECT, GetModuleHandle(NULL), NULL);
	HWND btn_select_file = CreateWindowEx(NULL, "BUTTON", NULL, WS_BORDER | WS_CHILD | WS_VISIBLE, 50, 100, 90, 20, hWnd, (HMENU)BTN_SELECT_FILE, GetModuleHandle(NULL), NULL);
	HWND btn_send_file = CreateWindowEx(NULL, "BUTTON", NULL, WS_BORDER | WS_CHILD | WS_VISIBLE, 150, 100, 90, 20, hWnd, (HMENU)BTN_SEND_FILE, GetModuleHandle(NULL), NULL);
	
	SetWindowText(label_ip, "IP: ");
	SetWindowText(label_port, "Port: ");
	SetWindowText(btn_connect, "Connect");
	SetWindowText(btn_disconnect, "Disconnect");
	SetWindowText(btn_select_file, "Select file");
	SetWindowText(btn_send_file, "Send file");

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	string ip;
	string port;
	switch (message)
	{
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		case BTN_CONNECT:
			ip = GetIpFromWindow();
			ip = "127.0.0.1";
			port = GetPortFromWindow();
			port = "54000";
			if (openSock(&sock, ip.c_str(), atoi(port.c_str()))) {
				MessageBox(hWnd, "Connected", "", MB_OK);
			}
			else {
				MessageBox(hWnd, "Could not connect!", "", MB_OK);
			}
			break;
		case BTN_DISCONNECT:
			if (!sockClose(sock)) {
				MessageBox(hWnd, "Disconnected", "", MB_OK);
			}
			else {
				MessageBox(hWnd, "Could not disconnect!", "", MB_OK);
			}
			break;
		case BTN_SELECT_FILE:
			file_data = GetFileData(fileName);
			break;
		case BTN_SEND_FILE:
			if (file_data)
				sendFile(sock, fileName.c_str(), file_data, nullIndexesChar, sizeof(file_data));
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code that uses hdc here...
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}