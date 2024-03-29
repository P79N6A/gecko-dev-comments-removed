








#include "SkSVGLine.h"
#include "SkSVGParser.h"

const SkSVGAttribute SkSVGLine::gAttributes[] = {
    SVG_ATTRIBUTE(x1),
    SVG_ATTRIBUTE(x2),
    SVG_ATTRIBUTE(y1),
    SVG_ATTRIBUTE(y2)
};

DEFINE_SVG_INFO(Line)

void SkSVGLine::translate(SkSVGParser& parser, bool defState) {
    parser._startElement("line");
    INHERITED::translate(parser, defState);
    SVG_ADD_ATTRIBUTE(x1);
    SVG_ADD_ATTRIBUTE(y1);
    SVG_ADD_ATTRIBUTE(x2);
    SVG_ADD_ATTRIBUTE(y2);
    parser._endElement();
}
