






#ifndef SkPictureData_DEFINED
#define SkPictureData_DEFINED

#include "SkBitmap.h"
#include "SkPathHeap.h"
#include "SkPicture.h"
#include "SkPictureFlat.h"
#include "SkPictureStateTree.h"

class SkData;
class SkPictureRecord;
class SkReader32;
class SkStream;
class SkWStream;
class SkBBoxHierarchy;
class SkMatrix;
class SkPaint;
class SkPath;
class SkPictureStateTree;
class SkReadBuffer;

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



class SkPictureContentInfo {
public:
    SkPictureContentInfo() { this->reset(); }

    SkPictureContentInfo(const SkPictureContentInfo& src) { this->set(src); }

    void set(const SkPictureContentInfo& src) {
        fNumPaintWithPathEffectUses = src.fNumPaintWithPathEffectUses;
        fNumFastPathDashEffects = src.fNumFastPathDashEffects;
        fNumAAConcavePaths = src.fNumAAConcavePaths;
        fNumAAHairlineConcavePaths = src.fNumAAHairlineConcavePaths;
    }

    void reset() {
        fNumPaintWithPathEffectUses = 0;
        fNumFastPathDashEffects = 0;
        fNumAAConcavePaths = 0;
        fNumAAHairlineConcavePaths = 0;
    }

    void swap(SkPictureContentInfo* other) {
        SkTSwap(fNumPaintWithPathEffectUses, other->fNumPaintWithPathEffectUses);
        SkTSwap(fNumFastPathDashEffects, other->fNumFastPathDashEffects);
        SkTSwap(fNumAAConcavePaths, other->fNumAAConcavePaths);
        SkTSwap(fNumAAHairlineConcavePaths, other->fNumAAHairlineConcavePaths);
    }

    void incPaintWithPathEffectUses() { ++fNumPaintWithPathEffectUses; }
    int numPaintWithPathEffectUses() const { return fNumPaintWithPathEffectUses; }

    void incFastPathDashEffects() { ++fNumFastPathDashEffects; }
    int numFastPathDashEffects() const { return fNumFastPathDashEffects; }

    void incAAConcavePaths() { ++fNumAAConcavePaths; }
    int numAAConcavePaths() const { return fNumAAConcavePaths; }

    void incAAHairlineConcavePaths() {
        ++fNumAAHairlineConcavePaths;
        SkASSERT(fNumAAHairlineConcavePaths <= fNumAAConcavePaths);
    }
    int numAAHairlineConcavePaths() const { return fNumAAHairlineConcavePaths; }

private:
    
    
    int fNumPaintWithPathEffectUses;
    
    
    int fNumFastPathDashEffects;
    
    
    int fNumAAConcavePaths;
    
    
    int fNumAAHairlineConcavePaths;
};

#ifdef SK_SUPPORT_LEGACY_PICTURE_CLONE




struct SkPictCopyInfo {
    SkPictCopyInfo() : controller(1024) {}

    SkChunkFlatController controller;
    SkTDArray<SkFlatData*> paintData;
};
#endif

class SkPictureData {
public:
#ifdef SK_SUPPORT_LEGACY_PICTURE_CLONE
    SkPictureData(const SkPictureData& src, SkPictCopyInfo* deepCopyInfo = NULL);
#endif
    SkPictureData(const SkPictureRecord& record, const SkPictInfo&, bool deepCopyOps);
    static SkPictureData* CreateFromStream(SkStream*,
                                           const SkPictInfo&,
                                           SkPicture::InstallPixelRefProc);
    static SkPictureData* CreateFromBuffer(SkReadBuffer&, const SkPictInfo&);

    virtual ~SkPictureData();

    const SkPicture::OperationList* getActiveOps(const SkIRect& queryRect) const;

    void serialize(SkWStream*, SkPicture::EncodeBitmap) const;
    void flatten(SkWriteBuffer&) const;

    void dumpSize() const;

    bool containsBitmaps() const;

    const SkData* opData() const { return fOpData; }

protected:
    explicit SkPictureData(const SkPictInfo& info);

    bool parseStream(SkStream*, SkPicture::InstallPixelRefProc);
    bool parseBuffer(SkReadBuffer& buffer);

public:
    const SkBitmap& getBitmap(SkReader32* reader) const {
        const int index = reader->readInt();
        if (SkBitmapHeap::INVALID_SLOT == index) {
#ifdef SK_DEBUG
            SkDebugf("An invalid bitmap was recorded!\n");
#endif
            return fBadBitmap;
        }
        return (*fBitmaps)[index];
    }

    const SkPath& getPath(SkReader32* reader) const {
        int index = reader->readInt() - 1;
        return (*fPathHeap.get())[index];
    }

    const SkPicture* getPicture(SkReader32* reader) const {
        int index = reader->readInt();
        SkASSERT(index > 0 && index <= fPictureCount);
        return fPictureRefs[index - 1];
    }

    const SkPaint* getPaint(SkReader32* reader) const {
        int index = reader->readInt();
        if (index == 0) {
            return NULL;
        }
        return &(*fPaints)[index - 1];
    }

    void initIterator(SkPictureStateTree::Iterator* iter, 
                      const SkTDArray<void*>& draws,
                      SkCanvas* canvas) const {
        if (NULL != fStateTree) {
            fStateTree->initIterator(iter, draws, canvas);
        }
    }

#ifdef SK_DEBUG_SIZE
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

#if SK_SUPPORT_GPU
    



    bool suitableForGpuRasterization(GrContext* context, const char **reason,
                                     int sampleCount = 0) const;

    



    bool suitableForGpuRasterization(GrContext* context, const char **reason,
                                     GrPixelConfig config, SkScalar dpi) const;
#endif

private:
    friend class SkPicture; 

    void init();

    
    bool parseStreamTag(SkStream*, uint32_t tag, uint32_t size, SkPicture::InstallPixelRefProc);
    bool parseBufferTag(SkReadBuffer&, uint32_t tag, uint32_t size);
    void flattenToBuffer(SkWriteBuffer&) const;

    
    
    SkBitmap fBadBitmap;

    SkAutoTUnref<SkBitmapHeap> fBitmapHeap;

    SkTRefArray<SkBitmap>* fBitmaps;
    SkTRefArray<SkPaint>* fPaints;

    SkData* fOpData;    

    SkAutoTUnref<const SkPathHeap> fPathHeap;  

    const SkPicture** fPictureRefs;
    int fPictureCount;

    SkBBoxHierarchy* fBoundingHierarchy;
    SkPictureStateTree* fStateTree;

    SkPictureContentInfo fContentInfo;

    SkTypefacePlayback fTFPlayback;
    SkFactoryPlayback* fFactoryPlayback;

    const SkPictInfo fInfo;

    static void WriteFactories(SkWStream* stream, const SkFactorySet& rec);
    static void WriteTypefaces(SkWStream* stream, const SkRefCntSet& rec);

    void initForPlayback() const;
};

#endif
