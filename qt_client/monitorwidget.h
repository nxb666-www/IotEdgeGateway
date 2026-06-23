#ifndef MONITORWIDGET_H
#define MONITORWIDGET_H

#include <QWidget>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>

namespace Ui {
class MonitorWidget;
}

class MonitorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MonitorWidget(QWidget *parent = nullptr);
    ~MonitorWidget();

signals:
    void connectionStatusChanged(bool online);

private slots:
    void pollSensorData();
    void pollVideoFrame();
    void onFrameReply();
    void cmd();
    void startStream();
    void stopStream();
    void takeSnapshot();
    void toggleRecord();

private:
    void addLog(const QString& msg);

    Ui::MonitorWidget *ui;
    QNetworkAccessManager *netManager_;
    QTimer *sensorTimer_;
    QTimer *frameTimer_;
    bool streamRunning_ = false;
    bool recording_ = false;
    int motorDir_ = 0;
};

#endif // MONITORWIDGET_H
