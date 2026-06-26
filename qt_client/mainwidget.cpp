#include "mainwidget.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkReply>
#include <QPixmap>
#include <QPushButton>
#include <QSettings>
#include <QSlider>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTabBar>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextEdit>
#include <QUrl>
#include <QUrlQuery>
#include <QVBoxLayout>

namespace {

const char *kPrimaryButton =
    "QPushButton{background:#2563eb;color:white;border:0;border-radius:6px;padding:8px 12px;font-weight:700;}"
    "QPushButton:hover{background:#1d4ed8;} QPushButton:disabled{background:#cbd5e1;color:#64748b;}";

const char *kSecondaryButton =
    "QPushButton{background:#f1f5f9;color:#334155;border:1px solid #dbe3ef;border-radius:6px;padding:8px 12px;font-weight:700;}"
    "QPushButton:hover{background:#e2e8f0;} QPushButton:checked{background:#dbeafe;color:#1d4ed8;border-color:#2563eb;}";

const char *kDangerButton =
    "QPushButton{background:#ef4444;color:white;border:0;border-radius:6px;padding:8px 12px;font-weight:700;}"
    "QPushButton:hover{background:#dc2626;}";

const char *kSuccessButton =
    "QPushButton{background:#10b981;color:white;border:0;border-radius:6px;padding:8px 12px;font-weight:700;}"
    "QPushButton:hover{background:#059669;}";

QString valueText(const QJsonObject &obj, const QString &key, int precision = 1)
{
    const QJsonValue v = obj.value(key);
    if (v.isUndefined() || v.isNull()) return "--";
    if (v.isString() && v.toString().isEmpty()) return "--";
    if (v.isDouble()) return QString::number(v.toDouble(), 'f', precision);
    return v.toVariant().toString();
}

QString tsText(double sec)
{
    if (sec <= 0) return "--";
    return QDateTime::fromSecsSinceEpoch(static_cast<qint64>(sec)).toString("yyyy-MM-dd HH:mm:ss");
}

} // namespace

MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent),
      net_(new QNetworkAccessManager(this)),
      statusTimer_(new QTimer(this))
{
    loadConfig();
    applyStyle();

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(14, 14, 14, 14);
    root->setSpacing(12);

    auto *header = new QFrame;
    header->setObjectName("header");
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(18, 8, 18, 8);

    auto *title = new QLabel(QStringLiteral("RK3568 智能网关控制系统"));
    title->setObjectName("headerTitle");
    connBadge_ = new QLabel(QStringLiteral("连接中"));
    connBadge_->setObjectName("statusTag");
    connBadge_->setAlignment(Qt::AlignCenter);
    serverEdit_ = new QLineEdit(apiBase_);
    serverEdit_->setMinimumWidth(120);
    serverEdit_->setMaximumWidth(220);
    serverEdit_->setObjectName("serverEdit");

    headerLayout->addWidget(title);
    headerLayout->addSpacing(20);
    auto *mqttBtn = button(QStringLiteral("MQTT"), "primary");
    mqttBtn->setMaximumWidth(80);
    auto *zigbeeBtn = button(QStringLiteral("Zigbee"));
    zigbeeBtn->setMaximumWidth(80);
    zigbeeBtn->setToolTip(QStringLiteral("Zigbee 待接入"));
    connect(mqttBtn, &QPushButton::clicked, this, [this]() {
        appendLog(QStringLiteral("当前通信: MQTT"));
    });
    connect(zigbeeBtn, &QPushButton::clicked, this, [this]() {
        appendLog(QStringLiteral("Zigbee 模块待接入"));
    });
    headerLayout->addWidget(mqttBtn);
    headerLayout->addWidget(zigbeeBtn);
    headerLayout->addStretch();
    headerLayout->addWidget(new QLabel(QStringLiteral("网关")));
    headerLayout->addWidget(serverEdit_);
    headerLayout->addWidget(connBadge_);
    root->addWidget(header);

    tabs_ = new QTabBar;
    tabs_->addTab(QStringLiteral("实时监控"));
    tabs_->addTab(QStringLiteral("历史与媒体"));
    tabs_->setExpanding(true);
    root->addWidget(tabs_);

    stack_ = new QStackedWidget;
    stack_->addWidget(buildMainPage());
    stack_->addWidget(buildHistoryPage());
    root->addWidget(stack_, 1);

    connect(tabs_, &QTabBar::currentChanged, stack_, &QStackedWidget::setCurrentIndex);
    connect(tabs_, &QTabBar::currentChanged, this, [this](int index) {
        if (index == 1) queryHistory();
    });
    connect(serverEdit_, &QLineEdit::editingFinished, this, [this]() {
        apiBase_ = serverEdit_->text().trimmed();
        appendLog(QStringLiteral("网关地址切换: ") + apiBase_);
        pollStatus();
    });
    connect(statusTimer_, &QTimer::timeout, this, &MainWidget::pollStatus);

    statusTimer_->start(1000);
    pollStatus();
    appendLog(QStringLiteral("Qt 客户端启动完成"));
}

