






#include "SkFlattenable.h"
#include "SkTypeface.h"

#include "SkMatrix.h"
#include "SkRegion.h"

void SkReadMatrix(SkReader32* reader, SkMatrix* matrix) {
    size_t size = matrix->unflatten(reader->peek());
    SkASSERT(SkAlign4(size) == size);
    (void)reader->skip(size);
}

void SkWriteMatrix(SkWriter32* writer, const SkMatrix& matrix) {
    size_t size = matrix.flatten(NULL);
    SkASSERT(SkAlign4(size) == size);
    matrix.flatten(writer->reserve(size));
}

void SkReadRegion(SkReader32* reader, SkRegion* rgn) {
    size_t size = rgn->unflatten(reader->peek());
    SkASSERT(SkAlign4(size) == size);
    (void)reader->skip(size);
}

void SkWriteRegion(SkWriter32* writer, const SkRegion& rgn) {
    size_t size = rgn.flatten(NULL);
    SkASSERT(SkAlign4(size) == size);
    rgn.flatten(writer->reserve(size));
}



void SkFlattenable::flatten(SkFlattenableWriteBuffer&)
{
    



}




SkFlattenableReadBuffer::SkFlattenableReadBuffer() {
    fRCArray = NULL;
    fRCCount = 0;

    fTFArray = NULL;
    fTFCount = 0;

    fFactoryTDArray = NULL;
    fFactoryArray = NULL;
    fFactoryCount = 0;
}

SkFlattenableReadBuffer::SkFlattenableReadBuffer(const void* data) :
        INHERITED(data, 1024 * 1024) {
    fRCArray = NULL;
    fRCCount = 0;

    fTFArray = NULL;
    fTFCount = 0;

    fFactoryTDArray = NULL;
    fFactoryArray = NULL;
    fFactoryCount = 0;
}

SkFlattenableReadBuffer::SkFlattenableReadBuffer(const void* data, size_t size)
        : INHERITED(data, size) {
    fRCArray = NULL;
    fRCCount = 0;

    fTFArray = NULL;
    fTFCount = 0;

    fFactoryTDArray = NULL;
    fFactoryArray = NULL;
    fFactoryCount = 0;
}

SkTypeface* SkFlattenableReadBuffer::readTypeface() {
    uint32_t index = this->readU32();
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

SkRefCnt* SkFlattenableReadBuffer::readRefCnt() {
    uint32_t index = this->readU32();
    if (0 == index || index > (unsigned)fRCCount) {
        return NULL;
    } else {
        SkASSERT(fRCArray);
        return fRCArray[index - 1];
    }
}

SkFlattenable* SkFlattenableReadBuffer::readFlattenable() {
    SkFlattenable::Factory factory = NULL;

    if (fFactoryCount > 0) {
        int32_t index = this->readU32();
        if (0 == index) {
            return NULL; 
        }
        index = -index; 
        index -= 1;     
        SkASSERT(index < fFactoryCount);
        factory = fFactoryArray[index];
    } else if (fFactoryTDArray) {
        const int32_t* peek = (const int32_t*)this->peek();
        if (*peek <= 0) {
            int32_t index = this->readU32();
            if (0 == index) {
                return NULL; 
            }
            index = -index; 
            index -= 1;     
            factory = (*fFactoryTDArray)[index];
        } else {
            const char* name = this->readString();
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
    uint32_t sizeRecorded = this->readU32();
    if (factory) {
        uint32_t offset = this->offset();
        obj = (*factory)(*this);
        
        uint32_t sizeRead = this->offset() - offset;
        if (sizeRecorded != sizeRead) {
            
            sk_throw();
        }
    } else {
        
        this->skip(sizeRecorded);
    }
    return obj;
}

void* SkFlattenableReadBuffer::readFunctionPtr() {
    void* proc;
    this->read(&proc, sizeof(proc));
    return proc;
}



SkFlattenableWriteBuffer::SkFlattenableWriteBuffer(size_t minSize) :
        INHERITED(minSize) {
    fFlags = (Flags)0;
    fRCSet = NULL;
    fTFSet = NULL;
    fFactorySet = NULL;
}

SkFlattenableWriteBuffer::~SkFlattenableWriteBuffer() {
    SkSafeUnref(fRCSet);
    SkSafeUnref(fTFSet);
    SkSafeUnref(fFactorySet);
}

SkRefCntSet* SkFlattenableWriteBuffer::setRefCntRecorder(SkRefCntSet* rec) {
    SkRefCnt_SafeAssign(fRCSet, rec);
    return rec;
}

SkRefCntSet* SkFlattenableWriteBuffer::setTypefaceRecorder(SkRefCntSet* rec) {
    SkRefCnt_SafeAssign(fTFSet, rec);
    return rec;
}

SkFactorySet* SkFlattenableWriteBuffer::setFactoryRecorder(SkFactorySet* rec) {
    SkRefCnt_SafeAssign(fFactorySet, rec);
    return rec;
}

void SkFlattenableWriteBuffer::writeTypeface(SkTypeface* obj) {
    if (NULL == obj || NULL == fTFSet) {
        this->write32(0);
    } else {
        this->write32(fTFSet->add(obj));
    }
}

void SkFlattenableWriteBuffer::writeRefCnt(SkRefCnt* obj) {
    if (NULL == obj || NULL == fRCSet) {
        this->write32(0);
    } else {
        this->write32(fRCSet->add(obj));
    }
}

void SkFlattenableWriteBuffer::writeFlattenable(SkFlattenable* flattenable) {
    











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
    
    flattenable->flatten(*this);
    uint32_t objSize = this->size() - offset;
    
    *this->peek32(offset - sizeof(uint32_t)) = objSize;
}

void SkFlattenableWriteBuffer::writeFunctionPtr(void* proc) {
    *(void**)this->reserve(sizeof(void*)) = proc;
}



SkRefCntSet::~SkRefCntSet() {
    
    this->reset();
}

void SkRefCntSet::incPtr(void* ptr) {
    ((SkRefCnt*)ptr)->ref();
}

void SkRefCntSet::decPtr(void* ptr) {
    ((SkRefCnt*)ptr)->unref();
}





#define MAX_PAIR_COUNT  64

struct Pair {
    const char*             fName;
    SkFlattenable::Factory  fFactory;
};

static int gCount;
static Pair gPairs[MAX_PAIR_COUNT];

void SkFlattenable::Register(const char name[], Factory factory) {
    SkASSERT(name);
    SkASSERT(factory);

    static bool gOnce;
    if (!gOnce) {
        gCount = 0;
        gOnce = true;
    }

    SkASSERT(gCount < MAX_PAIR_COUNT);

    gPairs[gCount].fName = name;
    gPairs[gCount].fFactory = factory;
    gCount += 1;
}

SkFlattenable::Factory SkFlattenable::NameToFactory(const char name[]) {
    const Pair* pairs = gPairs;
    for (int i = gCount - 1; i >= 0; --i) {
        if (strcmp(pairs[i].fName, name) == 0) {
            return pairs[i].fFactory;
        }
    }
    return NULL;
}

const char* SkFlattenable::FactoryToName(Factory fact) {
    const Pair* pairs = gPairs;
    for (int i = gCount - 1; i >= 0; --i) {
        if (pairs[i].fFactory == fact) {
            return pairs[i].fName;
        }
    }
    return NULL;
}

bool SkFlattenable::toDumpString(SkString* str) const {
    return false;
}

