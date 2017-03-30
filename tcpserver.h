#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <WinSock2.h>
#include <QList>
#include <QObject>
#include "tcpcommunicator.h"

#pragma comment(lib, "ws2_32.lib")


class TCPServer : public QObject
{
    Q_OBJECT

public:
    TCPServer();
    ~TCPServer();

    bool initSock();
    bool bindPort(const int port);
    bool listenToClient();
    unsigned long sendData(const int index, const char *data, const unsigned long size);
    unsigned long recvData(const int index, char *data, const unsigned long size);
    QString getClientIP(const int index);
    int getClientPort(const int index);
    int getServerPort();
    void setClientIndex(const int index);
    SOCKET getClientSocket(const int index);


public slots:
    void acceptClient();
    void waitReq();


signals:
    void error(QString err);
    void progress(int curSize, int totalSize);
    void acceptClientEnd();
    void waitReqEnd();
    void connected(int clientIndex);
    void request(char req);


private:
    SOCKET sock;
    QList<SOCKET> clientSocks;
    QList<struct sockaddr_in> clients;
    struct sockaddr_in server;
    int clientIndex;
};

#endif // TCPSERVER_H
