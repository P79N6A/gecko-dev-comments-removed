






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

#ifdef SK_SUPPORT_LEGACY_DRAWTEXT_VIRTUAL
    #define SK_LEGACY_DRAWTEXT_VIRTUAL  virtual
#else
    #define SK_LEGACY_DRAWTEXT_VIRTUAL
#endif

class SkCanvasClipVisitor;
class SkBaseDevice;
class SkDraw;
class SkDrawFilter;
class SkMetaData;
class SkPicture;
class SkRRect;
class SkSurface;
class SkSurface_Base;
class GrContext;
class GrRenderTarget;
















class SK_API SkCanvas : public SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(SkCanvas)

    






















    static SkCanvas* NewRaster(const SkImageInfo&);

    static SkCanvas* NewRasterN32(int width, int height) {
        return NewRaster(SkImageInfo::MakeN32Premul(width, height));
    }

    














    static SkCanvas* NewRasterDirect(const SkImageInfo&, void*, size_t);

    static SkCanvas* NewRasterDirectN32(int width, int height, SkPMColor* pixels, size_t rowBytes) {
        return NewRasterDirect(SkImageInfo::MakeN32Premul(width, height), pixels, rowBytes);
    }

    



    SkCanvas();

    




    SkCanvas(int width, int height);

    



    explicit SkCanvas(SkBaseDevice* device);

    



    explicit SkCanvas(const SkBitmap& bitmap);
    virtual ~SkCanvas();

    SkMetaData& getMetaData();

    



    SkImageInfo imageInfo() const;

    

    


    void flush();

    




    SkISize getBaseLayerSize() const;

    


    SkISize getDeviceSize() const { return this->getBaseLayerSize(); }

    





#ifndef SK_SUPPORT_LEGACY_GETDEVICE
protected:  
#endif
    SkBaseDevice* getDevice() const;
public:

    












#ifndef SK_SUPPORT_LEGACY_GETTOPDEVICE
private:
#endif
    SkBaseDevice* getTopDevice(bool updateMatrixClip = false) const;
