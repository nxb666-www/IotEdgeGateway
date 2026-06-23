#include "mainwidget.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("IoT Gateway 控制中心");
    MainWidget w;
    w.show();
    return a.exec();
}
