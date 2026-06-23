#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QByteArray>
#include <QNetworkAccessManager>
#include <QSerialPort>
#include <QTimer>
#include <QVector>
#include <QWidget>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSlider;
class QStackedWidget;
class QTableWidget;
class QTabBar;
class QTextEdit;
class TrendChart;

class MainWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MainWidget(QWidget *parent = nullptr);
    ~MainWidget() override;

private slots:
    void pollStatus();
    void sendControl();
    void queryHistory();
    void startStream();
    void stopStream();
    void refreshFrame();
    void takeSnapshot();
    void toggleRecord();
    void openSerial();
    void closeSerial();
    void readSerialData();

private:
    QWidget *buildMainPage();
    QWidget *buildHistoryPage();
    QWidget *card(const QString &title, QWidget *body);
    QWidget *sensorCard(const QString &name, const QString &unit, const QString &color, QLabel **valueLabel);
    QWidget *latestCard(const QString &name, const QString &unit, QLabel **valueLabel);
    QPushButton *button(const QString &text, const QString &type = "secondary");
    void loadConfig();
    void applyStyle();
    void appendLog(const QString &message);
    void updateAlarm(double temp, double humi, int light, int ir);
    void setOnline(bool online);
    void refreshSerialPorts();

    QNetworkAccessManager *net_ = nullptr;
    QSerialPort *serial_ = nullptr;
    QByteArray serialBuffer_;
    QString apiBase_;

    QTimer *statusTimer_ = nullptr;
    QTimer *videoTimer_ = nullptr;
    bool streaming_ = false;
    bool recording_ = false;
    int motorDir_ = 0;

    QTabBar *tabs_ = nullptr;
    QStackedWidget *stack_ = nullptr;
    QLabel *connBadge_ = nullptr;
    QLineEdit *serverEdit_ = nullptr;

    QLabel *tempValue_ = nullptr;
    QLabel *humiValue_ = nullptr;
    QLabel *lightValue_ = nullptr;
    QLabel *irValue_ = nullptr;
    QLabel *latestTemp_ = nullptr;
    QLabel *latestHumi_ = nullptr;
    QLabel *latestLight_ = nullptr;
    QLabel *latestIr_ = nullptr;
    QTextEdit *alarmText_ = nullptr;

    QLabel *videoLabel_ = nullptr;
    QPushButton *streamStartBtn_ = nullptr;
    QPushButton *streamStopBtn_ = nullptr;
    QPushButton *recordBtn_ = nullptr;

    QCheckBox *ledCheck_ = nullptr;
    QSlider *ledSlider_ = nullptr;
    QLabel *ledLabel_ = nullptr;
    QCheckBox *motorCheck_ = nullptr;
    QSlider *motorSlider_ = nullptr;
    QLabel *motorLabel_ = nullptr;
    QPushButton *dirForwardBtn_ = nullptr;
    QPushButton *dirReverseBtn_ = nullptr;
    QCheckBox *buzzerCheck_ = nullptr;

    QComboBox *historyDevice_ = nullptr;
    QComboBox *historyLimit_ = nullptr;
    QTableWidget *historyTable_ = nullptr;
    TrendChart *trendChart_ = nullptr;

    QComboBox *serialPortBox_ = nullptr;
    QTextEdit *serialText_ = nullptr;
    QTextEdit *logText_ = nullptr;
};

#endif
