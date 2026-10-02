#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QTimer>
#include <QAbstractSocket>
#include <windows.h>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void connectToServer();
    void disconnectFromServer();
    void onConnected();
    void onDisconnected();
    void onSocketError(QAbstractSocket::SocketError error);
    void sendMetrics();

private:
    ULONGLONG idle, kernel, user;
    ULONGLONG lastIdleTime;
    ULONGLONG lastKernelTime;
    ULONGLONG lastUserTime;
    void log(const QString &message);
    void setUIEnabled(bool connected);
    int getCpuUsage();
    int getRamUsageMB();
    Ui::MainWindow *ui;
    QTcpSocket *socket;
    QTimer *sendTimer;
};
#endif // MAINWINDOW_H
