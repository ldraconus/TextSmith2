#include "Main.h"

#include <QApplication>

int main(int argc, char *argv[]) {
    qInstallMessageHandler([](QtMsgType, const QMessageLogContext&, const QString&){}); // in the future add compile flags to allow/disallow certain messages
    QApplication a(argc, argv);
    Main w(&a);
    w.show();
    return a.exec();
}
