#include "SVGGroup.h"
#include <limits>
#include <algorithm>


SVGGroup::SVGGroup() {

}


void SVGGroup::addChild(std::unique_ptr<SVGElement> child) {

	m_children.push_back(std::move(child));
}


void SVGGroup::accept(NodeVisitor& visitor) {



	visitor.visitGroupBegin(*this);


	for (const auto& child : m_children) {
		if (child) {
			child->accept(visitor);
		}
	}


	visitor.visitGroupEnd(*this);
}



SVGRectF SVGGroup::localBox() const {




	if (m_children.empty()) {
		return { 0, 0, 0, 0 };
	}

	SVGNumber minX = std::numeric_limits<SVGNumber>::infinity();
	SVGNumber minY = std::numeric_limits<SVGNumber>::infinity();
	SVGNumber maxX = -std::numeric_limits<SVGNumber>::infinity();
	SVGNumber maxY = -std::numeric_limits<SVGNumber>::infinity();

	for (const auto& child : m_children) {
		if (!child) continue;




		SVGRectF childWorldBox = child->worldBox();


		if (childWorldBox.width >= 0 && childWorldBox.height >= 0) {

			minX = std::min(minX, childWorldBox.x);
			minY = std::min(minY, childWorldBox.y);
			maxX = std::max(maxX, childWorldBox.x + childWorldBox.width);
			maxY = std::max(maxY, childWorldBox.y + childWorldBox.height);
		}
	}


	if (minX == std::numeric_limits<SVGNumber>::infinity()) {
		return { 0, 0, 0, 0 };
	}


	return {
		minX,
		minY,
		maxX - minX,
		maxY - minY
	};
}

SVGRectF SVGGroup::worldBox() const {

	return SVGElement::worldBox();
}