#define NOMINMAX
#define WIN32_LEAD_AND_MEAN
#include <windows.h>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QString>
#include <QDateTime>
#include <QAbstractSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QByteArray>
#include <QNetworkInterface>
#include <QSettings>
//#include <QRandomGenerator>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // ساخت سوکت
    socket = new QTcpSocket(this);

    loadSettings();

    connect(ui->lineEdit_serverIP, &QLineEdit::textChanged, this, &MainWindow::saveSettings);
    connect(ui->spinBox_serverPort, &QSpinBox::textChanged, this, &MainWindow::saveSettings);
    connect(ui->lineEdit_agentName, &QLineEdit::textChanged, this, &MainWindow::saveSettings);

    // ساخت تایمر برای ارسال دوره ای
    sendTimer = new QTimer(this);
    sendTimer->setInterval(2000);  // هر 2 ثانیه

    connect(socket, &QTcpSocket::connected, this, &MainWindow::onConnected );
    connect(socket, &QTcpSocket::disconnected, this, &MainWindow::onDisconnected );
    connect(socket, &QTcpSocket::errorOccurred, this, &MainWindow::onSocketError );

    connect(sendTimer, &QTimer::timeout, this, &MainWindow::sendMetrics);

    connect(ui->pushButton_connect, &QPushButton::clicked, this, &MainWindow::connectToServer);
    connect(ui->pushButton_disconnect, &QPushButton::clicked, this, &MainWindow::disconnectFromServer);

    //  وضعیت اولیه
    setUIEnabled(false);
    ui->label_status->setText("Disconnected");
    log("Agent ready.");
}

void MainWindow::connectToServer(){

    QString ip = ui->lineEdit_serverIP->text();
    quint16 port = static_cast<quint16>(ui->spinBox_serverPort->value());

    log(QString("Connecting to %1 : %2...").arg(ip).arg(port));
    ui->label_status->setText("Connecting...");

    socket->connectToHost(ip,port);
}

void MainWindow::disconnectFromServer(){

    log(">>> disconnectFromServer clicked.");
    log("Socket state: " + QString::number(socket->state()));

    if(socket->state() == QAbstractSocket::ConnectedState){
        log(">>> Stopping timer...");
        sendTimer->stop();
        log(">>> Timer active? " + QString(sendTimer->isActive() ? "Yes" : "No"));
        socket->disconnectFromHost();
    } else log(">>> Candition was false, no action taken");
}

void MainWindow::onConnected(){

    log("Connect to dashboard");
    ui->label_status->setText("Connected");
    setUIEnabled(true);

    sendTimer->start();
}

void MainWindow::onDisconnected(){

    log("Disconnected from dashbord");
    ui->label_status->setText("Disconnected");

    setUIEnabled(false);
    sendTimer->stop();
}

void MainWindow::onSocketError(QAbstractSocket::SocketError error){

    Q_UNUSED(error);
    log("Socket error: " + socket->errorString());
    ui->label_status->setText("Error");
}

