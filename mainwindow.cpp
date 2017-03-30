#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <cstdio>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QString>
#include <QFile>
#include <QMessageBox>
#include <QThread>


const int MainWindow::EXIT_CODE_REBOOT = -8888;


/*===============================
Constructor
===============================*/
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);


    //Init winsock
    if(WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        qDebug() << "Failed to init winsock";

    clientSocket = new TCPClient();
    serverSocket = new TCPServer();
    clientIndex = -1;


    //Get local ip addresses
    QList<QHostAddress> addrList = QNetworkInterface::allAddresses();
    QString str = "";

    for(int i = 0; i < addrList.size(); ++i){
        QHostAddress addr = addrList.at(i);

        if(addr != QHostAddress::LocalHost && addr.toIPv4Address())
            str.append(addr.toString() + "\n");
    }

    ui->serverIpBrowser->verticalScrollBar();
    ui->serverIpBrowser->setText(str);


    //Create a drop area
    dropArea = new DropArea(ui->clientTab);
    dropArea->resize(420, 80);
    dropArea->move(20, 140);


    //Connect signals & slots
    connect(ui->clientConnectBtn, SIGNAL(clicked()), this, SLOT(clientConnect()));
    connect(ui->clientUploadBtn, SIGNAL(clicked()), this, SLOT(clientUpload()));
    connect(ui->serverListenBtn, SIGNAL(clicked()), this, SLOT(serverListen()));

    connect(serverSocket, SIGNAL(connected(int)), this, SLOT(onServerConnected(int)));
    connect(serverSocket, SIGNAL(request(char)), this, SLOT(onServerGetReq(char)));
    connect(serverSocket, SIGNAL(error(QString)), this, SLOT(setServerStatLabel(QString)));

    connect(clientSocket, SIGNAL(connected()), this, SLOT(onClientConnected()));
    connect(clientSocket, SIGNAL(error(QString)), this, SLOT(setClientStatLabel(QString)));

    connect(dropArea, SIGNAL(changed(const QMimeData*)), this, SLOT(setFileUrl(const QMimeData*)));
    connect(ui->actionRestart, SIGNAL(triggered()), this, SLOT(reboot()));

}


/*===============================
Destructor
===============================*/
MainWindow::~MainWindow()
{
    delete ui;
    delete clientSocket;
    delete serverSocket;
    delete dropArea;

    WSACleanup();
}


/*===============================
SLOT: Client connet to server
===============================*/
void MainWindow::clientConnect()
{
    if(clientSocket->isConnectedToServer()){
        qDebug() << "Already connected";
        return;
    }

    int port = ui->clientPortEdit->text().toInt();
    const char *ip = ui->clientIpEdit->text().toUtf8().constData();

    clientSocket->setServer(ip, port);


    //Start thread to connect
    setClientStatLabel(QString("Connecting to server: %1:%2 ...").arg(ip).arg(port));
    disconnect(ui->clientConnectBtn, SIGNAL(clicked()), this, SLOT(clientConnect()));

    QThread *thread = new QThread;
    clientSocket->moveToThread(thread);
    connect(thread, SIGNAL(started()), clientSocket, SLOT(connectToServer()));
    connect(clientSocket, SIGNAL(connectEnd()), thread, SLOT(quit()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    thread->start();
}


/*===============================
SLOT: server listen to clients
===============================*/
void MainWindow::serverListen()
{
    int port = ui->serverPortEdit->text().toInt();


    //Bind port
    if(!serverSocket->bindPort(port)){
        setServerStatLabel("Server failed to bind port");
        return;
    }


    //Listen to clients
    if(!serverSocket->listenToClient()){
        setServerStatLabel("Server failed to listen");
        return;
    }


    //Start a thread to accept
    setServerStatLabel("Server listening...");
    disconnect(ui->serverListenBtn, SIGNAL(clicked()), this, SLOT(serverListen()));

    QThread *thread = new QThread;
    serverSocket->moveToThread(thread);
    connect(thread, SIGNAL(started()), serverSocket, SLOT(acceptClient()));
    connect(serverSocket, SIGNAL(acceptClientEnd()), thread, SLOT(quit()));       //Shut down thread
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));       //Delete thread when it is shut down
    thread->start();
}


