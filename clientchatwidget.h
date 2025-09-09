#ifndef CLIENTCHATWIDGET_H
#define CLIENTCHATWIDGET_H

#include "clientmanager.h"

#include <QWidget>
#include <QTcpSocket>

namespace Ui {
class ClientChatWidget;
}

class ClientChatWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ClientChatWidget(QTcpSocket *client, QWidget *parent = nullptr);
    ~ClientChatWidget();

    QTcpSocket* socket() const;
    void displayMessage(const QString& snder, const QString& message);

    enum class Status {
        Available = 1,
        Away = 2,
        Busy = 3,
        Offline = 4
    };



private slots:

    void on_btnSend_clicked();
    void onReadyRead();
    void clientDisconnected();
    void textMessageReceived(QString message);
    void onTyping();

    void sendIsTyping();
    void handleIsTyping();
    void handleStatusChange(int status);
    void handleNameChange(QString name);

signals:
    void isTyping(QString message);
    void statusChanged(int status);
    void clientNameChanged(QTcpSocket* socket, QString name);

private:
    Ui::ClientChatWidget *ui;
    QTcpSocket *_socket;
    ClientManager *_client;
};

#endif // CLIENTCHATWIDGET_H
