








#ifndef SkCanvas_DEFINED
#define SkCanvas_DEFINED

#include "SkTypes.h"
#include "SkBitmap.h"
#include "SkDeque.h"
#include "SkClipStack.h"
#include "SkPaint.h"
#include "SkRefCnt.h"
#include "SkPath.h"
#include "SkRegion.h"
#include "SkScalarCompare.h"
#include "SkXfermode.h"

class SkBounder;
class SkDevice;
class SkDraw;
class SkDrawFilter;
class SkPicture;
















class SK_API SkCanvas : public SkRefCnt {
public:
    SkCanvas();

    



    explicit SkCanvas(SkDevice* device);

    



    explicit SkCanvas(const SkBitmap& bitmap);
    virtual ~SkCanvas();

    

    



    SkDevice* getDevice() const;

    



    SkDevice* setDevice(SkDevice* device);

    






    SkDevice* getTopDevice() const;

    



    SkDevice* setBitmapDevice(const SkBitmap& bitmap);

    



    SkDevice* createCompatibleDevice(SkBitmap::Config config, 
                                    int width, int height,
                                    bool isOpaque);

    

    





    bool readPixels(const SkIRect& srcRect, SkBitmap* bitmap);
    bool readPixels(SkBitmap* bitmap);

    







    void writePixels(const SkBitmap& bitmap, int x, int y);

    

    enum SaveFlags {
        
        kMatrix_SaveFlag            = 0x01,
        
        kClip_SaveFlag              = 0x02,
        
        kHasAlphaLayer_SaveFlag     = 0x04,
        
        kFullColorLayer_SaveFlag    = 0x08,
        
        kClipToLayer_SaveFlag       = 0x10,

        
        kMatrixClip_SaveFlag        = 0x03,
        kARGB_NoClipLayer_SaveFlag  = 0x0F,
        kARGB_ClipLayer_SaveFlag    = 0x1F
    };

    







    virtual int save(SaveFlags flags = kMatrixClip_SaveFlag);

    












    virtual int saveLayer(const SkRect* bounds, const SkPaint* paint,
                          SaveFlags flags = kARGB_ClipLayer_SaveFlag);

    











    int saveLayerAlpha(const SkRect* bounds, U8CPU alpha,
                       SaveFlags flags = kARGB_ClipLayer_SaveFlag);

    




    virtual void restore();

    


    int getSaveCount() const;

    




    void restoreToCount(int saveCount);

    




    virtual bool translate(SkScalar dx, SkScalar dy);

    




    virtual bool scale(SkScalar sx, SkScalar sy);

    



    virtual bool rotate(SkScalar degrees);

    




    virtual bool skew(SkScalar sx, SkScalar sy);

    



    virtual bool concat(const SkMatrix& matrix);

    


    virtual void setMatrix(const SkMatrix& matrix);

    

    void resetMatrix();

    




    virtual bool clipRect(const SkRect& rect,
                          SkRegion::Op op = SkRegion::kIntersect_Op,
                          bool doAntiAlias = false);

    




    virtual bool clipPath(const SkPath& path,
                          SkRegion::Op op = SkRegion::kIntersect_Op,
                          bool doAntiAlias = false);

    







    virtual bool clipRegion(const SkRegion& deviceRgn,
                            SkRegion::Op op = SkRegion::kIntersect_Op);

    





    bool setClipRegion(const SkRegion& deviceRgn) {
        return this->clipRegion(deviceRgn, SkRegion::kReplace_Op);
    }

    





    enum EdgeType {
        


        kBW_EdgeType,
        


        kAA_EdgeType
    };

    








    bool quickReject(const SkRect& rect, EdgeType et) const;

    










    bool quickReject(const SkPath& path, EdgeType et) const;

    









    bool quickRejectY(SkScalar top, SkScalar bottom, EdgeType et) const;

    




    bool getClipBounds(SkRect* bounds, EdgeType et = kAA_EdgeType) const;

    



    bool getClipDeviceBounds(SkIRect* bounds) const;
       

    







    void drawARGB(U8CPU a, U8CPU r, U8CPU g, U8CPU b,
                  SkXfermode::Mode mode = SkXfermode::kSrcOver_Mode);

    




    void drawColor(SkColor color,
                   SkXfermode::Mode mode = SkXfermode::kSrcOver_Mode);

    













    virtual void clear(SkColor);

    




    virtual void drawPaint(const SkPaint& paint);

    enum PointMode {
        
        kPoints_PointMode,
        
        kLines_PointMode,
        
