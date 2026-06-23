#ifndef HISTORYWIDGET_H
#define HISTORYWIDGET_H

#include <QWidget>
#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace Ui {
class HistoryWidget;
}

class HistoryWidget : public QWidget
{
    Q_OBJECT

public:
    explicit HistoryWidget(QWidget *parent = nullptr);
    ~HistoryWidget();

private slots:
    void queryHistory();

private:
    void refreshStats();

    Ui::HistoryWidget *ui;
    QNetworkAccessManager *netManager_;
};

#endif // HISTORYWIDGET_H
