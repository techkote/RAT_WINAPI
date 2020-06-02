#include "winsock2.h"
#include "intrin.h"
#include "ntdll.h"
#include "iphlpapi.h"
#include "stdio.h"
#include <string>
#include <iostream>

#pragma comment(lib, "iphlpapi")
#pragma comment(lib, "ws2_32")
#pragma warning(disable : 4996)

using namespace std;

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
	char URL[100];
};

//полностью копируем структуры из сервера
struct CMDiDATA
{
	DWORD CMD; //тут у нас будут номера комманд
	char DATA[1000]; //а тут данные которые мы хотим принять или отправить серверу
};

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

DWORD WINAPI ConsoleReader(LPVOID param)
{
	while (TRUE)
	{
		string line = { 0 }; //инициализируем класс стринг
		getline(cin, line); //считываем символы в этот класс из консоли
		char* mycmd = strdup(line.c_str()); //конвертируем эти сиволы в чары, вызывая метод с_str

		if (mycmd[0] != 0)
		{
			int i, j;
			char CMDCHAR[sizeof(int)];
			memset(CMDCHAR, 0, sizeof(CMDCHAR));
			int CMD = 0;
			for (i = 0, j = 0; ; i++, j++)
			{
				if (mycmd[i] == ':')
				{
					break;
				}
				CMDCHAR[j] = mycmd[i];
			}
			CMD = atoi(CMDCHAR);
			printf("CMD: %d\n", CMD);
			
			int x, y;
			char USERNAME[100];
			memset(USERNAME, 0, sizeof(USERNAME));
			for (x = i + 1, y = 0; ; x++, y++)
			{
				if (mycmd[x] == ':')
				{
					break;
				}
				USERNAME[y] = mycmd[x];
			}
			printf("USERNAME: %s\n", USERNAME);

			int a, b;
			char OSVER[100];
			memset(OSVER, 0, sizeof(OSVER));
			for (a = x + 1, b = 0; ; a++, b++)
			{
				if (mycmd[a] == ':')
				{
					break;
				}
				OSVER[b] = mycmd[a];
			}
			printf("OSVER: %s\n", OSVER);

			int n, m;
			char HWID[100];
			memset(HWID, 0, sizeof(HWID));
			for (n = a + 1, m = 0; ; n++, m++)
			{
				if (mycmd[n] == ':')
				{
					break;
				}
				HWID[m] = mycmd[n];
			}
			printf("HWID: %s\n", HWID);

			if (connect(Socket, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr)) == 0)
			{
				//если мы подключились то передаем хендшейк:)
				CMDiDATA indata; //создаем структуру отвечающую за протокол
				CLIENTS client; //создаем структуру отвечающую за инфу о клиенте
				memset(&indata, 0, sizeof(CMDiDATA)); //почистим 
				memset(&client, 0, sizeof(CLIENTS)); //почистим
				memcpy(client.HWID, HWID, sizeof(client.HWID));
				memcpy(client.USERNAME, USERNAME, sizeof(client.USERNAME));
				memcpy(client.OSVER, OSVER, sizeof(client.OSVER));
				indata.CMD = CMD;
				memcpy(indata.DATA, &client, sizeof(CLIENTS));
				send(Socket, (char*)&indata, sizeof(CMDiDATA), 0);
			}
			closesocket(Socket);
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

	SetConsoleTitleA("XAKEP");

	//инициализируем структуры винсока
	struct WSAData wsaData;
	WORD DLLVersion = MAKEWORD(2, 1);
	WSAStartup(DLLVersion, &wsaData);

	Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	ServerAddr.sin_family = AF_INET;
	//задаем конект на локалхост к восьмидесятому порту
	ServerAddr.sin_port = htons(80);
	ServerAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	//подключаемся к серверу!!!

	HANDLE RECV[2];
	RECV[0] = CreateThread(NULL, 0, RecvThread, NULL, 0, 0); //создаем тред который будет бесконечно принимать сообщения от сервера
	RECV[1] = CreateThread(NULL, 0, ConsoleReader, NULL, 0, 0);
	WaitForMultipleObjects(2, RECV, TRUE, INFINITE); //Бесконечно ожидаем пока этот тред работает...
}
