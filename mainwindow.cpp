#include "mainwindow.h"
#include "ui_mainwindow.h"

void delay(int millisecondsToWait)
{
    QTime dieTime = QTime::currentTime().addMSecs( millisecondsToWait );
    while( QTime::currentTime() < dieTime )
    {
        QCoreApplication::processEvents( QEventLoop::AllEvents, 100 );
    }
}

void displayError(QString errorMessage)
{
    QMessageBox errBox;
    errBox.setWindowTitle("Error");
    errBox.setText(errorMessage);
    errBox.exec();
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->remoteConnectButton, &QPushButton::clicked, this, &MainWindow::onRemoteConnectClicked);
    connect(ui->localConnectButton, &QPushButton::clicked, this, &MainWindow::onLocalConnectClicked);
    
    connect(&localSocket, &QWebSocket::connected, this, &MainWindow::onLocalSocketConnected);
    connect(&localSocket, &QWebSocket::disconnected, this, &MainWindow::onLocalSocketDisconnected);
    connect(&localSocket, &QWebSocket::textMessageReceived, this, &MainWindow::onLocalSocketMessageReceived);
    connect(&remoteSocket, &QWebSocket::connected, this, &MainWindow::onRemoteSocketConnected);
    connect(&remoteSocket, &QWebSocket::disconnected, this, &MainWindow::onRemoteSocketDisconnected);
    connect(&remoteSocket, &QWebSocket::textMessageReceived, this, &MainWindow::onRemoteSocketMessageReceived);
    
    reconnectTimer = new QTimer(this);
    reconnectTimer->setSingleShot(true);
    connect(reconnectTimer, SIGNAL(timeout()), this, SLOT(onReconnectTimerTimeout()));
    canTouchRemoteButton = true;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (localSocket.isValid()) {
        localSocket.close();
    }
    if (remoteSocket.isValid()) {
        remoteSocket.close();
    }
    event->accept();
}

void MainWindow::onReconnectTimerTimeout()
{
    reconnectTimerCurrentCountdown--;
    ui->remoteConnectButton->setText(QString("Reconnect in %1...").arg(reconnectTimerCurrentCountdown + 1));
    ui->remoteConnectButton->setEnabled(true);
    if (reconnectTimerCurrentCountdown == -1) {
        if (reconnectTimerTotal <= 30) {
            reconnectTimerTotal *= 2;
        }
        reconnectTimerCurrentCountdown = reconnectTimerTotal;
        if (localSocket.state() == QAbstractSocket::ConnectedState) {
            QString connectUrl = ui->remoteServerLineEdit->text();
            if (connectUrl.isEmpty()) {
                displayError(QString("The Remote Server URL cannot be empty!"));
                stopReconnectTimer();
                return;
            } else if (!connectUrl.startsWith("ws://")) {
                displayError(QString("The Remote Server URL must be a websocket URL (starting with \"ws://\")"));
                stopReconnectTimer();
                return;
            }
            ui->remoteConnectButton->setText("Connecting...");
            ui->remoteConnectButton->setEnabled(false);
            remoteSocket.open(QUrl(connectUrl));
            int connectLoops = 0;
            while (true) {
                if (remoteSocket.state() == QAbstractSocket::ConnectedState) {
                    stopReconnectTimer();
                    return;
                } else if (localSocket.state() == QAbstractSocket::ConnectingState) {
                    if (connectLoops > 100) {
                        localSocket.close();
                        break;
                    }
                    delay(100);
                    connectLoops++;
                } else {
                    ui->remoteConnectButton->setText("Connect Failed. Resetting...");
                    ui->remoteConnectButton->setEnabled(false);
                    break;
                }
            }
        } else {
            stopReconnectTimer();
            return;
        }
    }
    reconnectTimer->start(1000);
}

void MainWindow::startReconnectTimer()
{
    if (reconnectTimer->isActive()) {
        return;
    }
    reconnectTimerTotal = 1;
    reconnectTimerCurrentCountdown = 1;
    canTouchRemoteButton = false;
    reconnectTimer->start(100);
}