        kPolygon_PointMode
    };

    




















    virtual void drawPoints(PointMode mode, size_t count, const SkPoint pts[],
                            const SkPaint& paint);

    


    void drawPoint(SkScalar x, SkScalar y, const SkPaint& paint);

    




    void drawPoint(SkScalar x, SkScalar y, SkColor color);

    








    void drawLine(SkScalar x0, SkScalar y0, SkScalar x1, SkScalar y1,
                  const SkPaint& paint);

    




    virtual void drawRect(const SkRect& rect, const SkPaint& paint);

    




    void drawIRect(const SkIRect& rect, const SkPaint& paint)
    {
        SkRect r;
        r.set(rect);    
        this->drawRect(r, paint);
    }

    







    void drawRectCoords(SkScalar left, SkScalar top, SkScalar right,
                        SkScalar bottom, const SkPaint& paint);

    




    void drawOval(const SkRect& oval, const SkPaint&);

    







    void drawCircle(SkScalar cx, SkScalar cy, SkScalar radius,
                    const SkPaint& paint);

    










    void drawArc(const SkRect& oval, SkScalar startAngle, SkScalar sweepAngle,
                 bool useCenter, const SkPaint& paint);

    






    void drawRoundRect(const SkRect& rect, SkScalar rx, SkScalar ry,
                       const SkPaint& paint);

    




    virtual void drawPath(const SkPath& path, const SkPaint& paint);

    










    virtual void drawBitmap(const SkBitmap& bitmap, SkScalar left, SkScalar top,
                            const SkPaint* paint = NULL);

    







    virtual void drawBitmapRect(const SkBitmap& bitmap, const SkIRect* src,
                                const SkRect& dst, const SkPaint* paint = NULL);

    virtual void drawBitmapMatrix(const SkBitmap& bitmap, const SkMatrix& m,
                                  const SkPaint* paint = NULL);

    













    virtual void drawBitmapNine(const SkBitmap& bitmap, const SkIRect& center,
                                const SkRect& dst, const SkPaint* paint = NULL);

    










    virtual void drawSprite(const SkBitmap& bitmap, int left, int top,
                            const SkPaint* paint = NULL);

    







    virtual void drawText(const void* text, size_t byteLength, SkScalar x,
                          SkScalar y, const SkPaint& paint);

    






    virtual void drawPosText(const void* text, size_t byteLength,
                             const SkPoint pos[], const SkPaint& paint);

    








    virtual void drawPosTextH(const void* text, size_t byteLength,
                              const SkScalar xpos[], SkScalar constY,
                              const SkPaint& paint);

    











    void drawTextOnPathHV(const void* text, size_t byteLength,
                          const SkPath& path, SkScalar hOffset,
                          SkScalar vOffset, const SkPaint& paint);

    









    virtual void drawTextOnPath(const void* text, size_t byteLength,
                                const SkPath& path, const SkMatrix* matrix,
                                const SkPaint& paint);

#ifdef ANDROID
    








    void drawPosTextOnPath(const void* text, size_t byteLength,
                           const SkPoint pos[], const SkPaint& paint,
                           const SkPath& path, const SkMatrix* matrix);
#endif

    








    virtual void drawPicture(SkPicture& picture);

    enum VertexMode {
        kTriangles_VertexMode,
        kTriangleStrip_VertexMode,
        kTriangleFan_VertexMode
    };

    

















    virtual void drawVertices(VertexMode vmode, int vertexCount,
                              const SkPoint vertices[], const SkPoint texs[],
                              const SkColor colors[], SkXfermode* xmode,
                              const uint16_t indices[], int indexCount,
                              const SkPaint& paint);

    





    virtual void drawData(const void* data, size_t length);

    

    



    SkBounder*  getBounder() const { return fBounder; }

    







    virtual SkBounder* setBounder(SkBounder* bounder);

    



    SkDrawFilter* getDrawFilter() const;

    







    virtual SkDrawFilter* setDrawFilter(SkDrawFilter* filter);

    

    



    const SkMatrix& getTotalMatrix() const;

    enum ClipType {
        kEmpty_ClipType = 0,
        kRect_ClipType,
        kComplex_ClipType
    };

    


    ClipType getClipType() const;

    



    const SkRegion& getTotalClip() const;

    





    bool getTotalClipBounds(SkIRect* bounds) const;

    



    const SkClipStack& getTotalClipStack() const;

    void setExternalMatrix(const SkMatrix* = NULL);

    

    





    class SK_API LayerIter  {
    public:
        