MainWidget::~MainWidget()
{
    if (mjpegReply_) {
        mjpegReply_->abort();
    }
}

void MainWidget::loadConfig()
{
    QSettings cfg(QCoreApplication::applicationDirPath() + "/iotgw_qt_client.ini", QSettings::IniFormat);
    apiBase_ = cfg.value("server/url", "http://192.168.233.107:8081").toString();
}

void MainWidget::applyStyle()
{
    setMinimumSize(640, 480);
    setWindowTitle(QStringLiteral("IotEdgeGateway Qt Client"));
    setStyleSheet(
        "QWidget{font-family:'Microsoft YaHei','Segoe UI';font-size:13px;color:#1f2937;background:#eef2f7;}"
        "QFrame#header{background:#1f2937;border-radius:8px;min-height:48px;}"
        "#headerTitle{color:white;font-size:22px;font-weight:900;}"
        "#statusTag{background:#334155;color:white;border-radius:14px;padding:5px 14px;font-weight:700;}"
        "#serverEdit{background:#111827;color:white;border:1px solid #475569;border-radius:6px;padding:6px;}"
        "QTabBar::tab{height:38px;background:white;color:#64748b;font-weight:800;padding:0 20px;border:0;}"
        "QTabBar::tab:selected{background:#dbeafe;color:#1d4ed8;border-bottom:3px solid #2563eb;}"
        "QFrame#card{background:white;border:1px solid #e2e8f0;border-radius:8px;}"
        "QFrame#metric{background:#f8fafc;border:1px solid #e2e8f0;border-radius:8px;}"
        "QLabel#cardTitle{font-size:16px;font-weight:900;color:#111827;}"
        "QTextEdit{background:#0f172a;color:#38bdf8;border:1px solid #1e293b;border-radius:7px;padding:8px;font-family:Consolas,'Microsoft YaHei';}"
        "QTableWidget{background:white;border:1px solid #e2e8f0;border-radius:7px;gridline-color:#f1f5f9;alternate-background-color:#fafbfc;}"
        "QHeaderView::section{background:#f1f5f9;border:0;border-bottom:1px solid #e2e8f0;padding:8px;font-weight:800;color:#475569;}"
        "QComboBox,QLineEdit{background:white;border:1px solid #dbe3ef;border-radius:6px;padding:7px;}"
        "QSlider::groove:horizontal{height:6px;background:#dbe3ef;border-radius:3px;}"
        "QSlider::handle:horizontal{width:16px;margin:-5px 0;background:#2563eb;border-radius:8px;}"
        "QCheckBox{font-weight:700;color:#334155;background:transparent;}");
}

QPushButton *MainWidget::button(const QString &text, const QString &type)
{
    auto *btn = new QPushButton(text);
    if (type == "primary") btn->setStyleSheet(kPrimaryButton);
    else if (type == "success") btn->setStyleSheet(kSuccessButton);
    else if (type == "danger") btn->setStyleSheet(kDangerButton);
    else btn->setStyleSheet(kSecondaryButton);
    return btn;
}

