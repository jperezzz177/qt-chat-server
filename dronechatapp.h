#ifndef DRONECHATAPP_H
#define DRONECHATAPP_H

#include <QDialog>
#include "servermanager.h"
#include "clientchatwidget.h"


namespace Ui {
class DroneChatApp;
}

class DroneChatApp : public QDialog
{
    Q_OBJECT

public:
    explicit DroneChatApp(QWidget *parent = nullptr);
    ~DroneChatApp();

    void updatedOnTyping(QString status);


protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void newClientConnected(QTcpSocket *client);
    void clientDisconnected(QTcpSocket *client);
    void setClientName(QTcpSocket* client, QString name);
    void setClientStatus(QTcpSocket* client, int status);
    void on_btnDisconnectAll_clicked();

private:
    Ui::DroneChatApp *ui;
    ServerManager *_server;

private:
    void setupServer();
};

#endif // DRONECHATAPP_H
