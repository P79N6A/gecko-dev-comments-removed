







#include "SkOrderedWriteBuffer.h"
#include "SkBitmap.h"
#include "SkData.h"
#include "SkPixelRef.h"
#include "SkPtrRecorder.h"
#include "SkStream.h"
#include "SkTypeface.h"

SkOrderedWriteBuffer::SkOrderedWriteBuffer(size_t minSize)
    : INHERITED()
    , fFactorySet(NULL)
    , fNamedFactorySet(NULL)
    , fWriter(minSize)
    , fBitmapHeap(NULL)
    , fTFSet(NULL)
    , fBitmapEncoder(NULL) {
}

SkOrderedWriteBuffer::SkOrderedWriteBuffer(size_t minSize, void* storage, size_t storageSize)
    : INHERITED()
    , fFactorySet(NULL)
    , fNamedFactorySet(NULL)
    , fWriter(minSize, storage, storageSize)
    , fBitmapHeap(NULL)
    , fTFSet(NULL)
    , fBitmapEncoder(NULL) {
}

SkOrderedWriteBuffer::~SkOrderedWriteBuffer() {
    SkSafeUnref(fFactorySet);
    SkSafeUnref(fNamedFactorySet);
    SkSafeUnref(fBitmapHeap);
    SkSafeUnref(fTFSet);
}

void SkOrderedWriteBuffer::writeByteArray(const void* data, size_t size) {
    fWriter.write32(size);
    fWriter.writePad(data, size);
}

void SkOrderedWriteBuffer::writeBool(bool value) {
    fWriter.writeBool(value);
}

void SkOrderedWriteBuffer::writeFixed(SkFixed value) {
    fWriter.write32(value);
}

void SkOrderedWriteBuffer::writeScalar(SkScalar value) {
    fWriter.writeScalar(value);
}

void SkOrderedWriteBuffer::writeScalarArray(const SkScalar* value, uint32_t count) {
    fWriter.write32(count);
    fWriter.write(value, count * sizeof(SkScalar));
}

void SkOrderedWriteBuffer::writeInt(int32_t value) {
    fWriter.write32(value);
}

void SkOrderedWriteBuffer::writeIntArray(const int32_t* value, uint32_t count) {
    fWriter.write32(count);
    fWriter.write(value, count * sizeof(int32_t));
}

void SkOrderedWriteBuffer::writeUInt(uint32_t value) {
    fWriter.write32(value);
}

void SkOrderedWriteBuffer::write32(int32_t value) {
    fWriter.write32(value);
}

void SkOrderedWriteBuffer::writeString(const char* value) {
    fWriter.writeString(value);
}

void SkOrderedWriteBuffer::writeEncodedString(const void* value, size_t byteLength,
                                              SkPaint::TextEncoding encoding) {
    fWriter.writeInt(encoding);
    fWriter.writeInt(byteLength);
    fWriter.write(value, byteLength);
}


void SkOrderedWriteBuffer::writeColor(const SkColor& color) {
    fWriter.write32(color);
}

void SkOrderedWriteBuffer::writeColorArray(const SkColor* color, uint32_t count) {
    fWriter.write32(count);
    fWriter.write(color, count * sizeof(SkColor));
}

void SkOrderedWriteBuffer::writePoint(const SkPoint& point) {
    fWriter.writeScalar(point.fX);
    fWriter.writeScalar(point.fY);
}

void SkOrderedWriteBuffer::writePointArray(const SkPoint* point, uint32_t count) {
    fWriter.write32(count);
    fWriter.write(point, count * sizeof(SkPoint));
}

void SkOrderedWriteBuffer::writeMatrix(const SkMatrix& matrix) {
    fWriter.writeMatrix(matrix);
}

void SkOrderedWriteBuffer::writeIRect(const SkIRect& rect) {
    fWriter.write(&rect, sizeof(SkIRect));
}

void SkOrderedWriteBuffer::writeRect(const SkRect& rect) {
    fWriter.writeRect(rect);
}

void SkOrderedWriteBuffer::writeRegion(const SkRegion& region) {
    fWriter.writeRegion(region);
}

void SkOrderedWriteBuffer::writePath(const SkPath& path) {
    fWriter.writePath(path);
}

size_t SkOrderedWriteBuffer::writeStream(SkStream* stream, size_t length) {
    fWriter.write32(length);
    size_t bytesWritten = fWriter.readFromStream(stream, length);
    if (bytesWritten < length) {
        fWriter.reservePad(length - bytesWritten);
    }
    return bytesWritten;
}

