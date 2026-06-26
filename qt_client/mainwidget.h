#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QByteArray>
#include <QNetworkAccessManager>
#include <QString>
#include <QTimer>
#include <QWidget>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QNetworkReply;
class QPushButton;
class QSlider;
class QStackedWidget;
class QTableWidget;
class QTabBar;
class QTextEdit;

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
    void takeSnapshot();
    void toggleRecord();
    void loadPhotos();
    void loadVideos();
    void loadLogs();
    void clearRemoteLogs();
    void openHistoryItem(int row, int column);
    // TODO: Zigbee 串口接口，RK3568 烧写工具就绪后启用
    // void openSerial();
    // void closeSerial();

private:
    QWidget *buildMainPage();
    QWidget *buildHistoryPage();
    QWidget *card(const QString &title, QWidget *body);
    QPushButton *button(const QString &text, const QString &type = "secondary");
    QWidget *metricCard(const QString &title, const QString &unit, QLabel **value);
    void loadConfig();
    void applyStyle();
    void appendLog(const QString &message);
    void setOnline(bool online);
    void updateMetrics(const QJsonObject &obj);
    void setHistoryHeaders(const QStringList &headers);
    void startMjpegStream(const QUrl &url);
    void consumeMjpegData();
    void fillMediaTable(const QJsonArray &items, const QString &basePath, bool megabytes);

    QNetworkAccessManager *net_ = nullptr;
    QTimer *statusTimer_ = nullptr;
    QNetworkReply *mjpegReply_ = nullptr;
    QByteArray mjpegBuffer_;

    QString apiBase_;
    QString historyMode_ = "telemetry";
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
    QLabel *videoLabel_ = nullptr;
    QPushButton *streamStartBtn_ = nullptr;
    QPushButton *streamStopBtn_ = nullptr;
    QPushButton *recordBtn_ = nullptr;

    QCheckBox *ledCheck_ = nullptr;
    QSlider *ledSlider_ = nullptr;
    QLabel *ledText_ = nullptr;
    QCheckBox *motorCheck_ = nullptr;
    QSlider *motorSlider_ = nullptr;
    QLabel *motorText_ = nullptr;
    QPushButton *dirForwardBtn_ = nullptr;
    QPushButton *dirReverseBtn_ = nullptr;
    QCheckBox *buzzerCheck_ = nullptr;

    QComboBox *historyDevice_ = nullptr;
    QComboBox *historyLimit_ = nullptr;
    QTableWidget *historyTable_ = nullptr;
    QTextEdit *logText_ = nullptr;
    // TODO: Zigbee 串口显示区，启用时取消注释
    // QTextEdit *serialText_ = nullptr;
};

#endif
