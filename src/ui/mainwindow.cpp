#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "CanvasWidget.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);
    setFixedSize(size());
    statusBar()->hide();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_About_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->About_2);
}

void MainWindow::on_Browse_clicked()
{
}

void MainWindow::on_Back_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->mainViewer);
}

void MainWindow::on_Back_2_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->mainViewer);
}

void MainWindow::on_Premium_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->Premium_2);
}

void MainWindow::on_Copy_clicked()
{

}

void MainWindow::on_DownloadPNG_clicked()
{
}

void MainWindow::on_Flip_clicked()
{

}

void MainWindow::on_Fullscreen_clicked()
{
}

void MainWindow::on_Rotate_clicked()
{
}

void MainWindow::on_Upload_clicked()
{
}

void MainWindow::on_Zoom_clicked()
{
}

void MainWindow::on_ZoomOut_clicked()
{
}

void MainWindow::on_ZoomIn_clicked()
{
}
