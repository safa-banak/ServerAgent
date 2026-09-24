#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QString>
#include <QDateTime>
#include <QAbstractSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QByteArray>
#include <QRandomGenerator>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // ساخت سوکت
    socket = new QTcpSocket(this);

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


    int cpu = QRandomGenerator::global()->bounded(0, 101);
    int ram = QRandomGenerator::global()->bounded(512, 8192);
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

    log(QString("sent -> CPU: %1%%, RAM: %2 MB").arg(cpu).arg(ram));
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

MainWindow::~MainWindow()
{
    if(socket->state() == QAbstractSocket::ConnectedState)
        socket->disconnectFromHost();

    delete ui;
}
