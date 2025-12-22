#include "mainwindow.h"

#include <QApplication>
#include <QCursor>
#include <QLocale>
#include <QPixmap>
#include <QTranslator>

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/resources/images/logo.png"));

    // Set custom cursor
    QPixmap cursorPixmap(":/icons/resources/icons/cursor.png");
    if (!cursorPixmap.isNull()) {
        // Scale cursor to appropriate size (32x32 is standard)
        cursorPixmap = cursorPixmap.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QCursor customCursor(cursorPixmap);
        a.setOverrideCursor(customCursor);
    }

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString& locale : uiLanguages) {
        const QString baseName = "NaTrukiSVG_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }
    MainWindow w;
    w.show();
    return a.exec();
}
