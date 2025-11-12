#pragma once

#include <QMainWindow>



class QApplication;
class QCloseEvent;

QT_BEGIN_NAMESPACE
namespace Ui {
    class Main;
}
QT_END_NAMESPACE

class Main : public QMainWindow {
    Q_OBJECT

private:
    QApplication* mApp;
    Ui::Main*     mUi;

    void closeEvent(QCloseEvent* event) override;

public:
    Main(QApplication* app, QWidget* parent = nullptr);
    ~Main();

public slots:
};
