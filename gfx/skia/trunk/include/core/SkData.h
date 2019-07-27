









#ifndef SkData_DEFINED
#define SkData_DEFINED

#include "SkRefCnt.h"

struct SkFILE;






class SK_API SkData : public SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(SkData)

    


    size_t size() const { return fSize; }

    bool isEmpty() const { return 0 == fSize; }

    


    const void* data() const { return fPtr; }

    



    const uint8_t* bytes() const {
        return reinterpret_cast<const uint8_t*>(fPtr);
    }

    





    size_t copyRange(size_t offset, size_t length, void* buffer) const;

    



    bool equals(const SkData* other) const;

    



    typedef void (*ReleaseProc)(const void* ptr, size_t length, void* context);

    


    static SkData* NewWithCopy(const void* data, size_t length);

    





    static SkData* NewWithCString(const char cstr[]);

    



    static SkData* NewWithProc(const void* data, size_t length,
                               ReleaseProc proc, void* context);

    



    static SkData* NewFromMalloc(const void* data, size_t length);

    



    static SkData* NewFromFileName(const char path[]);

    






    static SkData* NewFromFILE(SkFILE* f);

    






    static SkData* NewFromFD(int fd);

    



    static SkData* NewSubset(const SkData* src, size_t offset, size_t length);

    



    static SkData* NewEmpty();

private:
    ReleaseProc fReleaseProc;
    void*       fReleaseProcContext;

    const void* fPtr;
    size_t      fSize;

    SkData(const void* ptr, size_t size, ReleaseProc, void* context);
    virtual ~SkData();

    
    static SkData* NewEmptyImpl();
    static void DeleteEmpty(SkData*);

    typedef SkRefCnt INHERITED;
};


typedef SkAutoTUnref<SkData> SkAutoDataUnref;

#endif