bool SkOrderedWriteBuffer::writeToStream(SkWStream* stream) {
    return fWriter.writeToStream(stream);
}

void SkOrderedWriteBuffer::writeBitmap(const SkBitmap& bitmap) {
    
    
    
    
    
    
    
    
    
    
    if (fBitmapHeap != NULL) {
        SkASSERT(NULL == fBitmapEncoder);
        
        this->writeUInt(0);
        int32_t slot = fBitmapHeap->insert(bitmap);
        fWriter.write32(slot);
        
        
        
        
        
        
        fWriter.write32(bitmap.getGenerationID());
        return;
    }
    bool encoded = false;
    
    
    SkPixelRef* ref = bitmap.pixelRef();
    if (ref != NULL) {
        SkAutoDataUnref data(ref->refEncodedData());
        if (data.get() != NULL) {
            
            
            
            this->writeUInt(data->size());
            fWriter.writePad(data->data(), data->size());
            encoded = true;
        }
    }
    if (fBitmapEncoder != NULL && !encoded) {
        SkASSERT(NULL == fBitmapHeap);
        SkDynamicMemoryWStream stream;
        if (fBitmapEncoder(&stream, bitmap)) {
            uint32_t offset = fWriter.bytesWritten();
            
            
            
            size_t length = stream.getOffset();
            this->writeUInt(length);
            if (stream.read(fWriter.reservePad(length), 0, length)) {
                encoded = true;
            } else {
                
                fWriter.rewindToOffset(offset);
            }
        }
    }
    if (encoded) {
        
        this->writeInt(bitmap.width());
        this->writeInt(bitmap.height());
    } else {
        
        this->writeUInt(0);
        bitmap.flatten(*this);
    }
}

void SkOrderedWriteBuffer::writeTypeface(SkTypeface* obj) {
    if (NULL == obj || NULL == fTFSet) {
        fWriter.write32(0);
    } else {
        fWriter.write32(fTFSet->add(obj));
    }
}

SkFactorySet* SkOrderedWriteBuffer::setFactoryRecorder(SkFactorySet* rec) {
    SkRefCnt_SafeAssign(fFactorySet, rec);
    if (fNamedFactorySet != NULL) {
        fNamedFactorySet->unref();
        fNamedFactorySet = NULL;
    }
    return rec;
}

SkNamedFactorySet* SkOrderedWriteBuffer::setNamedFactoryRecorder(SkNamedFactorySet* rec) {
    SkRefCnt_SafeAssign(fNamedFactorySet, rec);
    if (fFactorySet != NULL) {
        fFactorySet->unref();
        fFactorySet = NULL;
    }
    return rec;
}

SkRefCntSet* SkOrderedWriteBuffer::setTypefaceRecorder(SkRefCntSet* rec) {
    SkRefCnt_SafeAssign(fTFSet, rec);
    return rec;
}

void SkOrderedWriteBuffer::setBitmapHeap(SkBitmapHeap* bitmapHeap) {
    SkRefCnt_SafeAssign(fBitmapHeap, bitmapHeap);
    if (bitmapHeap != NULL) {
        SkASSERT(NULL == fBitmapEncoder);
        fBitmapEncoder = NULL;
    }
}

void SkOrderedWriteBuffer::setBitmapEncoder(SkPicture::EncodeBitmap bitmapEncoder) {
    fBitmapEncoder = bitmapEncoder;
    if (bitmapEncoder != NULL) {
        SkASSERT(NULL == fBitmapHeap);
        SkSafeUnref(fBitmapHeap);
        fBitmapHeap = NULL;
    }
}

void SkOrderedWriteBuffer::writeFlattenable(SkFlattenable* flattenable) {
    










    SkFlattenable::Factory factory = NULL;
    if (flattenable) {
        factory = flattenable->getFactory();
    }
    if (NULL == factory) {
        if (fFactorySet != NULL || fNamedFactorySet != NULL) {
            this->write32(0);
        } else {
            this->writeFunctionPtr(NULL);
        }
        return;
    }

    











    if (fFactorySet) {
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
    
    uint32_t offset = fWriter.size();
    
    flattenObject(flattenable, *this);
    uint32_t objSize = fWriter.size() - offset;
    
    *fWriter.peek32(offset - sizeof(uint32_t)) = objSize;
}
