#include "mainwidget.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
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
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QSerialPortInfo>
#include <QSettings>
#include <QSlider>
#include <QStackedWidget>
#include <QStyle>
#include <QTableWidget>
#include <QTabBar>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextEdit>
#include <QUrlQuery>
#include <QVBoxLayout>
#include <algorithm>

static const char *kPrimaryButton =
    "QPushButton{background:#5c67f2;color:white;border:0;border-radius:6px;padding:8px 12px;font-weight:700;}"
    "QPushButton:hover{background:#4f46e5;} QPushButton:disabled{background:#cbd5e1;color:#64748b;}";

static const char *kSecondaryButton =
    "QPushButton{background:#f1f5f9;color:#475569;border:0;border-radius:6px;padding:8px 12px;font-weight:700;}"
    "QPushButton:hover{background:#e2e8f0;} QPushButton:checked{background:#eef2ff;color:#5c67f2;border:1px solid #5c67f2;}";

static const char *kSuccessButton =
    "QPushButton{background:#10b981;color:white;border:0;border-radius:6px;padding:8px 12px;font-weight:700;}"
    "QPushButton:hover{background:#059669;}";

static const char *kDangerButton =
    "QPushButton{background:#ef4444;color:white;border:0;border-radius:6px;padding:8px 12px;font-weight:700;}"
    "QPushButton:hover{background:#dc2626;}";

class TrendChart : public QWidget
{
public:
    explicit TrendChart(QWidget *parent = nullptr) : QWidget(parent)
    {
        setMinimumHeight(220);
    }

    void setValues(const QVector<double> &values, const QString &name)
    {
        values_ = values;
        name_ = name;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.fillRect(rect(), QColor("#ffffff"));

        const QRectF plot = rect().adjusted(42, 22, -18, -34);
        p.setPen(QPen(QColor("#e2e8f0"), 1));
        for (int i = 0; i <= 4; ++i) {
            const qreal y = plot.top() + plot.height() * i / 4.0;
            p.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));
        }

        p.setPen(QColor("#64748b"));
        p.drawText(QRectF(0, 0, width(), 20), Qt::AlignCenter, name_.isEmpty() ? QStringLiteral("历史趋势") : name_);

        if (values_.isEmpty()) {
            p.setPen(QColor("#94a3b8"));
            p.drawText(plot, Qt::AlignCenter, QStringLiteral("暂无历史数据"));
            return;
        }

        auto minmax = std::minmax_element(values_.cbegin(), values_.cend());
        double minValue = *minmax.first;
        double maxValue = *minmax.second;
        if (qFuzzyCompare(minValue, maxValue)) {
            minValue -= 1.0;
            maxValue += 1.0;
        }

        QPainterPath path;
        for (int i = 0; i < values_.size(); ++i) {
            const qreal x = values_.size() == 1 ? plot.center().x()
                : plot.left() + plot.width() * i / double(values_.size() - 1);
            const qreal ratio = (values_.at(i) - minValue) / (maxValue - minValue);
            const qreal y = plot.bottom() - plot.height() * ratio;
            if (i == 0) path.moveTo(x, y);
            else path.lineTo(x, y);
        }

        p.setPen(QPen(QColor("#5c67f2"), 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawPath(path);
        p.setBrush(QColor("#5c67f2"));
        p.setPen(Qt::NoPen);
        for (int i = 0; i < values_.size(); ++i) {
            const qreal x = values_.size() == 1 ? plot.center().x()
                : plot.left() + plot.width() * i / double(values_.size() - 1);
            const qreal ratio = (values_.at(i) - minValue) / (maxValue - minValue);
            const qreal y = plot.bottom() - plot.height() * ratio;
            p.drawEllipse(QPointF(x, y), 3, 3);
        }

        p.setPen(QColor("#64748b"));
        p.drawText(QRectF(4, plot.top() - 6, 36, 16), Qt::AlignRight, QString::number(maxValue, 'f', 1));
        p.drawText(QRectF(4, plot.bottom() - 8, 36, 16), Qt::AlignRight, QString::number(minValue, 'f', 1));
        p.drawText(QRectF(plot.left(), plot.bottom() + 8, plot.width(), 18), Qt::AlignCenter,
                   QStringLiteral("按入库时间从旧到新"));
    }

private:
    QVector<double> values_;
    QString name_;
};

MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent),
      net_(new QNetworkAccessManager(this)),
      statusTimer_(new QTimer(this)),
      videoTimer_(new QTimer(this))
{
    loadConfig();
    applyStyle();

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(14, 14, 14, 14);
    root->setSpacing(12);

    auto *header = new QFrame;
    header->setObjectName("header");
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(20, 0, 20, 0);
    auto *title = new QLabel(QStringLiteral("RK3568 智能网关控制系统"));
    title->setObjectName("headerTitle");
    connBadge_ = new QLabel(QStringLiteral("连接中"));
    connBadge_->setObjectName("statusTag");
    connBadge_->setAlignment(Qt::AlignCenter);
    serverEdit_ = new QLineEdit(apiBase_);
    serverEdit_->setObjectName("serverEdit");
    serverEdit_->setFixedWidth(260);
    headerLayout->addWidget(title);
    headerLayout->addStretch();
    headerLayout->addWidget(new QLabel(QStringLiteral("网关")));
    headerLayout->addWidget(serverEdit_);
    headerLayout->addWidget(connBadge_);
    root->addWidget(header);

    tabs_ = new QTabBar;
    tabs_->setObjectName("tabs");
    tabs_->addTab(QStringLiteral("实时监控"));
    tabs_->addTab(QStringLiteral("历史数据"));
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
        appendLog(QStringLiteral("网关地址切换为 ") + apiBase_);
        pollStatus();
    });
    connect(statusTimer_, &QTimer::timeout, this, &MainWidget::pollStatus);
    connect(videoTimer_, &QTimer::timeout, this, &MainWidget::refreshFrame);

    statusTimer_->start(1000);
    pollStatus();
    appendLog(QStringLiteral("Qt 客户端启动完成"));
}

MainWidget::~MainWidget()
{
    if (serial_) {
        serial_->close();
    }
}

void MainWidget::loadConfig()
{
    const QString iniPath = QCoreApplication::applicationDirPath() + "/iotgw_qt_client.ini";
    QSettings cfg(iniPath, QSettings::IniFormat);
    apiBase_ = cfg.value("server/url", "http://192.168.233.107:8081").toString();
}

void MainWidget::applyStyle()
{
    setMinimumSize(1180, 760);
    setWindowTitle(QStringLiteral("RK3568 智能网关 Qt 客户端"));
    setStyleSheet(
        "QWidget{font-family:'Microsoft YaHei','Segoe UI';font-size:13px;color:#1f2937;background:#f0f2f5;}"
        "QLabel{background:transparent;}"
        "QFrame#header{background:qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 #4f46e5,stop:1 #7c3aed);border-radius:8px;min-height:48px;}"
        "#headerTitle{color:white;font-size:22px;font-weight:900;}"
        "#statusTag{background:rgba(255,255,255,0.22);color:white;border-radius:14px;padding:5px 14px;font-weight:700;}"
        "#serverEdit{background:rgba(255,255,255,0.18);color:white;border:1px solid rgba(255,255,255,0.35);border-radius:6px;padding:6px;}"
        "#tabs{background:white;border-radius:8px;}"
        "QTabBar::tab{height:38px;background:white;color:#64748b;font-weight:800;padding:0 18px;border:0;}"
        "QTabBar::tab:selected{background:#eef2ff;color:#5c67f2;border-bottom:3px solid #5c67f2;}"
        "QTabBar::tab:hover{background:#f8fafc;color:#5c67f2;}"
        "QFrame#card{background:white;border:0;border-radius:8px;}"
        "QFrame#controlItem{background:#f8fafc;border:1px solid #edf2f7;border-radius:7px;}"
        "QFrame#sensorCard{background:white;border:0;border-radius:8px;}"
        "QFrame#statCard{background:#ffffff;border:1px solid #edf2f7;border-radius:7px;}"
        "QLabel#cardTitle{font-size:16px;font-weight:900;color:#1f2937;}"
        "QTextEdit{background:#0f172a;color:#38bdf8;border:1px solid #1e293b;border-radius:7px;padding:8px;font-family:Consolas,'Microsoft YaHei';}"
        "QTableWidget{background:white;border:1px solid #e2e8f0;border-radius:7px;gridline-color:#f1f5f9;alternate-background-color:#fafbfc;}"
        "QHeaderView::section{background:#f1f5f9;border:0;border-bottom:1px solid #e2e8f0;padding:8px;font-weight:800;color:#475569;}"
        "QComboBox,QLineEdit{background:white;border:1px solid #e2e8f0;border-radius:6px;padding:7px;}"
        "QSlider::groove:horizontal{height:6px;background:#dbe3ef;border-radius:3px;}"
        "QSlider::handle:horizontal{width:16px;margin:-5px 0;background:#5c67f2;border-radius:8px;}"
        "QCheckBox{font-weight:700;color:#334155;background:transparent;}"
        "QCheckBox::indicator{width:38px;height:20px;border-radius:10px;background:#cbd5e1;}"
        "QCheckBox::indicator:checked{background:#5c67f2;}");
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
    layout->setContentsMargins(16, 14, 16, 16);
    layout->setSpacing(10);
    auto *label = new QLabel(title);
    label->setObjectName("cardTitle");
    layout->addWidget(label);
    layout->addWidget(body, 1);
    return frame;
}

