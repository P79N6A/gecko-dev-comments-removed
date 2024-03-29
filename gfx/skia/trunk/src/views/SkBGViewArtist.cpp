






#include "SkBGViewArtist.h"
#include "SkCanvas.h"
#include "SkParsePaint.h"

SkBGViewArtist::SkBGViewArtist(SkColor c)
{
    fPaint.setColor(c);
}

SkBGViewArtist::~SkBGViewArtist()
{
}

void SkBGViewArtist::onDraw(SkView*, SkCanvas* canvas)
{
    
    canvas->drawPaint(fPaint);
}

void SkBGViewArtist::onInflate(const SkDOM& dom, const SkDOM::Node* node)
{
    SkPaint_Inflate(&fPaint, dom, node);
}
