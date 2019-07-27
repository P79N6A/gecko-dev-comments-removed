






#ifndef SkReadBuffer_DEFINED
#define SkReadBuffer_DEFINED

#include "SkBitmapHeap.h"
#include "SkColorFilter.h"
#include "SkData.h"
#include "SkDrawLooper.h"
#include "SkImageFilter.h"
#include "SkMaskFilter.h"
#include "SkPath.h"
#include "SkPathEffect.h"
#include "SkPicture.h"
#include "SkRasterizer.h"
#include "SkReadBuffer.h"
#include "SkReader32.h"
#include "SkRefCnt.h"
#include "SkShader.h"
#include "SkWriteBuffer.h"
#include "SkXfermode.h"

class SkBitmap;

#if defined(SK_DEBUG) && defined(SK_BUILD_FOR_MAC)
    #define DEBUG_NON_DETERMINISTIC_ASSERT
#endif

class SkReadBuffer {
public:
    SkReadBuffer();
    SkReadBuffer(const void* data, size_t size);
    SkReadBuffer(SkStream* stream);
    virtual ~SkReadBuffer();

    enum Version {
        kFilterLevelIsEnum_Version         = 23,
        kGradientFlippedFlag_Version       = 24,
        kDashWritesPhaseIntervals_Version  = 25,
        kColorShaderNoBool_Version         = 26,
        kNoUnitMappers_Version             = 27,
        kNoMoreBitmapFlatten_Version       = 28,
        kSimplifyLocalMatrix_Version       = 30,
    };

    


    bool isVersionLT(Version targetVersion) const {
        SkASSERT(targetVersion > 0);
        return fVersion > 0 && fVersion < targetVersion;
    }

    
    void setVersion(int version) {
        SkASSERT(0 == fVersion || version == fVersion);
        fVersion = version;
    }

    enum Flags {
        kCrossProcess_Flag  = 1 << 0,
        kScalarIsFloat_Flag = 1 << 1,
        kPtrIs64Bit_Flag    = 1 << 2,
        kValidation_Flag    = 1 << 3,
    };

    void setFlags(uint32_t flags) { fFlags = flags; }
    uint32_t getFlags() const { return fFlags; }

    bool isCrossProcess() const {
        return this->isValidating() || SkToBool(fFlags & kCrossProcess_Flag);
    }
    bool isScalarFloat() const { return SkToBool(fFlags & kScalarIsFloat_Flag); }
    bool isPtr64Bit() const { return SkToBool(fFlags & kPtrIs64Bit_Flag); }
    bool isValidating() const { return SkToBool(fFlags & kValidation_Flag); }

    SkReader32* getReader32() { return &fReader; }

    size_t size() { return fReader.size(); }
    size_t offset() { return fReader.offset(); }
    bool eof() { return fReader.eof(); }
    virtual const void* skip(size_t size) { return fReader.skip(size); }
    void* readFunctionPtr() { return fReader.readPtr(); }

    
    virtual bool readBool();
    virtual SkColor readColor();
    virtual SkFixed readFixed();
    virtual int32_t readInt();
    virtual SkScalar readScalar();
    virtual uint32_t readUInt();
    virtual int32_t read32();

    
    virtual void readString(SkString* string);
    virtual void* readEncodedString(size_t* length, SkPaint::TextEncoding encoding);

    
    virtual void readPoint(SkPoint* point);
    SkPoint readPoint() { SkPoint p; this->readPoint(&p); return p; }
    virtual void readMatrix(SkMatrix* matrix);
    virtual void readIRect(SkIRect* rect);
    virtual void readRect(SkRect* rect);
    virtual void readRegion(SkRegion* region);
    virtual void readPath(SkPath* path);
    void readPaint(SkPaint* paint) { paint->unflatten(*this); }

    virtual SkFlattenable* readFlattenable(SkFlattenable::Type);
    template <typename T> T* readFlattenable() {
        return (T*) this->readFlattenable(T::GetFlattenableType());
    }
    SkColorFilter* readColorFilter() { return this->readFlattenable<SkColorFilter>(); }
    SkDrawLooper*  readDrawLooper()  { return this->readFlattenable<SkDrawLooper>(); }
    SkImageFilter* readImageFilter() { return this->readFlattenable<SkImageFilter>(); }
    SkMaskFilter*  readMaskFilter()  { return this->readFlattenable<SkMaskFilter>(); }
    SkPathEffect*  readPathEffect()  { return this->readFlattenable<SkPathEffect>(); }
    SkRasterizer*  readRasterizer()  { return this->readFlattenable<SkRasterizer>(); }
    SkShader*      readShader()      { return this->readFlattenable<SkShader>(); }
    SkXfermode*    readXfermode()    { return this->readFlattenable<SkXfermode>(); }

    



    virtual void skipFlattenable();

    
    virtual bool readByteArray(void* value, size_t size);
    virtual bool readColorArray(SkColor* colors, size_t size);
    virtual bool readIntArray(int32_t* values, size_t size);
    virtual bool readPointArray(SkPoint* points, size_t size);
    virtual bool readScalarArray(SkScalar* values, size_t size);

    SkData* readByteArrayAsData() {
        size_t len = this->getArrayCount();
        if (!this->validateAvailable(len)) {
            return SkData::NewEmpty();
        }
        void* buffer = sk_malloc_throw(len);
        this->readByteArray(buffer, len);
        return SkData::NewFromMalloc(buffer, len);
    }

    
    virtual uint32_t getArrayCount();

    



    bool readBitmap(SkBitmap* bitmap);

    virtual SkTypeface* readTypeface();

    void setBitmapStorage(SkBitmapHeapReader* bitmapStorage) {
        SkRefCnt_SafeAssign(fBitmapStorage, bitmapStorage);
    }

    void setTypefaceArray(SkTypeface* array[], int count) {
        fTFArray = array;
        fTFCount = count;
    }

    



    void setFactoryPlayback(SkFlattenable::Factory array[], int count) {
        fFactoryTDArray = NULL;
        fFactoryArray = array;
        fFactoryCount = count;
    }

    




    void setFactoryArray(SkTDArray<SkFlattenable::Factory>* array) {
        fFactoryTDArray = array;
        fFactoryArray = NULL;
        fFactoryCount = 0;
    }

    




    void setBitmapDecoder(SkPicture::InstallPixelRefProc bitmapDecoder) {
        fBitmapDecoder = bitmapDecoder;
    }

    
    virtual bool validate(bool isValid) { return true; }
    virtual bool isValid() const { return true; }
    virtual bool validateAvailable(size_t size) { return true; }

protected:
    SkReader32 fReader;

private:
    bool readArray(void* value, size_t size, size_t elementSize);

    uint32_t fFlags;
    int fVersion;

    void* fMemoryPtr;

    SkBitmapHeapReader* fBitmapStorage;
    SkTypeface** fTFArray;
    int        fTFCount;

    SkTDArray<SkFlattenable::Factory>* fFactoryTDArray;
    SkFlattenable::Factory* fFactoryArray;
    int                     fFactoryCount;

    SkPicture::InstallPixelRefProc fBitmapDecoder;

#ifdef DEBUG_NON_DETERMINISTIC_ASSERT
    
    
    int fDecodedBitmapIndex;
#endif 
};

#endif 
