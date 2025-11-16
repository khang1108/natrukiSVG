#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_About_clicked();

    void on_Browse_clicked();

    void on_Back_clicked();

    void on_Back_2_clicked();

    void on_Premium_clicked();

    void on_Copy_clicked();

    void on_DownloadPNG_clicked();

    void on_Flip_clicked();

    void on_Fullscreen_clicked();

    void on_Rotate_clicked();

    void on_Upload_clicked();

    void on_Zoom_clicked();

    void on_ZoomOut_clicked();

    void on_ZoomIn_clicked();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
