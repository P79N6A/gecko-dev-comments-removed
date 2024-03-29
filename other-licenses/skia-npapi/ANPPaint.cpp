

























#include "SkANP.h"
#include "SkTypeface.h"

static ANPPaint* anp_newPaint() {
    return new ANPPaint;
}

static void anp_deletePaint(ANPPaint* paint) {
    delete paint;
}

static ANPPaintFlags anp_getFlags(const ANPPaint* paint) {
    return paint->getFlags();
}

static void anp_setFlags(ANPPaint* paint, ANPPaintFlags flags) {
    paint->setFlags(flags);
}

static ANPColor anp_getColor(const ANPPaint* paint) {
    return paint->getColor();
}

static void anp_setColor(ANPPaint* paint, ANPColor color) {
    paint->setColor(color);
}

static ANPPaintStyle anp_getStyle(const ANPPaint* paint) {
    return paint->getStyle();
}

static void anp_setStyle(ANPPaint* paint, ANPPaintStyle style) {
    paint->setStyle(static_cast<SkPaint::Style>(style));
}

static float anp_getStrokeWidth(const ANPPaint* paint) {
    return SkScalarToFloat(paint->getStrokeWidth());
}

static float anp_getStrokeMiter(const ANPPaint* paint) {
    return SkScalarToFloat(paint->getStrokeMiter());
}

static ANPPaintCap anp_getStrokeCap(const ANPPaint* paint) {
    return paint->getStrokeCap();
}

static ANPPaintJoin anp_getStrokeJoin(const ANPPaint* paint) {
    return paint->getStrokeJoin();
}

static void anp_setStrokeWidth(ANPPaint* paint, float width) {
    paint->setStrokeWidth(SkFloatToScalar(width));
}

static void anp_setStrokeMiter(ANPPaint* paint, float miter) {
    paint->setStrokeMiter(SkFloatToScalar(miter));
}

static void anp_setStrokeCap(ANPPaint* paint, ANPPaintCap cap) {
    paint->setStrokeCap(static_cast<SkPaint::Cap>(cap));
}

static void anp_setStrokeJoin(ANPPaint* paint, ANPPaintJoin join) {
    paint->setStrokeJoin(static_cast<SkPaint::Join>(join));
}

static ANPTextEncoding anp_getTextEncoding(const ANPPaint* paint) {
    return paint->getTextEncoding();
}

static ANPPaintAlign anp_getTextAlign(const ANPPaint* paint) {
    return paint->getTextAlign();
}

static float anp_getTextSize(const ANPPaint* paint) {
    return SkScalarToFloat(paint->getTextSize());
}

static float anp_getTextScaleX(const ANPPaint* paint) {
    return SkScalarToFloat(paint->getTextScaleX());
}

static float anp_getTextSkewX(const ANPPaint* paint) {
    return SkScalarToFloat(paint->getTextSkewX());
}

static ANPTypeface* anp_getTypeface(const ANPPaint* paint) {
    return reinterpret_cast<ANPTypeface*>(paint->getTypeface());
}

static void anp_setTextEncoding(ANPPaint* paint, ANPTextEncoding encoding) {
    paint->setTextEncoding(static_cast<SkPaint::TextEncoding>(encoding));
}

static void anp_setTextAlign(ANPPaint* paint, ANPPaintAlign align) {
    paint->setTextAlign(static_cast<SkPaint::Align>(align));
}

static void anp_setTextSize(ANPPaint* paint, float textSize) {
    paint->setTextSize(SkFloatToScalar(textSize));
}

static void anp_setTextScaleX(ANPPaint* paint, float scaleX) {
    paint->setTextScaleX(SkFloatToScalar(scaleX));
}

static void anp_setTextSkewX(ANPPaint* paint, float skewX) {
    paint->setTextSkewX(SkFloatToScalar(skewX));
}

static void anp_setTypeface(ANPPaint* paint, ANPTypeface* tf) {
    paint->setTypeface(tf);
}

static float anp_measureText(ANPPaint* paint, const void* text,
                             uint32_t byteLength, ANPRectF* bounds) {
    SkScalar w = paint->measureText(text, byteLength,
                                    reinterpret_cast<SkRect*>(bounds));
    return SkScalarToFloat(w);
}






static int anp_getTextWidths(ANPPaint* paint, const void* text,
                       uint32_t byteLength, float widths[], ANPRectF bounds[]) {
    return paint->getTextWidths(text, byteLength, widths,
                                reinterpret_cast<SkRect*>(bounds));
}

static float anp_getFontMetrics(ANPPaint* paint, ANPFontMetrics* metrics) {
    SkPaint::FontMetrics fm;
    SkScalar spacing = paint->getFontMetrics(&fm);
    if (metrics) {
        metrics->fTop = SkScalarToFloat(fm.fTop);
        metrics->fAscent = SkScalarToFloat(fm.fAscent);
        metrics->fDescent = SkScalarToFloat(fm.fDescent);
        metrics->fBottom = SkScalarToFloat(fm.fBottom);
        metrics->fLeading = SkScalarToFloat(fm.fLeading);
    }
    return SkScalarToFloat(spacing);
}



#define ASSIGN(obj, name)   (obj)->name = anp_##name

void InitPaintInterface(ANPPaintInterfaceV0* i) {
    ASSIGN(i, newPaint);
    ASSIGN(i, deletePaint);
    ASSIGN(i, getFlags);
    ASSIGN(i, setFlags);
    ASSIGN(i, getColor);
    ASSIGN(i, setColor);
    ASSIGN(i, getStyle);
    ASSIGN(i, setStyle);
    ASSIGN(i, getStrokeWidth);
    ASSIGN(i, getStrokeMiter);
    ASSIGN(i, getStrokeCap);
    ASSIGN(i, getStrokeJoin);
    ASSIGN(i, setStrokeWidth);
    ASSIGN(i, setStrokeMiter);
    ASSIGN(i, setStrokeCap);
    ASSIGN(i, setStrokeJoin);
    ASSIGN(i, getTextEncoding);
    ASSIGN(i, getTextAlign);
    ASSIGN(i, getTextSize);
    ASSIGN(i, getTextScaleX);
    ASSIGN(i, getTextSkewX);
    ASSIGN(i, getTypeface);
    ASSIGN(i, setTextEncoding);
    ASSIGN(i, setTextAlign);
    ASSIGN(i, setTextSize);
    ASSIGN(i, setTextScaleX);
    ASSIGN(i, setTextSkewX);
    ASSIGN(i, setTypeface);
    ASSIGN(i, measureText);
    ASSIGN(i, getTextWidths);
    ASSIGN(i, getFontMetrics);
}
