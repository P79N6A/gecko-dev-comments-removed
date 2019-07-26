






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

    





    SkBaseDevice* getDevice() const;

    












#ifndef SK_SUPPORT_LEGACY_GETTOPDEVICE
private:
#endif
    SkBaseDevice* getTopDevice(bool updateMatrixClip = false) const;
public:

    



    SkSurface* newSurface(const SkImageInfo&);

    



    GrContext* getGrContext();

    

    











    void* accessTopLayerPixels(SkImageInfo* info, size_t* rowBytes);

    











    const void* peekPixels(SkImageInfo* info, size_t* rowBytes);

    











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

#ifdef SK_SUPPORT_LEGACY_WRITEPIXELSCONFIG
    















    void writePixels(const SkBitmap& bitmap, int x, int y, Config8888 config8888);
#endif

    

















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

    













    int save(SaveFlags flags = kMatrixClip_SaveFlag);

    












    int saveLayer(const SkRect* bounds, const SkPaint* paint,
                  SaveFlags flags = kARGB_ClipLayer_SaveFlag);

    











    int saveLayerAlpha(const SkRect* bounds, U8CPU alpha,
                       SaveFlags flags = kARGB_ClipLayer_SaveFlag);

    




    void restore();

    



    int getSaveCount() const;

    





    void restoreToCount(int saveCount);

    


    virtual bool isDrawingToLayer() const;

    




    bool translate(SkScalar dx, SkScalar dy);

    




    bool scale(SkScalar sx, SkScalar sy);

    



    bool rotate(SkScalar degrees);

    




    bool skew(SkScalar sx, SkScalar sy);

    



    bool concat(const SkMatrix& matrix);

    


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

    





    void EXPERIMENTAL_optimize(SkPicture* picture);

    





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

    





    void pushCull(const SkRect& cullRect) {
        ++fCullCount;
        this->onPushCull(cullRect);
    }

    


    void popCull() {
        if (fCullCount > 0) {
            --fCullCount;
            this->onPopCull();
        }
    }
    

    



    SkBounder*  getBounder() const { return fBounder; }

    







    virtual SkBounder* setBounder(SkBounder* bounder);

    



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

#ifdef SK_SUPPORT_LEGACY_GETTOTALCLIP
    




    const SkRegion& getTotalClip() const;
#endif

    




    const SkClipStack* getClipStack() const {
        return &fClipStack;
    }

    class ClipVisitor {
    public:
        virtual ~ClipVisitor();
        virtual void clipRect(const SkRect&, SkRegion::Op, bool antialias) = 0;
        virtual void clipRRect(const SkRRect&, SkRegion::Op, bool antialias) = 0;
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
    virtual void willSave(SaveFlags);
    virtual SaveLayerStrategy willSaveLayer(const SkRect*, const SkPaint*, SaveFlags);
    virtual void willRestore();

    virtual void didTranslate(SkScalar, SkScalar);
    virtual void didScale(SkScalar, SkScalar);
    virtual void didRotate(SkScalar);
    virtual void didSkew(SkScalar, SkScalar);
    virtual void didConcat(const SkMatrix&);
    virtual void didSetMatrix(const SkMatrix&);

    virtual void onDrawDRRect(const SkRRect&, const SkRRect&, const SkPaint&);

    enum ClipEdgeStyle {
        kHard_ClipEdgeStyle,
        kSoft_ClipEdgeStyle
    };

    virtual void onClipRect(const SkRect& rect, SkRegion::Op op, ClipEdgeStyle edgeStyle);
    virtual void onClipRRect(const SkRRect& rrect, SkRegion::Op op, ClipEdgeStyle edgeStyle);
    virtual void onClipPath(const SkPath& path, SkRegion::Op op, ClipEdgeStyle edgeStyle);
    virtual void onClipRegion(const SkRegion& deviceRgn, SkRegion::Op op);

    
    
    
    
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

    SkBounder*  fBounder;
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
    friend class SkDeferredDevice;  

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

#endif
