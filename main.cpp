
#include <QApplication>
#include "QtOSGWidget.h"


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    //a.setAttribute(Qt::AA_DontCheckOpenGLContextThreadAffinity);

    QtOSGWidget www;
    www.show();
    return a.exec();
}