public:

    




    SkSurface* newSurface(const SkImageInfo&);

    



    GrContext* getGrContext();

    

    










    void* accessTopLayerPixels(SkImageInfo* info, size_t* rowBytes, SkIPoint* origin = NULL);

    











    const void* peekPixels(SkImageInfo* info, size_t* rowBytes);

    


















    bool readPixels(const SkImageInfo& dstInfo, void* dstPixels, size_t dstRowBytes,
                    int srcX, int srcY);

    




    bool readPixels(SkBitmap* bitmap, int srcX, int srcY);

    





    bool readPixels(const SkIRect& srcRect, SkBitmap* bitmap);

    

















    bool writePixels(const SkImageInfo&, const void* pixels, size_t rowBytes, int x, int y);

    



    bool writePixels(const SkBitmap& bitmap, int x, int y);

    

    enum SaveFlags {
        
        
        kMatrix_SaveFlag            = 0x01,
        
        
        kClip_SaveFlag              = 0x02,
        
        kHasAlphaLayer_SaveFlag     = 0x04,
        
        kFullColorLayer_SaveFlag    = 0x08,
        




        kClipToLayer_SaveFlag       = 0x10,

        
        
        kMatrixClip_SaveFlag        = 0x03,
#ifdef SK_SUPPORT_LEGACY_CLIPTOLAYERFLAG
        kARGB_NoClipLayer_SaveFlag  = 0x0F,
#endif
        kARGB_ClipLayer_SaveFlag    = 0x1F
    };

    








    int save();

    











    int saveLayer(const SkRect* bounds, const SkPaint* paint);

    














    SK_ATTR_EXTERNALLY_DEPRECATED("SaveFlags use is deprecated")
    int saveLayer(const SkRect* bounds, const SkPaint* paint, SaveFlags flags);

    










    int saveLayerAlpha(const SkRect* bounds, U8CPU alpha);

    













    SK_ATTR_EXTERNALLY_DEPRECATED("SaveFlags use is deprecated")
    int saveLayerAlpha(const SkRect* bounds, U8CPU alpha, SaveFlags flags);

    




    void restore();

    



    int getSaveCount() const;

    





    void restoreToCount(int saveCount);

    


    virtual bool isDrawingToLayer() const;

    



    void translate(SkScalar dx, SkScalar dy);

    



    void scale(SkScalar sx, SkScalar sy);

    


    void rotate(SkScalar degrees);

    



    void skew(SkScalar sx, SkScalar sy);

    


    void concat(const SkMatrix& matrix);

    


    void setMatrix(const SkMatrix& matrix);

    

    void resetMatrix();

    





    void clipRect(const SkRect& rect,
                  SkRegion::Op op = SkRegion::kIntersect_Op,
                  bool doAntiAlias = false);

    





    void clipRRect(const SkRRect& rrect,
                   SkRegion::Op op = SkRegion::kIntersect_Op,
                   bool doAntiAlias = false);

    





    void clipPath(const SkPath& path,
                  SkRegion::Op op = SkRegion::kIntersect_Op,
                  bool doAntiAlias = false);

    



    void setAllowSoftClip(bool allow) {
        fAllowSoftClip = allow;
    }

    


    void setAllowSimplifyClip(bool allow) {
        fAllowSimplifyClip = allow;
    }

    






    void clipRegion(const SkRegion& deviceRgn,
                    SkRegion::Op op = SkRegion::kIntersect_Op);

    




    void setClipRegion(const SkRegion& deviceRgn) {
        this->clipRegion(deviceRgn, SkRegion::kReplace_Op);
    }

    







    bool quickReject(const SkRect& rect) const;

    









    bool quickReject(const SkPath& path) const;

    









    bool quickRejectY(SkScalar top, SkScalar bottom) const {
        SkASSERT(top <= bottom);

#ifndef SK_WILL_NEVER_DRAW_PERSPECTIVE_TEXT
        
        
        
        if (this->getTotalMatrix().hasPerspective()) {
            
            
            
            
            return false;
        }
#endif

        const SkRect& clipR = this->getLocalClipBounds();
        
        
        
        
        return top >= clipR.fBottom || bottom <= clipR.fTop;
    }

    




    virtual bool getClipBounds(SkRect* bounds) const;

    



    virtual bool getClipDeviceBounds(SkIRect* bounds) const;


    







    void drawARGB(U8CPU a, U8CPU r, U8CPU g, U8CPU b,
                  SkXfermode::Mode mode = SkXfermode::kSrcOver_Mode);

    




    void drawColor(SkColor color,
                   SkXfermode::Mode mode = SkXfermode::kSrcOver_Mode);

    













    virtual void clear(SkColor);

    











    void discard() { this->onDiscard(); }

    




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

    




    void drawIRect(const SkIRect& rect, const SkPaint& paint) {
        SkRect r;
        r.set(rect);    
        this->drawRect(r, paint);
    }

    







    void drawRectCoords(SkScalar left, SkScalar top, SkScalar right,
                        SkScalar bottom, const SkPaint& paint);

    




    virtual void drawOval(const SkRect& oval, const SkPaint&);

    






    virtual void drawRRect(const SkRRect& rrect, const SkPaint& paint);

    



    void drawDRRect(const SkRRect& outer, const SkRRect& inner, const SkPaint&);

    







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

    







    SK_LEGACY_DRAWTEXT_VIRTUAL void drawText(const void* text, size_t byteLength, SkScalar x,
                          SkScalar y, const SkPaint& paint);

    






    SK_LEGACY_DRAWTEXT_VIRTUAL void drawPosText(const void* text, size_t byteLength,
                             const SkPoint pos[], const SkPaint& paint);

    








    SK_LEGACY_DRAWTEXT_VIRTUAL void drawPosTextH(const void* text, size_t byteLength,
                              const SkScalar xpos[], SkScalar constY,
                              const SkPaint& paint);

    











    void drawTextOnPathHV(const void* text, size_t byteLength,
                          const SkPath& path, SkScalar hOffset,
                          SkScalar vOffset, const SkPaint& paint);

    









    SK_LEGACY_DRAWTEXT_VIRTUAL void drawTextOnPath(const void* text, size_t byteLength,
                                const SkPath& path, const SkMatrix* matrix,
                                const SkPaint& paint);

    





    void EXPERIMENTAL_optimize(const SkPicture* picture);

    





    void drawPicture(const SkPicture* picture);

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

    





    void pushCull(const SkRect& cullRect);

    


    void popCull();

    

    



    SkDrawFilter* getDrawFilter() const;

    







    virtual SkDrawFilter* setDrawFilter(SkDrawFilter* filter);

    

    






    virtual bool isClipEmpty() const;

    



    virtual bool isClipRect() const;

    



    const SkMatrix& getTotalMatrix() const;

#ifdef SK_SUPPORT_LEGACY_GETCLIPTYPE
    enum ClipType {
        kEmpty_ClipType = 0,
        kRect_ClipType,
        kComplex_ClipType
    };
    


    virtual ClipType getClipType() const;
#endif

    




    const SkClipStack* getClipStack() const {
        return &fClipStack;
    }

    typedef SkCanvasClipVisitor ClipVisitor;
    




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

    
    const SkRegion& internal_private_getTotalClip() const;
    
    void internal_private_getTotalClipAsPath(SkPath*) const;
    
    GrRenderTarget* internal_private_accessTopLayerRenderTarget();

