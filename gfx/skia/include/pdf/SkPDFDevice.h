








#ifndef SkPDFDevice_DEFINED
#define SkPDFDevice_DEFINED

#include "SkCanvas.h"
#include "SkDevice.h"
#include "SkPaint.h"
#include "SkPath.h"
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

    virtual uint32_t getDeviceCapabilities() { return kVector_Capability; }

    virtual void clear(SkColor color);

    virtual bool readPixels(const SkIRect& srcRect, SkBitmap* bitmap) {
        return false;
    }

    




    virtual void drawPaint(const SkDraw&, const SkPaint& paint);
    virtual void drawPoints(const SkDraw&, SkCanvas::PointMode mode,
                            size_t count, const SkPoint[],
                            const SkPaint& paint);
    virtual void drawRect(const SkDraw&, const SkRect& r, const SkPaint& paint);
    virtual void drawPath(const SkDraw&, const SkPath& origpath,
                          const SkPaint& paint, const SkMatrix* prePathMatrix,
                          bool pathIsMutable);
    virtual void drawBitmap(const SkDraw&, const SkBitmap& bitmap,
                            const SkIRect* srcRectOrNull,
                            const SkMatrix& matrix, const SkPaint& paint);
    virtual void drawSprite(const SkDraw&, const SkBitmap& bitmap, int x, int y,
                            const SkPaint& paint);
    virtual void drawText(const SkDraw&, const void* text, size_t len,
                          SkScalar x, SkScalar y, const SkPaint& paint);
    virtual void drawPosText(const SkDraw&, const void* text, size_t len,
                             const SkScalar pos[], SkScalar constY,
                             int scalarsPerPos, const SkPaint& paint);
    virtual void drawTextOnPath(const SkDraw&, const void* text, size_t len,
                                const SkPath& path, const SkMatrix* matrix,
                                const SkPaint& paint);
    virtual void drawVertices(const SkDraw&, SkCanvas::VertexMode,
                              int vertexCount, const SkPoint verts[],
                              const SkPoint texs[], const SkColor colors[],
                              SkXfermode* xmode, const uint16_t indices[],
                              int indexCount, const SkPaint& paint);
    virtual void drawDevice(const SkDraw&, SkDevice*, int x, int y,
                            const SkPaint&);

    enum DrawingArea {
        kContent_DrawingArea,  
        kMargin_DrawingArea,   
    };

    








    SK_API void setDrawingArea(DrawingArea drawingArea);

    

    

    SK_API SkPDFDict* getResourceDict();

    


    SK_API void getResources(SkTDArray<SkPDFObject*>* resourceList) const;

    

    SK_API const SkTDArray<SkPDFFont*>& getFontResources() const;

    

    SK_API SkRefPtr<SkPDFArray> getMediaBox() const;

    



    SK_API SkStream* content() const;

    


    SK_API SkData* copyContentToData() const;

    SK_API const SkMatrix& initialTransform() const {
        return fInitialTransform;
    }

    


    const SkPDFGlyphSetMap& getFontGlyphUsage() const {
        return *(fFontGlyphUsage.get());
    }

private:
    
    
    friend class ScopedContentEntry;

    SkISize fPageSize;
    SkISize fContentSize;
    SkMatrix fInitialTransform;
    SkClipStack fExistingClipStack;
    SkRegion fExistingClipRegion;
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

    
    SkTScopedPtr<ContentEntry>* getContentEntries();
    ContentEntry* getLastContentEntry();
    void setLastContentEntry(ContentEntry* contentEntry);

    
    SkTScopedPtr<SkPDFGlyphSetMap> fFontGlyphUsage;

    SkPDFDevice(const SkISize& layerSize, const SkClipStack& existingClipStack,
                const SkRegion& existingClipRegion);

    
    virtual SkDevice* onCreateCompatibleDevice(SkBitmap::Config config,
                                               int width, int height,
                                               bool isOpaque,
                                               Usage usage);

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

    
    SkPDFDevice(const SkPDFDevice&);
    void operator=(const SkPDFDevice&);
};

#endif