QWidget *MainWidget::sensorCard(const QString &name, const QString &unit, const QString &color, QLabel **valueLabel)
{
    auto *box = new QFrame;
    box->setObjectName("sensorCard");
    box->setMinimumHeight(96);
    auto *layout = new QVBoxLayout(box);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->setSpacing(3);
    auto *nameLabel = new QLabel(name);
    nameLabel->setAlignment(Qt::AlignCenter);
    nameLabel->setStyleSheet("color:#64748b;font-weight:700;");
    auto *value = new QLabel("--");
    value->setAlignment(Qt::AlignCenter);
    value->setStyleSheet(QString("font-size:25px;font-weight:900;color:%1;").arg(color));
    auto *unitLabel = new QLabel(unit);
    unitLabel->setAlignment(Qt::AlignCenter);
    unitLabel->setStyleSheet("color:#94a3b8;font-size:12px;");
    layout->addWidget(nameLabel);
    layout->addWidget(value);
    layout->addWidget(unitLabel);
    *valueLabel = value;
    return box;
}

QWidget *MainWidget::latestCard(const QString &name, const QString &unit, QLabel **valueLabel)
{
    auto *box = new QFrame;
    box->setObjectName("statCard");
    auto *layout = new QVBoxLayout(box);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(3);
    auto *nameLabel = new QLabel(name);
    nameLabel->setAlignment(Qt::AlignCenter);
    nameLabel->setStyleSheet("color:#94a3b8;font-weight:700;");
    auto *value = new QLabel("--");
    value->setAlignment(Qt::AlignCenter);
    value->setStyleSheet("font-size:24px;font-weight:900;color:#5c67f2;");
    auto *unitLabel = new QLabel(unit);
    unitLabel->setAlignment(Qt::AlignCenter);
    unitLabel->setStyleSheet("color:#94a3b8;font-size:11px;");
    layout->addWidget(nameLabel);
    layout->addWidget(value);
    layout->addWidget(unitLabel);
    *valueLabel = value;
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
    videoLayout->setSpacing(10);
    videoLabel_ = new QLabel(QStringLiteral("视频流未启动"));
    videoLabel_->setAlignment(Qt::AlignCenter);
    videoLabel_->setMinimumHeight(330);
    videoLabel_->setStyleSheet("background:#000;color:#94a3b8;border-radius:7px;font-size:18px;");
    videoLayout->addWidget(videoLabel_, 1);
    auto *videoBtns = new QGridLayout;
    videoBtns->setSpacing(8);
    streamStartBtn_ = button(QStringLiteral("开始推流"), "primary");
    streamStopBtn_ = button(QStringLiteral("停止推流"));
    auto *snapshotBtn = button(QStringLiteral("抓拍照片"), "success");
    recordBtn_ = button(QStringLiteral("视频录制"), "danger");
    streamStopBtn_->setEnabled(false);
    videoBtns->addWidget(streamStartBtn_, 0, 0);
    videoBtns->addWidget(streamStopBtn_, 0, 1);
    videoBtns->addWidget(snapshotBtn, 0, 2);
    videoBtns->addWidget(recordBtn_, 0, 3);
    videoLayout->addLayout(videoBtns);
    leftLayout->addWidget(card(QStringLiteral("实时画面监控"), videoBody), 1);

    auto *sensorRow = new QGridLayout;
    sensorRow->setSpacing(12);
    sensorRow->addWidget(sensorCard(QStringLiteral("温度"), QStringLiteral("℃"), "#ef4444", &tempValue_), 0, 0);
    sensorRow->addWidget(sensorCard(QStringLiteral("湿度"), "% RH", "#3b82f6", &humiValue_), 0, 1);
    sensorRow->addWidget(sensorCard(QStringLiteral("光照"), "Lux", "#f59e0b", &lightValue_), 0, 2);
    sensorRow->addWidget(sensorCard(QStringLiteral("红外"), "Status", "#7c3aed", &irValue_), 0, 3);
    leftLayout->addLayout(sensorRow);

    auto *right = new QWidget;
    auto *rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(12);

    auto *controlBody = new QWidget;
    auto *controlLayout = new QVBoxLayout(controlBody);
    controlLayout->setContentsMargins(0, 0, 0, 0);
    controlLayout->setSpacing(8);

    auto addControlItem = [&](const QString &name, QCheckBox **check, QSlider **slider, QLabel **label) {
        auto *item = new QFrame;
        item->setObjectName("controlItem");
        auto *itemLayout = new QVBoxLayout(item);
        itemLayout->setContentsMargins(10, 8, 10, 8);
        itemLayout->setSpacing(6);
        auto *top = new QHBoxLayout;
        auto *nameLabel = new QLabel(name);
        *check = new QCheckBox;
        top->addWidget(nameLabel);
        top->addStretch();
        top->addWidget(*check);
        *slider = new QSlider(Qt::Horizontal);
        (*slider)->setRange(0, 100);
        *label = new QLabel;
        (*label)->setStyleSheet("color:#64748b;");
        itemLayout->addLayout(top);
        itemLayout->addWidget(*slider);
        itemLayout->addWidget(*label);
        controlLayout->addWidget(item);
    };

    addControlItem(QStringLiteral("LED 照明灯"), &ledCheck_, &ledSlider_, &ledLabel_);
    ledSlider_->setValue(50);
    ledLabel_->setText(QStringLiteral("亮度 50%"));

    addControlItem(QStringLiteral("直流电机"), &motorCheck_, &motorSlider_, &motorLabel_);
    motorSlider_->setValue(30);
    motorLabel_->setText(QStringLiteral("转速 30%"));
    auto *dirRow = new QHBoxLayout;
    dirForwardBtn_ = button(QStringLiteral("正转"));
    dirReverseBtn_ = button(QStringLiteral("反转"));
    dirForwardBtn_->setCheckable(true);
    dirReverseBtn_->setCheckable(true);
    dirForwardBtn_->setChecked(true);
    dirRow->addWidget(dirForwardBtn_);
    dirRow->addWidget(dirReverseBtn_);
    controlLayout->addLayout(dirRow);

    auto *buzzerItem = new QFrame;
    buzzerItem->setObjectName("controlItem");
    auto *buzzerLayout = new QHBoxLayout(buzzerItem);
    buzzerLayout->setContentsMargins(10, 8, 10, 8);
    buzzerLayout->addWidget(new QLabel(QStringLiteral("蜂鸣报警")));
    buzzerLayout->addStretch();
    buzzerCheck_ = new QCheckBox;
    buzzerLayout->addWidget(buzzerCheck_);
    controlLayout->addWidget(buzzerItem);

    rightLayout->addWidget(card(QStringLiteral("硬件外设控制"), controlBody));

    auto *serialBody = new QWidget;
    auto *serialLayout = new QVBoxLayout(serialBody);
    serialLayout->setContentsMargins(0, 0, 0, 0);
    serialLayout->setSpacing(8);
    auto *serialTop = new QHBoxLayout;
    serialPortBox_ = new QComboBox;
    refreshSerialPorts();
    auto *openBtn = button(QStringLiteral("打开"));
    auto *closeBtn = button(QStringLiteral("关闭"), "danger");
    serialTop->addWidget(serialPortBox_, 1);
    serialTop->addWidget(openBtn);
    serialTop->addWidget(closeBtn);
    serialText_ = new QTextEdit;
    serialText_->setReadOnly(true);
    serialText_->setMaximumHeight(95);
    serialLayout->addLayout(serialTop);
    serialLayout->addWidget(serialText_);
    rightLayout->addWidget(card(QStringLiteral("串口模块"), serialBody));

    auto *alarmBody = new QWidget;
    auto *alarmLayout = new QVBoxLayout(alarmBody);
    alarmLayout->setContentsMargins(0, 0, 0, 0);
    alarmText_ = new QTextEdit;
    alarmText_->setReadOnly(true);
    alarmText_->setMaximumHeight(78);
    alarmText_->setText(QStringLiteral("暂无告警"));
    alarmLayout->addWidget(alarmText_);
    rightLayout->addWidget(card(QStringLiteral("告警模块"), alarmBody));

    auto *logBody = new QWidget;
    auto *logLayout = new QVBoxLayout(logBody);
    logLayout->setContentsMargins(0, 0, 0, 0);
    logText_ = new QTextEdit;
    logText_->setReadOnly(true);
    logLayout->addWidget(logText_);
    rightLayout->addWidget(card(QStringLiteral("系统日志"), logBody), 1);

    grid->addWidget(left, 0, 0);
    grid->addWidget(right, 0, 1);
    grid->setColumnStretch(0, 72);
    grid->setColumnStretch(1, 28);

    connect(streamStartBtn_, &QPushButton::clicked, this, &MainWidget::startStream);
    connect(streamStopBtn_, &QPushButton::clicked, this, &MainWidget::stopStream);
    connect(snapshotBtn, &QPushButton::clicked, this, &MainWidget::takeSnapshot);
    connect(recordBtn_, &QPushButton::clicked, this, &MainWidget::toggleRecord);
    connect(openBtn, &QPushButton::clicked, this, &MainWidget::openSerial);
    connect(closeBtn, &QPushButton::clicked, this, &MainWidget::closeSerial);

    connect(ledCheck_, &QCheckBox::toggled, this, &MainWidget::sendControl);
    connect(motorCheck_, &QCheckBox::toggled, this, &MainWidget::sendControl);
    connect(buzzerCheck_, &QCheckBox::toggled, this, &MainWidget::sendControl);
    connect(ledSlider_, &QSlider::valueChanged, this, [this](int v) {
        ledLabel_->setText(QStringLiteral("亮度 %1%").arg(v));
        sendControl();
    });
    connect(motorSlider_, &QSlider::valueChanged, this, [this](int v) {
        motorLabel_->setText(QStringLiteral("转速 %1%").arg(v));
        sendControl();
    });
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
    leftLayout->setSpacing(12);

    auto *queryBody = new QWidget;
    auto *queryLayout = new QVBoxLayout(queryBody);
    queryLayout->setContentsMargins(0, 0, 0, 0);
    queryLayout->setSpacing(8);
    historyDevice_ = new QComboBox;
    historyDevice_->addItem(QStringLiteral("温度传感器"), "temp");
    historyDevice_->addItem(QStringLiteral("湿度传感器"), "humi");
    historyDevice_->addItem(QStringLiteral("光照传感器"), "light");
    historyDevice_->addItem(QStringLiteral("红外传感器"), "ir");
    historyLimit_ = new QComboBox;
    historyLimit_->addItem(QStringLiteral("最近 20 条"), 20);
    historyLimit_->addItem(QStringLiteral("最近 50 条"), 50);
    historyLimit_->addItem(QStringLiteral("最近 100 条"), 100);
    auto *queryBtn = button(QStringLiteral("查询历史数据"), "primary");
    queryLayout->addWidget(new QLabel(QStringLiteral("选择设备")));
    queryLayout->addWidget(historyDevice_);
    queryLayout->addWidget(new QLabel(QStringLiteral("显示条数")));
    queryLayout->addWidget(historyLimit_);
    queryLayout->addWidget(queryBtn);
    leftLayout->addWidget(card(QStringLiteral("查询条件"), queryBody));

    auto *latestBody = new QWidget;
    auto *latestGrid = new QGridLayout(latestBody);
    latestGrid->setContentsMargins(0, 0, 0, 0);
    latestGrid->setSpacing(8);
    latestGrid->addWidget(latestCard(QStringLiteral("温度"), QStringLiteral("℃"), &latestTemp_), 0, 0);
    latestGrid->addWidget(latestCard(QStringLiteral("湿度"), "% RH", &latestHumi_), 0, 1);
    latestGrid->addWidget(latestCard(QStringLiteral("光照"), "Lux", &latestLight_), 1, 0);
    latestGrid->addWidget(latestCard(QStringLiteral("红外"), "Status", &latestIr_), 1, 1);
    leftLayout->addWidget(card(QStringLiteral("最新数据"), latestBody));
    leftLayout->addStretch();

    auto *right = new QWidget;
    auto *rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(12);
    trendChart_ = new TrendChart;
    rightLayout->addWidget(card(QStringLiteral("数据可视化"), trendChart_));

    historyTable_ = new QTableWidget(0, 3);
    historyTable_->setHorizontalHeaderLabels({QStringLiteral("数值"), QStringLiteral("设备时间戳"), QStringLiteral("入库时间")});
    historyTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    historyTable_->verticalHeader()->setVisible(false);
    historyTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    historyTable_->setAlternatingRowColors(true);
    rightLayout->addWidget(card(QStringLiteral("数据库历史遥测数据"), historyTable_), 1);

    grid->addWidget(left, 0, 0);
    grid->addWidget(right, 0, 1);
    grid->setColumnStretch(0, 22);
    grid->setColumnStretch(1, 78);

    connect(queryBtn, &QPushButton::clicked, this, &MainWidget::queryHistory);
    return page;
}

