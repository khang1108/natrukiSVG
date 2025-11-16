/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.10.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFrame>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QStackedWidget *stackedWidget;
    QWidget *mainViewer;
    QFrame *viewerFrame;
    QPushButton *About;
    QPushButton *Premium;
    QPushButton *Upload;
    QPushButton *Rotate;
    QPushButton *Flip;
    QPushButton *Copy;
    QPushButton *Zoom;
    QFrame *Logo;
    QPushButton *Browse;
    QPushButton *Fullscreen;
    QPushButton *DownloadPNG;
    QPushButton *ZoomIn;
    QPushButton *ZoomOut;
    QWidget *About_2;
    QFrame *Background;
    QFrame *Description;
    QFrame *Logo_2;
    QFrame *Khang;
    QFrame *Cao;
    QFrame *Nghia;
    QFrame *Nhat;
    QPushButton *Back;
    QWidget *Premium_2;
    QFrame *Background_2;
    QFrame *Logo_7;
    QFrame *Upgrade;
    QPushButton *Back_2;
    QFrame *Free;
    QFrame *Go;
    QFrame *Plus;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(1440, 1024);
        MainWindow->setMinimumSize(QSize(1440, 1024));
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        QSizePolicy sizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(centralwidget->sizePolicy().hasHeightForWidth());
        centralwidget->setSizePolicy(sizePolicy);
        stackedWidget = new QStackedWidget(centralwidget);
        stackedWidget->setObjectName("stackedWidget");
        stackedWidget->setGeometry(QRect(0, 0, 1440, 1024));
        sizePolicy.setHeightForWidth(stackedWidget->sizePolicy().hasHeightForWidth());
        stackedWidget->setSizePolicy(sizePolicy);
        mainViewer = new QWidget();
        mainViewer->setObjectName("mainViewer");
        viewerFrame = new QFrame(mainViewer);
        viewerFrame->setObjectName("viewerFrame");
        viewerFrame->setGeometry(QRect(0, 0, 1440, 1024));
        viewerFrame->setStyleSheet(QString::fromUtf8("QFrame#viewerFrame {\n"
"    /* Tham chi\341\272\277u \304\221\341\272\277n file SVG */\n"
"    background-image: url(:/images/resources/images/Frame 1.png); \n"
"    \n"
"    /* \304\220\341\272\243m b\341\272\243o \341\272\243nh kh\303\264ng l\341\272\267p l\341\272\241i */\n"
"    background-repeat: no-repeat; \n"
"\n"
"    /* C\304\203n gi\341\273\257a \341\272\243nh */\n"
"    background-position: center; \n"
"    \n"
"    /* \304\220\341\272\243m b\341\272\243o n\341\273\201n QFrame s\341\272\241ch s\341\272\275 */\n"
"    border: none;\n"
"    \n"
"  	background-color: transparent;\n"
"}"));
        viewerFrame->setFrameShape(QFrame::Shape::StyledPanel);
        viewerFrame->setFrameShadow(QFrame::Shadow::Raised);
        About = new QPushButton(viewerFrame);
        About->setObjectName("About");
        About->setGeometry(QRect(941, 45, 191, 60));
        About->setStyleSheet(QString::fromUtf8("QPushButton#About{\n"
"    /* Tham chi\341\272\277u \304\221\341\272\277n file SVG */\n"
"    background-image: url(:/icons/resources/icons/About Button.png); \n"
"    \n"
"    background-repeat: no-repeat; \n"
"\n"
"    background-position: center; \n"
"    \n"
"    border: none;\n"
"\n"
"}"));
        Premium = new QPushButton(viewerFrame);
        Premium->setObjectName("Premium");
        Premium->setGeometry(QRect(1187, 45, 191, 59));
        Premium->setStyleSheet(QString::fromUtf8("QPushButton#Premium{\n"
"    /* Tham chi\341\272\277u \304\221\341\272\277n file SVG */\n"
"    background-image:url(:/icons/resources/icons/Premium Button.png); \n"
"    \n"
"    background-repeat: no-repeat; \n"
"\n"
"    background-position: center; \n"
"    \n"
"    border: none;\n"
"\n"
"}"));
        Upload = new QPushButton(viewerFrame);
        Upload->setObjectName("Upload");
        Upload->setGeometry(QRect(214, 925, 81, 81));
        Upload->setStyleSheet(QString::fromUtf8("QPushButton#Upload{\n"
"    /* Tham chi\341\272\277u \304\221\341\272\277n file SVG */\n"
"    background-image: url(:/icons/resources/icons/UploadFile button.png); \n"
"    \n"
"    background-repeat: no-repeat; \n"
"\n"
"    background-position: center; \n"
"    \n"
"    border: none;\n"
"\n"
"}"));
        Rotate = new QPushButton(viewerFrame);
        Rotate->setObjectName("Rotate");
        Rotate->setGeometry(QRect(305, 924, 81, 82));
        Rotate->setStyleSheet(QString::fromUtf8("QPushButton#Rotate{\n"
"    /* Tham chi\341\272\277u \304\221\341\272\277n file SVG */\n"
"    background-image: url(:/icons/resources/icons/Rotate Button.png); \n"
"    \n"
"    background-repeat: no-repeat; \n"
"\n"
"    background-position: center; \n"
"    \n"
"    border: none;\n"
"\n"
"}"));
        Flip = new QPushButton(viewerFrame);
        Flip->setObjectName("Flip");
        Flip->setGeometry(QRect(396, 924, 81, 82));
        Flip->setStyleSheet(QString::fromUtf8("QPushButton#Flip{\n"
"    /* Tham chi\341\272\277u \304\221\341\272\277n file SVG */\n"
"    background-image: url(:/icons/resources/icons/Flip Button.png); \n"
"    \n"
"    background-repeat: no-repeat; \n"
"\n"
"    background-position: center; \n"
"    \n"
"    border: none;\n"
"\n"
"}"));
        Copy = new QPushButton(viewerFrame);
        Copy->setObjectName("Copy");
        Copy->setGeometry(QRect(638, 924, 81, 82));
        Copy->setStyleSheet(QString::fromUtf8("QPushButton#Copy{\n"
"    /* Tham chi\341\272\277u \304\221\341\272\277n file SVG */\n"
"    background-image:url(:/icons/resources/icons/Copy button.png); \n"
"    \n"
"    background-repeat: no-repeat; \n"
"\n"
"    background-position: center; \n"
"    \n"
"    border: none;\n"
"\n"
"}"));
        Zoom = new QPushButton(viewerFrame);
        Zoom->setObjectName("Zoom");
        Zoom->setGeometry(QRect(752, 926, 244, 79));
        Zoom->setStyleSheet(QString::fromUtf8("QPushButton#Zoom{\n"
"    /* Tham chi\341\272\277u \304\221\341\272\277n file SVG */\n"
"    background-image:url(:/icons/resources/icons/Size button.png); \n"
"    \n"
"    background-repeat: no-repeat; \n"
"\n"
"    background-position: center; \n"
"    \n"
"    border: none;\n"
"\n"
"}"));
        Logo = new QFrame(viewerFrame);
        Logo->setObjectName("Logo");
        Logo->setGeometry(QRect(-12, 30, 640, 90));
        Logo->setStyleSheet(QString::fromUtf8("QFrame#Logo{\n"
"    /* Tham chi\341\272\277u \304\221\341\272\277n file SVG */\n"
"    background-image: url(:/images/resources/images/NaTruKiSVG.png); \n"
"    \n"
"    background-repeat: no-repeat; \n"
"\n"
"    background-position: center; \n"
"    \n"
"    border: none;\n"
"\n"
"}"));
        Logo->setFrameShape(QFrame::Shape::StyledPanel);
        Logo->setFrameShadow(QFrame::Shadow::Raised);
        Browse = new QPushButton(viewerFrame);
        Browse->setObjectName("Browse");
        Browse->setGeometry(QRect(65, 158, 1313, 746));
        Browse->setStyleSheet(QString::fromUtf8("QPushButton#Browse{\n"
"    /* Tham chi\341\272\277u \304\221\341\272\277n file SVG */\n"
"    background-image: url(:/icons/resources/icons/Upload.png); \n"
"    \n"
"    background-repeat: no-repeat; \n"
"\n"
"    background-position: center; \n"
"    \n"
"    border: none;\n"
"\n"
"}"));
        Fullscreen = new QPushButton(viewerFrame);
        Fullscreen->setObjectName("Fullscreen");
        Fullscreen->setGeometry(QRect(1143, 925, 81, 81));
        Fullscreen->setStyleSheet(QString::fromUtf8("QPushButton#Fullscreen{\n"
"    /* Tham chi\341\272\277u \304\221\341\272\277n file SVG */\n"
"    background-image: url(:/icons/resources/icons/Zoom Button.png); \n"
"    \n"
"    background-repeat: no-repeat; \n"
"\n"
"    background-position: center; \n"
"    \n"
"    border: none;\n"
"\n"
"}"));
        DownloadPNG = new QPushButton(viewerFrame);
        DownloadPNG->setObjectName("DownloadPNG");
        DownloadPNG->setGeometry(QRect(1029, 924, 81, 82));
        DownloadPNG->setStyleSheet(QString::fromUtf8("QPushButton#DownloadPNG{\n"
"    /* Tham chi\341\272\277u \304\221\341\272\277n file SVG */\n"
"    background-image: url(:/icons/resources/icons/Download button.png); \n"
"    \n"
"    background-repeat: no-repeat; \n"
"\n"
"    background-position: center; \n"
"    \n"
"    border: none;\n"
"\n"
"}"));
        ZoomIn = new QPushButton(viewerFrame);
        ZoomIn->setObjectName("ZoomIn");
        ZoomIn->setGeometry(QRect(933, 934, 58, 60));
        ZoomIn->setStyleSheet(QString::fromUtf8("QPushButton#ZoomIn{\n"
"    /* Tham chi\341\272\277u \304\221\341\272\277n file SVG */\n"
"    background-image:url(:/icons/resources/icons/zoom in button.png); \n"
"    \n"
"    background-repeat: no-repeat; \n"
"\n"
"    background-position: center; \n"
"    \n"
"    border: none;\n"
"\n"
"}"));
        ZoomOut = new QPushButton(viewerFrame);
        ZoomOut->setObjectName("ZoomOut");
        ZoomOut->setGeometry(QRect(757, 934, 58, 60));
        ZoomOut->setStyleSheet(QString::fromUtf8("QPushButton#ZoomOut{\n"
"    /* Tham chi\341\272\277u \304\221\341\272\277n file SVG */\n"
"    background-image:url(:/icons/resources/icons/zoom out button.png); \n"
"    \n"
"    background-repeat: no-repeat; \n"
"\n"
"    background-position: center; \n"
"    \n"
"    border: none;\n"
"\n"
"}"));
        stackedWidget->addWidget(mainViewer);
        About_2 = new QWidget();
        About_2->setObjectName("About_2");
        Background = new QFrame(About_2);
        Background->setObjectName("Background");
        Background->setGeometry(QRect(0, 0, 1440, 1024));
        Background->setStyleSheet(QString::fromUtf8("QFrame#Background{\n"
"    /* Tham chi\341\272\277u \304\221\341\272\277n file SVG */\n"
"    background-image: url(:/about/images/resources/about/images/Gradient.png); \n"
"    \n"
"    background-repeat: no-repeat; \n"
"\n"
"    background-position: center; \n"
"    \n"
"    border: none;\n"
"\n"
"}"));
        Background->setFrameShape(QFrame::Shape::StyledPanel);
        Background->setFrameShadow(QFrame::Shadow::Raised);
        Description = new QFrame(Background);
        Description->setObjectName("Description");
        Description->setGeometry(QRect(63, 130, 593, 809));
        Description->setStyleSheet(QString::fromUtf8("QFrame#Description{\n"
"    /* Tham chi\341\272\277u \304\221\341\272\277n file SVG */\n"
"    background-image: url(:/about/icons/resources/about/icons/Khung text.png); \n"
"    \n"
"    background-repeat: no-repeat; \n"
"\n"
"    background-position: center; \n"
"    \n"
"    border: none;\n"
"\n"
"}"));
        Description->setFrameShape(QFrame::Shape::StyledPanel);
        Description->setFrameShadow(QFrame::Shadow::Raised);
        Logo_2 = new QFrame(Background);
        Logo_2->setObjectName("Logo_2");
        Logo_2->setGeometry(QRect(32, 22, 651, 99));
        Logo_2->setStyleSheet(QString::fromUtf8("QFrame#Logo_2{\n"
"    /* Tham chi\341\272\277u \304\221\341\272\277n file SVG */\n"
"    background-image: url(:/about/images/resources/about/images/About NaTruKiSVG.png); \n"
"    \n"
"    background-repeat: no-repeat; \n"
"\n"
"    background-position: center; \n"
"    \n"
"    border: none;\n"
"\n"
"}"));
        Logo_2->setFrameShape(QFrame::Shape::StyledPanel);
        Logo_2->setFrameShadow(QFrame::Shadow::Raised);
        Khang = new QFrame(Background);
        Khang->setObjectName("Khang");
        Khang->setGeometry(QRect(720, 130, 302, 379));
        Khang->setStyleSheet(QString::fromUtf8("QFrame#Khang{\n"
"    /* Tham chi\341\272\277u \304\221\341\272\277n file SVG */\n"
"    background-image: url(:/about/icons/resources/about/icons/about Khang.png); \n"
"    \n"
"    background-repeat: no-repeat; \n"
"\n"
"    background-position: center; \n"
"    \n"
"    border: none;\n"
"\n"
"}"));
        Khang->setFrameShape(QFrame::Shape::StyledPanel);
        Khang->setFrameShadow(QFrame::Shadow::Raised);
        Cao = new QFrame(Background);
        Cao->setObjectName("Cao");
        Cao->setGeometry(QRect(1084, 130, 302, 379));
        Cao->setStyleSheet(QString::fromUtf8("QFrame#Cao{\n"
"    /* Tham chi\341\272\277u \304\221\341\272\277n file SVG */\n"
"    background-image: url(:/about/icons/resources/about/icons/About Cao.png); \n"
"    \n"
"    background-repeat: no-repeat; \n"
"\n"
"    background-position: center; \n"
"    \n"
"    border: none;\n"
"\n"
"}"));
        Cao->setFrameShape(QFrame::Shape::StyledPanel);
        Cao->setFrameShadow(QFrame::Shadow::Raised);
        Nghia = new QFrame(Background);
        Nghia->setObjectName("Nghia");
        Nghia->setGeometry(QRect(720, 560, 302, 379));
        Nghia->setStyleSheet(QString::fromUtf8("QFrame#Nghia{\n"
"    /* Tham chi\341\272\277u \304\221\341\272\277n file SVG */\n"
"    background-image: url(:/about/icons/resources/about/icons/About Nghia.png); \n"
"    \n"
"    background-repeat: no-repeat; \n"
"\n"
"    background-position: center; \n"
"    \n"
"    border: none;\n"
"\n"
"}"));
        Nghia->setFrameShape(QFrame::Shape::StyledPanel);
        Nghia->setFrameShadow(QFrame::Shadow::Raised);
        Nhat = new QFrame(Background);
        Nhat->setObjectName("Nhat");
        Nhat->setGeometry(QRect(1084, 560, 302, 379));
        Nhat->setStyleSheet(QString::fromUtf8("QFrame#Nhat{\n"
"    /* Tham chi\341\272\277u \304\221\341\272\277n file SVG */\n"
"    background-image: url(:/about/icons/resources/about/icons/About Nhat.png); \n"
"    \n"
"    background-repeat: no-repeat; \n"
"\n"
"    background-position: center; \n"
"    \n"
"    border: none;\n"
"\n"
"}"));
        Nhat->setFrameShape(QFrame::Shape::StyledPanel);
        Nhat->setFrameShadow(QFrame::Shadow::Raised);
        Back = new QPushButton(Background);
        Back->setObjectName("Back");
        Back->setGeometry(QRect(1195, 41, 191, 60));
        Back->setStyleSheet(QString::fromUtf8("QPushButton#Back{\n"
"    /* Tham chi\341\272\277u \304\221\341\272\277n file SVG */\n"
"    background-image: url(:/about/icons/resources/about/icons/Back Button.png); \n"
"    \n"
"    background-repeat: no-repeat; \n"
"\n"
"    background-position: center; \n"
"    \n"
"    border: none;\n"
"\n"
"}"));
        stackedWidget->addWidget(About_2);
        Premium_2 = new QWidget();
        Premium_2->setObjectName("Premium_2");
        Background_2 = new QFrame(Premium_2);
        Background_2->setObjectName("Background_2");
        Background_2->setGeometry(QRect(0, 0, 1440, 1024));
        Background_2->setStyleSheet(QString::fromUtf8("QFrame#Background_2{\n"
"    /* Tham chi\341\272\277u \304\221\341\272\277n file SVG */\n"
"    background-image: url(:/premium/images/resources/premium/images/Gradient.png); \n"
"    \n"
"    background-repeat: no-repeat; \n"
"\n"
"    background-position: center; \n"
"    \n"
"    border: none;\n"
"\n"
"}"));
        Background_2->setFrameShape(QFrame::Shape::StyledPanel);
        Background_2->setFrameShadow(QFrame::Shadow::Raised);
        Logo_7 = new QFrame(Background_2);
        Logo_7->setObjectName("Logo_7");
        Logo_7->setGeometry(QRect(-64, 22, 640, 90));
        Logo_7->setStyleSheet(QString::fromUtf8("QFrame#Logo_7{\n"
"    /* Tham chi\341\272\277u \304\221\341\272\277n file SVG */\n"
"    background-image: url(:/premium/images/resources/premium/images/NaTruKiSVG.png); \n"
"    \n"
"    background-repeat: no-repeat; \n"
"\n"
"    background-position: center; \n"
"    \n"
"    border: none;\n"
"\n"
"}"));
        Logo_7->setFrameShape(QFrame::Shape::StyledPanel);
        Logo_7->setFrameShadow(QFrame::Shadow::Raised);
        Upgrade = new QFrame(Background_2);
        Upgrade->setObjectName("Upgrade");
        Upgrade->setGeometry(QRect(493, 108, 455, 79));
        Upgrade->setStyleSheet(QString::fromUtf8("QFrame#Upgrade{\n"
"    /* Tham chi\341\272\277u \304\221\341\272\277n file SVG */\n"
"    background-image: url(:/premium/images/resources/premium/images/Upgrade your tool.png); \n"
"    \n"
"    background-repeat: no-repeat; \n"
"\n"
"    background-position: center; \n"
"    \n"
"    border: none;\n"
"\n"
"}"));
        Upgrade->setFrameShape(QFrame::Shape::StyledPanel);
        Upgrade->setFrameShadow(QFrame::Shadow::Raised);
        Back_2 = new QPushButton(Background_2);
        Back_2->setObjectName("Back_2");
        Back_2->setGeometry(QRect(1205, 37, 191, 58));
        Back_2->setStyleSheet(QString::fromUtf8("QPushButton#Back_2{\n"
"    /* Tham chi\341\272\277u \304\221\341\272\277n file SVG */\n"
"    background-image: url(:/premium/icons/resources/premium/icons/Back Button.png); \n"
"    \n"
"    background-repeat: no-repeat; \n"
"\n"
"    background-position: center; \n"
"    \n"
"    border: none;\n"
"\n"
"}"));
        Free = new QFrame(Background_2);
        Free->setObjectName("Free");
        Free->setGeometry(QRect(43, 214, 425, 726));
        Free->setStyleSheet(QString::fromUtf8("QFrame#Free{\n"
"    /* Tham chi\341\272\277u \304\221\341\272\277n file SVG */\n"
"    background-image: url(:/premium/icons/resources/premium/icons/NatrukiFree.png); \n"
"    \n"
"    background-repeat: no-repeat; \n"
"\n"
"    background-position: center; \n"
"    \n"
"    border: none;\n"
"\n"
"}"));
        Free->setFrameShape(QFrame::Shape::StyledPanel);
        Free->setFrameShadow(QFrame::Shadow::Raised);
        Go = new QFrame(Background_2);
        Go->setObjectName("Go");
        Go->setGeometry(QRect(507, 214, 425, 726));
        Go->setStyleSheet(QString::fromUtf8("QFrame#Go{\n"
"    /* Tham chi\341\272\277u \304\221\341\272\277n file SVG */\n"
"    background-image: url(:/premium/icons/resources/premium/icons/NatrukiGo.png); \n"
"    \n"
"    background-repeat: no-repeat; \n"
"\n"
"    background-position: center; \n"
"    \n"
"    border: none;\n"
"\n"
"}"));
        Go->setFrameShape(QFrame::Shape::StyledPanel);
        Go->setFrameShadow(QFrame::Shadow::Raised);
        Plus = new QFrame(Background_2);
        Plus->setObjectName("Plus");
        Plus->setGeometry(QRect(971, 214, 425, 726));
        Plus->setStyleSheet(QString::fromUtf8("QFrame#Plus{\n"
"    /* Tham chi\341\272\277u \304\221\341\272\277n file SVG */\n"
"    background-image: url(:/premium/icons/resources/premium/icons/NatrukiPlus.png); \n"
"    \n"
"    background-repeat: no-repeat; \n"
"\n"
"    background-position: center; \n"
"    \n"
"    border: none;\n"
"\n"
"}"));
        Plus->setFrameShape(QFrame::Shape::StyledPanel);
        Plus->setFrameShadow(QFrame::Shadow::Raised);
        stackedWidget->addWidget(Premium_2);
        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 1440, 22));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        About->setText(QString());
        Premium->setText(QString());
        Upload->setText(QString());
        Rotate->setText(QString());
        Flip->setText(QString());
        Copy->setText(QString());
        Zoom->setText(QString());
        Browse->setText(QString());
        Fullscreen->setText(QString());
        DownloadPNG->setText(QString());
        ZoomIn->setText(QString());
        ZoomOut->setText(QString());
        Back->setText(QString());
        Back_2->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