QWidget *MainWidget::card(const QString &title, QWidget *body)
{
    auto *frame = new QFrame;
    frame->setObjectName("card");
    auto *layout = new QVBoxLayout(frame);
    layout->setContentsMargins(14, 12, 14, 14);
    layout->setSpacing(10);
    auto *label = new QLabel(title);
    label->setObjectName("cardTitle");
    layout->addWidget(label);
    layout->addWidget(body, 1);
    return frame;
}

QWidget *MainWidget::metricCard(const QString &title, const QString &unit, QLabel **value)
{
    auto *box = new QFrame;
    box->setObjectName("metric");
    auto *layout = new QVBoxLayout(box);
    layout->setContentsMargins(10, 10, 10, 10);
    auto *name = new QLabel(title);
    name->setAlignment(Qt::AlignCenter);
    name->setStyleSheet("color:#64748b;font-weight:700;");
    auto *val = new QLabel("--");
    val->setAlignment(Qt::AlignCenter);
    val->setStyleSheet("font-size:26px;font-weight:900;color:#2563eb;");
    auto *unitLabel = new QLabel(unit);
    unitLabel->setAlignment(Qt::AlignCenter);
    unitLabel->setStyleSheet("color:#94a3b8;font-size:12px;");
    layout->addWidget(name);
    layout->addWidget(val);
    layout->addWidget(unitLabel);
    *value = val;
    return box;
}

