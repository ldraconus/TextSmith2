#include "Main.h"
#include "ui_Main.h"

#include <QApplication>
#include <QCloseEvent>

/*
    textEdit->setStyleSheet(
        "QTextEdit {"
        "   background-color: #f0f8ff;"   // Light blue background
        "   color: #003366;"              // Dark blue text
        "   font-family: 'Courier New';"
        "   font-size: 14px;"
        "   border: 2px solid #003366;"
        "   border-radius: 5px;"
        "   padding: 5px;"
        "}"
    );
 */

void Main::closeEvent(QCloseEvent *event) {

}

Main::Main(QApplication *app, QWidget *parent)
    : QMainWindow(parent)
    , mApp(app)
    , mUi(new Ui::Main)
{
    mUi->setupUi(this);
}

Main::~Main() {
    delete mUi;
}
