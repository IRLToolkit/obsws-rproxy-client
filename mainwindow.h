#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtWebSockets/QWebSocket>
#include <QSettings>
#include <QTime>
#include <QTimer>
#include <QString>
#include <QCloseEvent>
#include <QAbstractButton>
#include <QMessageBox>
#include <QPixmap>
#include <QDebug>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private Q_SLOTS:
    void closeEvent(QCloseEvent *event);
    void onReconnectTimerTimeout();
    void onLocalConnectClicked();
    void onRemoteConnectClicked();
    void onLocalSocketConnected();
    void onLocalSocketDisconnected();
    void onLocalSocketMessageReceived(QString message);
    void onRemoteSocketConnected();
    void onRemoteSocketDisconnected();
    void onRemoteSocketMessageReceived(QString message);

private:
    Ui::MainWindow *ui;
    QSettings settings;
    QWebSocket localSocket;
    QWebSocket remoteSocket;
    QTimer *reconnectTimer;
    bool canTouchRemoteButton;
    int reconnectTimerTotal;
    int reconnectTimerCurrentCountdown;
    void startReconnectTimer();
    void stopReconnectTimer();
};

#endif // MAINWINDOW_H