QWidget *MainWidget::buildMainPage()
{
    auto *page = new QWidget;
    auto *grid = new QGridLayout(page);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setSpacing(12);

    auto *left = new QWidget;
    auto *leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(12);

    auto *videoBody = new QWidget;
    auto *videoLayout = new QVBoxLayout(videoBody);
    videoLayout->setContentsMargins(0, 0, 0, 0);
    videoLabel_ = new QLabel(QStringLiteral("视频流未启动"));
    videoLabel_->setAlignment(Qt::AlignCenter);
    videoLabel_->setMinimumHeight(220);
    videoLabel_->setStyleSheet("background:#020617;color:#94a3b8;border-radius:8px;font-size:18px;");
    videoLayout->addWidget(videoLabel_, 1);
    auto *videoBtns = new QHBoxLayout;
    streamStartBtn_ = button(QStringLiteral("开启视频"), "primary");
    streamStopBtn_ = button(QStringLiteral("停止视频"));
    auto *snapshotBtn = button(QStringLiteral("拍照"), "success");
    recordBtn_ = button(QStringLiteral("开始录像"), "danger");
    streamStopBtn_->setEnabled(false);
    videoBtns->addWidget(streamStartBtn_);
    videoBtns->addWidget(streamStopBtn_);
    videoBtns->addWidget(snapshotBtn);
    videoBtns->addWidget(recordBtn_);
    videoLayout->addLayout(videoBtns);
    leftLayout->addWidget(card(QStringLiteral("视频监控"), videoBody), 1);

    auto *metrics = new QWidget;
    auto *metricGrid = new QGridLayout(metrics);
    metricGrid->setContentsMargins(0, 0, 0, 0);
    metricGrid->setSpacing(10);
    metricGrid->addWidget(metricCard(QStringLiteral("温度"), QStringLiteral("C"), &tempValue_), 0, 0);
    metricGrid->addWidget(metricCard(QStringLiteral("湿度"), QStringLiteral("%RH"), &humiValue_), 0, 1);
    metricGrid->addWidget(metricCard(QStringLiteral("光照"), QStringLiteral("Lux"), &lightValue_), 0, 2);
    metricGrid->addWidget(metricCard(QStringLiteral("红外"), QStringLiteral("状态"), &irValue_), 0, 3);
    leftLayout->addWidget(metrics);

    auto *right = new QWidget;
    auto *rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(12);

    auto *controlBody = new QWidget;
    auto *controlLayout = new QVBoxLayout(controlBody);
    controlLayout->setContentsMargins(0, 0, 0, 0);
    controlLayout->setSpacing(10);

    auto makeSlider = [&](const QString &title, QCheckBox **check, QSlider **slider, QLabel **text, int value) {
        auto *box = new QFrame;
        box->setObjectName("metric");
        auto *layout = new QVBoxLayout(box);
        auto *top = new QHBoxLayout;
        top->addWidget(new QLabel(title));
        top->addStretch();
        *check = new QCheckBox;
        (*check)->hide();
        auto *toggle = button(QStringLiteral("OFF"), "success");
        top->addWidget(toggle);
        top->addWidget(*check);
        *slider = new QSlider(Qt::Horizontal);
        (*slider)->setRange(0, 100);
        (*slider)->setValue(value);
        *text = new QLabel(QString::number(value) + "%");
        layout->addLayout(top);
        layout->addWidget(*slider);
        layout->addWidget(*text);
        controlLayout->addWidget(box);
        connect(toggle, &QPushButton::clicked, this, [check, toggle]() {
            (*check)->setChecked(!(*check)->isChecked());
            toggle->setText((*check)->isChecked() ? QStringLiteral("ON") : QStringLiteral("OFF"));
        });
    };

    makeSlider(QStringLiteral("LED 照明"), &ledCheck_, &ledSlider_, &ledText_, 50);
    makeSlider(QStringLiteral("风扇电机"), &motorCheck_, &motorSlider_, &motorText_, 30);

    auto *dirRow = new QHBoxLayout;
    dirForwardBtn_ = button(QStringLiteral("正转"));
    dirReverseBtn_ = button(QStringLiteral("反转"));
    dirForwardBtn_->setCheckable(true);
    dirReverseBtn_->setCheckable(true);
    dirForwardBtn_->setChecked(true);
    dirRow->addWidget(dirForwardBtn_);
    dirRow->addWidget(dirReverseBtn_);
    controlLayout->addLayout(dirRow);

    auto *buzzerRow = new QHBoxLayout;
    buzzerRow->addWidget(new QLabel(QStringLiteral("蜂鸣器")));
    buzzerRow->addStretch();
    buzzerCheck_ = new QCheckBox;
    buzzerCheck_->hide();
    auto *buzzerToggleBtn = button(QStringLiteral("OFF"), "success");
    buzzerRow->addWidget(buzzerToggleBtn);
    buzzerRow->addWidget(buzzerCheck_);
    controlLayout->addLayout(buzzerRow);
    rightLayout->addWidget(card(QStringLiteral("外设控制"), controlBody));

    logText_ = new QTextEdit;
    logText_->setReadOnly(true);
    rightLayout->addWidget(card(QStringLiteral("运行日志"), logText_), 1);

    grid->addWidget(left, 0, 0);
    grid->addWidget(right, 0, 1);
    grid->setColumnStretch(0, 70);
    grid->setColumnStretch(1, 30);

    connect(streamStartBtn_, &QPushButton::clicked, this, &MainWidget::startStream);
    connect(streamStopBtn_, &QPushButton::clicked, this, &MainWidget::stopStream);
    connect(snapshotBtn, &QPushButton::clicked, this, &MainWidget::takeSnapshot);
    connect(recordBtn_, &QPushButton::clicked, this, &MainWidget::toggleRecord);
    connect(ledCheck_, &QCheckBox::toggled, this, &MainWidget::sendControl);
    connect(motorCheck_, &QCheckBox::toggled, this, &MainWidget::sendControl);
    connect(buzzerCheck_, &QCheckBox::toggled, this, &MainWidget::sendControl);
    connect(buzzerToggleBtn, &QPushButton::clicked, this, [this, buzzerToggleBtn]() {
        buzzerCheck_->setChecked(!buzzerCheck_->isChecked());
        buzzerToggleBtn->setText(buzzerCheck_->isChecked() ? QStringLiteral("ON") : QStringLiteral("OFF"));
    });
    connect(ledSlider_, &QSlider::valueChanged, this, [this](int v) { ledText_->setText(QString::number(v) + "%"); });
    connect(motorSlider_, &QSlider::valueChanged, this, [this](int v) { motorText_->setText(QString::number(v) + "%"); });
    connect(ledSlider_, &QSlider::sliderReleased, this, &MainWidget::sendControl);
    connect(motorSlider_, &QSlider::sliderReleased, this, &MainWidget::sendControl);
    connect(dirForwardBtn_, &QPushButton::clicked, this, [this]() {
        motorDir_ = 0;
        dirForwardBtn_->setChecked(true);
        dirReverseBtn_->setChecked(false);
        sendControl();
    });
    connect(dirReverseBtn_, &QPushButton::clicked, this, [this]() {
        motorDir_ = 1;
        dirForwardBtn_->setChecked(false);
        dirReverseBtn_->setChecked(true);
        sendControl();
    });
    return page;
}

