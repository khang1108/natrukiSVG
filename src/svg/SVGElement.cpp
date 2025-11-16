#include "SVGElement.h"
#include <algorithm>





void SVGElement::setStyle(const SVGStyle& style) {
	m_style = style;
}

void SVGElement::setTransform(const SVGTransform& transform) {
	m_transform = transform;
}

SVGStyle& SVGElement::getStyle() {
	return m_style;
}

const SVGStyle& SVGElement::getStyle() const {
	return m_style;
}

SVGTransform& SVGElement::getTransform() {
	return m_transform;
}

const SVGTransform& SVGElement::getTransform() const {
	return m_transform;
}




SVGRectF SVGElement::worldBox() const {

	SVGRectF box = this->localBox();


	SVGPointF p1 = { box.x, box.y };
	SVGPointF p2 = { box.x + box.width, box.y };
	SVGPointF p3 = { box.x + box.width, box.y + box.height };
	SVGPointF p4 = { box.x, box.y + box.height };



	SVGPointF t_p1 = m_transform.map(p1);
	SVGPointF t_p2 = m_transform.map(p2);
	SVGPointF t_p3 = m_transform.map(p3);
	SVGPointF t_p4 = m_transform.map(p4);



	SVGNumber minX = std::min({ t_p1.x, t_p2.x, t_p3.x, t_p4.x });
	SVGNumber minY = std::min({ t_p1.y, t_p2.y, t_p3.y, t_p4.y });
	SVGNumber maxX = std::max({ t_p1.x, t_p2.x, t_p3.x, t_p4.x });
	SVGNumber maxY = std::max({ t_p1.y, t_p2.y, t_p3.y, t_p4.y });


	return { minX, minY, maxX - minX, maxY - minY };
}