        LayerIter(SkCanvas*, bool skipEmptyClips);
        ~LayerIter();

        
        bool done() const { return fDone; }
        
        void next();

        

        SkDevice*       device() const;
        const SkMatrix& matrix() const;
        const SkRegion& clip() const;
        const SkPaint&  paint() const;
        int             x() const;
        int             y() const;

    private:
        
        
        
        
        
        intptr_t          fStorage[32];
        class SkDrawIter* fImpl;    
        SkPaint           fDefaultPaint;
        bool              fDone;
    };

protected:
    
    virtual void commonDrawBitmap(const SkBitmap&, const SkIRect*,
                                  const SkMatrix&, const SkPaint& paint);

private:
    class MCRec;

    SkClipStack fClipStack;
    SkDeque     fMCStack;
    
    MCRec*      fMCRec;
    
    uint32_t    fMCRecStorage[32];

    SkBounder*  fBounder;
    SkDevice*   fLastDeviceToGainFocus;

    void prepareForDeviceDraw(SkDevice*, const SkMatrix&, const SkRegion&,
                              const SkClipStack& clipStack);

    bool fDeviceCMDirty;            
    void updateDeviceCMCache();

    friend class SkDrawIter;    

    SkDevice* createLayerDevice(SkBitmap::Config, int width, int height, 
                                bool isOpaque);

    SkDevice* init(SkDevice*);

    
    
    void internalDrawBitmap(const SkBitmap&, const SkIRect*, const SkMatrix& m,
                                  const SkPaint* paint);
    void internalDrawBitmapRect(const SkBitmap& bitmap, const SkIRect* src,
                                const SkRect& dst, const SkPaint* paint);
    void internalDrawBitmapNine(const SkBitmap& bitmap, const SkIRect& center,
                                const SkRect& dst, const SkPaint* paint);
    void internalDrawPaint(const SkPaint& paint);

        
    void drawDevice(SkDevice*, int x, int y, const SkPaint*);
    
    int internalSave(SaveFlags flags);
    void internalRestore();
    static void DrawRect(const SkDraw& draw, const SkPaint& paint,
                         const SkRect& r, SkScalar textSize);
    static void DrawTextDecorations(const SkDraw& draw, const SkPaint& paint,
                                    const char text[], size_t byteLength,
                                    SkScalar x, SkScalar y);

    


    mutable SkRectCompareType fLocalBoundsCompareType;
    mutable bool              fLocalBoundsCompareTypeDirty;

    mutable SkRectCompareType fLocalBoundsCompareTypeBW;
    mutable bool              fLocalBoundsCompareTypeDirtyBW;

    

    const SkRectCompareType& getLocalClipBoundsCompareType() const {
        return getLocalClipBoundsCompareType(kAA_EdgeType);
    }

    const SkRectCompareType& getLocalClipBoundsCompareType(EdgeType et) const {
        if (et == kAA_EdgeType) {
            if (fLocalBoundsCompareTypeDirty) {
                this->computeLocalClipBoundsCompareType(et);
                fLocalBoundsCompareTypeDirty = false;
            }
            return fLocalBoundsCompareType;
        } else {
            if (fLocalBoundsCompareTypeDirtyBW) {
                this->computeLocalClipBoundsCompareType(et);
                fLocalBoundsCompareTypeDirtyBW = false;
            }
            return fLocalBoundsCompareTypeBW;
        }
    }
    void computeLocalClipBoundsCompareType(EdgeType et) const;

    SkMatrix    fExternalMatrix, fExternalInverse;
    bool        fUseExternalMatrix;

    class AutoValidateClip : ::SkNoncopyable {
    public:
        explicit AutoValidateClip(SkCanvas* canvas) : fCanvas(canvas) {
            fCanvas->validateClip();
        }
        ~AutoValidateClip() { fCanvas->validateClip(); }

    private:
        const SkCanvas* fCanvas;
    };

#ifdef SK_DEBUG
    void validateClip() const;
#else
    void validateClip() const {}
#endif
};





class SkAutoCanvasRestore : SkNoncopyable {
public:
    SkAutoCanvasRestore(SkCanvas* canvas, bool doSave) : fCanvas(canvas) {
        SkASSERT(canvas);
        fSaveCount = canvas->getSaveCount();
        if (doSave) {
            canvas->save();
        }
    }
    ~SkAutoCanvasRestore() {
        fCanvas->restoreToCount(fSaveCount);
    }

private:
    SkCanvas*   fCanvas;
    int         fSaveCount;
};

#endif
