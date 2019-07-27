






#ifndef GrTextContext_DEFINED
#define GrTextContext_DEFINED

#include "GrGlyph.h"
#include "GrPaint.h"
#include "SkDeviceProperties.h"

#include "SkPostConfig.h"

class GrContext;
class GrDrawTarget;
class GrFontScaler;




class GrTextContext {
public:
    virtual ~GrTextContext() {}
    virtual void drawText(const GrPaint&, const SkPaint&, const char text[], size_t byteLength,
                          SkScalar x, SkScalar y) = 0;
    virtual void drawPosText(const GrPaint&, const SkPaint&,
                             const char text[], size_t byteLength,
                             const SkScalar pos[], SkScalar constY,
                             int scalarsPerPosition) = 0;

    virtual bool canDraw(const SkPaint& paint) = 0;

protected:
    GrTextContext(GrContext*, const SkDeviceProperties&);

    static GrFontScaler* GetGrFontScaler(SkGlyphCache* cache);
    static void MeasureText(SkGlyphCache* cache, SkDrawCacheProc glyphCacheProc,
                            const char text[], size_t byteLength, SkVector* stopVector);

    void init(const GrPaint&, const SkPaint&);
    void finish() { fDrawTarget = NULL; }

    GrContext*         fContext;
    SkDeviceProperties fDeviceProperties;

    GrDrawTarget*      fDrawTarget;
    SkIRect            fClipRect;
    GrPaint            fPaint;
    SkPaint            fSkPaint;
};

#endif
