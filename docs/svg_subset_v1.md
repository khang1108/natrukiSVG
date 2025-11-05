# SVG Subset v1.0

Tài liệu này định nghĩa các tính năng SVG tối thiểu (MVP - Minimum Viable Product) mà dự án NaTruKiSVG sẽ hỗ trợ.

---

## 1. Các Phần tử (Elements)

Dự án sẽ hỗ trợ phân tích (parsing) và hiển thị (rendering) 6 loại shape cơ bản sau:

* `<rect>` (Hình chữ nhật)
* `<circle>` (Hình tròn)
* `<ellipse>` (Hình elip)
* `<line>` (Đường thẳng)
* `<polyline>` (Đa tuyến)
* `<polygon>` (Đa giác)

---

## 2. Các Thuộc tính Style

Các thuộc tính style cơ bản sau sẽ được hỗ trợ và có thể kế thừa (cascading) từ các thẻ cha (như `<g>`):

* `fill`
* `stroke`
* `stroke-width`
* `opacity` (bao gồm cả `fill-opacity` và `stroke-opacity`)
* `fill-rule`

---

## 3. Các Phép biến đổi (Transform)

Các phép biến đổi 2D sau sẽ được hỗ trợ thông qua thuộc tính `transform` và sẽ được tính toán bằng cách nhân ma trận (transform stack):

* `translate` (Tịnh tiến)
* `scale` (Co giãn - bao gồm cả lật (flip) khi dùng giá trị âm)
* `rotate` (Xoay)