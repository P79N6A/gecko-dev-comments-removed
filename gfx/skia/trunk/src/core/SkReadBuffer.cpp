







#include "SkBitmap.h"
#include "SkErrorInternals.h"
#include "SkReadBuffer.h"
#include "SkStream.h"
#include "SkTypeface.h"

static uint32_t default_flags() {
    uint32_t flags = 0;
    flags |= SkReadBuffer::kScalarIsFloat_Flag;
    if (8 == sizeof(void*)) {
        flags |= SkReadBuffer::kPtrIs64Bit_Flag;
    }
    return flags;
}

SkReadBuffer::SkReadBuffer() {
    fFlags = default_flags();
    fVersion = 0;
    fMemoryPtr = NULL;

    fBitmapStorage = NULL;
    fTFArray = NULL;
    fTFCount = 0;

    fFactoryTDArray = NULL;
    fFactoryArray = NULL;
    fFactoryCount = 0;
    fBitmapDecoder = NULL;
#ifdef DEBUG_NON_DETERMINISTIC_ASSERT
    fDecodedBitmapIndex = -1;
#endif 
}

SkReadBuffer::SkReadBuffer(const void* data, size_t size) {
    fFlags = default_flags();
    fVersion = 0;
    fReader.setMemory(data, size);
    fMemoryPtr = NULL;

    fBitmapStorage = NULL;
    fTFArray = NULL;
    fTFCount = 0;

    fFactoryTDArray = NULL;
    fFactoryArray = NULL;
    fFactoryCount = 0;
    fBitmapDecoder = NULL;
#ifdef DEBUG_NON_DETERMINISTIC_ASSERT
    fDecodedBitmapIndex = -1;
#endif 
}

SkReadBuffer::SkReadBuffer(SkStream* stream) {
    fFlags = default_flags();
    fVersion = 0;
    const size_t length = stream->getLength();
    fMemoryPtr = sk_malloc_throw(length);
    stream->read(fMemoryPtr, length);
    fReader.setMemory(fMemoryPtr, length);

    fBitmapStorage = NULL;
    fTFArray = NULL;
    fTFCount = 0;

    fFactoryTDArray = NULL;
    fFactoryArray = NULL;
    fFactoryCount = 0;
    fBitmapDecoder = NULL;
#ifdef DEBUG_NON_DETERMINISTIC_ASSERT
    fDecodedBitmapIndex = -1;
#endif 
}

SkReadBuffer::~SkReadBuffer() {
    sk_free(fMemoryPtr);
    SkSafeUnref(fBitmapStorage);
}

bool SkReadBuffer::readBool() {
    return fReader.readBool();
}

SkColor SkReadBuffer::readColor() {
    return fReader.readInt();
}

SkFixed SkReadBuffer::readFixed() {
    return fReader.readS32();
}

int32_t SkReadBuffer::readInt() {
    return fReader.readInt();
}

SkScalar SkReadBuffer::readScalar() {
    return fReader.readScalar();
}

uint32_t SkReadBuffer::readUInt() {
    return fReader.readU32();
}

int32_t SkReadBuffer::read32() {
    return fReader.readInt();
}

void SkReadBuffer::readString(SkString* string) {
    size_t len;
    const char* strContents = fReader.readString(&len);
    string->set(strContents, len);
}

void* SkReadBuffer::readEncodedString(size_t* length, SkPaint::TextEncoding encoding) {
    SkDEBUGCODE(int32_t encodingType = ) fReader.readInt();
    SkASSERT(encodingType == encoding);
    *length =  fReader.readInt();
    void* data = sk_malloc_throw(*length);
    memcpy(data, fReader.skip(SkAlign4(*length)), *length);
    return data;
}

void SkReadBuffer::readPoint(SkPoint* point) {
    point->fX = fReader.readScalar();
    point->fY = fReader.readScalar();
}

void SkReadBuffer::readMatrix(SkMatrix* matrix) {
    fReader.readMatrix(matrix);
}

void SkReadBuffer::readIRect(SkIRect* rect) {
    memcpy(rect, fReader.skip(sizeof(SkIRect)), sizeof(SkIRect));
}

void SkReadBuffer::readRect(SkRect* rect) {
    memcpy(rect, fReader.skip(sizeof(SkRect)), sizeof(SkRect));
}

void SkReadBuffer::readRegion(SkRegion* region) {
    fReader.readRegion(region);
}

void SkReadBuffer::readPath(SkPath* path) {
    fReader.readPath(path);
}

bool SkReadBuffer::readArray(void* value, size_t size, size_t elementSize) {
    const size_t count = this->getArrayCount();
    if (count == size) {
        (void)fReader.skip(sizeof(uint32_t)); 
        const size_t byteLength = count * elementSize;
        memcpy(value, fReader.skip(SkAlign4(byteLength)), byteLength);
        return true;
    }
    SkASSERT(false);
    fReader.skip(fReader.available());
    return false;
}

bool SkReadBuffer::readByteArray(void* value, size_t size) {
    return readArray(static_cast<unsigned char*>(value), size, sizeof(unsigned char));
}

bool SkReadBuffer::readColorArray(SkColor* colors, size_t size) {
    return readArray(colors, size, sizeof(SkColor));
}

