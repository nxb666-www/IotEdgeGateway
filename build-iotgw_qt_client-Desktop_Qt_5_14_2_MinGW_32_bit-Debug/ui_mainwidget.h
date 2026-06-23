/********************************************************************************
** Form generated from reading UI file 'mainwidget.ui'
**
** Created by: Qt User Interface Compiler version 5.14.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWIDGET_H
#define UI_MAINWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QTabBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWidget
{
public:
    QVBoxLayout *root;
    QHBoxLayout *header;
    QLabel *labelTitle;
    QSpacerItem *h1;
    QLabel *labelStatus;
    QTabBar *tabBar;
    QStackedWidget *stack;

    void setupUi(QWidget *MainWidget)
    {
        if (MainWidget->objectName().isEmpty())
            MainWidget->setObjectName(QString::fromUtf8("MainWidget"));
        MainWidget->resize(1280, 800);
        MainWidget->setMinimumSize(QSize(1000, 650));
        root = new QVBoxLayout(MainWidget);
        root->setSpacing(8);
        root->setObjectName(QString::fromUtf8("root"));
        root->setContentsMargins(8, 8, 8, 8);
        header = new QHBoxLayout();
        header->setObjectName(QString::fromUtf8("header"));
        labelTitle = new QLabel(MainWidget);
        labelTitle->setObjectName(QString::fromUtf8("labelTitle"));

        header->addWidget(labelTitle);

        h1 = new QSpacerItem(40, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);

        header->addItem(h1);

        labelStatus = new QLabel(MainWidget);
        labelStatus->setObjectName(QString::fromUtf8("labelStatus"));

        header->addWidget(labelStatus);


        root->addLayout(header);

        tabBar = new QTabBar(MainWidget);
        tabBar->setObjectName(QString::fromUtf8("tabBar"));

        root->addWidget(tabBar);

        stack = new QStackedWidget(MainWidget);
        stack->setObjectName(QString::fromUtf8("stack"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(1);
        sizePolicy.setHeightForWidth(stack->sizePolicy().hasHeightForWidth());
        stack->setSizePolicy(sizePolicy);

        root->addWidget(stack);


        retranslateUi(MainWidget);

        QMetaObject::connectSlotsByName(MainWidget);
    } // setupUi

    void retranslateUi(QWidget *MainWidget)
    {
        MainWidget->setWindowTitle(QCoreApplication::translate("MainWidget", "RK3568 \346\231\272\350\203\275\346\216\247\345\210\266\344\270\255\345\277\203", nullptr));
        labelTitle->setText(QCoreApplication::translate("MainWidget", "RK3568 \346\231\272\350\203\275\347\275\221\345\205\263\346\216\247\345\210\266\347\263\273\347\273\237", nullptr));
        labelTitle->setStyleSheet(QCoreApplication::translate("MainWidget", "font-size:18px;font-weight:800;color:white;padding:0 20px;", nullptr));
        labelStatus->setText(QCoreApplication::translate("MainWidget", "\350\277\236\346\216\245\346\226\255\345\274\200", nullptr));
        labelStatus->setStyleSheet(QCoreApplication::translate("MainWidget", "background:rgba(255,255,255,0.2);color:white;padding:4px 16px;border-radius:20px;font-size:12px;", nullptr));
        tabBar->setStyleSheet(QCoreApplication::translate("MainWidget", "QTabBar::tab{background:#fff;border:1px solid #e5e7eb;border-bottom:none;border-radius:8px 8px 0 0;padding:8px 28px;font-size:14px;font-weight:600;color:#64748b;margin-right:2px;} QTabBar::tab:selected{background:#eef2ff;color:#5c67f2;border-bottom:3px solid #5c67f2;} QTabBar::tab:hover{background:#f8fafc;}", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWidget: public Ui_MainWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWIDGET_H
