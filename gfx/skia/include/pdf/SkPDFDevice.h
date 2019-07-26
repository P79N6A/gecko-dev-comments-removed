








#ifndef SkPDFDevice_DEFINED
#define SkPDFDevice_DEFINED

#include "SkCanvas.h"
#include "SkDevice.h"
#include "SkPaint.h"
#include "SkPath.h"
#include "SkRect.h"
#include "SkRefCnt.h"
#include "SkStream.h"
#include "SkTScopedPtr.h"

class SkPDFArray;
class SkPDFDevice;
class SkPDFDict;
class SkPDFFont;
class SkPDFFormXObject;
class SkPDFGlyphSetMap;
class SkPDFGraphicState;
class SkPDFObject;
class SkPDFShader;
class SkPDFStream;


struct ContentEntry;
struct GraphicStateEntry;





class SkPDFDevice : public SkDevice {
public:
    

















    
    SK_API SkPDFDevice(const SkISize& pageSize, const SkISize& contentSize,
                       const SkMatrix& initialTransform);
    SK_API virtual ~SkPDFDevice();

    virtual uint32_t getDeviceCapabilities() SK_OVERRIDE;

    virtual void clear(SkColor color) SK_OVERRIDE;

    




    virtual void drawPaint(const SkDraw&, const SkPaint& paint) SK_OVERRIDE;
    virtual void drawPoints(const SkDraw&, SkCanvas::PointMode mode,
                            size_t count, const SkPoint[],
                            const SkPaint& paint) SK_OVERRIDE;
    virtual void drawRect(const SkDraw&, const SkRect& r, const SkPaint& paint);
    virtual void drawPath(const SkDraw&, const SkPath& origpath,
                          const SkPaint& paint, const SkMatrix* prePathMatrix,
                          bool pathIsMutable) SK_OVERRIDE;
    virtual void drawBitmap(const SkDraw&, const SkBitmap& bitmap,
                            const SkIRect* srcRectOrNull,
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
    virtual void drawDevice(const SkDraw&, SkDevice*, int x, int y,
                            const SkPaint&) SK_OVERRIDE;

    virtual void onAttachToCanvas(SkCanvas* canvas) SK_OVERRIDE;
    virtual void onDetachFromCanvas() SK_OVERRIDE;

    enum DrawingArea {
        kContent_DrawingArea,  
        kMargin_DrawingArea,   
    };

    








    SK_API void setDrawingArea(DrawingArea drawingArea);

    

    

    SK_API SkPDFDict* getResourceDict();

    





    SK_API void getResources(SkTDArray<SkPDFObject*>* resourceList,
                             bool recursive) const;

    

    SK_API const SkTDArray<SkPDFFont*>& getFontResources() const;

    

    SK_API SkRefPtr<SkPDFArray> getMediaBox() const;

    

    SK_API SkRefPtr<SkPDFArray> getAnnotations() const;

    



    SK_API SkStream* content() const;

    


    SK_API SkData* copyContentToData() const;

    SK_API const SkMatrix& initialTransform() const {
        return fInitialTransform;
    }

    


    const SkPDFGlyphSetMap& getFontGlyphUsage() const {
        return *(fFontGlyphUsage.get());
    }

protected:
    virtual bool onReadPixels(const SkBitmap& bitmap, int x, int y,
                              SkCanvas::Config8888) SK_OVERRIDE;

    virtual bool allowImageFilter(SkImageFilter*) SK_OVERRIDE;

private:
    
    
    friend class ScopedContentEntry;

    SkISize fPageSize;
    SkISize fContentSize;
    SkMatrix fInitialTransform;
    SkClipStack fExistingClipStack;
    SkRegion fExistingClipRegion;
    SkRefPtr<SkPDFArray> fAnnotations;
    SkRefPtr<SkPDFDict> fResourceDict;

    SkTDArray<SkPDFGraphicState*> fGraphicStateResources;
    SkTDArray<SkPDFObject*> fXObjectResources;
    SkTDArray<SkPDFFont*> fFontResources;
    SkTDArray<SkPDFObject*> fShaderResources;

    SkTScopedPtr<ContentEntry> fContentEntries;
    ContentEntry* fLastContentEntry;
    SkTScopedPtr<ContentEntry> fMarginContentEntries;
    ContentEntry* fLastMarginContentEntry;
    DrawingArea fDrawingArea;

    const SkClipStack* fClipStack;

    
    SkTScopedPtr<ContentEntry>* getContentEntries();
    ContentEntry* getLastContentEntry();
    void setLastContentEntry(ContentEntry* contentEntry);

    
    SkTScopedPtr<SkPDFGlyphSetMap> fFontGlyphUsage;

    SkPDFDevice(const SkISize& layerSize, const SkClipStack& existingClipStack,
                const SkRegion& existingClipRegion);

    
    virtual SkDevice* onCreateCompatibleDevice(SkBitmap::Config config,
                                               int width, int height,
                                               bool isOpaque,
                                               Usage usage) SK_OVERRIDE;

    void init();
    void cleanUp(bool clearFontUsage);
    void createFormXObjectFromDevice(SkRefPtr<SkPDFFormXObject>* xobject);

    
    void clearClipFromContent(const SkClipStack* clipStack,
                              const SkRegion& clipRegion);
    void drawFormXObjectWithClip(SkPDFFormXObject* form,
                                 const SkClipStack* clipStack,
                                 const SkRegion& clipRegion,
                                 bool invertClip);

    
    
    
    
    ContentEntry* setUpContentEntry(const SkClipStack* clipStack,
                                    const SkRegion& clipRegion,
                                    const SkMatrix& matrix,
                                    const SkPaint& paint,
                                    bool hasText,
                                    SkRefPtr<SkPDFFormXObject>* dst);
    void finishContentEntry(SkXfermode::Mode xfermode,
                            SkPDFFormXObject* dst);
    bool isContentEmpty();

    void populateGraphicStateEntryFromPaint(const SkMatrix& matrix,
                                            const SkClipStack& clipStack,
                                            const SkRegion& clipRegion,
                                            const SkPaint& paint,
                                            bool hasText,
                                            GraphicStateEntry* entry);
    int addGraphicStateResource(SkPDFGraphicState* gs);

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

    bool handleAnnotations(const SkRect& r, const SkMatrix& matrix,
                           const SkPaint& paint);

    typedef SkDevice INHERITED;
};

#endif