void MainWidget::refreshSerialPorts()
{
    if (!serialPortBox_) return;
    serialPortBox_->clear();
    for (const QSerialPortInfo &info : QSerialPortInfo::availablePorts()) {
        serialPortBox_->addItem(info.portName());
    }
    if (serialPortBox_->count() == 0) {
        serialPortBox_->addItem(QStringLiteral("无可用串口"));
    }
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
    connBadge_->setText(online ? QStringLiteral("设备在线") : QStringLiteral("连接断开"));
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
        const QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
        const double temp = obj.value("temp").toDouble();
        const double humi = obj.value("humi").toDouble();
        const int light = obj.value("light").toInt();
        const int ir = obj.value("ir").toInt();

        const QString tempText = temp > 0 ? QString::number(temp, 'f', 1) : "--";
        const QString humiText = humi > 0 ? QString::number(humi, 'f', 1) : "--";
        const QString lightText = QString::number(light);
        const QString irText = ir ? QStringLiteral("有人") : QStringLiteral("安全");

        tempValue_->setText(tempText);
        humiValue_->setText(humiText);
        lightValue_->setText(lightText);
        irValue_->setText(irText);
        if (latestTemp_) latestTemp_->setText(tempText);
        if (latestHumi_) latestHumi_->setText(humiText);
        if (latestLight_) latestLight_->setText(lightText);
        if (latestIr_) latestIr_->setText(irText);
        updateAlarm(temp, humi, light, ir);
    });
}

