#include "ui/CanvasWidget.h"
#include "svg/SVGDocument.h"
#include <QPainter>
#include <QPaintEvent>
#include <QDebug>
#include <utility>
#include <QWidget>
#include "ui/QtRenderer.h"

// Constructor
CanvasWidget::CanvasWidget(QWidget *parent)
    : QWidget(parent), m_document(std::make_unique<SVGDocument>())
{
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
}

// Destructor
CanvasWidget::~CanvasWidget() = default;

// Setter để Mainwindow nạp tài liệu SVG
void CanvasWidget::setDocument(std::unique_ptr<SVGDocument> document)
{
    m_document = std::move(document);
    update(); // Yêu cầu vẽ lại widget
}

// Hàm vẽ chính
void CanvasWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Thiết lập nền trắng
    painter.fillRect(rect(), Qt::white);

    if (!m_document)
        return;

    // Tính toán các phép biến đổi
    painter.translate(width() / 2 + m_panOffset.x(), height() / 2 + m_panOffset.y());
    painter.scale(m_scale, m_scale);
    painter.rotate(m_rotation);
    if (m_isFlipped)
    {
        painter.scale(1, -1); // Lật dọc
    }
    painter.translate(-width() / 2, -height() / 2);

    // Vẽ tài liệu SVG
    QtRenderer renderer(&painter);
    m_document->draw(renderer);
}

void CanvasWidget::zoomIn()
{
    m_scale *= 1.2;
    update(); // Yêu cầu vẽ lại widget
}
void CanvasWidget::zoomOut()
{
    m_scale /= 1.2;
    update(); // Yêu cầu vẽ lại widget
}
void CanvasWidget::zoomReset()
{
    m_scale = 1.0;
    m_panOffset = QPointF(0, 0);
    m_rotation = 0;
    m_isFlipped = false;
    update(); // Yêu cầu vẽ lại widget
}
void CanvasWidget::rotate()
{
    m_rotation += 90;
    update(); // Yêu cầu vẽ lại widget
}
void CanvasWidget::flip()
{
    m_isFlipped = !m_isFlipped;
    update(); // Yêu cầu vẽ lại widget
}