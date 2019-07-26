






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
#include "SkXfermode.h"

class SkBounder;
class SkBaseDevice;
class SkDraw;
class SkDrawFilter;
class SkMetaData;
class SkPicture;
class SkRRect;
class SkSurface;
class SkSurface_Base;
class GrContext;
















class SK_API SkCanvas : public SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(SkCanvas)

    



    SkCanvas();

    




    SkCanvas(int width, int height);

    



    explicit SkCanvas(SkBaseDevice* device);

    



    explicit SkCanvas(const SkBitmap& bitmap);
    virtual ~SkCanvas();

    SkMetaData& getMetaData();

    

    


    void flush();

    




    SkISize getDeviceSize() const;

    



    SkBaseDevice* getDevice() const;

    












    SkBaseDevice* getTopDevice(bool updateMatrixClip = false) const;

    



    SkSurface* newSurface(const SkImageInfo&);

    



    GrContext* getGrContext();

    

    











    enum Config8888 {
        







        kNative_Premul_Config8888,
        kNative_Unpremul_Config8888,
        


        kBGRA_Premul_Config8888,
        kBGRA_Unpremul_Config8888,
        


        kRGBA_Premul_Config8888,
        kRGBA_Unpremul_Config8888
    };

    



































    bool readPixels(SkBitmap* bitmap,
                    int x, int y,
                    Config8888 config8888 = kNative_Premul_Config8888);

    





    bool readPixels(const SkIRect& srcRect, SkBitmap* bitmap);

    














    void writePixels(const SkBitmap& bitmap,
                     int x, int y,
                     Config8888 config8888 = kNative_Premul_Config8888);

    

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

    


    virtual bool isDrawingToLayer() const;

    




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

    






    virtual bool clipRRect(const SkRRect& rrect,
                           SkRegion::Op op = SkRegion::kIntersect_Op,
                           bool doAntiAlias = false);

    






    virtual bool clipPath(const SkPath& path,
                          SkRegion::Op op = SkRegion::kIntersect_Op,
                          bool doAntiAlias = false);

    



    void setAllowSoftClip(bool allow) {
        fAllowSoftClip = allow;
    }

    


    void setAllowSimplifyClip(bool allow) {
        fAllowSimplifyClip = allow;
    }

    







    virtual bool clipRegion(const SkRegion& deviceRgn,
                            SkRegion::Op op = SkRegion::kIntersect_Op);

    





    bool setClipRegion(const SkRegion& deviceRgn) {
        return this->clipRegion(deviceRgn, SkRegion::kReplace_Op);
    }

    







    bool quickReject(const SkRect& rect) const;

    









    bool quickReject(const SkPath& path) const;

    









    bool quickRejectY(SkScalar top, SkScalar bottom) const {
        SkASSERT(top <= bottom);
        const SkRect& clipR = this->getLocalClipBounds();
        
        
        
        
        return top >= clipR.fBottom || bottom <= clipR.fTop;
    }

    




    bool getClipBounds(SkRect* bounds) const;

    



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

    




    virtual void drawOval(const SkRect& oval, const SkPaint&);

    






    virtual void drawRRect(const SkRRect& rrect, const SkPaint& paint);

    







    void drawCircle(SkScalar cx, SkScalar cy, SkScalar radius,
                    const SkPaint& paint);

    










    void drawArc(const SkRect& oval, SkScalar startAngle, SkScalar sweepAngle,
                 bool useCenter, const SkPaint& paint);

    






    void drawRoundRect(const SkRect& rect, SkScalar rx, SkScalar ry,
                       const SkPaint& paint);

    




    virtual void drawPath(const SkPath& path, const SkPaint& paint);

    















    virtual void drawBitmap(const SkBitmap& bitmap, SkScalar left, SkScalar top,
                            const SkPaint* paint = NULL);

    enum DrawBitmapRectFlags {
        kNone_DrawBitmapRectFlag            = 0x0,
        




        kBleed_DrawBitmapRectFlag           = 0x1,
    };

    







    virtual void drawBitmapRectToRect(const SkBitmap& bitmap, const SkRect* src,
                                      const SkRect& dst,
                                      const SkPaint* paint = NULL,
                                      DrawBitmapRectFlags flags = kNone_DrawBitmapRectFlag);

    void drawBitmapRect(const SkBitmap& bitmap, const SkRect& dst,
                        const SkPaint* paint = NULL) {
        this->drawBitmapRectToRect(bitmap, NULL, dst, paint, kNone_DrawBitmapRectFlag);
    }

    void drawBitmapRect(const SkBitmap& bitmap, const SkIRect* isrc,
                        const SkRect& dst, const SkPaint* paint = NULL,
                        DrawBitmapRectFlags flags = kNone_DrawBitmapRectFlag) {
        SkRect realSrcStorage;
        SkRect* realSrcPtr = NULL;
        if (isrc) {
            realSrcStorage.set(*isrc);
            realSrcPtr = &realSrcStorage;
        }
        this->drawBitmapRectToRect(bitmap, realSrcPtr, dst, paint, flags);
    }

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

    





    virtual void drawData(const void* data, size_t length) {
        
    }

    



    virtual void beginCommentGroup(const char* description) {
        
    }
    virtual void addComment(const char* kywd, const char* value) {
        
    }
    virtual void endCommentGroup() {
        
    }


    

    



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

    




    const SkClipStack* getClipStack() const {
        return &fClipStack;
    }

    class ClipVisitor {
    public:
        virtual ~ClipVisitor();
        virtual void clipRect(const SkRect&, SkRegion::Op, bool antialias) = 0;
        virtual void clipPath(const SkPath&, SkRegion::Op, bool antialias) = 0;
    };

    




    void replayClips(ClipVisitor*) const;

    

    





    class SK_API LayerIter  {
    public:
        
        LayerIter(SkCanvas*, bool skipEmptyClips);
        ~LayerIter();

        
        bool done() const { return fDone; }
        
        void next();

        

        SkBaseDevice*   device() const;
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
    
    virtual SkSurface* onNewSurface(const SkImageInfo&);

    
    
    
    
    virtual SkCanvas* canvasForDrawIter();

    
    
    
    
    bool clipRectBounds(const SkRect* bounds, SaveFlags flags,
                        SkIRect* intersection,
                        const SkImageFilter* imageFilter = NULL);

    
    
    bool updateClipConservativelyUsingBounds(const SkRect&, SkRegion::Op,
                                             bool inverseFilled);

    
    
    void predrawNotify();

    








    virtual SkBaseDevice* setDevice(SkBaseDevice* device);

private:
    class MCRec;

    SkClipStack fClipStack;
    SkDeque     fMCStack;
    
    MCRec*      fMCRec;
    
    uint32_t    fMCRecStorage[32];

    SkBounder*  fBounder;
    int         fSaveLayerCount;    

    SkMetaData* fMetaData;

    SkSurface_Base*  fSurfaceBase;
    SkSurface_Base* getSurfaceBase() const { return fSurfaceBase; }
    void setSurfaceBase(SkSurface_Base* sb) {
        fSurfaceBase = sb;
    }
    friend class SkSurface_Base;
    friend class SkSurface_Gpu;

    bool fDeviceCMDirty;            
    void updateDeviceCMCache();

    friend class SkDrawIter;    
    friend class AutoDrawLooper;

    SkBaseDevice* createLayerDevice(SkBitmap::Config, int width, int height,
                                    bool isOpaque);

    SkBaseDevice* init(SkBaseDevice*);

    
    
    void internalDrawBitmap(const SkBitmap&, const SkMatrix& m, const SkPaint* paint);
    void internalDrawBitmapRect(const SkBitmap& bitmap, const SkRect* src,
                                const SkRect& dst, const SkPaint* paint,
                                DrawBitmapRectFlags flags);
    void internalDrawBitmapNine(const SkBitmap& bitmap, const SkIRect& center,
                                const SkRect& dst, const SkPaint* paint);
    void internalDrawPaint(const SkPaint& paint);
    int internalSaveLayer(const SkRect* bounds, const SkPaint* paint,
                          SaveFlags, bool justForImageFilter);
    void internalDrawDevice(SkBaseDevice*, int x, int y, const SkPaint*);

    
    int internalSave(SaveFlags flags);
    void internalRestore();
    static void DrawRect(const SkDraw& draw, const SkPaint& paint,
                         const SkRect& r, SkScalar textSize);
    static void DrawTextDecorations(const SkDraw& draw, const SkPaint& paint,
                                    const char text[], size_t byteLength,
                                    SkScalar x, SkScalar y);

    


    mutable SkRect fCachedLocalClipBounds;
    mutable bool   fCachedLocalClipBoundsDirty;
    bool fAllowSoftClip;
    bool fAllowSimplifyClip;

    const SkRect& getLocalClipBounds() const {
        if (fCachedLocalClipBoundsDirty) {
            if (!this->getClipBounds(&fCachedLocalClipBounds)) {
                fCachedLocalClipBounds.setEmpty();
            }
            fCachedLocalClipBoundsDirty = false;
        }
        return fCachedLocalClipBounds;
    }

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

    typedef SkRefCnt INHERITED;
};