void MainWindow::stopReconnectTimer()
{
    if (reconnectTimer->isActive()) {
        reconnectTimer->stop();
    }
    reconnectTimerTotal = 0;
    reconnectTimerCurrentCountdown = 0;
    canTouchRemoteButton = true;
    if (remoteSocket.state() == QAbstractSocket::ConnectedState) {
        ui->remoteConnectButton->setText("Disconnect");
    } else {
        ui->remoteConnectButton->setText("Connect");
    }
    ui->remoteConnectButton->setEnabled(true);
}

void MainWindow::onLocalConnectClicked()
{
    if (localSocket.state() == QAbstractSocket::ConnectedState) {
        localSocket.close();
    } else {
        if (ui->wsHostnameLineEdit->text().isEmpty()) {
            displayError(QString("The OBSWS Hostname cannot be empty!"));
            return;
        }
        ui->localConnectButton->setText("Connecting...");
        ui->localConnectButton->setEnabled(false);
        QString connectUrl = QString("ws://%1:%2").arg(ui->wsHostnameLineEdit->text()).arg(ui->wsPortSpinBox->value());
        localSocket.open(QUrl(connectUrl));
        int connectLoops = 0;
        while (true) {
            if (localSocket.state() == QAbstractSocket::ConnectedState) {
                return;
            } else if (localSocket.state() == QAbstractSocket::ConnectingState) {
                if (connectLoops > 100) {
                    displayError(QString("Failed to connect to local obs-websocket server because the request timed out."));
                    localSocket.close();
                    return;
                }
                delay(100);
                connectLoops++;
            } else {
                displayError(QString("Failed to connect to local obs-websocket server with error: %1").arg(localSocket.errorString()));
                return;
            }
        }
    }
}

void MainWindow::onRemoteConnectClicked()
{
    if (reconnectTimer->isActive()) {
        stopReconnectTimer();
    } else if (remoteSocket.state() == QAbstractSocket::ConnectedState) {
        remoteSocket.close();
    } else {
        if (localSocket.state() == QAbstractSocket::ConnectedState) {
            QString connectUrl = ui->remoteServerLineEdit->text();
            if (connectUrl.isEmpty()) {
                displayError(QString("The Remote Server URL cannot be empty!"));
                return;
            } else if (!connectUrl.startsWith("ws://")) {
                displayError(QString("The Remote Server URL must be a websocket URL (starting with \"ws://\")"));
                return;
            }
            ui->remoteConnectButton->setText("Connecting...");
            ui->remoteConnectButton->setEnabled(false);
            remoteSocket.open(QUrl(connectUrl));
        } else {
            displayError(QString("You must be connected to the obs-websocket Server before you can connect to the remote server."));
            return;
        }
    }
}

void MainWindow::onLocalSocketConnected()
{
    ui->localStatusIcon->setPixmap(QPixmap(":/data/images/checkmark"));
    ui->localConnectButton->setText("Disconnect");
    ui->localConnectButton->setEnabled(true);
}

void MainWindow::onLocalSocketDisconnected()
{
    if (remoteSocket.state() == QAbstractSocket::ConnectedState) {
        remoteSocket.close();
    }
    ui->localStatusIcon->setPixmap(QPixmap(":/data/images/times"));
    ui->localConnectButton->setText("Connect");
    ui->localConnectButton->setEnabled(true);
}

void MainWindow::onLocalSocketMessageReceived(QString message)
{
    if (remoteSocket.isValid()) {
        remoteSocket.sendTextMessage(message);
    }
}

void MainWindow::onRemoteSocketConnected()
{
    stopReconnectTimer();
    ui->remoteStatusIcon->setPixmap(QPixmap(":/data/images/checkmark"));
}

void MainWindow::onRemoteSocketDisconnected()
{
    ui->remoteStatusIcon->setPixmap(QPixmap(":/data/images/times"));
    if (canTouchRemoteButton) {
        ui->remoteConnectButton->setText("Connect");
        ui->remoteConnectButton->setEnabled(true);
    }
    if (ui->remoteServerReconnectCheckBox->checkState() == Qt::Checked) {
        startReconnectTimer();
    }
}

void MainWindow::onRemoteSocketMessageReceived(QString message)
{
    if (localSocket.isValid()) {
        localSocket.sendTextMessage(message);
    }
}