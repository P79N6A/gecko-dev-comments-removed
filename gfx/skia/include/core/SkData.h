









#ifndef SkData_DEFINED
#define SkData_DEFINED

#include "SkRefCnt.h"






class SkData : public SkRefCnt {
public:
    


    size_t size() const { return fSize; }

    


    const void* data() const { return fPtr; }

    



    const uint8_t* bytes() const {
        return reinterpret_cast<const uint8_t*>(fPtr);
    }

    





    size_t copyRange(size_t offset, size_t length, void* buffer) const;

    



    typedef void (*ReleaseProc)(const void* ptr, size_t length, void* context);
    
    


    static SkData* NewWithCopy(const void* data, size_t length);

    



    static SkData* NewWithProc(const void* data, size_t length,
                               ReleaseProc proc, void* context);

    



    static SkData* NewFromMalloc(const void* data, size_t length);

    



    static SkData* NewSubset(const SkData* src, size_t offset, size_t length);

    



    static SkData* NewEmpty();

private:
    ReleaseProc fReleaseProc;
    void*       fReleaseProcContext;

    const void* fPtr;
    size_t      fSize;

    SkData(const void* ptr, size_t size, ReleaseProc, void* context);
    ~SkData();
};





class SkAutoDataUnref : SkNoncopyable {
public:
    SkAutoDataUnref(SkData* data) : fRef(data) {
        if (data) {
            fData = data->data();
            fSize = data->size();
        } else {
            fData = NULL;
            fSize = 0;
        }
    }
    ~SkAutoDataUnref() {
        SkSafeUnref(fRef);
    }

    const void* data() const { return fData; }
    const uint8_t* bytes() const {
        return reinterpret_cast<const uint8_t*> (fData);
    }
    size_t size() const { return fSize; }
    SkData* get() const { return fRef; }

    void release() {
        if (fRef) {
            fRef->unref();
            fRef = NULL;
            fData = NULL;
            fSize = 0;
        }
    }

private:
    SkData*     fRef;
    const void* fData;
    size_t      fSize;
};

#endif
