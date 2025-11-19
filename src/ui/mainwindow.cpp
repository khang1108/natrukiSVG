#include "mainwindow.h"

#include "svg/SVGDocument.h"
#include "ui_mainwindow.h"

#include <QClipboard>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImage>
#include <QMessageBox>
#include <memory>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
  ui->setupUi(this);
  setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);
  setFixedSize(size());
  m_initialSize = size();
  statusBar()->hide();
  initializeCanvas();
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::initializeCanvas()
{
  if (m_canvas) {
    return;
  }
  m_canvas = new CanvasWidget(ui->viewerFrame);
  m_canvas->setGeometry(ui->Browse->geometry());
  m_canvas->hide();
  m_canvas->raise();
}

void MainWindow::toggleStackedPage(QWidget* page)
{
  if (page) {
    ui->stackedWidget->setCurrentWidget(page);
  }
}

void MainWindow::updateCanvasVisibility()
{
  if (!m_canvas)
    return;
  bool hasDoc = m_canvas->hasDocument();
  m_canvas->setVisible(hasDoc);
  ui->Browse->setVisible(!hasDoc);
}

void MainWindow::showTransientMessage(const QString& message, int timeoutMs)
{
  statusBar()->showMessage(message, timeoutMs);
}

bool MainWindow::loadSvgFromFile(const QString& path)
{
  if (path.isEmpty()) {
    return false;
  }

  auto document = std::make_unique<SVGDocument>();
  if (!document->load(path.toStdString())) {
    QMessageBox::warning(this, tr("Load Failed"), tr("Unable to parse SVG file:\n%1").arg(path));
    return false;
  }

  if (!m_canvas) {
    initializeCanvas();
  }

  m_canvas->setDocument(std::move(document));
  m_currentSvgPath = path;
  updateCanvasVisibility();
  showTransientMessage(tr("Loaded %1").arg(QFileInfo(path).fileName()));
  return true;
}

void MainWindow::promptAndLoadSvg()
{
  QString startDir =
      m_currentSvgPath.isEmpty() ? QDir::homePath() : QFileInfo(m_currentSvgPath).absolutePath();

  QString filePath = QFileDialog::getOpenFileName(this, tr("Open SVG"), startDir,
                                                  tr("SVG Files (*.svg);;All Files (*)"));

  if (!filePath.isEmpty()) {
    loadSvgFromFile(filePath);
  }
}

bool MainWindow::ensureDocumentAvailable()
{
  if (m_canvas && m_canvas->hasDocument()) {
    return true;
  }
  QMessageBox::information(this, tr("No Document"), tr("Please upload an SVG file first."));
  return false;
}

void MainWindow::on_About_clicked() { toggleStackedPage(ui->About_2); }

void MainWindow::on_Browse_clicked() { promptAndLoadSvg(); }

void MainWindow::on_Back_clicked() { toggleStackedPage(ui->mainViewer); }

void MainWindow::on_Back_2_clicked() { toggleStackedPage(ui->mainViewer); }

void MainWindow::on_Premium_clicked() { toggleStackedPage(ui->Premium_2); }

void MainWindow::on_Copy_clicked()
{
  if (m_currentSvgPath.isEmpty()) {
    QMessageBox::information(this, tr("Nothing to copy"), tr("Load an SVG before copying."));
    return;
  }

  QFile file(m_currentSvgPath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QMessageBox::warning(this, tr("Copy failed"),
                         tr("Unable to open file:\n%1").arg(m_currentSvgPath));
    return;
  }

  QString content = QString::fromUtf8(file.readAll());
  QGuiApplication::clipboard()->setText(content);
  showTransientMessage(tr("SVG content copied to clipboard"));
}

void MainWindow::on_DownloadPNG_clicked()
{
  if (!ensureDocumentAvailable()) {
    return;
  }

  QString targetPath = QFileDialog::getSaveFileName(
      this, tr("Export PNG"), QDir::homePath() + "/render.png", tr("PNG Images (*.png)"));
  if (targetPath.isEmpty()) {
    return;
  }

  if (!targetPath.endsWith(".png", Qt::CaseInsensitive)) {
    targetPath += ".png";
  }

  QImage image = m_canvas->renderToImage();
  if (!image.save(targetPath)) {
    QMessageBox::warning(this, tr("Export failed"),
                         tr("Unable to save image to:\n%1").arg(targetPath));
    return;
  }

  showTransientMessage(tr("Exported PNG to %1").arg(QFileInfo(targetPath).fileName()));
}

void MainWindow::on_Flip_clicked()
{
  if (!ensureDocumentAvailable()) {
    return;
  }
  m_canvas->flip();
}

void MainWindow::on_Fullscreen_clicked()
{
  if (isFullScreen()) {
    showNormal();
    setFixedSize(m_initialSize);
  }
  else {
    showFullScreen();
  }
}

void MainWindow::on_Rotate_clicked()
{
  if (!ensureDocumentAvailable()) {
    return;
  }
  m_canvas->rotate();
}

void MainWindow::on_Upload_clicked() { promptAndLoadSvg(); }

void MainWindow::on_Zoom_clicked()
{
  if (!ensureDocumentAvailable()) {
    return;
  }
  m_canvas->zoomReset();
}

void MainWindow::on_ZoomOut_clicked()
{
  if (!ensureDocumentAvailable()) {
    return;
  }
  m_canvas->zoomOut();
}

void MainWindow::on_ZoomIn_clicked()
{
  if (!ensureDocumentAvailable()) {
    return;
  }
  m_canvas->zoomIn();
}