void MainWidget::updateAlarm(double temp, double humi, int light, int ir)
{
    QStringList alarms;
    if (temp > 35) alarms << QStringLiteral("温度过高: %1 ℃").arg(temp, 0, 'f', 1);
    if (humi > 80) alarms << QStringLiteral("湿度过高: %1 %").arg(humi, 0, 'f', 1);
    if (light < 20) alarms << QStringLiteral("光照偏低: %1").arg(light);
    if (ir) alarms << QStringLiteral("红外检测到目标");
    alarmText_->setText(alarms.isEmpty() ? QStringLiteral("暂无告警") : alarms.join('\n'));
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
        if (reply->error() == QNetworkReply::NoError) {
            appendLog(QStringLiteral("控制命令已下发"));
        } else {
            appendLog(QStringLiteral("控制命令失败: ") + reply->errorString());
        }
    });
}

void MainWidget::startStream()
{
    if (streaming_) return;
    streaming_ = true;
    streamStartBtn_->setEnabled(false);
    streamStopBtn_->setEnabled(true);
    videoLabel_->setText(QStringLiteral("正在连接视频流..."));
    net_->get(QNetworkRequest(QUrl(apiBase_ + "/api/camera/start_stream")));
    videoTimer_->start(160);
    appendLog(QStringLiteral("视频流启动"));
}

