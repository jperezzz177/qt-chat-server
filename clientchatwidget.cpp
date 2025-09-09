#include "clientchatwidget.h"
#include "ui_clientchatwidget.h"
#include "chatitemwidget.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>

ClientChatWidget::ClientChatWidget(QTcpSocket *socket, QWidget *parent)
    : QWidget(parent),
    ui(new Ui::ClientChatWidget),
    _socket(socket)
{
    ui->setupUi(this);
    ui->wdgSend->setEnabled(true);

    connect(_socket, &QTcpSocket::disconnected, this, &ClientChatWidget::clientDisconnected);

    //connect(_socket, &QTcpSocket::readyRead, this, &ClientChatWidget::onReadyRead);
    connect(ui->lnMessage, &QLineEdit::returnPressed, this, &ClientChatWidget::on_btnSend_clicked);
    QTimer* typingTimer = new QTimer(this);
    typingTimer->setInterval(750); // 600ms cooldown
    typingTimer->setSingleShot(true);

    connect(ui->lnMessage, &QLineEdit::textChanged, this, [this, typingTimer]() {
        if (!typingTimer->isActive()) {
            sendIsTyping();
            typingTimer->start();
        }
    });
}

ClientChatWidget::~ClientChatWidget()
{
    delete ui;
}

QTcpSocket *ClientChatWidget::socket() const
{
    return _socket;
}

void ClientChatWidget::on_btnSend_clicked()
{
    QString message = ui->lnMessage->text().trimmed();
    if (message.isEmpty() || !_socket || _socket->state() != QAbstractSocket::ConnectedState)
        return;

    QJsonObject obj;
    obj["type"] = "message";
    obj["content"] = message;

    obj["sender_id"] = 0;
    obj["sender"] = "Server";
    obj["recipient"] = _socket->property("id").toInt();

    QJsonDocument doc(obj);
    _socket->write(doc.toJson(QJsonDocument::Compact)+ "\n");
    _socket->flush();

    displayMessage("Server", message);
    ui->lnMessage->clear();
    qDebug() << "[Server] Message sent to client. Socket still valid? " << _socket->isValid();
}

void ClientChatWidget::onReadyRead()
{
    while (_socket->canReadLine()) {
        QByteArray line = _socket->readLine().trimmed();
        if (line.isEmpty()) continue;

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(line, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            qDebug() << "Invalid JSON from client:" << line;
            continue;
        }

        QJsonObject obj = doc.object();
        QString type = obj["type"].toString();


        if (type == "is_typing") {
            qDebug() << "Client is typing...";
        } else if (type == "set_name") {
            QString name = obj["name"].toString();
            handleNameChange(name);
        } else if (type == "set_status") {
            int status = obj["status"].toInt();
            handleStatusChange(status);
        }
    }
}

void ClientChatWidget::displayMessage(const QString& sender, const QString& message)
{
    ChatItemWidget* chatWidget = new ChatItemWidget(this);
    bool isServerMessage = (sender.compare("Server", Qt::CaseInsensitive) == 0);
    chatWidget->setMessage(message, isServerMessage);

    QListWidgetItem* listItem = new QListWidgetItem(ui->lstMessages);
    listItem->setSizeHint(chatWidget->sizeHint());
    ui->lstMessages->addItem(listItem);
    ui->lstMessages->setItemWidget(listItem, chatWidget);
}

void ClientChatWidget::sendIsTyping()
{
    QJsonObject obj;
    obj["type"] = "is_typing";

    QJsonDocument doc(obj);
    _socket->write(doc.toJson(QJsonDocument::Compact) + "\n");
}

void ClientChatWidget::handleIsTyping()
{
    qDebug() << "Client is typing...";
}

void ClientChatWidget::handleStatusChange(int status)
{
    qDebug() << "Status changed to:" << status;
    emit statusChanged(status);
}

void ClientChatWidget::handleNameChange(QString name)
{
    qDebug() << "Client name changed to:" << name;
    emit clientNameChanged(_socket, name);
}

void ClientChatWidget::clientDisconnected()
{
    ui->wdgSend->setEnabled(false);
}

void ClientChatWidget::textMessageReceived(QString message)
{
    displayMessage("Client", message);
}

void ClientChatWidget::onTyping()
{
    emit isTyping(QString("%1 is typing...").arg("Client"));
}

