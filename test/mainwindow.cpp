#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFile>
#include <QDataStream>
#include <QFileInfo>
#include <QHostAddress>
#include <QDir>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      server(new QTcpServer(this)),
      socket(nullptr)
{
    ipEdit = new QLineEdit(this);
    ipEdit->setPlaceholderText("接收端IP，例如 192.168.1.10");

    portEdit = new QLineEdit(this);
    portEdit->setPlaceholderText("端口");
    portEdit->setText("8888");

    fileEdit = new QLineEdit(this);
    fileEdit->setPlaceholderText("请选择要发送的文件");

    logEdit = new QTextEdit(this);
    logEdit->setReadOnly(true);

    startServerButton = new QPushButton("启动接收端", this);
    connectButton = new QPushButton("连接接收端", this);
    sendFileButton = new QPushButton("发送文件", this);
    selectFileButton = new QPushButton("选择文件", this);

    QHBoxLayout *ipPortLayout = new QHBoxLayout;
    ipPortLayout->addWidget(ipEdit);
    ipPortLayout->addWidget(portEdit);

    QHBoxLayout *fileLayout = new QHBoxLayout;
    fileLayout->addWidget(fileEdit);
    fileLayout->addWidget(selectFileButton);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(startServerButton);
    buttonLayout->addWidget(connectButton);
    buttonLayout->addWidget(sendFileButton);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(ipPortLayout);
    mainLayout->addLayout(fileLayout);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(logEdit);

    QWidget *central = new QWidget(this);
    central->setLayout(mainLayout);
    setCentralWidget(central);

    setWindowTitle("Qt跨平台网络文件传输工具");
    resize(650, 420);

    connect(startServerButton, &QPushButton::clicked, this, &MainWindow::onStartServer);
    connect(connectButton, &QPushButton::clicked, this, &MainWindow::onConnectToHost);
    connect(sendFileButton, &QPushButton::clicked, this, &MainWindow::onSendFile);
    connect(selectFileButton, &QPushButton::clicked, this, &MainWindow::onSelectFile);
    connect(server, &QTcpServer::newConnection, this, &MainWindow::onNewConnection);
}

MainWindow::~MainWindow()
{
    if (socket) {
        socket->disconnectFromHost();
    }
}

void MainWindow::setSocket(QTcpSocket *newSocket)
{
    if (socket && socket != newSocket) {
        disconnect(socket, nullptr, this, nullptr);
        socket->disconnectFromHost();
        socket->deleteLater();
    }

    socket = newSocket;
    recvBuffer.clear();

    connect(socket, &QTcpSocket::readyRead, this, &MainWindow::onReadyRead);
    connect(socket, &QTcpSocket::disconnected, this, &MainWindow::onDisconnected);
}

void MainWindow::onStartServer()
{
    quint16 port = portEdit->text().toUShort();

    if (server->isListening()) {
        server->close();
    }

    if (!server->listen(QHostAddress::Any, port)) {
        logEdit->append("启动接收端失败：" + server->errorString());
    } else {
        logEdit->append(QString("接收端启动成功，监听端口：%1").arg(port));
        logEdit->append("等待对方连接...");
    }
}

void MainWindow::onConnectToHost()
{
    QString ip = ipEdit->text().trimmed();
    quint16 port = portEdit->text().toUShort();

    if (ip.isEmpty()) {
        logEdit->append("请先输入接收端IP！");
        return;
    }

    QTcpSocket *clientSocket = new QTcpSocket(this);
    clientSocket->connectToHost(ip, port);

    logEdit->append(QString("正在连接 %1:%2 ...").arg(ip).arg(port));

    if (!clientSocket->waitForConnected(3000)) {
        logEdit->append("连接失败：" + clientSocket->errorString());
        clientSocket->deleteLater();
        return;
    }

    setSocket(clientSocket);
    logEdit->append("连接成功，可以发送文件。请求连接的目标IP：" + ip);
}

void MainWindow::onNewConnection()
{
    QTcpSocket *newSocket = server->nextPendingConnection();
    setSocket(newSocket);
    logEdit->append("有客户端连接：" + socket->peerAddress().toString());
}

void MainWindow::onReadyRead()
{
    if (!socket) {
        return;
    }

    recvBuffer.append(socket->readAll());
    processReceiveBuffer();
}

void MainWindow::processReceiveBuffer()
{
    const int headerSize = static_cast<int>(sizeof(quint64));

    while (recvBuffer.size() >= headerSize) {
        quint64 payloadSize = 0;
        QByteArray header = recvBuffer.left(headerSize);
        QDataStream headerStream(&header, QIODevice::ReadOnly);
        headerStream.setVersion(QDataStream::Qt_5_9);
        headerStream >> payloadSize;

        if (recvBuffer.size() < headerSize + static_cast<int>(payloadSize)) {
            return;
        }

        QByteArray payload = recvBuffer.mid(headerSize, static_cast<int>(payloadSize));
        recvBuffer.remove(0, headerSize + static_cast<int>(payloadSize));

        QString fileName;
        QByteArray fileData;
        QDataStream in(&payload, QIODevice::ReadOnly);
        in.setVersion(QDataStream::Qt_5_9);
        in >> fileName >> fileData;

        if (fileName.isEmpty() || fileData.isEmpty()) {
            logEdit->append("收到的数据不完整，已忽略。");
            continue;
        }

        QDir dir(QDir::currentPath());
        if (!dir.exists("received_files")) {
            dir.mkdir("received_files");
        }

        QString safeName = QFileInfo(fileName).fileName();
        QString savePath = dir.filePath("received_files/" + safeName);
        QFile file(savePath);

        if (!file.open(QIODevice::WriteOnly)) {
            logEdit->append("保存文件失败：" + savePath);
            continue;
        }

        file.write(fileData);
        file.close();
        logEdit->append("接收到文件：" + savePath);
    }
}

void MainWindow::onSelectFile()
{
    QString filePath = QFileDialog::getOpenFileName(this, "选择文件");
    if (!filePath.isEmpty()) {
        fileEdit->setText(filePath);
    }
}

void MainWindow::onSendFile()
{
    if (!socket || socket->state() != QAbstractSocket::ConnectedState) {
        logEdit->append("尚未连接接收端，请先点击“连接接收端”，或等待客户端连接。");
        return;
    }

    QString selectedFilePath = fileEdit->text().trimmed();
    if (selectedFilePath.isEmpty()) {
        logEdit->append("请先选择要发送的文件！");
        return;
    }

    QFile file(selectedFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        logEdit->append("无法打开文件：" + selectedFilePath);
        return;
    }

    QByteArray fileData = file.readAll();
    file.close();

    QString fileName = QFileInfo(selectedFilePath).fileName();

    QByteArray payload;
    QDataStream payloadStream(&payload, QIODevice::WriteOnly);
    payloadStream.setVersion(QDataStream::Qt_5_9);
    payloadStream << fileName << fileData;

    QByteArray block;
    QDataStream blockStream(&block, QIODevice::WriteOnly);
    blockStream.setVersion(QDataStream::Qt_5_9);
    blockStream << static_cast<quint64>(payload.size());
    block.append(payload);

    qint64 written = socket->write(block);
    socket->flush();

    if (written == -1) {
        logEdit->append("文件发送失败：" + socket->errorString());
    } else {
        logEdit->append(QString("文件发送成功：%1，大小：%2 字节").arg(fileName).arg(fileData.size()));
    }
}

void MainWindow::onDisconnected()
{
    logEdit->append("连接已断开。");
}
