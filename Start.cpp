#include "Main.h"

#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    QStringList args = a.arguments();
    Main w(&a);
    w.show();
    return a.exec();
}