void MainWidget::stopStream()
{
    streaming_ = false;
    videoTimer_->stop();
    streamStartBtn_->setEnabled(true);
    streamStopBtn_->setEnabled(false);
    videoLabel_->clear();
    videoLabel_->setText(QStringLiteral("视频流未启动"));
    net_->get(QNetworkRequest(QUrl(apiBase_ + "/api/camera/stop_stream")));
    appendLog(QStringLiteral("视频流停止"));
}

void MainWidget::refreshFrame()
{
    if (!streaming_) return;
    auto *reply = net_->get(QNetworkRequest(QUrl(apiBase_ + "/stream/live.jpg")));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) return;
        QImage img;
        if (img.loadFromData(reply->readAll(), "JPEG")) {
            videoLabel_->setPixmap(QPixmap::fromImage(img).scaled(
                videoLabel_->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    });
}

void MainWidget::takeSnapshot()
{
    auto *reply = net_->get(QNetworkRequest(QUrl(apiBase_ + "/api/camera/snapshot")));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        appendLog(reply->error() == QNetworkReply::NoError
            ? QStringLiteral("抓拍命令已发送")
            : QStringLiteral("抓拍失败: ") + reply->errorString());
    });
}

void MainWidget::toggleRecord()
{
    const QString api = recording_ ? "/api/camera/stop_record" : "/api/camera/start_record";
    auto *reply = net_->get(QNetworkRequest(QUrl(apiBase_ + api)));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            appendLog(QStringLiteral("录制命令失败: ") + reply->errorString());
            return;
        }
        recording_ = !recording_;
        recordBtn_->setText(recording_ ? QStringLiteral("停止录制") : QStringLiteral("视频录制"));
        appendLog(recording_ ? QStringLiteral("开始录制") : QStringLiteral("停止录制"));
    });
}

