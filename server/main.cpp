#include "mwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MWindow w;
    w.show();
    return a.exec();
}
