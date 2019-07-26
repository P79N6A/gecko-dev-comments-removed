






#ifndef SkPicturePlayback_DEFINED
#define SkPicturePlayback_DEFINED

#include "SkPicture.h"
#include "SkReader32.h"

#include "SkBitmap.h"
#include "SkData.h"
#include "SkMatrix.h"
#include "SkReadBuffer.h"
#include "SkPaint.h"
#include "SkPath.h"
#include "SkPathHeap.h"
#include "SkRegion.h"
#include "SkRRect.h"
#include "SkPictureFlat.h"

#ifdef SK_BUILD_FOR_ANDROID
#include "SkThread.h"
#endif

class SkPictureRecord;
class SkStream;
class SkWStream;
class SkBBoxHierarchy;
class SkPictureStateTree;
class SkOffsetTable;

struct SkPictInfo {
    enum Flags {
        kCrossProcess_Flag      = 1 << 0,
        kScalarIsFloat_Flag     = 1 << 1,
        kPtrIs64Bit_Flag        = 1 << 2,
    };

    char        fMagic[8];
    uint32_t    fVersion;
    uint32_t    fWidth;
    uint32_t    fHeight;
    uint32_t    fFlags;
};

#define SK_PICT_READER_TAG     SkSetFourByteTag('r', 'e', 'a', 'd')
#define SK_PICT_FACTORY_TAG    SkSetFourByteTag('f', 'a', 'c', 't')
#define SK_PICT_TYPEFACE_TAG   SkSetFourByteTag('t', 'p', 'f', 'c')
#define SK_PICT_PICTURE_TAG    SkSetFourByteTag('p', 'c', 't', 'r')


#define SK_PICT_BUFFER_SIZE_TAG     SkSetFourByteTag('a', 'r', 'a', 'y')

#define SK_PICT_BITMAP_BUFFER_TAG  SkSetFourByteTag('b', 't', 'm', 'p')
#define SK_PICT_PAINT_BUFFER_TAG   SkSetFourByteTag('p', 'n', 't', ' ')
#define SK_PICT_PATH_BUFFER_TAG    SkSetFourByteTag('p', 't', 'h', ' ')


#define SK_PICT_EOF_TAG     SkSetFourByteTag('e', 'o', 'f', ' ')





struct SkPictCopyInfo {
    SkPictCopyInfo() : initialized(false), controller(1024) {}

    bool initialized;
    SkChunkFlatController controller;
    SkTDArray<SkFlatData*> paintData;
};

class SkPicturePlayback {
public:
    SkPicturePlayback();
    SkPicturePlayback(const SkPicturePlayback& src, SkPictCopyInfo* deepCopyInfo = NULL);
    explicit SkPicturePlayback(const SkPictureRecord& record, bool deepCopy = false);
    static SkPicturePlayback* CreateFromStream(SkStream*, const SkPictInfo&,
                                               SkPicture::InstallPixelRefProc);
    static SkPicturePlayback* CreateFromBuffer(SkReadBuffer&);

    virtual ~SkPicturePlayback();

    void draw(SkCanvas& canvas, SkDrawPictureCallback*);

    void serialize(SkWStream*, SkPicture::EncodeBitmap) const;
    void flatten(SkWriteBuffer&) const;

    void dumpSize() const;

    bool containsBitmaps() const;

#ifdef SK_BUILD_FOR_ANDROID
    
    
    void abort() { fAbortCurrentPlayback = true; }
#endif

protected:
    bool parseStream(SkStream*, const SkPictInfo&,
                     SkPicture::InstallPixelRefProc);
    bool parseBuffer(SkReadBuffer& buffer);
#ifdef SK_DEVELOPER
    virtual bool preDraw(int opIndex, int type);
    virtual void postDraw(int opIndex);
#endif

    void preLoadBitmaps(const SkTDArray<void*>& results);

private:
    class TextContainer {
    public:
        size_t length() { return fByteLength; }
        const void* text() { return (const void*) fText; }
        size_t fByteLength;
        const char* fText;
    };

