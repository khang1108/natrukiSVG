// File: /include/ui/CanvasWidget.h

#ifndef UI_CANVAS_WIDGET_H
#define UI_CANVAS_WIDGET_H

#include <QWidget>
#include <memory>
#include <QPointF>

// --- Khai báo trước (Forward Declarations) ---
class SVGDocument;
class QPaintEvent;

/**
 * @brief Widget tùy chỉnh (custom widget) dùng để hiển thị (render)
 * nội dung của một 'SVGDocument'.
 *
 * Nó cũng quản lý trạng thái camera (Pan, Zoom, Rotate, Flip).
 *
 */
class CanvasWidget : public QWidget {
    Q_OBJECT // Macro bắt buộc

private:
    /**
     * @brief Con trỏ thông minh "sở hữu" tài liệu SVG.
     */
    std::unique_ptr<SVGDocument> m_document;

    // --- CÁC BIẾN LƯU TRẠNG THÁI CAMERA (VIEW) ---
    // (Role C
    // sẽ đọc các biến này trong 'paintEvent'
    // để thiết lập QPainter)

    /**
     * @brief Mức độ phóng to (Scale). Mặc định là 1.0.
     */
    double m_scale = 1.0;

    /**
     * @brief Vị trí dịch chuyển (Pan).
     */
    QPointF m_panOffset;

    /**
     * @brief Góc xoay (tính bằng độ).
     */
    double m_rotation = 0.0;

    /**
     * @brief Cờ (flag) cho biết hình có bị lật (Flip) hay không.
     */
    bool m_isFlipped = false;

public:
    /**
     * @brief Hàm khởi tạo (Constructor).
     *
     * * - Vai trò của Role C (Implement):
     * Viết logic trong 'CanvasWidget.cpp'.
     */
    explicit CanvasWidget(QWidget* parent = nullptr);

    /**
     * @brief Hàm hủy (Destructor).
     *
     * * - Vai trò của Role C (Implement):
     * Viết logic trong 'CanvasWidget.cpp'.
     */
    virtual ~CanvasWidget();

    /**
     * @brief Hàm (setter) để 'MainWindow' nạp tài liệu SVG vào đây.
     * @param document Con trỏ thông minh đến tài liệu đã được parse.
     */
    void setDocument(std::unique_ptr<SVGDocument> document);

// --- CÁC HÀM (SLOTS) CÔNG KHAI ĐỂ MAINWINDOW GỌI ---
public slots:
    /**
     * @brief Tăng mức độ phóng to.
     * * - Role C (Implement):
     * Sẽ tăng 'm_scale' (ví dụ: m_scale *= 1.2;)
     * và gọi 'update()' để vẽ lại.
     */
    void zoomIn();

    /**
     * @brief Giảm mức độ phóng to.
     * * - Role C (Implement):
     * Sẽ giảm 'm_scale' (ví dụ: m_scale /= 1.2;)
     * và gọi 'update()' để vẽ lại.
     */
    void zoomOut();

    /**
     * @brief Đặt lại 'm_scale' = 1.0 và các trạng thái khác.
     * * - Role C (Implement):
     * Sẽ reset 'm_scale', 'm_panOffset', 'm_rotation', ...
     * và gọi 'update()' để vẽ lại.
     */
    void zoomReset();

    /**
     * @brief Xoay hình (ví dụ: thêm 90 độ).
     * * - Role C (Implement):
     * Sẽ cập nhật 'm_rotation += 90;'
     * và gọi 'update()' để vẽ lại.
     */
    void rotate();

    /**
     * @brief Lật hình (ngang hoặc dọc).
     * * - Role C (Implement):
     * Sẽ thay đổi 'm_isFlipped = !m_isFlipped;'
     * và gọi 'update()' để vẽ lại.
     */
    void flip();

protected:
    /**
     * @brief Hàm vẽ chính.
     *
     * * - Vai trò của Role C (Implement):
     * Viết logic trong 'CanvasWidget.cpp'. Logic này sẽ:
     * 1. Tạo 'QPainter'.
     * 2. ĐỌC CÁC BIẾN ('m_scale', 'm_panOffset', 'm_rotation', 'm_isFlipped')
     * và dùng chúng để thiết lập 'QPainter' (ví dụ: painter.translate(...),
     * painter.scale(...), painter.rotate(...)).
     * 3. Tạo 'QtRenderer renderer(&painter);'.
     * 4. Gọi 'm_document->draw(renderer);'.
     */
    void paintEvent(QPaintEvent* event) override;

    // * - Role C (Implement):
    // Cũng sẽ implement các hàm sự kiện (mousePress, wheelEvent)
    // để cập nhật 'm_scale' và 'm_panOffset' cho Pan/Zoom.
    //
};

#endif // UI_CANVAS_WIDGET_H