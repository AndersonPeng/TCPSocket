#include "tcpcommunicator.h"

#include <QFile>
#include <QDebug>


/*============================
Constructor
============================*/
TCPCommunicator::TCPCommunicator()
{
    this->sock = INVALID_SOCKET;
    this->req = -1;
}


/*============================
Destructor
============================*/
TCPCommunicator::~TCPCommunicator()
{

}


/*============================
Set socket
============================*/
void TCPCommunicator::setSocket(SOCKET sock)
{
    this->sock = sock;
}


/*============================
Set file name
============================*/
void TCPCommunicator::setFilename(QString filename)
{
    this->filename = filename;
}


/*============================
Set request
============================*/
void TCPCommunicator::setRequest(char req)
{
    this->req = req;
}


/*============================
SLOT: Send a file through TCP
      (for a thread)
============================*/
void TCPCommunicator::sendFile()
{
    //Check file===================================
    QFile file(filename);
    if(!file.open(QFile::ReadOnly)){
        emit error("Could not open the file");
        emit sendFileEnd();
        return;
    }

    char buf[65536] = {0};
    unsigned long dataSize = 0;
    quint64 curSize = 0;
    fileInfo info;

    info.size = file.size();
    strcpy(info.name, getFileNameFromPath(filename).toLocal8Bit().constData());


    //Send file info===============================
    dataSize = send(sock, (char*)&info, sizeof(info), 0);

    if(dataSize == 0){
        file.close();
        emit error("Disconnected");
        emit sendFileEnd();
        return;
    }
    else if(dataSize == SOCKET_ERROR){
        file.close();
        emit error("Send failed");
        emit sendFileEnd();
        return;
    }


    //Send file====================================
    while(true){
       dataSize = file.read(buf, sizeof(buf));

       if(dataSize > 0){
           dataSize = send(sock, buf, dataSize, 0);
           curSize += dataSize;

           emit progress(curSize, info.size);

           if(curSize >= info.size) break;

           if(dataSize == 0){
               emit error("Disconnected");
               break;
           }
           else if(dataSize == SOCKET_ERROR){
               emit error("Send failed");
               break;
           }
       }
       else break;
    }

    file.close();
    emit sendFileEnd();
}


/*============================
SLOT: Recv a file through TCP
      (for a thread)
============================*/
void TCPCommunicator::recvFile()
{
    char buf[65536] = {0};
    unsigned long dataSize = 0;
    quint64 curSize = 0;
    fileInfo info;


    //Recv file info================================
    dataSize = recv(sock, (char*)&info, sizeof(info), 0);

    if(dataSize == 0){
        emit error("Disconnected");
        emit recvFileEnd();
        return;
    }
    else if(dataSize == SOCKET_ERROR){
        emit error("Recv failed");
        emit recvFileEnd();
        return;
    }


    //Check file====================================
    QFile file(info.name);
    if(!file.open(QFile::ReadWrite)){
        emit error("Could not open the file");
        emit recvFileEnd();
        return;
    }


    //Recv file====================================
    while(true){
       dataSize = recv(sock, buf, sizeof(buf), 0);

       if(dataSize == 0){
           emit error("Disconnected");
           break;
       }
       else if(dataSize == SOCKET_ERROR){
           emit error("Failed to send data");
           break;
       }
       else{
           file.write(buf, dataSize);
           curSize += dataSize;

           emit progress(curSize, info.size);

           if(curSize >= info.size) break;
       }
    }

    file.close();
    emit recvFileEnd();
}



/*============================
SLOT: Send a request through TCP
      (for a thread)
============================*/
void TCPCommunicator::sendReq()
{
    if(req < 0){
        emit error("Wrong request to send");
        emit sendReqEnd();
        return;
    }


    //Send request============================
    char res;
    unsigned long sendSize = send(sock, &req, 1, 0);
    if(sendSize == SOCKET_ERROR){
        emit error("Failed to send request");
        emit sendReqEnd();
        return;
    }


    //Recv response===========================
    unsigned long recvSize = recv(sock, &res, 1, 0);
    if(recvSize == 0) emit error("Disconnected");
    if(recvSize > 0)  emit response(req, res);
    else              emit error("Recv failed");

    emit sendReqEnd();
}


/*===============================
STATIC: Set file name from file path
===============================*/
QString TCPCommunicator::getFileNameFromPath(QString filePath)
{
    const char* filePath_str = filePath.toLocal8Bit().constData();
    char buf[256];
    char filename[256];
    int j = 0;

    for(size_t i = strlen(filePath_str)-1; i >= 0; --i, ++j){
        if(filePath_str[i] == '/' || j >= 256) break;
        buf[j] = filePath_str[i];
    }

    for(int i = 0; i < j; ++i) filename[i] = buf[j-i-1];
    filename[j] = '\0';

    return QString(filename);
}