/*===============================
SLOT: Client upload a file
===============================*/
void MainWindow::clientUpload()
{
    if(fileUrlList.size() <= 0) return;


    //Start a thread to send upload request===========
    setClientStatLabel("Request to upload...");
    disconnect(ui->clientUploadBtn, SIGNAL(clicked()), this, SLOT(clientUpload()));

    QThread *thread = new QThread;
    TCPCommunicator *sender = new TCPCommunicator;
    sender->setSocket(clientSocket->getSocket());
    sender->setRequest(REQ_UPLOAD);
    sender->moveToThread(thread);
    connect(sender, SIGNAL(error(QString)), this, SLOT(setClientStatLabel(QString)));
    connect(sender, SIGNAL(response(char, char)), this, SLOT(onClientGetRes(char, char)));
    connect(thread, SIGNAL(started()), sender, SLOT(sendReq()));
    connect(sender, SIGNAL(sendReqEnd()), thread, SLOT(quit()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    thread->start();
}


/*===============================
SLOT: client get a response
===============================*/
void MainWindow::onClientGetRes(char req, char res)
{
    if(res == RES_OK){
        if(req == REQ_UPLOAD){
            setClientStatLabel("Start to upload...");

            QThread *thread = new QThread;
            TCPCommunicator *sender = new TCPCommunicator;
            sender->setSocket(clientSocket->getSocket());
            sender->setFilename(fileUrlList.at(0).toLocalFile());
            sender->moveToThread(thread);
            connect(sender, SIGNAL(error(QString)), this, SLOT(setClientStatLabel(QString)));
            connect(sender, SIGNAL(progress(ulong,ulong)), this, SLOT(onClientProgress(ulong,ulong)));
            connect(thread, SIGNAL(started()), sender, SLOT(sendFile()));
            connect(sender, SIGNAL(sendFileEnd()), thread, SLOT(quit()));
            connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
            thread->start();
        }
        else if(req == REQ_DOWNLOAD){
            setClientStatLabel("Start to download...");
        }
        else{
            setClientStatLabel("ERROR: wrong request");
        }
    }
    else if(res == RES_DENIED){
        setClientStatLabel("ERROR: request denied");
    }
}


/*===============================
SLOT: server get a requset
===============================*/
void MainWindow::onServerGetReq(char req)
{
    if(req == REQ_DOWNLOAD){
        //Download request: Start thread to send
        setServerStatLabel("Download request");
    }
    else if(req == REQ_UPLOAD){
        //Upload request: Start thread to recv
        setServerStatLabel("Receiving data...");

        QThread *thread = new QThread;
        TCPCommunicator *receiver = new TCPCommunicator;
        receiver->setSocket(serverSocket->getClientSocket(clientIndex));
        receiver->moveToThread(thread);
        connect(receiver, SIGNAL(error(QString)), this, SLOT(setServerStatLabel(QString)));
        connect(receiver, SIGNAL(progress(ulong,ulong)), this, SLOT(onServerProgress(ulong,ulong)));
        connect(thread, SIGNAL(started()), receiver, SLOT(recvFile()));
        connect(receiver, SIGNAL(recvFileEnd()), thread, SLOT(quit()));
        connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
        thread->start();
    }
    else
        setServerStatLabel("Wrong request");
}


/*===============================
SLOT: Set file name
===============================*/
void MainWindow::setFileUrl(const QMimeData *mimeData)
{
    if(!mimeData) return;

    if(mimeData->hasUrls())
        fileUrlList = mimeData->urls();
}


/*===============================
SLOT: Set server status label
===============================*/
void MainWindow::setServerStatLabel(QString msg)
{
    ui->serverStatLabel->setText(msg);
    ui->serverStatLabel->repaint();
}


/*===============================
SLOT: Set client status label
===============================*/
void MainWindow::setClientStatLabel(QString msg)
{
    ui->clientStatLabel->setText(msg);
    ui->clientStatLabel->repaint();
}


/*===============================
SLOT: when client connect to server
===============================*/
void MainWindow::onClientConnected()
{
    setClientStatLabel("Connected");
    ui->clientServerInfoLabel->setText(QString("Connected to server:\n%1:%2")
                       .arg(clientSocket->getServerIp())
                       .arg(clientSocket->getServerPort()));
    ui->clientServerInfoLabel->repaint();
    connect(ui->clientConnectBtn, SIGNAL(clicked()), this, SLOT(clientConnect()));
}


/*===============================
SLOT: when server connect to client
===============================*/
void MainWindow::onServerConnected(int clientIndex)
{
    this->clientIndex = clientIndex;

    setServerStatLabel("Connected");
    ui->serverClientInfoLabel->setText(QString("Connected to client:\n%1:%2")
                       .arg(serverSocket->getClientIP(clientIndex))
                       .arg(serverSocket->getClientPort(clientIndex)));
    ui->serverClientInfoLabel->repaint();
    connect(ui->serverListenBtn, SIGNAL(clicked()), this, SLOT(serverListen()));


    //Start a thread to wait requests
    QThread *thread = new QThread;
    serverSocket->setClientIndex(clientIndex);
    serverSocket->moveToThread(thread);
    connect(thread, SIGNAL(started()), serverSocket, SLOT(waitReq()));
    connect(serverSocket, SIGNAL(waitReqEnd()), thread, SLOT(quit()));      //Shut down thread
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));       //Delete thread when it is shut down
    thread->start();
}


/*===============================
SLOT: when there is a progress
===============================*/
void MainWindow::onClientProgress(unsigned long curSize, unsigned long totalSize)
{
    ui->clientProgBar->setValue((float)curSize / (float)totalSize * 100);
    ui->clientProgBar->repaint();
    setClientStatLabel(QString("%1/%2").arg(curSize).arg(totalSize));
}


/*===============================
SLOT: when there is a progress
===============================*/
void MainWindow::onServerProgress(unsigned long curSize, unsigned long totalSize)
{
    ui->serverProgBar->setValue((float)curSize / (float)totalSize * 100);
    ui->serverProgBar->repaint();
    setServerStatLabel(QString("%1/%2").arg(curSize).arg(totalSize));
}


/*===============================
SLOT: reboot application
===============================*/
void MainWindow::reboot()
{
    qApp->exit(MainWindow::EXIT_CODE_REBOOT);
}
