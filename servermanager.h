#ifndef SERVERMANAGER_H
#define SERVERMANAGER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonObject>
#include <map>

class ServerManager : public QObject
{
    Q_OBJECT
public:
    explicit ServerManager(ushort port = 4500, QObject *parent = nullptr);
    ~ServerManager();
    void stopServer();
    QList<QTcpSocket*> clients() const {return _clients;}
    void broadcastClientList();
    void handleClientMessage(QTcpSocket* client, const QJsonObject& obj);

    enum class Status{
        Available = 1,
        Away = 2,
        Busy = 3,
        Offline = 4
    };


private slots:
    void newClientConnectionRecieved();
    void onClientDisconnected();
    void onClientReadyRead();

    void handleInit(QTcpSocket* client, const QJsonObject& obj);
    void handleLogin(QTcpSocket* client, const QJsonObject& obj);

signals:
    void newClientConnected(QTcpSocket *client);
    void clientDisconnected(QTcpSocket *client);
    void messageReceived(QTcpSocket *client, QString message);
    void clientIsTyping(QString status);
    void clientNameChanged(QTcpSocket* client, const QString& name);
    void clientStatusChanged(QTcpSocket* client, int status);
    void newMessageReceived(const QJsonObject &message);

private:
    QTcpServer *_server;
    QList<QTcpSocket *> _clients;
    QHash <int, QTcpSocket*> _byId;
    int _nextId =1;
    using HandlerFunc = void (ServerManager::*)(QTcpSocket*, const QJsonObject&);
    std::map<QString, HandlerFunc> _handlerMap;
    void handleMessage(QTcpSocket* client, const QJsonObject& obj);
    void handleTyping(QTcpSocket* client, const QJsonObject& obj);
    void handleSetName(QTcpSocket* client, const QJsonObject& obj);
    void handleSetStatus(QTcpSocket* client, const QJsonObject& obj);

private:
    void setupServer(ushort port);
};

#endif // SERVERMANAGER_H
