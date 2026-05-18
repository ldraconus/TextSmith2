#include "Main.h"

#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

#ifdef QT_NO_DEBUG
    qInstallMessageHandler([](QtMsgType, const QMessageLogContext&, const QString&){}); // in the future add compile flags to allow/disallow certain messages
#ifdef Q_OS_WIN
    FILE* nul = nullptr;
    freopen_s(&nul, "NUL", "w", stderr);
#endif
#endif

    Main w(&a);
    w.show();
    return a.exec();
}