    const SkBitmap& getBitmap(SkReader32& reader) {
        const int index = reader.readInt();
        if (SkBitmapHeap::INVALID_SLOT == index) {
#ifdef SK_DEBUG
            SkDebugf("An invalid bitmap was recorded!\n");
#endif
            return fBadBitmap;
        }
        return (*fBitmaps)[index];
    }

    void getMatrix(SkReader32& reader, SkMatrix* matrix) {
        reader.readMatrix(matrix);
    }

    const SkPath& getPath(SkReader32& reader) {
        return (*fPathHeap)[reader.readInt() - 1];
    }

    SkPicture& getPicture(SkReader32& reader) {
        int index = reader.readInt();
        SkASSERT(index > 0 && index <= fPictureCount);
        return *fPictureRefs[index - 1];
    }

    const SkPaint* getPaint(SkReader32& reader) {
        int index = reader.readInt();
        if (index == 0) {
            return NULL;
        }
        return &(*fPaints)[index - 1];
    }

    const SkRect* getRectPtr(SkReader32& reader) {
        if (reader.readBool()) {
            return &reader.skipT<SkRect>();
        } else {
            return NULL;
        }
    }

    const SkIRect* getIRectPtr(SkReader32& reader) {
        if (reader.readBool()) {
            return &reader.skipT<SkIRect>();
        } else {
            return NULL;
        }
    }

    void getRegion(SkReader32& reader, SkRegion* region) {
        reader.readRegion(region);
    }

    void getText(SkReader32& reader, TextContainer* text) {
        size_t length = text->fByteLength = reader.readInt();
        text->fText = (const char*)reader.skip(length);
    }

    void init();

#ifdef SK_DEBUG_SIZE
public:
    int size(size_t* sizePtr);
    int bitmaps(size_t* size);
    int paints(size_t* size);
    int paths(size_t* size);
#endif

#ifdef SK_DEBUG_DUMP
private:
    void dumpBitmap(const SkBitmap& bitmap) const;
    void dumpMatrix(const SkMatrix& matrix) const;
    void dumpPaint(const SkPaint& paint) const;
    void dumpPath(const SkPath& path) const;
    void dumpPicture(const SkPicture& picture) const;
    void dumpRegion(const SkRegion& region) const;
    int dumpDrawType(char* bufferPtr, char* buffer, DrawType drawType);
    int dumpInt(char* bufferPtr, char* buffer, char* name);
    int dumpRect(char* bufferPtr, char* buffer, char* name);
    int dumpPoint(char* bufferPtr, char* buffer, char* name);
    void dumpPointArray(char** bufferPtrPtr, char* buffer, int count);
    int dumpPtr(char* bufferPtr, char* buffer, char* name, void* ptr);
    int dumpRectPtr(char* bufferPtr, char* buffer, char* name);
    int dumpScalar(char* bufferPtr, char* buffer, char* name);
    void dumpText(char** bufferPtrPtr, char* buffer);
    void dumpStream();

public:
    void dump() const;
#endif

private:    
    bool parseStreamTag(SkStream*, const SkPictInfo&, uint32_t tag, size_t size,
                        SkPicture::InstallPixelRefProc);
    bool parseBufferTag(SkReadBuffer&, uint32_t tag, size_t size);
    void flattenToBuffer(SkWriteBuffer&) const;

private:
    
    
    SkBitmap fBadBitmap;

    SkAutoTUnref<SkBitmapHeap> fBitmapHeap;
    SkAutoTUnref<SkPathHeap> fPathHeap;

    SkTRefArray<SkBitmap>* fBitmaps;
    SkTRefArray<SkPaint>* fPaints;

    SkData* fOpData;    
    SkAutoTUnref<SkOffsetTable> fBitmapUseOffsets;

    SkPicture** fPictureRefs;
    int fPictureCount;

    SkBBoxHierarchy* fBoundingHierarchy;
    SkPictureStateTree* fStateTree;

    SkTypefacePlayback fTFPlayback;
    SkFactoryPlayback* fFactoryPlayback;
#ifdef SK_BUILD_FOR_ANDROID
    SkMutex fDrawMutex;
    bool fAbortCurrentPlayback;
#endif
};

#endif
