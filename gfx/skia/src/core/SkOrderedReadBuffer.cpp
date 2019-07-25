







#include "SkOrderedReadBuffer.h"
#include "SkTypeface.h"


SkOrderedReadBuffer::SkOrderedReadBuffer(const void* data, size_t size)
        : INHERITED() {
    fReader.setMemory(data, size);
}

SkTypeface* SkOrderedReadBuffer::readTypeface() {
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

SkRefCnt* SkOrderedReadBuffer::readRefCnt() {
    uint32_t index = fReader.readU32();
    if (0 == index || index > (unsigned)fRCCount) {
        return NULL;
    } else {
        SkASSERT(fRCArray);
        return fRCArray[index - 1];
    }
}

SkFlattenable* SkOrderedReadBuffer::readFlattenable() {
    SkFlattenable::Factory factory = NULL;

    if (fFactoryCount > 0) {
        int32_t index = fReader.readU32();
        if (0 == index) {
            return NULL; 
        }
        index = -index; 
        index -= 1;     
        SkASSERT(index < fFactoryCount);
        factory = fFactoryArray[index];
    } else if (fFactoryTDArray) {
        const int32_t* peek = (const int32_t*)fReader.peek();
        if (*peek <= 0) {
            int32_t index = fReader.readU32();
            if (0 == index) {
                return NULL; 
            }
            index = -index; 
            index -= 1;     
            factory = (*fFactoryTDArray)[index];
        } else {
            const char* name = fReader.readString();
            factory = SkFlattenable::NameToFactory(name);
            if (factory) {
                SkASSERT(fFactoryTDArray->find(factory) < 0);
                *fFactoryTDArray->append() = factory;
            } else {

            }
            
            
        }
    } else {
        factory = (SkFlattenable::Factory)readFunctionPtr();
        if (NULL == factory) {
            return NULL; 
        }
    }

    
    
    SkFlattenable* obj = NULL;
    uint32_t sizeRecorded = fReader.readU32();
    if (factory) {
        uint32_t offset = fReader.offset();
        obj = (*factory)(*this);
        
        uint32_t sizeRead = fReader.offset() - offset;
        if (sizeRecorded != sizeRead) {
            
            sk_throw();
        }
    } else {
        
        fReader.skip(sizeRecorded);
    }
    return obj;
}

void* SkOrderedReadBuffer::readFunctionPtr() {
    void* proc;
    fReader.read(&proc, sizeof(proc));
    return proc;
}
