







#include "SkWriteBuffer.h"
#include "SkBitmap.h"
#include "SkData.h"
#include "SkPixelRef.h"
#include "SkPtrRecorder.h"
#include "SkStream.h"
#include "SkTypeface.h"

SkWriteBuffer::SkWriteBuffer(uint32_t flags)
    : fFlags(flags)
    , fFactorySet(NULL)
    , fNamedFactorySet(NULL)
    , fBitmapHeap(NULL)
    , fTFSet(NULL)
    , fBitmapEncoder(NULL) {
}

SkWriteBuffer::SkWriteBuffer(void* storage, size_t storageSize, uint32_t flags)
    : fFlags(flags)
    , fFactorySet(NULL)
    , fNamedFactorySet(NULL)
    , fWriter(storage, storageSize)
    , fBitmapHeap(NULL)
    , fTFSet(NULL)
    , fBitmapEncoder(NULL) {
}

SkWriteBuffer::~SkWriteBuffer() {
    SkSafeUnref(fFactorySet);
    SkSafeUnref(fNamedFactorySet);
    SkSafeUnref(fBitmapHeap);
    SkSafeUnref(fTFSet);
}

void SkWriteBuffer::writeByteArray(const void* data, size_t size) {
    fWriter.write32(SkToU32(size));
    fWriter.writePad(data, size);
}

void SkWriteBuffer::writeBool(bool value) {
    fWriter.writeBool(value);
}

void SkWriteBuffer::writeFixed(SkFixed value) {
    fWriter.write32(value);
}

void SkWriteBuffer::writeScalar(SkScalar value) {
    fWriter.writeScalar(value);
}

void SkWriteBuffer::writeScalarArray(const SkScalar* value, uint32_t count) {
    fWriter.write32(count);
    fWriter.write(value, count * sizeof(SkScalar));
}

void SkWriteBuffer::writeInt(int32_t value) {
    fWriter.write32(value);
}

void SkWriteBuffer::writeIntArray(const int32_t* value, uint32_t count) {
    fWriter.write32(count);
    fWriter.write(value, count * sizeof(int32_t));
}

void SkWriteBuffer::writeUInt(uint32_t value) {
    fWriter.write32(value);
}

void SkWriteBuffer::write32(int32_t value) {
    fWriter.write32(value);
}

void SkWriteBuffer::writeString(const char* value) {
    fWriter.writeString(value);
}

void SkWriteBuffer::writeEncodedString(const void* value, size_t byteLength,
                                              SkPaint::TextEncoding encoding) {
    fWriter.writeInt(encoding);
    fWriter.writeInt(SkToU32(byteLength));
    fWriter.write(value, byteLength);
}


void SkWriteBuffer::writeColor(const SkColor& color) {
    fWriter.write32(color);
}

void SkWriteBuffer::writeColorArray(const SkColor* color, uint32_t count) {
    fWriter.write32(count);
    fWriter.write(color, count * sizeof(SkColor));
}

void SkWriteBuffer::writePoint(const SkPoint& point) {
    fWriter.writeScalar(point.fX);
    fWriter.writeScalar(point.fY);
}

void SkWriteBuffer::writePointArray(const SkPoint* point, uint32_t count) {
    fWriter.write32(count);
    fWriter.write(point, count * sizeof(SkPoint));
}

void SkWriteBuffer::writeMatrix(const SkMatrix& matrix) {
    fWriter.writeMatrix(matrix);
}

void SkWriteBuffer::writeIRect(const SkIRect& rect) {
    fWriter.write(&rect, sizeof(SkIRect));
}

void SkWriteBuffer::writeRect(const SkRect& rect) {
    fWriter.writeRect(rect);
}

void SkWriteBuffer::writeRegion(const SkRegion& region) {
    fWriter.writeRegion(region);
}

void SkWriteBuffer::writePath(const SkPath& path) {
    fWriter.writePath(path);
}

size_t SkWriteBuffer::writeStream(SkStream* stream, size_t length) {
    fWriter.write32(SkToU32(length));
    size_t bytesWritten = fWriter.readFromStream(stream, length);
    if (bytesWritten < length) {
        fWriter.reservePad(length - bytesWritten);
    }
    return bytesWritten;
}

bool SkWriteBuffer::writeToStream(SkWStream* stream) {
    return fWriter.writeToStream(stream);
}

static void write_encoded_bitmap(SkWriteBuffer* buffer, SkData* data,
                                 const SkIPoint& origin) {
    buffer->writeUInt(SkToU32(data->size()));
    buffer->getWriter32()->writePad(data->data(), data->size());
    buffer->write32(origin.fX);
    buffer->write32(origin.fY);
}

