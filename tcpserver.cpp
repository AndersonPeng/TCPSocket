#include "tcpserver.h"

#include <QDebug>
#include <QApplication>
#include <QFile>


/*========================
Constructor
========================*/
TCPServer::TCPServer()
{
    if(!initSock())
        qDebug() << "Could not create socket: " << WSAGetLastError();

    clientIndex = -1;
}


/*========================
Destructor
========================*/
TCPServer::~TCPServer()
{
    closesocket(sock);
}


/*========================
Init socket
========================*/
bool TCPServer::initSock()
{
    sock = socket(AF_INET,      //IPv4
                  SOCK_STREAM,  //TCP
                  0);
    if(sock == INVALID_SOCKET) return false;
    return true;
}


/*========================
Bind port
========================*/
bool TCPServer::bindPort(const int port)
{
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);

    if(bind(sock, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) return false;
    return true;
}


/*========================
Listen to clients
========================*/
bool TCPServer::listenToClient()
{
    if(sock == INVALID_SOCKET) return false;

    listen(sock, 3);
    return true;
}


/*========================
SLOT: accept a client
      (for a thread)
========================*/
void TCPServer::acceptClient()
{
    int len = sizeof(struct sockaddr_in);
    struct sockaddr_in client;
    SOCKET clientSock;


    //Accept================================
    clientSock = accept(sock, (struct sockaddr*)&client, &len);
    if(clientSock == INVALID_SOCKET){
        moveToThread(qApp->thread());
        emit error("Failed to connect to a client");
        emit acceptClientEnd();
        return;
    }

    clients.push_back(client);
    clientSocks.push_back(clientSock);

    moveToThread(qApp->thread());
    emit connected(clients.size() - 1);
    emit acceptClientEnd();
}


/*========================
SLOT: wait client request
      (for a thread)
========================*/
void TCPServer::waitReq()
{
    if(clients.size()-1 < clientIndex || clientSocks.at(clientIndex) == INVALID_SOCKET){
        moveToThread(qApp->thread());
        emit error("No client connected");
        emit waitReqEnd();
        return;
    }

    char req, res;

    while(true){
        //Recv request
        unsigned long recvSize = recvData(clientIndex, &req, 1);

        if(recvSize == 0){
            emit error("Disconnected");
            break;
        }
        else if(recvSize > 0){
            //Download request
            if(req == REQ_DOWNLOAD){
                res = RES_DENIED;
                unsigned long sendSize = sendData(clientIndex, &res, 1);
                if(sendSize == SOCKET_ERROR) emit error("Failed to send response");
            }
            //Upload request
            else if(req == REQ_UPLOAD){
                res = RES_OK;
                unsigned long sendSize = sendData(clientIndex, &res, 1);
                if(sendSize == SOCKET_ERROR) emit error("Failed to send response");
                else emit request(req);
                break;
            }
            //Wrong request
            else{
                res = RES_ERR;
                unsigned long sendSize = sendData(clientIndex, &res, 1);
                if(sendSize == SOCKET_ERROR) emit error("Failed to send response");
            }
        }
        else{
            emit error("Recv failed");
            break;
        }
    }

    moveToThread(qApp->thread());
    emit waitReqEnd();
}


/*========================
Send data
========================*/
unsigned long TCPServer::sendData(const int index, const char *data, const unsigned long size)
{
    if(clients.size()-1 < index) return -1;
    return send(clientSocks.at(index), data, size, 0);
}


/*========================
Receive data
========================*/
unsigned long TCPServer::recvData(const int index, char *data, const unsigned long size)
{
    if(clients.size()-1 < index) return -1;
    return recv(clientSocks.at(index), data, size, 0);
}


/*========================
Get client ip address
========================*/
QString TCPServer::getClientIP(const int clientIndex)
{
    if(clientIndex >= clients.size()) return "";
    return inet_ntoa(clients.at(clientIndex).sin_addr);
}


/*========================
Get client port
========================*/
int TCPServer::getClientPort(const int clientIndex)
{
    if(clientIndex >= clients.size()) return -1;
    return ntohs(clients.at(clientIndex).sin_port);
}


/*========================
Get server port
========================*/
int TCPServer::getServerPort()
{
    return server.sin_port;
}


/*========================
Get client socket
========================*/
SOCKET TCPServer::getClientSocket(const int index)
{
    if(clientIndex >= clientSocks.size()) return INVALID_SOCKET;

    return clientSocks.at(index);
}


/*========================
Set client index for
communication
========================*/
void TCPServer::setClientIndex(const int index)
{
    if(clients.size() - 1 < index) clientIndex = -1;
    else clientIndex = index;
}
