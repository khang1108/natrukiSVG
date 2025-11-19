#ifndef SVG_PATH_H
#define SVG_PATH_H

#include "SVGElement.h"
#include "Types.h"

#include <string>

/**
 * @brief Đại diện cho phần tử <path>.
 */
class SVGPath : public SVGElement
{
  private:
    std::string m_pathData;
    SVGRectF m_localBounds;

    void computeLocalBounds();
    static void skipSeparators(const std::string& data, size_t& index);
    static bool readNumber(const std::string& data, size_t& index, SVGNumber& value);

  public:
    explicit SVGPath(const std::string& pathData);

    void accept(NodeVisitor& visitor) override { visitor.visit(*this); }

    SVGRectF localBox() const override;
    SVGRectF worldBox() const override;

    const std::string& getPath() const { return m_pathData; }
};

#endif // SVG_PATH_H