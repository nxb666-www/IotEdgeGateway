#include "monitorwidget.h"
#include "ui_monitorwidget.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QSettings>
#include <QCoreApplication>
#include <QDir>

static QString API_BASE;

static void loadApiBase() {
    // 从 exe 同目录的 ini 文件读取
    QString iniPath = QCoreApplication::applicationDirPath() + "/iotgw_qt_client.ini";
    QSettings cfg(iniPath, QSettings::IniFormat);
    API_BASE = cfg.value("server/url", "http://192.168.137.170:8080").toString();
}

MonitorWidget::MonitorWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MonitorWidget),
    netManager_(new QNetworkAccessManager(this)),
    sensorTimer_(new QTimer(this)),
    frameTimer_(new QTimer(this))
{
    ui->setupUi(this);
    loadApiBase();

    // LED 控制
    connect(ui->checkLed, &QCheckBox::toggled, this, &MonitorWidget::cmd);
    connect(ui->sliderLed, &QSlider::valueChanged, this, &MonitorWidget::cmd);

    // 电机控制
    connect(ui->checkMotor, &QCheckBox::toggled, this, &MonitorWidget::cmd);
    connect(ui->sliderMotor, &QSlider::valueChanged, this, &MonitorWidget::cmd);
    connect(ui->btnDirF, &QPushButton::clicked, this, [this]() {
        motorDir_ = 0;
        ui->btnDirF->setChecked(true);
        ui->btnDirR->setChecked(false);
        addLog("电机正转");
        cmd();
    });
    connect(ui->btnDirR, &QPushButton::clicked, this, [this]() {
        motorDir_ = 1;
        ui->btnDirF->setChecked(false);
        ui->btnDirR->setChecked(true);
        addLog("电机反转");
        cmd();
    });

    // 蜂鸣器
    connect(ui->checkBuzzer, &QCheckBox::toggled, this, &MonitorWidget::cmd);

    // 视频推流
    connect(ui->btnStreamStart, &QPushButton::clicked, this, &MonitorWidget::startStream);
    connect(ui->btnStreamStop, &QPushButton::clicked, this, &MonitorWidget::stopStream);
    connect(ui->btnSnapshot, &QPushButton::clicked, this, &MonitorWidget::takeSnapshot);
    connect(ui->btnRecord, &QPushButton::clicked, this, &MonitorWidget::toggleRecord);

    // 定时轮询
    connect(sensorTimer_, &QTimer::timeout, this, &MonitorWidget::pollSensorData);
    connect(frameTimer_, &QTimer::timeout, this, &MonitorWidget::pollVideoFrame);
    sensorTimer_->start(1000);

    addLog("[系统] 页面加载完成");
    addLog(QString("[系统] 服务器地址: %1").arg(API_BASE));
}

MonitorWidget::~MonitorWidget()
{
    delete ui;
}

void MonitorWidget::addLog(const QString &msg)
{
    ui->textLog->append(QString("<span style='color:#94a3b8;'>[%1]</span> %2")
        .arg(QDateTime::currentDateTime().toString("HH:mm:ss"), msg));
}

void MonitorWidget::pollSensorData()
{
    QNetworkReply *reply = netManager_->get(QNetworkRequest(QUrl(API_BASE + "/api/status")));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            // 连接断开
            emit connectionStatusChanged(false);
            reply->deleteLater();
            return;
        }
        emit connectionStatusChanged(true);

        QByteArray raw = reply->readAll();
        reply->deleteLater();

        QJsonObject obj = QJsonDocument::fromJson(raw).object();

        // 跟 web 一致：0 值显示 "--"
        auto valOrDash = [](const QJsonValue &v, int prec) -> QString {
            double d = v.toDouble();
            return (d == 0) ? "--" : QString::number(d, 'f', prec);
        };

        ui->valTemp->setText(valOrDash(obj.value("temp"), 1));
        ui->valHumi->setText(valOrDash(obj.value("humi"), 1));
        ui->valLight->setText(valOrDash(obj.value("light"), 0));

        int ir = obj.value("ir").toInt();
        ui->valIr->setText(ir > 2000 ? "有人" : "安全");
        ui->valIr->setStyleSheet(QString("font-size:16px; font-weight:800; color:%1;")
            .arg(ir > 2000 ? "#ef4444" : "#10b981"));
    });
}