QWidget *MainWidget::buildHistoryPage()
{
    auto *page = new QWidget;
    auto *grid = new QGridLayout(page);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setSpacing(12);

    auto *left = new QWidget;
    auto *leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(10);

    historyDevice_ = new QComboBox;
    historyDevice_->addItem(QStringLiteral("温度"), "temp");
    historyDevice_->addItem(QStringLiteral("湿度"), "humi");
    historyDevice_->addItem(QStringLiteral("光照"), "light");
    historyDevice_->addItem(QStringLiteral("红外"), "ir");
    historyLimit_ = new QComboBox;
    historyLimit_->addItem(QStringLiteral("最近 20 条"), 20);
    historyLimit_->addItem(QStringLiteral("最近 50 条"), 50);
    historyLimit_->addItem(QStringLiteral("最近 100 条"), 100);

    auto *queryBody = new QWidget;
    auto *queryLayout = new QVBoxLayout(queryBody);
    queryLayout->setContentsMargins(0, 0, 0, 0);
    queryLayout->addWidget(new QLabel(QStringLiteral("设备")));
    queryLayout->addWidget(historyDevice_);
    queryLayout->addWidget(new QLabel(QStringLiteral("数量")));
    queryLayout->addWidget(historyLimit_);
    auto *queryBtn = button(QStringLiteral("查询遥测"), "primary");
    queryLayout->addWidget(queryBtn);
    leftLayout->addWidget(card(QStringLiteral("历史数据"), queryBody));

    auto *mediaBody = new QWidget;
    auto *mediaGrid = new QGridLayout(mediaBody);
    mediaGrid->setContentsMargins(0, 0, 0, 0);
    auto *photoBtn = button(QStringLiteral("照片记录"));
    auto *videoBtn = button(QStringLiteral("录像记录"));
    auto *logBtn = button(QStringLiteral("查看日志"));
    auto *clearBtn = button(QStringLiteral("清空日志"), "danger");
    mediaGrid->addWidget(photoBtn, 0, 0);
    mediaGrid->addWidget(videoBtn, 0, 1);
    mediaGrid->addWidget(logBtn, 1, 0);
    mediaGrid->addWidget(clearBtn, 1, 1);
    leftLayout->addWidget(card(QStringLiteral("媒体与日志"), mediaBody));
    leftLayout->addStretch();

    historyTable_ = new QTableWidget(0, 3);
    historyTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    historyTable_->verticalHeader()->setVisible(false);
    historyTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    historyTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    historyTable_->setAlternatingRowColors(true);

    grid->addWidget(left, 0, 0);
    grid->addWidget(card(QStringLiteral("记录列表（双击打开/查看）"), historyTable_), 0, 1);
    grid->setColumnStretch(0, 22);
    grid->setColumnStretch(1, 78);

    connect(queryBtn, &QPushButton::clicked, this, &MainWidget::queryHistory);
    connect(photoBtn, &QPushButton::clicked, this, &MainWidget::loadPhotos);
    connect(videoBtn, &QPushButton::clicked, this, &MainWidget::loadVideos);
    connect(logBtn, &QPushButton::clicked, this, &MainWidget::loadLogs);
    connect(clearBtn, &QPushButton::clicked, this, &MainWidget::clearRemoteLogs);
    connect(historyTable_, &QTableWidget::cellDoubleClicked, this, &MainWidget::openHistoryItem);
    queryHistory();
    return page;
}

void MainWidget::appendLog(const QString &message)
{
    if (!logText_) return;
    logText_->append(QString("[%1] %2").arg(QDateTime::currentDateTime().toString("HH:mm:ss"), message));
    while (logText_->document()->blockCount() > 300) {
        QTextCursor cursor(logText_->document()->begin());
        cursor.select(QTextCursor::BlockUnderCursor);
        cursor.removeSelectedText();
        cursor.deleteChar();
    }
}

