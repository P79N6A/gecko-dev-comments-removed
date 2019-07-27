








#ifndef SkPDFDevice_DEFINED
#define SkPDFDevice_DEFINED

#include "SkDevice.h"
#include "SkBitmap.h"
#include "SkCanvas.h"
#include "SkPaint.h"
#include "SkPath.h"
#include "SkPicture.h"
#include "SkRect.h"
#include "SkRefCnt.h"
#include "SkStream.h"
#include "SkTDArray.h"
#include "SkTemplates.h"

class SkPDFArray;
class SkPDFDevice;
class SkPDFDict;
class SkPDFFont;
class SkPDFFormXObject;
class SkPDFGlyphSetMap;
class SkPDFGraphicState;
class SkPDFObject;
class SkPDFResourceDict;
class SkPDFShader;
class SkPDFStream;
class SkRRect;
template <typename T> class SkTSet;


struct ContentEntry;
struct GraphicStateEntry;
struct NamedDestination;





class SkPDFDevice : public SkBaseDevice {
public:
    

















    
    SK_API SkPDFDevice(const SkISize& pageSize, const SkISize& contentSize,
                       const SkMatrix& initialTransform);
    SK_API virtual ~SkPDFDevice();

    virtual void clear(SkColor color) SK_OVERRIDE;

    




    virtual void drawPaint(const SkDraw&, const SkPaint& paint) SK_OVERRIDE;
    virtual void drawPoints(const SkDraw&, SkCanvas::PointMode mode,
                            size_t count, const SkPoint[],
                            const SkPaint& paint) SK_OVERRIDE;
    virtual void drawRect(const SkDraw&, const SkRect& r, const SkPaint& paint);
    virtual void drawOval(const SkDraw&, const SkRect& oval, const SkPaint& paint) SK_OVERRIDE;
    virtual void drawRRect(const SkDraw&, const SkRRect& rr, const SkPaint& paint) SK_OVERRIDE;
    virtual void drawPath(const SkDraw&, const SkPath& origpath,
                          const SkPaint& paint, const SkMatrix* prePathMatrix,
                          bool pathIsMutable) SK_OVERRIDE;
    virtual void drawBitmapRect(const SkDraw& draw, const SkBitmap& bitmap,
                                const SkRect* src, const SkRect& dst,
                                const SkPaint& paint,
                                SkCanvas::DrawBitmapRectFlags flags) SK_OVERRIDE;
    virtual void drawBitmap(const SkDraw&, const SkBitmap& bitmap,
                            const SkMatrix& matrix, const SkPaint&) SK_OVERRIDE;
    virtual void drawSprite(const SkDraw&, const SkBitmap& bitmap, int x, int y,
                            const SkPaint& paint) SK_OVERRIDE;
    virtual void drawText(const SkDraw&, const void* text, size_t len,
                          SkScalar x, SkScalar y, const SkPaint&) SK_OVERRIDE;
    virtual void drawPosText(const SkDraw&, const void* text, size_t len,
                             const SkScalar pos[], SkScalar constY,
                             int scalarsPerPos, const SkPaint&) SK_OVERRIDE;
    virtual void drawTextOnPath(const SkDraw&, const void* text, size_t len,
                                const SkPath& path, const SkMatrix* matrix,
                                const SkPaint& paint) SK_OVERRIDE;
    virtual void drawVertices(const SkDraw&, SkCanvas::VertexMode,
                              int vertexCount, const SkPoint verts[],
                              const SkPoint texs[], const SkColor colors[],
                              SkXfermode* xmode, const uint16_t indices[],
                              int indexCount, const SkPaint& paint) SK_OVERRIDE;
    virtual void drawDevice(const SkDraw&, SkBaseDevice*, int x, int y,
                            const SkPaint&) SK_OVERRIDE;

    virtual void onAttachToCanvas(SkCanvas* canvas) SK_OVERRIDE;
    virtual void onDetachFromCanvas() SK_OVERRIDE;
    virtual SkImageInfo imageInfo() const SK_OVERRIDE;    

    enum DrawingArea {
        kContent_DrawingArea,  
        kMargin_DrawingArea,   
    };

    








    SK_API void setDrawingArea(DrawingArea drawingArea);

    










    void setDCTEncoder(SkPicture::EncodeBitmap encoder) {
        fEncoder = encoder;
    }

    

    

    SK_API SkPDFResourceDict* getResourceDict();

    

    SK_API const SkTDArray<SkPDFFont*>& getFontResources() const;

    



    void appendDestinations(SkPDFDict* dict, SkPDFObject* page);

    


    SK_API SkPDFArray* copyMediaBox() const;

    

    SK_API SkPDFArray* getAnnotations() const { return fAnnotations; }

    



    SK_API SkStream* content() const;

    


    SK_API SkData* copyContentToData() const;

    SK_API const SkMatrix& initialTransform() const {
        return fInitialTransform;
    }

    


    const SkPDFGlyphSetMap& getFontGlyphUsage() const {
        return *(fFontGlyphUsage.get());
    }


    