void MainWidget::queryHistory()
{
    if (!historyDevice_ || !historyLimit_) return;
    QUrl url(apiBase_ + "/api/history");
    QUrlQuery query;
    query.addQueryItem("device_id", historyDevice_->currentData().toString());
    query.addQueryItem("limit", QString::number(historyLimit_->currentData().toInt()));
    url.setQuery(query);

    auto *reply = net_->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            appendLog(QStringLiteral("历史数据查询失败: ") + reply->errorString());
            return;
        }
        const QJsonArray items = QJsonDocument::fromJson(reply->readAll()).object().value("items").toArray();
        historyTable_->setRowCount(0);
        QVector<double> values;
        for (int i = items.size() - 1; i >= 0; --i) {
            const QJsonObject item = items.at(i).toObject();
            const int row = historyTable_->rowCount();
            historyTable_->insertRow(row);
            const double value = item.value("value").toDouble();
            values.append(value);
            historyTable_->setItem(row, 0, new QTableWidgetItem(QString::number(value, 'f', 2)));
            historyTable_->setItem(row, 1, new QTableWidgetItem(item.value("ts").toVariant().toString()));
            historyTable_->setItem(row, 2, new QTableWidgetItem(item.value("created_at").toString()));
        }
        if (trendChart_) {
            trendChart_->setValues(values, historyDevice_->currentText());
        }
        appendLog(QStringLiteral("历史数据查询完成: %1 条").arg(items.size()));
    });
}

void MainWidget::openSerial()
{
    if (serial_) return;
    if (serialPortBox_->currentText() == QStringLiteral("无可用串口")) {
        serialText_->append(QStringLiteral("没有可用串口"));
        return;
    }
    serial_ = new QSerialPort(this);
    serial_->setPortName(serialPortBox_->currentText());
    serial_->setBaudRate(QSerialPort::Baud115200);
    serial_->setDataBits(QSerialPort::Data8);
    serial_->setParity(QSerialPort::NoParity);
    serial_->setStopBits(QSerialPort::OneStop);
    if (!serial_->open(QIODevice::ReadWrite)) {
        serialText_->append(QStringLiteral("打开失败: ") + serial_->errorString());
        serial_->deleteLater();
        serial_ = nullptr;
        return;
    }
    connect(serial_, &QSerialPort::readyRead, this, &MainWidget::readSerialData);
    serialText_->append(QStringLiteral("串口已打开: ") + serial_->portName());
    appendLog(QStringLiteral("串口已打开: ") + serial_->portName());
}

void MainWidget::closeSerial()
{
    if (!serial_) return;
    const QString name = serial_->portName();
    serial_->close();
    serial_->deleteLater();
    serial_ = nullptr;
    serialText_->append(QStringLiteral("串口已关闭: ") + name);
    appendLog(QStringLiteral("串口已关闭: ") + name);
}

void MainWidget::readSerialData()
{
    if (!serial_) return;
    serialBuffer_.append(serial_->readAll());
    while (serialBuffer_.contains('\n')) {
        const int pos = serialBuffer_.indexOf('\n');
        const QByteArray line = serialBuffer_.left(pos).trimmed();
        serialBuffer_.remove(0, pos + 1);
        if (!line.isEmpty()) serialText_->append(QString::fromUtf8(line));
    }
}
