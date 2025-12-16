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
#include <QInputDialog>
#include <QMessageBox>
#include <QMouseEvent>
#include <memory>

/**
 * @brief Constructor for MainWindow - initializes the main application window.
 *
 * Algorithm:
 * 1. Setup UI from .ui file (Qt Designer)
 * 2. Remove maximize button from window
 * 3. Fix window size (prevent resizing)
 * 4. Store initial size for fullscreen toggle
 * 5. Hide status bar (shown only when needed)
 * 6. Initialize canvas widget for SVG rendering
 *
 * @param parent Parent widget (nullptr for top-level window)
 */
MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);
    setFixedSize(size());
    m_initialSize = size();
    statusBar()->hide();
    initializeCanvas();

    // Install event filter for scaleLabel to handle double-click
    ui->scaleLabel->installEventFilter(this);
}

MainWindow::~MainWindow() { delete ui; }

/**
 * @brief Initializes the canvas widget for SVG rendering.
 *
 * Algorithm:
 * - Creates CanvasWidget if it doesn't exist
 * - Positions it to match the Browse button geometry
 * - Hides it initially (shown when document is loaded)
 * - Raises it above other widgets (ensures it's on top)
 */
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

/**
 * @brief Switches the stacked widget to show a specific page.
 *
 * Algorithm:
 * - Sets the current widget of the stacked widget to the specified page
 * - Used for navigation between different UI pages (main viewer, about, premium, etc.)
 *
 * @param page The widget page to show (nullptr = no change)
 */
void MainWindow::toggleStackedPage(QWidget* page)
{
    if (page) {
        ui->stackedWidget->setCurrentWidget(page);
    }
}

/**
 * @brief Updates visibility of canvas and browse button based on document state.
 *
 * Algorithm:
 * - If document is loaded: show canvas, hide browse button
 * - If no document: hide canvas, show browse button
 *
 * This provides a clean UI: browse button when no file, canvas when file is loaded.
 */
void MainWindow::updateCanvasVisibility()
{
    if (!m_canvas)
        return;
    bool hasDoc = m_canvas->hasDocument();
    m_canvas->setVisible(hasDoc);
    ui->Browse->setVisible(!hasDoc);
}

/**
 * @brief Shows a transient message in the status bar.
 *
 * Algorithm:
 * - Temporarily shows the status bar
 * - Displays the message for the specified timeout
 * - Status bar hides automatically after timeout
 *
 * @param message The message to display
 * @param timeoutMs Timeout in milliseconds (0 = show until next message)
 */
void MainWindow::showTransientMessage(const QString& message, int timeoutMs)
{
    statusBar()->showMessage(message, timeoutMs);
}

/**
 * @brief Loads an SVG file from disk and displays it in the canvas.
 *
 * Algorithm:
 * 1. Validate path (must not be empty)
 * 2. Create SVGDocument and attempt to load file
 * 3. If loading fails: show error message and return false
 * 4. If loading succeeds:
 *    - Ensure canvas is initialized
 *    - Set document in canvas
 *    - Store current file path
 *    - Update UI visibility (show canvas, hide browse button)
 *    - Show success message in status bar
 *
 * @param path File path to the SVG file to load
 * @return true if file was successfully loaded, false otherwise
 */
