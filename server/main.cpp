#include "mwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    //FreeConsole();
    //AllocConsole();
    //AttachConsole(GetCurrentProcessId());

    //freopen("CON", "w", stdout);
    //freopen("CON", "w", stderr);
    //freopen("CON", "r", stdin);

    QApplication a(argc, argv);
    MWindow w;
    w.show();
    return a.exec();
}
