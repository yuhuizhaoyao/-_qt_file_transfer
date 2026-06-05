#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QFileDialog>
#include <QByteArray>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onStartServer();
    void onConnectToHost();
    void onNewConnection();
    void onReadyRead();
    void onSelectFile();
    void onSendFile();
    void onDisconnected();

private:
    void setSocket(QTcpSocket *newSocket);
    void processReceiveBuffer();

private:
    QTcpServer *server;
    QTcpSocket *socket;

    QLineEdit *ipEdit;
    QLineEdit *portEdit;
    QLineEdit *fileEdit;
    QTextEdit *logEdit;

    QPushButton *startServerButton;
    QPushButton *connectButton;
    QPushButton *sendFileButton;
    QPushButton *selectFileButton;

    QByteArray recvBuffer;
};

#endif // MAINWINDOW_H