void MainWindow::sendMetrics(){

    if(socket->state() != QAbstractSocket::ConnectedState)  return;


    int cpu = getCpuUsage();
    int ram = getRamUsageMB();
    // ساخت پیام JSON
    QJsonObject obj;
    obj["agent"] = ui->lineEdit_agentName->text();
    obj["cpu"] = cpu;
    obj["ram"] = ram;
    obj["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    QJsonDocument doc (obj);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    data.append('\n');      // جداکننده پیامها

    socket->write(data);
    socket->flush();

    log(QString("sent -> CPU: %1 %, RAM: %2 MB").arg(cpu).arg(ram));
}

void MainWindow::log(const QString &message){

    QString time = QDateTime::currentDateTime().toString("HH:mm:ss");
    ui->textEdit_log->append(QString("[%1] %2").arg(time).arg(message));
}

void MainWindow::setUIEnabled(bool connected){

    ui->pushButton_connect->setEnabled(!connected);
    ui->pushButton_disconnect->setEnabled(connected);
    ui->lineEdit_serverIP->setEnabled(!connected);
    ui->spinBox_serverPort->setEnabled(!connected);
    ui->lineEdit_agentName->setEnabled(!connected);
}

int MainWindow::getCpuUsage()
{
    FILETIME idleTime, kernelTime, userTime;
    if(!GetSystemTimes(&idleTime, &kernelTime, &userTime)) return 0;

    ULONGLONG idle = ( static_cast<ULONGLONG>(idleTime.dwHighDateTime) << 32 ) | idleTime.dwLowDateTime;
    ULONGLONG kernel = (static_cast<ULONGLONG>(kernelTime.dwHighDateTime) << 32) | kernelTime.dwLowDateTime;
    ULONGLONG user = (static_cast<ULONGLONG>(userTime.dwHighDateTime) << 32 ) | userTime.dwLowDateTime;

    if (lastIdleTime == 0){
        lastIdleTime = idle;
        lastKernelTime = kernel;
        lastUserTime = user;
        return 0;
    }

    ULONGLONG idleDiff = idle - lastIdleTime;
    ULONGLONG totalDiff = (kernel - lastKernelTime) + (user - lastUserTime);

    lastIdleTime = idle;
    lastKernelTime = kernel;
    lastUserTime = user;

    if(totalDiff == 0) return 0;
    return static_cast<int>((totalDiff - idleDiff) * 100 / totalDiff);
}

int MainWindow::getRamUsageMB()
{
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (!GlobalMemoryStatusEx(&memInfo))  return 0;

    ULONGLONG usedBytes = memInfo.ullTotalPhys - memInfo.ullAvailPhys;
    return static_cast<int>(usedBytes / (1024 * 1024));
}

QString MainWindow::getMacBasedName()
{
    const auto interfaces = QNetworkInterface::allInterfaces();
    for(const auto &iface : interfaces) {                         // فیلتر: فقط interface های معتبر و غیر loopback
        if(iface.flags().testFlag(QNetworkInterface::IsUp) &&
            iface.flags().testFlag(QNetworkInterface::IsRunning) &&
            !iface.flags().testAnyFlags(QNetworkInterface::IsLoopBack) &&
            !iface.hardwareAddress().isEmpty())
        {
            QString mac = iface.hardwareAddress();
            mac.remove(':');                        // تبدیل "AA:BB:CC:DD:EE:FF" به "AABBCC"
            mac.remove('-');
            return "agent-" + mac.right(8);
        }
    }
    return "agent-unknown";
}
void MainWindow::loadSettings()
{
    QSettings settings("SafaBanak", "ServerAgent");
    ui->lineEdit_serverIP->setText(settings.value("serverIP", "127.0.0.1").toString());
    ui->spinBox_serverPort->setValue(settings.value("serverPort", 12345).toInt());
    //اگر اسم ذخیره شده داشت همونو بزار وگرنه از MAC بساز
    //ui->lineEdit_agentName->setText(settings.value("agentName", getMacBasedName()).toString());

    QString defaultName = settings.value("agentName").toString();
    if (defaultName.isEmpty()) {
        defaultName = getMacBasedName();
    }
    ui->lineEdit_agentName->setText(defaultName);

}
void MainWindow::saveSettings()
{
    QSettings settings("SafaBanak", "ServerAgent");
    settings.setValue("serverIP", ui->lineEdit_serverIP->text());
    settings.setValue("serverPort", ui->spinBox_serverPort->value());
    settings.setValue("agentName", ui->lineEdit_agentName->text());
}
void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSettings();
    QMainWindow::closeEvent(event);
}

MainWindow::~MainWindow()
{
    if(socket->state() == QAbstractSocket::ConnectedState)
        socket->disconnectFromHost();

    delete ui;
}
