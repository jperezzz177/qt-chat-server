//#include "clientchatwidget.h"
#include "servermanager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QDebug>
#include <QTimer>
#include <QJsonArray>
ServerManager::ServerManager(ushort port, QObject *parent)
    : QObject{parent}
{
    setupServer(port);

    _handlerMap["init"] = &ServerManager::handleInit;
    _handlerMap["login"] = &ServerManager::handleLogin;
    _handlerMap["message"] = &ServerManager::handleMessage;
    _handlerMap["is_typing"] = &ServerManager::handleTyping;
    _handlerMap["set_name"] = &ServerManager::handleSetName;
    _handlerMap["set_status"] = &ServerManager::handleSetStatus;
}

void ServerManager::newClientConnectionRecieved()
{
    auto client = _server->nextPendingConnection();
    if (!client) {
        qWarning() << "Warning: nextPendingConnection() returned nullptr!";
        return;
    }

    client->setParent(this);


    int id = _nextId++;
    client->setProperty("id", id);
    client->setProperty("name", QString("Client %1").arg(id));
    client->setProperty("status", static_cast<int>(ServerManager::Status::Available));
    _clients << client;
    _byId.insert(id, client);



    qDebug() << "New Client Connected:" << client;
    qDebug() << "[Server] Assigned ID to new client:" << id;

    QJsonObject idMsg;
    idMsg["type"] = "assign_id";
    idMsg["id"] = id;
    client->write(QJsonDocument(idMsg).toJson(QJsonDocument::Compact) + "\n");
    client->flush();

    QTimer::singleShot(50, this, [this]{
        broadcastClientList();
    });

    emit newClientConnected(client);

    connect(client, &QTcpSocket::disconnected, this, &ServerManager::onClientDisconnected);

    connect(client, &QTcpSocket::readyRead, this, &ServerManager::onClientReadyRead);

    connect(client, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
            this, [client](QAbstractSocket::SocketError e){
                qWarning() << "[Server] client socket error" << e << "id=" << client->property("id").toInt();
            });


}

void ServerManager::onClientDisconnected()
{
    auto client = qobject_cast<QTcpSocket *>(sender());
    if (!client) return;

    const int id = client->property("id").toInt();
    _byId.remove(id);

    _clients.removeOne(client);
    emit clientDisconnected(client);
    client->deleteLater();
    broadcastClientList();
}

void ServerManager::onClientReadyRead()
{
    auto client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;

    while (client->canReadLine()) {
        QByteArray line = client->readLine();
        if (line.size() > 65536) {
            qWarning() << "[Server] Dropping oversized line from id="
                       << client->property("id").toInt() << "size=" << line.size();
            continue;
        }

        line = line.trimmed();
        if (line.isEmpty()) continue;

        qDebug() << "[Server DEBUG] Raw incoming line:" << line;

        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(line, &error);
        if (error.error != QJsonParseError::NoError || !doc.isObject()) {
            qWarning() << "[Server] Invalid JSON from id="
                       << client->property("id").toInt() << ":" << error.errorString();
            continue;
        }

        handleClientMessage(client, doc.object());
    }
}

void ServerManager::handleClientMessage(QTcpSocket *client, const QJsonObject &obj)
{
    QString type = obj["type"].toString();

    qDebug() << "Received packet of type: " << type;

    auto it = _handlerMap.find(type);
    if (it != _handlerMap.end()) {
        HandlerFunc handler = it->second;
        (this->*handler)(client, obj);
    } else {
        qDebug() << "Unknown message type received:" << type;
    }
}

void ServerManager::handleInit(QTcpSocket *client, const QJsonObject &obj)
{
    QString os = obj["os"].toString();
    QString version = obj["version"].toString();
    qDebug() << "Init received from client:" << os << version;

    QJsonObject successResp{{"type", "init_ack"}, {"status", "ok"}};
    client->write(QJsonDocument(successResp).toJson(QJsonDocument::Compact) + "\n");
    client->flush();
}

void ServerManager::handleLogin(QTcpSocket *client, const QJsonObject &obj)
{
    QString username = obj["username"].toString();
    QString password = obj["password"].toString();

    QJsonObject response{{"type", "login_ack"}};
    if (username == "test" && password == "test") {
        response["status"] = "ok";
    } else {
        response["status"] = "error";
        response["reason"] = "Invalid username or password";
    }

    client->write(QJsonDocument(response).toJson(QJsonDocument::Compact) + "\n");
    client->flush();
}