void SkWriteBuffer::writeBitmap(const SkBitmap& bitmap) {
    
    
    this->writeInt(bitmap.width());
    this->writeInt(bitmap.height());

    
    
    
    
    
    
    
    
    
    bool useBitmapHeap = fBitmapHeap != NULL;
    
    
    this->writeBool(useBitmapHeap);
    if (useBitmapHeap) {
        SkASSERT(NULL == fBitmapEncoder);
        int32_t slot = fBitmapHeap->insert(bitmap);
        fWriter.write32(slot);
        
        
        
        
        
        
        fWriter.write32(bitmap.getGenerationID());
        return;
    }

    
    if (bitmap.pixelRef()) {
        SkAutoDataUnref data(bitmap.pixelRef()->refEncodedData());
        if (data.get() != NULL) {
            write_encoded_bitmap(this, data, bitmap.pixelRefOrigin());
            return;
        }
    }

    
    if (fBitmapEncoder != NULL) {
        SkASSERT(NULL == fBitmapHeap);
        size_t offset = 0;  
        
        
        SkAutoDataUnref data(fBitmapEncoder(&offset, bitmap));
        if (data.get() != NULL) {
            write_encoded_bitmap(this, data, SkIPoint::Make(0, 0));
            return;
        }
    }

    this->writeUInt(0); 
    SkBitmap::WriteRawPixels(this, bitmap);
}

void SkWriteBuffer::writeTypeface(SkTypeface* obj) {
    if (NULL == obj || NULL == fTFSet) {
        fWriter.write32(0);
    } else {
        fWriter.write32(fTFSet->add(obj));
    }
}

SkFactorySet* SkWriteBuffer::setFactoryRecorder(SkFactorySet* rec) {
    SkRefCnt_SafeAssign(fFactorySet, rec);
    if (fNamedFactorySet != NULL) {
        fNamedFactorySet->unref();
        fNamedFactorySet = NULL;
    }
    return rec;
}

SkNamedFactorySet* SkWriteBuffer::setNamedFactoryRecorder(SkNamedFactorySet* rec) {
    SkRefCnt_SafeAssign(fNamedFactorySet, rec);
    if (fFactorySet != NULL) {
        fFactorySet->unref();
        fFactorySet = NULL;
    }
    return rec;
}

SkRefCntSet* SkWriteBuffer::setTypefaceRecorder(SkRefCntSet* rec) {
    SkRefCnt_SafeAssign(fTFSet, rec);
    return rec;
}

void SkWriteBuffer::setBitmapHeap(SkBitmapHeap* bitmapHeap) {
    SkRefCnt_SafeAssign(fBitmapHeap, bitmapHeap);
    if (bitmapHeap != NULL) {
        SkASSERT(NULL == fBitmapEncoder);
        fBitmapEncoder = NULL;
    }
}

void SkWriteBuffer::setBitmapEncoder(SkPicture::EncodeBitmap bitmapEncoder) {
    fBitmapEncoder = bitmapEncoder;
    if (bitmapEncoder != NULL) {
        SkASSERT(NULL == fBitmapHeap);
        SkSafeUnref(fBitmapHeap);
        fBitmapHeap = NULL;
    }
}

void SkWriteBuffer::writeFlattenable(const SkFlattenable* flattenable) {
    









    if (NULL == flattenable) {
        if (this->isValidating()) {
            this->writeString("");
        } else if (fFactorySet != NULL || fNamedFactorySet != NULL) {
            this->write32(0);
        } else {
            this->writeFunctionPtr(NULL);
        }
        return;
    }

    SkFlattenable::Factory factory = flattenable->getFactory();
    SkASSERT(factory != NULL);

    











    if (this->isValidating()) {
        this->writeString(flattenable->getTypeName());
    } else if (fFactorySet) {
        this->write32(fFactorySet->add(factory));
    } else if (fNamedFactorySet) {
        int32_t index = fNamedFactorySet->find(factory);
        this->write32(index);
        if (0 == index) {
            return;
        }
    } else {
        this->writeFunctionPtr((void*)factory);
    }

    
    (void)fWriter.reserve(sizeof(uint32_t));
    
    size_t offset = fWriter.bytesWritten();
    
    flattenable->flatten(*this);
    size_t objSize = fWriter.bytesWritten() - offset;
    
    fWriter.overwriteTAt(offset - sizeof(uint32_t), SkToU32(objSize));
}