    void setRasterDpi(SkScalar rasterDpi) {
        fRasterDpi = rasterDpi;
    }

protected:
    virtual const SkBitmap& onAccessBitmap() SK_OVERRIDE {
        return fLegacyBitmap;
    }

    virtual bool allowImageFilter(const SkImageFilter*) SK_OVERRIDE {
        return false;
    }

    virtual SkSurface* newSurface(const SkImageInfo&) SK_OVERRIDE;

private:
    
    
    friend class ScopedContentEntry;

    SkISize fPageSize;
    SkISize fContentSize;
    SkMatrix fInitialTransform;
    SkClipStack fExistingClipStack;
    SkRegion fExistingClipRegion;
    SkPDFArray* fAnnotations;
    SkPDFResourceDict* fResourceDict;
    SkTDArray<NamedDestination*> fNamedDestinations;

    SkTDArray<SkPDFGraphicState*> fGraphicStateResources;
    SkTDArray<SkPDFObject*> fXObjectResources;
    SkTDArray<SkPDFFont*> fFontResources;
    SkTDArray<SkPDFObject*> fShaderResources;

    SkAutoTDelete<ContentEntry> fContentEntries;
    ContentEntry* fLastContentEntry;
    SkAutoTDelete<ContentEntry> fMarginContentEntries;
    ContentEntry* fLastMarginContentEntry;
    DrawingArea fDrawingArea;

    const SkClipStack* fClipStack;

    
    SkAutoTDelete<ContentEntry>* getContentEntries();
    ContentEntry* getLastContentEntry();
    void setLastContentEntry(ContentEntry* contentEntry);

    
    SkAutoTDelete<SkPDFGlyphSetMap> fFontGlyphUsage;

    SkPicture::EncodeBitmap fEncoder;
    SkScalar fRasterDpi;

    SkBitmap fLegacyBitmap;

    SkPDFDevice(const SkISize& layerSize, const SkClipStack& existingClipStack,
                const SkRegion& existingClipRegion);

    
    virtual SkBaseDevice* onCreateDevice(const SkImageInfo&, Usage) SK_OVERRIDE;

    void init();
    void cleanUp(bool clearFontUsage);
    SkPDFFormXObject* createFormXObjectFromDevice();

    void drawFormXObjectWithMask(int xObjectIndex,
                                 SkPDFFormXObject* mask,
                                 const SkClipStack* clipStack,
                                 const SkRegion& clipRegion,
                                 SkXfermode::Mode mode,
                                 bool invertClip);

    
    
    
    
    ContentEntry* setUpContentEntry(const SkClipStack* clipStack,
                                    const SkRegion& clipRegion,
                                    const SkMatrix& matrix,
                                    const SkPaint& paint,
                                    bool hasText,
                                    SkPDFFormXObject** dst);
    void finishContentEntry(SkXfermode::Mode xfermode,
                            SkPDFFormXObject* dst,
                            SkPath* shape);
    bool isContentEmpty();

    void populateGraphicStateEntryFromPaint(const SkMatrix& matrix,
                                            const SkClipStack& clipStack,
                                            const SkRegion& clipRegion,
                                            const SkPaint& paint,
                                            bool hasText,
                                            GraphicStateEntry* entry);
    int addGraphicStateResource(SkPDFGraphicState* gs);
    int addXObjectResource(SkPDFObject* xObject);

    void updateFont(const SkPaint& paint, uint16_t glyphID,
                    ContentEntry* contentEntry);
    int getFontResourceIndex(SkTypeface* typeface, uint16_t glyphID);

    void internalDrawPaint(const SkPaint& paint, ContentEntry* contentEntry);
    void internalDrawBitmap(const SkMatrix& matrix,
                            const SkClipStack* clipStack,
                            const SkRegion& clipRegion,
                            const SkBitmap& bitmap,
                            const SkIRect* srcRect,
                            const SkPaint& paint);

    


    void copyContentEntriesToData(ContentEntry* entry, SkWStream* data) const;

#ifdef SK_PDF_USE_PATHOPS
    bool handleInversePath(const SkDraw& d, const SkPath& origPath,
                           const SkPaint& paint, bool pathIsMutable,
                           const SkMatrix* prePathMatrix = NULL);
#endif
    bool handleRectAnnotation(const SkRect& r, const SkMatrix& matrix,
                              const SkPaint& paint);
    bool handlePointAnnotation(const SkPoint* points, size_t count,
                               const SkMatrix& matrix, const SkPaint& paint);
    SkPDFDict* createLinkAnnotation(const SkRect& r, const SkMatrix& matrix);
    void handleLinkToURL(SkData* urlData, const SkRect& r,
                         const SkMatrix& matrix);
    void handleLinkToNamedDest(SkData* nameData, const SkRect& r,
                               const SkMatrix& matrix);
    void defineNamedDestination(SkData* nameData, const SkPoint& point,
                                const SkMatrix& matrix);

    typedef SkBaseDevice INHERITED;

    
    
    
    
};

#endif