void MonitorWidget::pollVideoFrame()
{
    if (!streamRunning_) return;
    QNetworkReply *reply = netManager_->get(QNetworkRequest(QUrl(API_BASE + "/stream/live.jpg")));
    connect(reply, &QNetworkReply::finished, this, &MonitorWidget::onFrameReply);
}

void MonitorWidget::onFrameReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QImage img;
        if (img.loadFromData(data)) {
            QPixmap pix = QPixmap::fromImage(img);
            ui->labelVideo->setPixmap(pix.scaled(
                ui->labelVideo->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    }
    reply->deleteLater();
}

void MonitorWidget::cmd()
{
    QJsonObject payload;
    payload["led_on"] = ui->checkLed->isChecked() ? 1 : 0;
    payload["led_br"] = ui->sliderLed->value();
    payload["motor_on"] = ui->checkMotor->isChecked() ? 1 : 0;
    payload["motor_sp"] = ui->sliderMotor->value();
    payload["motor_dir"] = motorDir_;
    payload["buzzer"] = ui->checkBuzzer->isChecked() ? 1 : 0;

    addLog(QString("指令: %1").arg(QString(QJsonDocument(payload).toJson(QJsonDocument::Compact))));

    QJsonObject body;
    body["type"] = "control";
    body["payload"] = payload;
    QNetworkRequest req(QUrl(API_BASE + "/api/control"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = netManager_->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            addLog(QString("发送失败: %1").arg(reply->errorString()));
        }
        reply->deleteLater();
    });
}

void MonitorWidget::startStream()
{
    if (streamRunning_) return;
    streamRunning_ = true;
    ui->btnStreamStart->setEnabled(false);
    ui->btnStreamStop->setEnabled(true);
    frameTimer_->start(100);
    addLog("正在启动视频流...");

    QNetworkReply *reply = netManager_->get(QNetworkRequest(QUrl(API_BASE + "/api/camera/start_stream")));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonObject res = QJsonDocument::fromJson(reply->readAll()).object();
            addLog(QString("视频流已启动(%1)").arg(res.value("backend").toString("ffmpeg")));
        } else {
            addLog("视频流启动失败: " + reply->errorString());
        }
        reply->deleteLater();
    });
}

void MonitorWidget::stopStream()
{
    if (!streamRunning_) return;
    streamRunning_ = false;
    frameTimer_->stop();
    ui->btnStreamStart->setEnabled(true);
    ui->btnStreamStop->setEnabled(false);
    ui->labelVideo->clear();
    addLog("视频流已停止");

    netManager_->get(QNetworkRequest(QUrl(API_BASE + "/api/camera/stop_stream")));
}

void MonitorWidget::takeSnapshot()
{
    addLog("正在拍照...");
    QNetworkReply *reply = netManager_->get(QNetworkRequest(QUrl(API_BASE + "/api/camera/snapshot")));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonObject res = QJsonDocument::fromJson(reply->readAll()).object();
            addLog(QString("拍照%1").arg(res.value("result").toString() == "ok" ? "成功" : "失败"));
        } else {
            addLog("拍照失败: " + reply->errorString());
        }
        reply->deleteLater();
    });
}

void MonitorWidget::toggleRecord()
{
    if (!streamRunning_) { addLog("请先启动视频流"); return; }
    if (!recording_) {
        QNetworkReply *reply = netManager_->get(QNetworkRequest(QUrl(API_BASE + "/api/camera/start_record")));
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            if (reply->error() == QNetworkReply::NoError) {
                recording_ = true;
                ui->btnRecord->setText("停止录制");
                addLog("录像已开始");
            } else {
                addLog("录像启动失败: " + reply->errorString());
            }
            reply->deleteLater();
        });
    } else {
        QNetworkReply *reply = netManager_->get(QNetworkRequest(QUrl(API_BASE + "/api/camera/stop_record")));
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            if (reply->error() == QNetworkReply::NoError) {
                recording_ = false;
                ui->btnRecord->setText("视频录制");
                addLog("录像已保存");
            }
            reply->deleteLater();
        });
    }
}
