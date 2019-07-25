








#include "SkTextToPath.h"
#include "SkAnimateMaker.h"
#include "SkDrawPaint.h"
#include "SkDrawPath.h"
#include "SkDrawText.h"
#include "SkPaint.h"

#if SK_USE_CONDENSED_INFO == 0

const SkMemberInfo SkTextToPath::fInfo[] = {
    SK_MEMBER(paint, Paint),
    SK_MEMBER(path, Path),
    SK_MEMBER(text, Text)
};

#endif

DEFINE_GET_MEMBER(SkTextToPath);

SkTextToPath::SkTextToPath() : paint(NULL), path(NULL), text(NULL) {
}

bool SkTextToPath::draw(SkAnimateMaker& maker) {
    path->draw(maker);
    return false;
}

void SkTextToPath::onEndElement(SkAnimateMaker& maker) {
    if (paint == NULL || path == NULL || text == NULL) {
        
        maker.setErrorCode(SkDisplayXMLParserError::kErrorInAttributeValue);
        return;
    }
    SkPaint realPaint;
    paint->setupPaint(&realPaint);
    realPaint.getTextPath(text->getText(), text->getSize(), text->x, 
        text->y, &path->getPath());
}

