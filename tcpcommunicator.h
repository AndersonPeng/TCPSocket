#ifndef TCPUTIL_H
#define TCPUTIL_H

#include <QObject>
#include <QString>
#include <WinSock2.h>


#define TCP_REQ_RES
#define REQ_UPLOAD   0
#define REQ_DOWNLOAD 1

#define RES_OK       0
#define RES_DENIED   1
#define RES_ERR      2


typedef struct{
    quint64 size;
    char name[256];
}fileInfo;


class TCPCommunicator : public QObject
{
    Q_OBJECT

public:
    TCPCommunicator();
    virtual ~TCPCommunicator();
    void setSocket(SOCKET sock);
    void setFilename(QString filename);
    void setRequest(char req);
    static QString getFileNameFromPath(QString filePath);


public slots:
    void sendFile();
    void recvFile();
    void sendReq();


signals:
    void error(QString err);
    void progress(unsigned long long curSize, unsigned long long totalSize);
    void response(char req, char res);
    void sendFileEnd();
    void recvFileEnd();
    void sendReqEnd();


private:
    SOCKET sock;
    QString filename;
    char req;
};

#endif // TCPUTIL_H