class SkAutoCanvasRestore : SkNoncopyable {
public:
    SkAutoCanvasRestore(SkCanvas* canvas, bool doSave) : fCanvas(canvas), fSaveCount(0) {
        if (fCanvas) {
            fSaveCount = canvas->getSaveCount();
            if (doSave) {
                canvas->save();
            }
        }
    }
    ~SkAutoCanvasRestore() {
        if (fCanvas) {
            fCanvas->restoreToCount(fSaveCount);
        }
    }

    



    void restore() {
        if (fCanvas) {
            fCanvas->restoreToCount(fSaveCount);
            fCanvas = NULL;
        }
    }

private:
    SkCanvas*   fCanvas;
    int         fSaveCount;
};
#define SkAutoCanvasRestore(...) SK_REQUIRE_LOCAL_VAR(SkAutoCanvasRestore)



class SkAutoCommentBlock : SkNoncopyable {
public:
    SkAutoCommentBlock(SkCanvas* canvas, const char* description) {
        fCanvas = canvas;
        if (NULL != fCanvas) {
            fCanvas->beginCommentGroup(description);
        }
    }

    ~SkAutoCommentBlock() {
        if (NULL != fCanvas) {
            fCanvas->endCommentGroup();
        }
    }

private:
    SkCanvas* fCanvas;
};
#define SkAutoCommentBlock(...) SK_REQUIRE_LOCAL_VAR(SkAutoCommentBlock)

#endif