bool MainWindow::loadSvgFromFile(const QString& path)
{
    if (path.isEmpty()) {
        return false;
    }

    auto document = std::make_unique<SVGDocument>();
    if (!document->load(path.toStdString())) {
        QMessageBox::warning(this, tr("Load Failed"),
                             tr("Unable to parse SVG file:\n%1").arg(path));
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

/**
 * @brief Prompts user to select an SVG file and loads it.
 *
 * Algorithm:
 * 1. Determine starting directory:
 *    - If a file was previously loaded: use that file's directory
 *    - Otherwise: use home directory
 * 2. Show file dialog for selecting SVG file
 * 3. If user selected a file: load it using loadSvgFromFile()
 *
 * This provides a user-friendly way to browse and load SVG files.
 */
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

/**
 * @brief Checks if a document is available, shows message if not.
 *
 * Algorithm:
 * - Checks if canvas exists and has a document loaded
 * - If document available: return true
 * - If no document: show informational message and return false
 *
 * This is used by operations that require a loaded document (export, zoom, etc.)
 *
 * @return true if document is available, false otherwise
 */
bool MainWindow::ensureDocumentAvailable()
{
    if (m_canvas && m_canvas->hasDocument()) {
        return true;
    }
    QMessageBox::information(this, tr("No Document"), tr("Please upload an SVG file first."));
    return false;
}

/**
 * @brief Slot: Handles About button click - shows About page.
 */
void MainWindow::on_About_clicked() { toggleStackedPage(ui->About_2); }

/**
 * @brief Slot: Handles Browse button click - prompts to load SVG file.
 */
void MainWindow::on_Browse_clicked() { promptAndLoadSvg(); }

/**
 * @brief Slot: Handles Back button click - returns to main viewer.
 */
void MainWindow::on_Back_clicked() { toggleStackedPage(ui->mainViewer); }

/**
 * @brief Slot: Handles Back button click (from About page) - returns to main viewer.
 */
void MainWindow::on_Back_2_clicked() { toggleStackedPage(ui->mainViewer); }

/**
 * @brief Slot: Handles Premium button click - shows Premium page.
 */
void MainWindow::on_Premium_clicked() { toggleStackedPage(ui->Premium_2); }

/**
 * @brief Slot: Handles Copy button click - copies SVG content to clipboard.
 *
 * Algorithm:
 * 1. Check if a file is loaded
 * 2. Open the SVG file for reading
 * 3. Read entire file content as UTF-8 text
 * 4. Copy content to system clipboard
 * 5. Show success message
 */
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

/**
 * @brief Slot: Handles Download PNG button click - exports SVG as PNG image.
 *
 * Algorithm:
 * 1. Ensure document is available
 * 2. Show save file dialog (default: ~/render.png)
 * 3. If user canceled: return
 * 4. Ensure filename ends with .png extension
 * 5. Render document to QImage
 * 6. Save image to file
 * 7. Show success message
 */
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

/**
 * @brief Slot: Handles Flip button click - toggles horizontal flip.
 */
void MainWindow::on_Flip_clicked()
{
    if (!ensureDocumentAvailable()) {
        return;
    }
    m_canvas->flip();
}

/**
 * @brief Slot: Handles Fullscreen button click - toggles fullscreen mode.
 *
 * Algorithm:
 * - If currently fullscreen: exit fullscreen, restore fixed size
 * - If not fullscreen: enter fullscreen mode
 */
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

/**
 * @brief Slot: Handles Rotate button click - rotates view by 90 degrees.
 */
void MainWindow::on_Rotate_clicked()
{
    if (!ensureDocumentAvailable()) {
        return;
    }
    m_canvas->rotate();
}

/**
 * @brief Slot: Handles Upload button click - prompts to load SVG file.
 */
void MainWindow::on_Upload_clicked() { promptAndLoadSvg(); }

/**
 * @brief Slot: Handles Zoom button click - resets zoom to fit viewport.
 */
void MainWindow::on_Zoom_clicked()
{
    if (!ensureDocumentAvailable()) {
        return;
    }
    m_canvas->zoomReset();
    updateScaleLabel();
}

/**
 * @brief Slot: Handles Zoom Out button click - zooms out.
 */
void MainWindow::on_ZoomOut_clicked()
{
    if (!ensureDocumentAvailable()) {
        return;
    }
    m_canvas->zoomOut();
    updateScaleLabel();
}

/**
 * @brief Slot: Handles Zoom In button click - zooms in.
 */
void MainWindow::on_ZoomIn_clicked()
{
    if (!ensureDocumentAvailable()) {
        return;
    }
    m_canvas->zoomIn();
    updateScaleLabel();
}

void MainWindow::updateScaleLabel()
{
    if (m_canvas) {
        double scale = m_canvas->getScale();
        int percentage = static_cast<int>(scale * 100);
        ui->scaleLabel->setText(QString::number(percentage) + "%");
    }
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == ui->scaleLabel && event->type() == QEvent::MouseButtonDblClick) {
        bool ok;
        int currentPercentage = static_cast<int>(m_canvas->getScale() * 100);
        int newPercentage =
            QInputDialog::getInt(this, tr("Scale Input"), tr("Enter scale percentage (5-4000):"),
                                 currentPercentage, 5, 4000, 1, &ok);
        if (ok) {
            m_canvas->setScale(newPercentage / 100.0);
            updateScaleLabel();
        }
        return true;
    }
    return QMainWindow::eventFilter(obj, event);
}
