#include "tcpclient.h"

#include <QDebug>
#include <QApplication>
#include <QFile>


/*========================
Constructor
========================*/
TCPClient::TCPClient()
{
    if(!initSock())
        qDebug() << "Could not create socket: " << WSAGetLastError();

    isConnected = false;
}


/*========================
Destructor
========================*/
TCPClient::~TCPClient()
{
    closesocket(sock);
}


/*========================
Init socket
========================*/
bool TCPClient::initSock()
{
    sock = socket(AF_INET,      //IPv4
                  SOCK_STREAM,  //TCP
                  0);
    if(sock == INVALID_SOCKET) return false;
    return true;
}


/*========================
SLOT: Connect to server
========================*/
void TCPClient::setServer(const char* ip, const int port)
{
    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
}



/*========================
SLOT: Connect to server
========================*/
void TCPClient::connectToServer()
{
    if(server.sin_port == 0 || sock == INVALID_SOCKET){
        moveToThread(qApp->thread());
        emit error("Invalid socket");
        emit connectEnd();
        return;
    }


    //::connect for winsock, this->connect for signals & slots
    if(::connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0)
        emit error("Connot connect to server");

    isConnected = true;

    moveToThread(qApp->thread());
    emit connected();
    emit connectEnd();
}


/*========================
Get server ip
========================*/
QString TCPClient::getServerIp()
{
    return inet_ntoa(server.sin_addr);
}


/*========================
Get server port
========================*/
int TCPClient::getServerPort()
{
    return ntohs(server.sin_port);
}


/*========================
Get client socket
========================*/
SOCKET TCPClient::getSocket()
{
    return sock;
}



/*========================
Check if connected
========================*/
bool TCPClient::isConnectedToServer()
{
    return isConnected;
}
