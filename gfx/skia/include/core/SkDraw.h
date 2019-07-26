








#ifndef SkDraw_DEFINED
#define SkDraw_DEFINED

#include "SkBitmap.h"
#include "SkCanvas.h"
#include "SkMask.h"
#include "SkMatrix.h"
#include "SkPaint.h"
#include "SkRect.h"

class SkBounder;
class SkClipStack;
class SkDevice;
class SkPath;
class SkRegion;
class SkRasterClip;
struct SkDrawProcs;

class SkDraw {
public:
    SkDraw();
    SkDraw(const SkDraw& src);

    void    drawPaint(const SkPaint&) const;
    void    drawPoints(SkCanvas::PointMode, size_t count, const SkPoint[],
                       const SkPaint&, bool forceUseDevice = false) const;
    void    drawRect(const SkRect&, const SkPaint&) const;
    








    void    drawPath(const SkPath& srcPath, const SkPaint&,
                     const SkMatrix* prePathMatrix, bool pathIsMutable) const;
    void    drawBitmap(const SkBitmap&, const SkMatrix&, const SkPaint&) const;
    void    drawSprite(const SkBitmap&, int x, int y, const SkPaint&) const;
    void    drawText(const char text[], size_t byteLength, SkScalar x,
                     SkScalar y, const SkPaint& paint) const;
    void    drawPosText(const char text[], size_t byteLength,
                        const SkScalar pos[], SkScalar constY,
                        int scalarsPerPosition, const SkPaint& paint) const;
    void    drawTextOnPath(const char text[], size_t byteLength,
                        const SkPath&, const SkMatrix*, const SkPaint&) const;
#ifdef SK_BUILD_FOR_ANDROID
    void    drawPosTextOnPath(const char text[], size_t byteLength,
                              const SkPoint pos[], const SkPaint& paint,
                              const SkPath& path, const SkMatrix* matrix) const;
#endif
    void    drawVertices(SkCanvas::VertexMode mode, int count,
                         const SkPoint vertices[], const SkPoint textures[],
                         const SkColor colors[], SkXfermode* xmode,
                         const uint16_t indices[], int ptCount,
                         const SkPaint& paint) const;

    void drawPath(const SkPath& src, const SkPaint& paint) const {
        this->drawPath(src, paint, NULL, false);
    }

    




    static bool DrawToMask(const SkPath& devPath, const SkIRect* clipBounds,
                           const SkMaskFilter*, const SkMatrix* filterMatrix,
                           SkMask* mask, SkMask::CreateMode mode,
                           SkPaint::Style style);

    enum RectType {
        kHair_RectType,
        kFill_RectType,
        kStroke_RectType,
        kPath_RectType
    };

    







    static RectType ComputeRectType(const SkPaint&, const SkMatrix&,
                                    SkPoint* strokeSize);

private:
    void    drawText_asPaths(const char text[], size_t byteLength,
                             SkScalar x, SkScalar y, const SkPaint&) const;
    void    drawDevMask(const SkMask& mask, const SkPaint&) const;
    void    drawBitmapAsMask(const SkBitmap&, const SkPaint&) const;

    







    bool SK_WARN_UNUSED_RESULT
    computeConservativeLocalClipBounds(SkRect* bounds) const;

public:
    const SkBitmap* fBitmap;        
    const SkMatrix* fMatrix;        
    const SkRegion* fClip;          
    const SkRasterClip* fRC;        

    const SkClipStack* fClipStack;  
    SkDevice*       fDevice;        
    SkBounder*      fBounder;       
    SkDrawProcs*    fProcs;         

#ifdef SK_DEBUG
    void validate() const;
#else
    void validate() const {}
#endif
};

#endif