bool SkReadBuffer::readIntArray(int32_t* values, size_t size) {
    return readArray(values, size, sizeof(int32_t));
}

bool SkReadBuffer::readPointArray(SkPoint* points, size_t size) {
    return readArray(points, size, sizeof(SkPoint));
}

bool SkReadBuffer::readScalarArray(SkScalar* values, size_t size) {
    return readArray(values, size, sizeof(SkScalar));
}

uint32_t SkReadBuffer::getArrayCount() {
    return *(uint32_t*)fReader.peek();
}

bool SkReadBuffer::readBitmap(SkBitmap* bitmap) {
    const int width = this->readInt();
    const int height = this->readInt();
    
    
    if (this->readBool()) {
        
        
        const uint32_t index = this->readUInt();
        this->readUInt(); 
        if (fBitmapStorage) {
            *bitmap = *fBitmapStorage->getBitmap(index);
            fBitmapStorage->releaseRef(index);
            return true;
        } else {
            
            
            SkErrorInternals::SetError(kParseError_SkError, "SkWriteBuffer::writeBitmap "
                                       "stored the SkBitmap in an SkBitmapHeap, but "
                                       "SkReadBuffer has no SkBitmapHeapReader to "
                                       "retrieve the SkBitmap.");
        }
    } else {
        
        const size_t length = this->readUInt();
        if (length > 0) {
#ifdef DEBUG_NON_DETERMINISTIC_ASSERT
            fDecodedBitmapIndex++;
#endif 
            
            
            const void* data = this->skip(length);
            const int32_t xOffset = this->readInt();
            const int32_t yOffset = this->readInt();
            if (fBitmapDecoder != NULL && fBitmapDecoder(data, length, bitmap)) {
                if (bitmap->width() == width && bitmap->height() == height) {
#ifdef DEBUG_NON_DETERMINISTIC_ASSERT
                    if (0 != xOffset || 0 != yOffset) {
                        SkDebugf("SkReadBuffer::readBitmap: heights match,"
                                 " but offset is not zero. \nInfo about the bitmap:"
                                 "\n\tIndex: %d\n\tDimensions: [%d %d]\n\tEncoded"
                                 " data size: %d\n\tOffset: (%d, %d)\n",
                                 fDecodedBitmapIndex, width, height, length, xOffset,
                                 yOffset);
                    }
#endif 
                    
                    SkASSERT(0 == xOffset && 0 == yOffset);
                    return true;
                }

                
                
                
                
                
                
                
                

                SkBitmap subsetBm;
                SkIRect subset = SkIRect::MakeXYWH(xOffset, yOffset, width, height);
                if (bitmap->extractSubset(&subsetBm, subset)) {
                    bitmap->swap(subsetBm);
                    return true;
                }
            }
            
            
            SkErrorInternals::SetError(kParseError_SkError,
                                       "Could not decode bitmap. Resulting bitmap will be red.");
        } else {
            
            if (this->isVersionLT(kNoMoreBitmapFlatten_Version)) {
                SkBitmap tmp;
                tmp.legacyUnflatten(*this);
                
            } else {
                if (SkBitmap::ReadRawPixels(this, bitmap)) {
                    return true;
                }
            }
        }
    }
    
    bitmap->setInfo(SkImageInfo::MakeUnknown(width, height));
    return false;
}

SkTypeface* SkReadBuffer::readTypeface() {

    uint32_t index = fReader.readU32();
    if (0 == index || index > (unsigned)fTFCount) {
        if (index) {
            SkDebugf("====== typeface index %d\n", index);
        }
        return NULL;
    } else {
        SkASSERT(fTFArray);
        return fTFArray[index - 1];
    }
}

SkFlattenable* SkReadBuffer::readFlattenable(SkFlattenable::Type ft) {
    
    
    

    SkFlattenable::Factory factory = NULL;

    if (fFactoryCount > 0) {
        int32_t index = fReader.readU32();
        if (0 == index) {
            return NULL; 
        }
        index -= 1;     
        SkASSERT(index < fFactoryCount);
        factory = fFactoryArray[index];
    } else if (fFactoryTDArray) {
        int32_t index = fReader.readU32();
        if (0 == index) {
            return NULL; 
        }
        index -= 1;     
        factory = (*fFactoryTDArray)[index];
    } else {
        factory = (SkFlattenable::Factory)readFunctionPtr();
        if (NULL == factory) {
            return NULL; 
        }
    }

    
    
    SkFlattenable* obj = NULL;
    uint32_t sizeRecorded = fReader.readU32();
    if (factory) {
        size_t offset = fReader.offset();
        obj = (*factory)(*this);
        
        size_t sizeRead = fReader.offset() - offset;
        if (sizeRecorded != sizeRead) {
            
            sk_throw();
        }
    } else {
        
        fReader.skip(sizeRecorded);
    }
    return obj;
}





void SkReadBuffer::skipFlattenable() {
    if (fFactoryCount > 0) {
        if (0 == fReader.readU32()) {
            return;
        }
    } else if (fFactoryTDArray) {
        if (0 == fReader.readU32()) {
            return;
        }
    } else {
        if (NULL == this->readFunctionPtr()) {
            return;
        }
    }
    uint32_t sizeRecorded = fReader.readU32();
    fReader.skip(sizeRecorded);
}
