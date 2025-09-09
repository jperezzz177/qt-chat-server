#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "dronechatapp.h"


QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_Login_clicked();

private:
    Ui::MainWindow *ui;
    DroneChatApp *droneChatApp;
    static bool checkPassword(const QString& password, const QByteArray& salt, const QByteArray& expectedHashHex);

};
#endif // MAINWINDOW_H
