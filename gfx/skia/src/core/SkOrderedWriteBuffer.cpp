







#include "SkOrderedWriteBuffer.h"
#include "SkTypeface.h"

SkOrderedWriteBuffer::SkOrderedWriteBuffer(size_t minSize) :
    INHERITED(),
    fWriter(minSize) {
}

SkOrderedWriteBuffer::SkOrderedWriteBuffer(size_t minSize, void* storage, size_t storageSize) :
    INHERITED(),
    fWriter(minSize, storage, storageSize) {
}

void SkOrderedWriteBuffer::writeFlattenable(SkFlattenable* flattenable) {
    











    SkFlattenable::Factory factory = NULL;
    if (flattenable) {
        factory = flattenable->getFactory();
    }
    if (NULL == factory) {
        if (fFactorySet) {
            this->write32(0);
        } else {
            this->writeFunctionPtr(NULL);
        }
        return;
    }

    












    if (fFactorySet) {
        if (this->inlineFactoryNames()) {
            int index = fFactorySet->find(factory);
            if (index) {
                
                
                this->write32(-index);
            } else {
                const char* name = SkFlattenable::FactoryToName(factory);
                if (NULL == name) {
                    this->write32(0);
                    return;
                }
                this->writeString(name);
                index = fFactorySet->add(factory);
            }
        } else {
            
            
            this->write32(-(int)fFactorySet->add(factory));
        }
    } else {
        this->writeFunctionPtr((void*)factory);
    }

    
    (void)this->reserve(sizeof(uint32_t));
    
    uint32_t offset = this->size();
    
    flattenObject(flattenable, *this);
    uint32_t objSize = this->size() - offset;
    
    *fWriter.peek32(offset - sizeof(uint32_t)) = objSize;
}

void SkOrderedWriteBuffer::writeFunctionPtr(void* proc) {
    *(void**)this->reserve(sizeof(void*)) = proc;
}
