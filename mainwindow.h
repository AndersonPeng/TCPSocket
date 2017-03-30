#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QUrl>

#include "droparea.h"
#include "tcpclient.h"
#include "tcpserver.h"
#include "tcpcommunicator.h"


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    static const int EXIT_CODE_REBOOT;

signals:


private slots:
    void clientConnect();
    void clientUpload();
    void onClientConnected();
    void onClientGetRes(char req, char res);

    void serverListen();
    void onServerConnected(int clientIndex);
    void onServerGetReq(char req);

    void setFileUrl(const QMimeData *mimeData);
    void setServerStatLabel(QString msg);
    void setClientStatLabel(QString msg);
    void onClientProgress(unsigned long curSize, unsigned long totalSize);
    void onServerProgress(unsigned long curSize, unsigned long totalSize);

    void reboot();


private:
    Ui::MainWindow *ui;
    WSADATA wsa;
    TCPClient *clientSocket;
    TCPServer *serverSocket;
    DropArea *dropArea;
    QList<QUrl> fileUrlList;
    int clientIndex;
};

#endif // MAINWINDOW_H
