// File: /include/ui/MainWindow.h

#ifndef UI_MAIN_WINDOW_H
#define UI_MAIN_WINDOW_H

#include <QMainWindow> // Kế thừa từ QMainWindow của Qt6

// --- Khai báo trước (Forward Declarations) ---
class CanvasWidget; // Cửa sổ chính "sở hữu" một CanvasWidget
class QAction;      // Dùng cho các mục menu/nút bấm

/**
 * @brief Lớp cửa sổ chính của ứng dụng.
 *
 * Chứa các thanh công cụ và 'CanvasWidget' dựa trên
 * thiết kế Figma.
 */
class MainWindow : public QMainWindow {
    Q_OBJECT // Macro bắt buộc cho các lớp Qt có signal/slot

private:
    /**
     * @brief Con trỏ đến widget trung tâm, nơi vẽ SVG.
     *
     */
    CanvasWidget* m_canvas;

    // --- CÁC ACTIONS CHO NÚT BẤM ---
    // (Role C sẽ tạo và kết nối chúng)
    QAction* m_uploadAction;    // Cho nút "Browse" hoặc "Mở file"
    QAction* m_aboutAction;   // Cho nút "About"
    QAction* m_premiumAction; // Cho nút "Premium"
    
    QAction* m_zoomInAction;  // Cho nút "+"
    QAction* m_zoomOutAction; // Cho nút "-"
    QAction* m_zoomResetAction; // Cho nút "100%"
    QAction* m_rotateAction;    // Cho nút "Rotate"
    QAction* m_flipHAction;    // Cho nút "Flip"
    QAction* m_copyAction;    // Cho nút "Copy PNG"
    QAction* m_downloadAction;   // Cho nút "Download PNG"
    QAction* m_fullscreenAction; // Cho nút "Fullscreen"
    QAction* m_exitFullscreenAction; // Cho nút "Exit Fullscreen"
    QAction* m_backtoviewerAction; // CHo nút "Back" ở 2 chỗ (trang About & Premium)
public:
    /**
     * @brief Hàm khởi tạo (Constructor).
     *
     * * - Vai trò của Role C (Implement):
     * Viết logic trong 'MainWindow.cpp'. Sẽ tạo 'm_canvas',
     * tạo các thanh công cụ (Toolbars) theo thiết kế Figma,
     * và gọi 'createActions()'.
     */
    MainWindow(QWidget* parent = nullptr);

    /**
     * @brief Hàm hủy (Destructor).
     *
     * * - Vai trò của Role C (Implement):
     * Viết logic trong 'MainWindow.cpp'.
     */
    virtual ~MainWindow();

private:
    /**
     * @brief (Hàm helper) Tạo các đối tượng QAction.
     *
     * * - Vai trò của Role C (Implement):
     * Viết logic trong 'MainWindow.cpp'.
     */
    void createActions();
    
    /**
     * @brief (Hàm helper) Tạo và sắp xếp các ToolBars
     * (thanh trên và thanh dưới) theo thiết kế Figma.
     *
     * * - Vai trò của Role C (Implement):
     * Viết logic trong 'MainWindow.cpp'.
     */
    void createToolBars();

// --- KHAI BÁO CÁC CHỨC NĂNG (SLOTS) ---
private slots:
    /**
     * @brief Được gọi khi nhấp vào "Browse" / "Upload".
     * * - Role C (Implement):
     * Sẽ mở hộp thoại chọn file và gọi logic của Role B (Parser)
     * để nạp file vào 'm_canvas'.
     */
    void onUpload();

    /**
     * @brief Được gọi khi nhấp vào "About".
     * * - Role C (Implement):
     * Sẽ hiển thị trang About.
     */
    void onAbout();

    /**
     * @brief Được gọi khi nhấp vào "Premium".
     * * - Role C (Implement):
     * Sẽ hiển thị trang khều donate.
     */
    void onPremium();

    /**
     * @brief Được gọi khi nhấp vào "+".
     * * - Role C (Implement):
     * Sẽ gọi một hàm trong 'm_canvas' để zoom to
     * (thay đổi 'm_scale').
     */
    void onZoomIn();

    /**
     * @brief Được gọi khi nhấp vào "-".
     * * - Role C (Implement):
     * Sẽ gọi một hàm trong 'm_canvas' để thu nhỏ.
     *
     */
    void onZoomOut();

    /**
     * @brief Được gọi khi nhấp vào "100%".
     * * - Vai trò của Role C (Implement):
     * Sẽ gọi một hàm trong 'm_canvas' để reset 'm_scale' = 1.0.
     *
     */
    void onZoomReset();

    /**
     * @brief Được gọi khi nhấp vào "Fullscreen".
     *
     * * - Vai trò của Role C (Implement):
     * Sẽ gọi hàm 'showFullScreen()' hoặc 'showNormal()'
     * của 'MainWindow' để bật/tắt chế độ toàn màn hình.
     */
    void onFullscreen();

    /**
     * @brief Được gọi khi nhấp vào "Rotate".
     * * - Role C (Implement):
     * Sẽ gọi một hàm trong 'm_canvas' để xoay.
     */
    void onRotate();

    /**
     * @brief Được gọi khi nhấp vào "Flip".
     * * - Role C (Implement):
     * Sẽ gọi một hàm trong 'm_canvas' để lật hình.
     */
    void onFlip();

    /**
     * @brief Được gọi khi nhấp vào "Copy PNG".
     * * - Role C (Implement):
     * Sẽ render 'm_canvas' ra một QPixmap và copy vào clipboard.
     */
    void onCopy();

    /**
     * @brief Được gọi khi nhấp vào "Download PNG".
     * * - Role C (Implement):
     * Sẽ render 'm_canvas' ra QPixmap và mở hộp thoại lưu file.
     */
    void onDownload();

    /**
     * @brief Được gọi khi nhấp vào "Back" (trang About & Premium).
     * * - Vai trò của Role C (Implement):
     * Sẽ quay về trang chính (canvas viewer).
     */
    void onBack();

    /**
     * @brief Được gọi khi nhấp vào "Exit Fullscreen = ESC".
     * * - Vai trò của Role C (Implement):
     * Sẽ thoát chế độ toàn màn hình.
     */
    void onExitFullscreen();
};

#endif // UI_MAIN_WINDOW_H