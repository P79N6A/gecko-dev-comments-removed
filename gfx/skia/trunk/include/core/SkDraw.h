








#ifndef SkDraw_DEFINED
#define SkDraw_DEFINED

#include "SkCanvas.h"
#include "SkMask.h"
#include "SkPaint.h"

class SkBitmap;
class SkClipStack;
class SkBaseDevice;
class SkMatrix;
class SkPath;
class SkRegion;
class SkRasterClip;
struct SkDrawProcs;
struct SkRect;
class SkRRect;

class SkDraw {
public:
    SkDraw();
    SkDraw(const SkDraw& src);

    void    drawPaint(const SkPaint&) const;
    void    drawPoints(SkCanvas::PointMode, size_t count, const SkPoint[],
                       const SkPaint&, bool forceUseDevice = false) const;
    void    drawRect(const SkRect&, const SkPaint&) const;
    void    drawRRect(const SkRRect&, const SkPaint&) const;
    








    void    drawPath(const SkPath& path, const SkPaint& paint,
                     const SkMatrix* prePathMatrix, bool pathIsMutable) const {
        this->drawPath(path, paint, prePathMatrix, pathIsMutable, false);
    }

    void drawPath(const SkPath& path, const SkPaint& paint) const {
        this->drawPath(path, paint, NULL, false, false);
    }

    void    drawBitmap(const SkBitmap&, const SkMatrix&, const SkPaint&) const;
    void    drawSprite(const SkBitmap&, int x, int y, const SkPaint&) const;
    void    drawText(const char text[], size_t byteLength, SkScalar x,
                     SkScalar y, const SkPaint& paint) const;
    void    drawPosText(const char text[], size_t byteLength,
                        const SkScalar pos[], SkScalar constY,
                        int scalarsPerPosition, const SkPaint& paint) const;
    void    drawTextOnPath(const char text[], size_t byteLength,
                        const SkPath&, const SkMatrix*, const SkPaint&) const;
    void    drawVertices(SkCanvas::VertexMode mode, int count,
                         const SkPoint vertices[], const SkPoint textures[],
                         const SkColor colors[], SkXfermode* xmode,
                         const uint16_t indices[], int ptCount,
                         const SkPaint& paint) const;

    





    void drawPathCoverage(const SkPath& src, const SkPaint& paint) const {
        this->drawPath(src, paint, NULL, false, true);
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

    static bool ShouldDrawTextAsPaths(const SkPaint&, const SkMatrix&);
    void        drawText_asPaths(const char text[], size_t byteLength,
                                 SkScalar x, SkScalar y, const SkPaint&) const;
    void        drawPosText_asPaths(const char text[], size_t byteLength,
                                    const SkScalar pos[], SkScalar constY,
                                    int scalarsPerPosition, const SkPaint&) const;

private:
    void    drawDevMask(const SkMask& mask, const SkPaint&) const;
    void    drawBitmapAsMask(const SkBitmap&, const SkPaint&) const;

    void    drawPath(const SkPath&, const SkPaint&, const SkMatrix* preMatrix,
                     bool pathIsMutable, bool drawCoverage) const;

    







    bool SK_WARN_UNUSED_RESULT
    computeConservativeLocalClipBounds(SkRect* bounds) const;

public:
    const SkBitmap* fBitmap;        
    const SkMatrix* fMatrix;        
    const SkRegion* fClip;          
    const SkRasterClip* fRC;        

    const SkClipStack* fClipStack;  
    SkBaseDevice*   fDevice;        
    SkDrawProcs*    fProcs;         

#ifdef SK_DEBUG
    void validate() const;
#else
    void validate() const {}
#endif
};

#endif
