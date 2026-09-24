#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QTimer>
#include <QAbstractSocket>

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
    void log(const QString &message);
    void setUIEnabled(bool connected);

    Ui::MainWindow *ui;
    QTcpSocket *socket;
    QTimer *sendTimer;
};
#endif // MAINWINDOW_H