void ServerManager::setupServer(ushort port)
{
    _server = new QTcpServer(this);
    connect(_server, &QTcpServer::newConnection, this, &ServerManager::newClientConnectionRecieved);
    if(!_server->listen(QHostAddress::Any, port)){
        qDebug() << "Server failed to bind port:" << port << ", error:" << _server->errorString();
    }else{
        qDebug() << "Server listening on port" << port;
    }

}

ServerManager::~ServerManager()
{
    stopServer();
    qDebug() << "server closed properly in destructor";
}

void ServerManager::stopServer()
{
    for (auto client : std::as_const(_clients)) {
        if (client->state() != QAbstractSocket::UnconnectedState) {
            client->disconnectFromHost();
            if (client->state() != QAbstractSocket::UnconnectedState)
                client->waitForDisconnected(250);
        }
        client->deleteLater();
    }
    _clients.clear();

    if (_server && _server->isListening())
        _server->close();

    qDebug() << "[Server] Stopped and all clients cleared.";
}

void ServerManager::broadcastClientList()
{
    QJsonArray clientArray;
    for (auto client : _clients) {
        QJsonObject cObj;
        cObj["id"] = client->property("id").toInt();
        cObj["name"] = client->property("name").toString();
        cObj["status"] = client->property("status").toInt();
        clientArray.append(cObj);
    }

    QJsonObject serverObj;
    serverObj["id"] = 0;
    serverObj["name"] = "Server";
    serverObj["status"] = static_cast<int>(ServerManager::Status::Available);

    clientArray.prepend(serverObj);

    QJsonObject response;
    response["type"] = "client_list";
    response["clients"] = clientArray;

    QByteArray json = QJsonDocument(response).toJson(QJsonDocument::Compact) + '\n';

    for (auto client : _clients)
        client->write(json);
}

void ServerManager::handleTyping(QTcpSocket* client, const QJsonObject& obj) {

    const QString name= client->property("name").toString();
    const int id = client->property("id").toInt();

    emit clientIsTyping(QString("%1 (%2) is typing...").arg(name).arg(id));

    if (obj.contains("recipient")) {
        int targetId = obj["recipient"].toInt();

        QTcpSocket* recipient = _byId.value(targetId, nullptr);

        if (recipient) {
            QJsonObject fwd;
            fwd["type"] = "is_typing";
            fwd["sender_id"] = id;
            fwd["sender"] = name;
            recipient->write(QJsonDocument(fwd).toJson(QJsonDocument::Compact) + "\n");
            recipient->flush();
        }
    }
}

void ServerManager::handleMessage(QTcpSocket* client, const QJsonObject& obj) {

    qDebug() << "handleMessage triggered";
    if (!obj.contains("recipient") || !obj.contains("content")) {
        qWarning() << "Missing 'recipient' or 'content' in message:" << obj;
        return;
    }

    int targetId = obj["recipient"].toInt();
    QString content = obj["content"].toString();
    int senderId = client->property("id").toInt();
    QString senderName = client->property("name").toString();

    QJsonObject forward{
        {"type", "message"},
        {"sender_id", senderId},
        {"sender", senderName},
        {"recipient", targetId},
        {"content", content}
    };

    if (targetId == 0 && senderId != 0) {
        emit messageReceived(client, content);
        emit newMessageReceived(forward);
        return;
    }

    QTcpSocket* recipient = _byId.value(targetId, nullptr);
    if (!recipient) {
        qWarning() << "Recipient with ID" << targetId << "not found!";
        return;
    }


    recipient->write(QJsonDocument(forward).toJson(QJsonDocument::Compact) + "\n");
    recipient->flush();

    client->write(QJsonDocument(forward).toJson(QJsonDocument::Compact) + "\n");
    client->flush();

    qDebug() << "Message routed from Client" << senderId << "to Client" << targetId;

}

void ServerManager::handleSetName(QTcpSocket* client, const QJsonObject& obj) {
    if (obj.contains("name")) {
        QString newName = obj["name"].toString();
        client->setProperty("name", newName);
        broadcastClientList();
        emit clientNameChanged(client, newName);
    }
}

void ServerManager::handleSetStatus(QTcpSocket* client, const QJsonObject& obj) {
    if (obj.contains("status")) {
        int s = obj["status"].toInt();
        client->setProperty("status", s);
        broadcastClientList();
        emit clientStatusChanged(client, s);
    }
}

