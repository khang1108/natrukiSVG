#ifndef SVG_PATH_H
#define SVG_PATH_H

#include "SVGElement.h"
#include "Types.h" // Chứa định nghĩa SVGPointF, SVGNumber

#include <string>
#include <vector>

struct PathCommand
{
    char type;                   // M, L, C, Z, ...
    std::vector<SVGNumber> args; // Danh sách tham số đi kèm
};

class SVGPath : public SVGElement
{
  public:
    SVGPath(const std::string& dData);

    void accept(NodeVisitor& visitor) override;
    SVGRectF localBox() const override;

    // Getter để Renderer lấy dữ liệu
    const std::vector<PathCommand>& getCommands() const;

  private:
    void parseD(const std::string& d); // Hàm tách chuỗi d="..."

    std::vector<PathCommand> m_commands;
    SVGRectF m_cachedBBox; // Lưu cache bbox để không phải tính lại nhiều lần
};

#endif