void MainWidget::setOnline(bool online)
{
    if (!connBadge_) return;
    connBadge_->setText(online ? QStringLiteral("在线") : QStringLiteral("离线"));
    connBadge_->setStyleSheet(online
        ? "background:#16a34a;color:white;border-radius:14px;padding:5px 14px;font-weight:700;"
        : "background:#ef4444;color:white;border-radius:14px;padding:5px 14px;font-weight:700;");
}

void MainWidget::pollStatus()
{
    auto *reply = net_->get(QNetworkRequest(QUrl(apiBase_ + "/api/status")));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            setOnline(false);
            return;
        }
        setOnline(true);
        updateMetrics(QJsonDocument::fromJson(reply->readAll()).object());
    });
}

void MainWidget::updateMetrics(const QJsonObject &obj)
{
    tempValue_->setText(valueText(obj, "temp"));
    humiValue_->setText(valueText(obj, "humi"));
    lightValue_->setText(valueText(obj, "light", 0));
    const int ir = obj.value("ir").toInt(0);
    irValue_->setText(ir ? QStringLiteral("遮挡") : QStringLiteral("安全"));
}

void MainWidget::sendControl()
{
    if (!ledCheck_ || !motorCheck_ || !buzzerCheck_) return;
    QJsonObject payload;
    payload["led_on"] = ledCheck_->isChecked() ? 1 : 0;
    payload["led_br"] = ledSlider_->value();
    payload["motor_on"] = motorCheck_->isChecked() ? 1 : 0;
    payload["motor_sp"] = motorSlider_->value();
    payload["motor_dir"] = motorDir_;
    payload["buzzer"] = buzzerCheck_->isChecked() ? 1 : 0;

    QJsonObject body;
    body["type"] = "control";
    body["payload"] = payload;

    QNetworkRequest req(QUrl(apiBase_ + "/api/control"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    auto *reply = net_->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        appendLog(reply->error() == QNetworkReply::NoError
            ? QStringLiteral("控制命令已发送")
            : QStringLiteral("控制命令失败: ") + reply->errorString());
    });
}

void MainWidget::startStream()
{
    if (streaming_) return;
    streaming_ = true;
    streamStartBtn_->setEnabled(false);
    streamStopBtn_->setEnabled(true);
    videoLabel_->setText(QStringLiteral("正在连接视频流..."));

    auto *reply = net_->get(QNetworkRequest(QUrl(apiBase_ + "/api/camera/start_stream")));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const QByteArray body = reply->readAll();
        const QJsonObject obj = QJsonDocument::fromJson(body).object();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError || status >= 400 || obj.contains("error")) {
            appendLog(QStringLiteral("视频启动失败: ") + obj.value("error").toString(QString::fromUtf8(body)));
            stopStream();
            return;
        }

        const int port = obj.value("port").toInt(0);
        const QString path = obj.value("path").toString("/api/camera/stream");
        if (port > 0) {
            QUrl base(apiBase_);
            QUrl streamUrl;
            streamUrl.setScheme(base.scheme().isEmpty() ? "http" : base.scheme());
            streamUrl.setHost(base.host());
            streamUrl.setPort(port);
            streamUrl.setPath(path);
            startMjpegStream(streamUrl);
        } else {
            startMjpegStream(QUrl(apiBase_ + path));
        }
        appendLog(QStringLiteral("视频流已启动"));
    });
}

void MainWidget::stopStream()
{
    streaming_ = false;
    if (mjpegReply_) {
        mjpegReply_->abort();
        mjpegReply_->deleteLater();
        mjpegReply_ = nullptr;
    }
    mjpegBuffer_.clear();
    streamStartBtn_->setEnabled(true);
    streamStopBtn_->setEnabled(false);
    videoLabel_->clear();
    videoLabel_->setText(QStringLiteral("视频流未启动"));
    net_->get(QNetworkRequest(QUrl(apiBase_ + "/api/camera/stop_stream")));
    appendLog(QStringLiteral("视频流已停止"));
}

