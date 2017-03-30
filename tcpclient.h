#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <WinSock2.h>
#include <QObject>

#pragma comment(lib, "ws2_32.lib")


class TCPClient : public QObject
{
    Q_OBJECT

public:
    TCPClient();
    ~TCPClient();

    bool initSock();
    void setServer(const char* ip, const int port);
    QString getServerIp();
    int getServerPort();
    SOCKET getSocket();
    bool isConnectedToServer();


public slots:
    void connectToServer();


signals:
    void connectEnd();
    void error(QString err);
    void connected();


private:
    SOCKET sock;
    struct sockaddr_in server;
    bool isConnected;
};

#endif // TCPCLIENT_H
