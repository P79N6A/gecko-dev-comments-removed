






#ifndef SkValidatingReadBuffer_DEFINED
#define SkValidatingReadBuffer_DEFINED

#include "SkRefCnt.h"
#include "SkBitmapHeap.h"
#include "SkReadBuffer.h"
#include "SkWriteBuffer.h"
#include "SkPath.h"
#include "SkPicture.h"
#include "SkReader32.h"

class SkBitmap;

class SkValidatingReadBuffer : public SkReadBuffer {
public:
    SkValidatingReadBuffer(const void* data, size_t size);
    virtual ~SkValidatingReadBuffer();

    virtual const void* skip(size_t size) SK_OVERRIDE;

    
    virtual bool readBool() SK_OVERRIDE;
    virtual SkColor readColor() SK_OVERRIDE;
    virtual SkFixed readFixed() SK_OVERRIDE;
    virtual int32_t readInt() SK_OVERRIDE;
    virtual SkScalar readScalar() SK_OVERRIDE;
    virtual uint32_t readUInt() SK_OVERRIDE;
    virtual int32_t read32() SK_OVERRIDE;

    
    virtual void readString(SkString* string) SK_OVERRIDE;
    virtual void* readEncodedString(size_t* length, SkPaint::TextEncoding encoding) SK_OVERRIDE;

    
    virtual SkFlattenable* readFlattenable(SkFlattenable::Type type) SK_OVERRIDE;
    virtual void skipFlattenable() SK_OVERRIDE;
    virtual void readPoint(SkPoint* point) SK_OVERRIDE;
    virtual void readMatrix(SkMatrix* matrix) SK_OVERRIDE;
    virtual void readIRect(SkIRect* rect) SK_OVERRIDE;
    virtual void readRect(SkRect* rect) SK_OVERRIDE;
    virtual void readRegion(SkRegion* region) SK_OVERRIDE;
    virtual void readPath(SkPath* path) SK_OVERRIDE;

    
    virtual bool readByteArray(void* value, size_t size) SK_OVERRIDE;
    virtual bool readColorArray(SkColor* colors, size_t size) SK_OVERRIDE;
    virtual bool readIntArray(int32_t* values, size_t size) SK_OVERRIDE;
    virtual bool readPointArray(SkPoint* points, size_t size) SK_OVERRIDE;
    virtual bool readScalarArray(SkScalar* values, size_t size) SK_OVERRIDE;

    
    virtual uint32_t getArrayCount() SK_OVERRIDE;

    
    virtual SkTypeface* readTypeface() SK_OVERRIDE;

    virtual bool validate(bool isValid) SK_OVERRIDE;
    virtual bool isValid() const SK_OVERRIDE;

    virtual bool validateAvailable(size_t size) SK_OVERRIDE;

private:
    bool readArray(void* value, size_t size, size_t elementSize);

    void setMemory(const void* data, size_t size);

    static bool IsPtrAlign4(const void* ptr) {
        return SkIsAlign4((uintptr_t)ptr);
    }

    bool fError;

    typedef SkReadBuffer INHERITED;
};

#endif 