protected:
    
    virtual SkSurface* onNewSurface(const SkImageInfo&);

    
    virtual const void* onPeekPixels(SkImageInfo*, size_t* rowBytes);
    virtual void* onAccessTopLayerPixels(SkImageInfo*, size_t* rowBytes);

    
    
    
    enum SaveLayerStrategy {
        kFullLayer_SaveLayerStrategy,
        kNoLayer_SaveLayerStrategy
    };

    virtual void willSave() {}
    virtual SaveLayerStrategy willSaveLayer(const SkRect*, const SkPaint*, SaveFlags) {
        return kFullLayer_SaveLayerStrategy;
    }
    virtual void willRestore() {}
    virtual void didConcat(const SkMatrix&) {}
    virtual void didSetMatrix(const SkMatrix&) {}

    virtual void onDrawDRRect(const SkRRect&, const SkRRect&, const SkPaint&);

    virtual void onDrawText(const void* text, size_t byteLength, SkScalar x,
                            SkScalar y, const SkPaint& paint);

    virtual void onDrawPosText(const void* text, size_t byteLength,
                               const SkPoint pos[], const SkPaint& paint);

    virtual void onDrawPosTextH(const void* text, size_t byteLength,
                                const SkScalar xpos[], SkScalar constY,
                                const SkPaint& paint);

    virtual void onDrawTextOnPath(const void* text, size_t byteLength,
                                  const SkPath& path, const SkMatrix* matrix,
                                  const SkPaint& paint);

    enum ClipEdgeStyle {
        kHard_ClipEdgeStyle,
        kSoft_ClipEdgeStyle
    };

    virtual void onClipRect(const SkRect& rect, SkRegion::Op op, ClipEdgeStyle edgeStyle);
    virtual void onClipRRect(const SkRRect& rrect, SkRegion::Op op, ClipEdgeStyle edgeStyle);
    virtual void onClipPath(const SkPath& path, SkRegion::Op op, ClipEdgeStyle edgeStyle);
    virtual void onClipRegion(const SkRegion& deviceRgn, SkRegion::Op op);

    virtual void onDiscard();

    virtual void onDrawPicture(const SkPicture* picture);

    
    
    
    
    virtual SkCanvas* canvasForDrawIter();

    
    
    
    
    bool clipRectBounds(const SkRect* bounds, SaveFlags flags,
                        SkIRect* intersection,
                        const SkImageFilter* imageFilter = NULL);

    
    
    void updateClipConservativelyUsingBounds(const SkRect&, SkRegion::Op,
                                             bool inverseFilled);

    
    
    void predrawNotify();

    virtual void onPushCull(const SkRect& cullRect);
    virtual void onPopCull();

private:
    class MCRec;

    SkClipStack fClipStack;
    SkDeque     fMCStack;
    
    MCRec*      fMCRec;
    
    uint32_t    fMCRecStorage[32];

    int         fSaveLayerCount;    
    int         fCullCount;         

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
    friend class SkLua;             
    friend class SkDebugCanvas;     
    friend class SkDeferredDevice;  
    friend class SkSurface_Raster;  

    SkBaseDevice* createLayerDevice(const SkImageInfo&);

    SkBaseDevice* init(SkBaseDevice*);

    






    SkBaseDevice* setRootDevice(SkBaseDevice* device);

    



    SkISize getTopLayerSize() const;
    SkIPoint getTopLayerOrigin() const;

    
    
    void internalDrawBitmap(const SkBitmap&, const SkMatrix& m, const SkPaint* paint);
    void internalDrawBitmapRect(const SkBitmap& bitmap, const SkRect* src,
                                const SkRect& dst, const SkPaint* paint,
                                DrawBitmapRectFlags flags);
    void internalDrawBitmapNine(const SkBitmap& bitmap, const SkIRect& center,
                                const SkRect& dst, const SkPaint* paint);
    void internalDrawPaint(const SkPaint& paint);
    int internalSaveLayer(const SkRect* bounds, const SkPaint* paint,
                          SaveFlags, bool justForImageFilter, SaveLayerStrategy strategy);
    void internalDrawDevice(SkBaseDevice*, int x, int y, const SkPaint*);

    
    int internalSave();
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
    
    SkTDArray<SkIRect> fCullStack;
    void validateCull(const SkIRect&);
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



















class SkAutoROCanvasPixels : SkNoncopyable {
public:
    SkAutoROCanvasPixels(SkCanvas* canvas);

    
    const void* addr() const { return fAddr; }

    
    size_t rowBytes() const { return fRowBytes; }

    
    const SkImageInfo& info() const { return fInfo; }

    
    
    
    bool asROBitmap(SkBitmap*) const;

private:
    SkBitmap    fBitmap;    
    const void* fAddr;      
    SkImageInfo fInfo;
    size_t      fRowBytes;
};

static inline SkCanvas::SaveFlags operator|(const SkCanvas::SaveFlags lhs,
                                            const SkCanvas::SaveFlags rhs) {
    return static_cast<SkCanvas::SaveFlags>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

static inline SkCanvas::SaveFlags& operator|=(SkCanvas::SaveFlags& lhs,
                                              const SkCanvas::SaveFlags rhs) {
    lhs = lhs | rhs;
    return lhs;
}

class SkCanvasClipVisitor {
public:
    virtual ~SkCanvasClipVisitor();
    virtual void clipRect(const SkRect&, SkRegion::Op, bool antialias) = 0;
    virtual void clipRRect(const SkRRect&, SkRegion::Op, bool antialias) = 0;
    virtual void clipPath(const SkPath&, SkRegion::Op, bool antialias) = 0;
};

#endif
