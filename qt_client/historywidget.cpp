#include "historywidget.h"
#include "ui_historywidget.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QNetworkReply>
#include <QSettings>
#include <QHeaderView>
#include <QCoreApplication>

static QString getApiBase() {
    QString iniPath = QCoreApplication::applicationDirPath() + "/iotgw_qt_client.ini";
    QSettings cfg(iniPath, QSettings::IniFormat);
    return cfg.value("server/url", "http://192.168.233.107:8081").toString();
}

HistoryWidget::HistoryWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::HistoryWidget),
    netManager_(new QNetworkAccessManager(this))
{
    ui->setupUi(this);

    // 设备列表（跟 web 一致）
    ui->comboDevice->addItem("温度传感器", "temp");
    ui->comboDevice->addItem("湿度传感器", "humi");
    ui->comboDevice->addItem("光照传感器", "light");
    ui->comboDevice->addItem("红外传感器", "ir");

    // 条数（跟 web 一致）
    ui->comboLimit->addItem("最近 20 条", 20);
    ui->comboLimit->addItem("最近 50 条", 50);
    ui->comboLimit->addItem("最近 100 条", 100);

    // 表格列宽
    ui->tableHistory->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    connect(ui->btnQuery, &QPushButton::clicked, this, &HistoryWidget::queryHistory);
}

HistoryWidget::~HistoryWidget()
{
    delete ui;
}

void HistoryWidget::refreshStats()
{
    QNetworkReply *reply = netManager_->get(QNetworkRequest(QUrl(getApiBase() + "/api/status")));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) { reply->deleteLater(); return; }
        QJsonObject data = QJsonDocument::fromJson(reply->readAll()).object();
        reply->deleteLater();

        ui->statTemp->setText(data.contains("temp") ? QString::number(data["temp"].toDouble(), 'f', 1) : "--");
        ui->statHumi->setText(data.contains("humi") ? QString::number(data["humi"].toDouble(), 'f', 1) : "--");
        ui->statLight->setText(data.contains("light") ? QString::number(data["light"].toDouble(), 'f', 0) : "--");

        int ir = data.value("ir").toInt();
        ui->statIr->setText(ir == 1 ? "有人" : "安全");
        ui->statIr->setStyleSheet(QString("font-size:16px; font-weight:800; color:%1;")
            .arg(ir == 1 ? "#ef4444" : "#10b981"));
    });
}

void HistoryWidget::queryHistory()
{
    QString deviceId = ui->comboDevice->currentData().toString();
    int limit = ui->comboLimit->currentData().toInt();

    ui->tableHistory->setRowCount(0);

    // 先刷新最新数据
    refreshStats();

    QString url = QString("%1/api/history?device_id=%2&limit=%3")
                      .arg(getApiBase(), deviceId).arg(limit);

    QNetworkReply *reply = netManager_->get(QNetworkRequest(QUrl(url)));
    connect(reply, &QNetworkReply::finished, this, [this, reply, deviceId]() {
        if (reply->error() != QNetworkReply::NoError) {
            reply->deleteLater();
            return;
        }
        QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
        reply->deleteLater();

        QJsonArray items = obj.value("items").toArray();
        if (items.isEmpty()) {
            ui->tableHistory->setRowCount(1);
            ui->tableHistory->setItem(0, 0, new QTableWidgetItem("暂无数据"));
            ui->tableHistory->setItem(0, 1, new QTableWidgetItem(""));
            ui->tableHistory->setItem(0, 2, new QTableWidgetItem(""));
            ui->tableHistory->item(0, 0)->setTextAlignment(Qt::AlignCenter);
            return;
        }

        // 反转（跟 web 一致：data.items.reverse()）
        QJsonArray reversed;
        for (int i = items.size() - 1; i >= 0; i--) reversed.append(items[i]);

        ui->tableHistory->setRowCount(reversed.size());
        for (int i = 0; i < reversed.size(); i++) {
            QJsonObject item = reversed[i].toObject();
            double value = item.value("value").toDouble();
            QString ts = item.value("ts").toString();
            QString created = item.value("created_at").toString();

            // 格式化入库时间
            QDateTime dt = QDateTime::fromString(created, Qt::ISODate);
            QString createdStr = dt.isValid() ? dt.toString("yyyy/M/d HH:mm:ss") : created;

            QTableWidgetItem *valItem = new QTableWidgetItem(QString::number(value, 'f', 1));
            valItem->setForeground(QColor("#5c67f2"));
            valItem->setFont(QFont("", -1, QFont::Bold));

            ui->tableHistory->setItem(i, 0, valItem);
            ui->tableHistory->setItem(i, 1, new QTableWidgetItem(ts));
            ui->tableHistory->setItem(i, 2, new QTableWidgetItem(createdStr));

            // 右对齐时间列
            ui->tableHistory->item(i, 1)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            ui->tableHistory->item(i, 2)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        }
    });
}
