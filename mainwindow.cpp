#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "dronechatapp.h"

#include <QCryptographicHash>
#include <QMessageBox>
#include <QTimer>
#include <QPushButton>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->lineEdit_Password->setEchoMode(QLineEdit::Password);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_Login_clicked()
{
    static int failCount = 0;

    const QString username = ui->lineEdit_Username->text().trimmed();
    const QString password = ui->lineEdit_Password->text();

    if (username.isEmpty()) {
        QMessageBox::warning(this, "Login", "Username cannot be empty");
        return;
    }
    if (username.contains(' ')) {
        QMessageBox::warning(this, "Login", "Username cannot contain spaces");
        return;
    }
    if (username.length() > 32) {
        QMessageBox::warning(this, "Login", "Username cannot be longer than 32 characters");
        return;
    }

    // Simple check: username = "test", password = "test"
    if (username.compare("test", Qt::CaseInsensitive) == 0 &&
        password == "test")
    {
        ui->lineEdit_Password->clear();
        hide();

        droneChatApp = new DroneChatApp(this);
        droneChatApp->show();
        connect(droneChatApp, &QDialog::finished, this, &MainWindow::close);
    }
    else
    {
        ui->lineEdit_Password->clear();
        failCount++;

        if (failCount >= 5) {
            QMessageBox::warning(this, "Login",
                                 "Too many attempts. Try again in 30 seconds.");
            ui->pushButton_Login->setEnabled(false);

            QTimer::singleShot(30000, this, [this](){
                ui->pushButton_Login->setEnabled(true);
            });

            failCount = 0;
        } else {
            QMessageBox::warning(this, "Login", "Invalid credentials");
        }
    }
}


bool MainWindow::checkPassword(const QString &password,
                               const QByteArray &salt,
                               const QByteArray &expectedHashHex)
{
    const QByteArray data = salt + password.toUtf8();
    const QByteArray hash = QCryptographicHash::hash(
                                data, QCryptographicHash::Sha256).toHex();
    return hash == expectedHashHex;
}