void MainWidget::startMjpegStream(const QUrl &url)
{
    if (mjpegReply_) {
        mjpegReply_->abort();
        mjpegReply_->deleteLater();
    }
    mjpegBuffer_.clear();
    QNetworkRequest req(url);
    req.setRawHeader("Cache-Control", "no-cache");
    mjpegReply_ = net_->get(req);
    connect(mjpegReply_, &QNetworkReply::readyRead, this, [this]() {
        if (!mjpegReply_) return;
        mjpegBuffer_.append(mjpegReply_->readAll());
        consumeMjpegData();
    });
    connect(mjpegReply_, &QNetworkReply::finished, this, [this]() {
        if (!mjpegReply_) return;
        mjpegReply_->deleteLater();
        mjpegReply_ = nullptr;
        if (streaming_) {
            streaming_ = false;
            streamStartBtn_->setEnabled(true);
            streamStopBtn_->setEnabled(false);
            videoLabel_->setText(QStringLiteral("视频流已断开"));
            appendLog(QStringLiteral("视频流连接断开"));
        }
    });
}

void MainWidget::consumeMjpegData()
{
    const QByteArray soi = QByteArray::fromHex("ffd8");
    const QByteArray eoi = QByteArray::fromHex("ffd9");
    while (true) {
        int start = mjpegBuffer_.indexOf(soi);
        if (start < 0) {
            if (mjpegBuffer_.size() > 1024 * 1024) mjpegBuffer_.clear();
            return;
        }
        int end = mjpegBuffer_.indexOf(eoi, start + 2);
        if (end < 0) {
            if (start > 0) mjpegBuffer_.remove(0, start);
            return;
        }
        const QByteArray jpg = mjpegBuffer_.mid(start, end - start + 2);
        mjpegBuffer_.remove(0, end + 2);
        QImage img;
        if (img.loadFromData(jpg, "JPEG")) {
            videoLabel_->setPixmap(QPixmap::fromImage(img).scaled(
                videoLabel_->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    }
}

void MainWidget::takeSnapshot()
{
    auto *reply = net_->get(QNetworkRequest(QUrl(apiBase_ + "/api/camera/snapshot")));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        appendLog(reply->error() == QNetworkReply::NoError
            ? QStringLiteral("拍照完成，可到历史与媒体查看")
            : QStringLiteral("拍照失败: ") + reply->errorString());
        loadPhotos();
    });
}

void MainWidget::toggleRecord()
{
    const QString api = recording_ ? "/api/camera/stop_record" : "/api/camera/start_record";
    auto *reply = net_->get(QNetworkRequest(QUrl(apiBase_ + api)));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            appendLog(QStringLiteral("录像操作失败: ") + reply->errorString());
            return;
        }
        recording_ = !recording_;
        recordBtn_->setText(recording_ ? QStringLiteral("停止录像") : QStringLiteral("开始录像"));
        appendLog(recording_ ? QStringLiteral("开始录像") : QStringLiteral("录像已保存，可到历史与媒体查看"));
        if (!recording_) loadVideos();
    });
}

void MainWidget::setHistoryHeaders(const QStringList &headers)
{
    historyTable_->clear();
    historyTable_->setColumnCount(headers.size());
    historyTable_->setHorizontalHeaderLabels(headers);
    historyTable_->setRowCount(0);
    historyTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

void MainWidget::queryHistory()
{
    historyMode_ = "telemetry";
    setHistoryHeaders({QStringLiteral("数值"), QStringLiteral("设备时间"), QStringLiteral("入库时间")});
    QUrl url(apiBase_ + "/api/history");
    QUrlQuery query;
    query.addQueryItem("device_id", historyDevice_->currentData().toString());
    query.addQueryItem("limit", QString::number(historyLimit_->currentData().toInt()));
    url.setQuery(query);

    auto *reply = net_->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            appendLog(QStringLiteral("历史查询失败: ") + reply->errorString());
            return;
        }
        const QJsonArray items = QJsonDocument::fromJson(reply->readAll()).object().value("items").toArray();
        historyTable_->setRowCount(0);
        for (const auto &v : items) {
            const QJsonObject item = v.toObject();
            const int row = historyTable_->rowCount();
            historyTable_->insertRow(row);
            historyTable_->setItem(row, 0, new QTableWidgetItem(QString::number(item.value("value").toDouble(), 'f', 2)));
            historyTable_->setItem(row, 1, new QTableWidgetItem(item.value("ts").toVariant().toString()));
            historyTable_->setItem(row, 2, new QTableWidgetItem(item.value("created_at").toString()));
        }
        appendLog(QStringLiteral("历史数据已刷新"));
    });
}

