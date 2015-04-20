/********************************************************************************
** Form generated from reading UI file 'dialog.ui'
**
** Created by: Qt User Interface Compiler version 5.9.5
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_DIALOG_H
#define UI_DIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QDialog>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Dialog
{
public:
    QWidget *layoutWidget;
    QVBoxLayout *verticalLayout;
    QLabel *ST_SRV;
    QLabel *ST_QL;
    QLabel *ST_QP;
    QLabel *ST_QS;
    QLabel *ST_QD;

    void setupUi(QDialog *Dialog)
    {
        if (Dialog->objectName().isEmpty())
            Dialog->setObjectName(QStringLiteral("Dialog"));
        Dialog->resize(718, 650);
        layoutWidget = new QWidget(Dialog);
        layoutWidget->setObjectName(QStringLiteral("layoutWidget"));
        layoutWidget->setGeometry(QRect(9, 19, 691, 291));
        layoutWidget->setMinimumSize(QSize(691, 291));
        verticalLayout = new QVBoxLayout(layoutWidget);
        verticalLayout->setSpacing(6);
        verticalLayout->setContentsMargins(11, 11, 11, 11);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        ST_SRV = new QLabel(layoutWidget);
        ST_SRV->setObjectName(QStringLiteral("ST_SRV"));
        ST_SRV->setMinimumSize(QSize(461, 53));
        ST_SRV->setMaximumSize(QSize(461, 16777215));
        QFont font;
        font.setStrikeOut(false);
        ST_SRV->setFont(font);
        ST_SRV->setFrameShape(QFrame::Box);

        verticalLayout->addWidget(ST_SRV);

        ST_QL = new QLabel(layoutWidget);
        ST_QL->setObjectName(QStringLiteral("ST_QL"));
        ST_QL->setMinimumSize(QSize(469, 53));
        ST_QL->setMaximumSize(QSize(469, 16777215));
        ST_QL->setFrameShape(QFrame::WinPanel);

        verticalLayout->addWidget(ST_QL);

        ST_QP = new QLabel(layoutWidget);
        ST_QP->setObjectName(QStringLiteral("ST_QP"));
        ST_QP->setMinimumSize(QSize(469, 53));
        ST_QP->setMaximumSize(QSize(469, 16777215));
        ST_QP->setFrameShape(QFrame::WinPanel);

        verticalLayout->addWidget(ST_QP);

        ST_QS = new QLabel(layoutWidget);
        ST_QS->setObjectName(QStringLiteral("ST_QS"));
        ST_QS->setMinimumSize(QSize(469, 53));
        ST_QS->setMaximumSize(QSize(469, 16777215));
        ST_QS->setFrameShape(QFrame::WinPanel);

        verticalLayout->addWidget(ST_QS);

        ST_QD = new QLabel(layoutWidget);
        ST_QD->setObjectName(QStringLiteral("ST_QD"));
        ST_QD->setMinimumSize(QSize(469, 53));
        ST_QD->setMaximumSize(QSize(469, 16777215));
        ST_QD->setFrameShape(QFrame::WinPanel);

        verticalLayout->addWidget(ST_QD);


        retranslateUi(Dialog);

        QMetaObject::connectSlotsByName(Dialog);
    } // setupUi

    void retranslateUi(QDialog *Dialog)
    {
        Dialog->setWindowTitle(QApplication::translate("Dialog", "Dialog", Q_NULLPTR));
        ST_SRV->setText(QApplication::translate("Dialog", "TextLabel", Q_NULLPTR));
        ST_QL->setText(QApplication::translate("Dialog", "TextLabel", Q_NULLPTR));
        ST_QP->setText(QApplication::translate("Dialog", "TextLabel", Q_NULLPTR));
        ST_QS->setText(QApplication::translate("Dialog", "TextLabel", Q_NULLPTR));
        ST_QD->setText(QApplication::translate("Dialog", "TextLabel", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class Dialog: public Ui_Dialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DIALOG_H
