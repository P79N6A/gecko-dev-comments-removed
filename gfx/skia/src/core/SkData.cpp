









#include "SkData.h"

SkData::SkData(const void* ptr, size_t size, ReleaseProc proc, void* context) {
    fPtr = ptr;
    fSize = size;
    fReleaseProc = proc;
    fReleaseProcContext = context;
}

SkData::~SkData() {
    if (fReleaseProc) {
        fReleaseProc(fPtr, fSize, fReleaseProcContext);
    }
}

size_t SkData::copyRange(size_t offset, size_t length, void* buffer) const {
    size_t available = fSize;
    if (offset >= available || 0 == length) {
        return 0;
    }
    available -= offset;
    if (length > available) {
        length = available;
    }
    SkASSERT(length > 0);

    memcpy(buffer, this->bytes() + offset, length);
    return length;
}



SkData* SkData::NewEmpty() {
    static SkData* gEmptyRef;
    if (NULL == gEmptyRef) {
        gEmptyRef = new SkData(NULL, 0, NULL, NULL);
    }
    gEmptyRef->ref();
    return gEmptyRef;
}


static void sk_free_releaseproc(const void* ptr, size_t, void*) {
    sk_free((void*)ptr);
}

SkData* SkData::NewFromMalloc(const void* data, size_t length) {
    return new SkData(data, length, sk_free_releaseproc, NULL);
}

SkData* SkData::NewWithCopy(const void* data, size_t length) {
    if (0 == length) {
        return SkData::NewEmpty();
    }

    void* copy = sk_malloc_throw(length); 
    memcpy(copy, data, length);
    return new SkData(copy, length, sk_free_releaseproc, NULL);
}

SkData* SkData::NewWithProc(const void* data, size_t length,
                            ReleaseProc proc, void* context) {
    return new SkData(data, length, proc, context);
}


static void sk_dataref_releaseproc(const void*, size_t, void* context) {
    SkData* src = reinterpret_cast<SkData*>(context);
    src->unref();
}

SkData* SkData::NewSubset(const SkData* src, size_t offset, size_t length) {
    





    size_t available = src->size();
    if (offset >= available || 0 == length) {
        return SkData::NewEmpty();
    }
    available -= offset;
    if (length > available) {
        length = available;
    }
    SkASSERT(length > 0);

    src->ref(); 
    return new SkData(src->bytes() + offset, length, sk_dataref_releaseproc,
                         const_cast<SkData*>(src));
}

