#include "clientchatwidget.h"
#include "dronechatapp.h"
#include "ui_dronechatapp.h"
#include <QCloseEvent>
#include <QTimer>
#include <QJsonObject>
#include <QJsonValue>


DroneChatApp::DroneChatApp(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DroneChatApp)
{
    ui->setupUi(this);

    _server= new ServerManager(4500,this);
    setupServer();


}

DroneChatApp::~DroneChatApp()
{
    delete ui;
}

void DroneChatApp::newClientConnected(QTcpSocket *client)
{
    auto id = client -> property("id").toInt();
    ui->lstClients->addItem(QString("New Client Added: %1").arg(id));
    auto chatWidget = new ClientChatWidget(client, this);
    ui->tbChat->addTab(chatWidget, QString("Client (%1)").arg(id));

}

void DroneChatApp::clientDisconnected(QTcpSocket *client)
{
    qDebug() << "Client disconnected after message? ID:" << client->property("id").toInt();
    auto id = client -> property("id").toInt();
    ui->lstClients->addItem(QString("Client disconnected: %1").arg(id));
}

void DroneChatApp::setClientName(QTcpSocket* client, QString name)
{
    for (int i = 0; i < ui->tbChat->count(); ++i) {
        auto chatWidget = qobject_cast<ClientChatWidget*>(ui->tbChat->widget(i));
        if (chatWidget && chatWidget->socket() == client) {
            ui->tbChat->setTabText(i, name);
            break;
        }
    }
}

void DroneChatApp::setClientStatus(QTcpSocket* client, int status)
{
    int index = -1;
    for (int i = 0; i < ui->tbChat->count(); ++i) {
        auto chatWidget = qobject_cast<ClientChatWidget*>(ui->tbChat->widget(i));
        if (chatWidget && chatWidget->socket() == client) { index = i; break; }
    }

    if (index < 0) return;

    QString iconName = ":/Icons/";
    switch (status) {
    case static_cast<int>(ClientChatWidget::Status::Available): iconName.append("available.png"); break;
    case static_cast<int>(ClientChatWidget::Status::Away):      iconName.append("away.png"); break;
    case static_cast<int>(ClientChatWidget::Status::Busy):      iconName.append("busy.png"); break;
    default: iconName = ""; break;
    }

    ui->tbChat->setTabIcon(index, QIcon(iconName));
}

void DroneChatApp::on_btnDisconnectAll_clicked()
{
    qDebug() << "[Debug] Disconnect All triggered";
    //for (auto client : _server -> clients()) {
    //    client -> disconnectFromHost();
    //}
}

void DroneChatApp::setupServer()
{
    connect(_server, &ServerManager::clientIsTyping, this, &DroneChatApp::updatedOnTyping);

    connect(_server, &ServerManager::messageReceived, this,
            [this](QTcpSocket* senderClient, const QString& message) {
                for (int i = 0; i < ui->tbChat->count(); ++i) {
                    auto chatWidget = qobject_cast<ClientChatWidget*>(ui->tbChat->widget(i));
                    if (chatWidget && chatWidget->socket() == senderClient) {
                        chatWidget->displayMessage(senderClient->property("name").toString(), message);
                        break;
                    }
                }
            });

    connect(_server, &ServerManager::newClientConnected, this, &DroneChatApp::newClientConnected);
    connect(_server, &ServerManager::clientNameChanged, this, &DroneChatApp::setClientName);
    connect(_server, &ServerManager::clientStatusChanged, this, &DroneChatApp::setClientStatus);

}

void DroneChatApp::closeEvent(QCloseEvent *event)
{
    if(_server){
        _server->stopServer();
        delete _server;
        _server= nullptr;
    }

    QDialog::closeEvent(event);
}

void DroneChatApp::updatedOnTyping(QString status)
{
    qDebug() << "[DEBUG] updatedOnTyping received status:" << status << "| Length:" << status.length();

    ui->lblStatus->setText(status);
    ui->lblStatus->setStyleSheet("background-color: #ffffcc; padding: 4px; font-style: italic;");
    ui->lblStatus->repaint();

    static QTimer* clearTimer = nullptr;
    if (clearTimer == nullptr) {
        clearTimer = new QTimer(this);
        clearTimer->setSingleShot(true);
        connect(clearTimer, &QTimer::timeout, this, [this]() {
            ui->lblStatus->clear();
        });
    }
    clearTimer->start(1000);

}