void MainWidget::fillMediaTable(const QJsonArray &items, const QString &basePath, bool megabytes)
{
    historyTable_->setRowCount(0);
    for (const auto &v : items) {
        const QJsonObject item = v.toObject();
        const QString name = item.value("name").toString();
        const int row = historyTable_->rowCount();
        historyTable_->insertRow(row);
        auto *nameItem = new QTableWidgetItem(name);
        nameItem->setData(Qt::UserRole, apiBase_ + basePath + name);
        historyTable_->setItem(row, 0, nameItem);
        const double size = item.value("size").toDouble();
        historyTable_->setItem(row, 1, new QTableWidgetItem(megabytes
            ? QString::number(size / 1024.0 / 1024.0, 'f', 2) + " MB"
            : QString::number(size / 1024.0, 'f', 1) + " KB"));
        historyTable_->setItem(row, 2, new QTableWidgetItem(tsText(item.value("mtime").toDouble())));
    }
}

void MainWidget::loadPhotos()
{
    historyMode_ = "photos";
    setHistoryHeaders({QStringLiteral("照片文件"), QStringLiteral("大小"), QStringLiteral("时间")});
    auto *reply = net_->get(QNetworkRequest(QUrl(apiBase_ + "/api/camera/photos")));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) return;
        fillMediaTable(QJsonDocument::fromJson(reply->readAll()).object().value("photos").toArray(),
                       "/data/media/photos/", false);
    });
}

void MainWidget::loadVideos()
{
    historyMode_ = "videos";
    setHistoryHeaders({QStringLiteral("录像文件"), QStringLiteral("大小"), QStringLiteral("时间")});
    auto *reply = net_->get(QNetworkRequest(QUrl(apiBase_ + "/api/camera/videos")));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) return;
        fillMediaTable(QJsonDocument::fromJson(reply->readAll()).object().value("videos").toArray(),
                       "/data/media/videos/", true);
    });
}

void MainWidget::loadLogs()
{
    historyMode_ = "logs";
    setHistoryHeaders({QStringLiteral("日志文件"), QStringLiteral("大小"), QStringLiteral("时间")});
    auto *reply = net_->get(QNetworkRequest(QUrl(apiBase_ + "/api/camera/logs")));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) return;
        fillMediaTable(QJsonDocument::fromJson(reply->readAll()).object().value("logs").toArray(),
                       "/api/camera/logs/", false);
    });
}

void MainWidget::clearRemoteLogs()
{
    auto *reply = net_->get(QNetworkRequest(QUrl(apiBase_ + "/api/camera/clear_logs")));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        appendLog(reply->error() == QNetworkReply::NoError ? QStringLiteral("日志内容已清空")
                                                           : QStringLiteral("清空日志失败"));
        loadLogs();
    });
}

void MainWidget::openHistoryItem(int row, int)
{
    if (!historyTable_ || row < 0 || !historyTable_->item(row, 0)) return;
    const QString url = historyTable_->item(row, 0)->data(Qt::UserRole).toString();
    if (url.isEmpty()) return;
    if (historyMode_ == "logs") {
        auto *reply = net_->get(QNetworkRequest(QUrl(url)));
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            reply->deleteLater();
            if (reply->error() == QNetworkReply::NoError) {
                logText_->setPlainText(QString::fromUtf8(reply->readAll()));
            }
        });
        return;
    }
    QDesktopServices::openUrl(QUrl(url));
}

// TODO: Zigbee 串口接口，RK3568 烧写工具就绪后启用
// void MainWidget::openSerial()
// {
//     if (serialText_) serialText_->append(QStringLiteral("Zigbee 串口后续接入"));
// }
//
// void MainWidget::closeSerial()
// {
//     if (serialText_) serialText_->append(QStringLiteral("Zigbee 串口未打开"));
// }
