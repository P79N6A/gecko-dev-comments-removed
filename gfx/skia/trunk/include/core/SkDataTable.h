






#ifndef SkDataTable_DEFINED
#define SkDataTable_DEFINED

#include "SkChunkAlloc.h"
#include "SkData.h"
#include "SkString.h"
#include "SkTDArray.h"






class SK_API SkDataTable : public SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(SkDataTable)

    


    bool isEmpty() const { return 0 == fCount; }

    


    int count() const { return fCount; }

    



    size_t atSize(int index) const;

    






    const void* at(int index, size_t* size = NULL) const;

    template <typename T>
    const T* atT(int index, size_t* size = NULL) const {
        return reinterpret_cast<const T*>(this->at(index, size));
    }

    



    const char* atStr(int index) const {
        size_t size;
        const char* str = this->atT<const char>(index, &size);
        SkASSERT(strlen(str) + 1 == size);
        return str;
    }

    typedef void (*FreeProc)(void* context);

    static SkDataTable* NewEmpty();

    








    static SkDataTable* NewCopyArrays(const void * const * ptrs,
                                      const size_t sizes[], int count);

    







    static SkDataTable* NewCopyArray(const void* array, size_t elemSize,
                                     int count);

    static SkDataTable* NewArrayProc(const void* array, size_t elemSize,
                                     int count, FreeProc proc, void* context);

private:
    struct Dir {
        const void* fPtr;
        uintptr_t   fSize;
    };

    int         fCount;
    size_t      fElemSize;
    union {
        const Dir*  fDir;
        const char* fElems;
    } fU;

    FreeProc    fFreeProc;
    void*       fFreeProcContext;

    SkDataTable();
    SkDataTable(const void* array, size_t elemSize, int count,
                FreeProc, void* context);
    SkDataTable(const Dir*, int count, FreeProc, void* context);
    virtual ~SkDataTable();

    friend class SkDataTableBuilder;    

    typedef SkRefCnt INHERITED;
};





class SK_API SkDataTableBuilder : SkNoncopyable {
public:
    SkDataTableBuilder(size_t minChunkSize);
    ~SkDataTableBuilder();

    int  count() const { return fDir.count(); }
    size_t minChunkSize() const { return fMinChunkSize; }

    


    void reset(size_t minChunkSize);
    void reset() {
        this->reset(fMinChunkSize);
    }

    


    void append(const void* data, size_t size);

    



    void appendStr(const char str[]) {
        this->append(str, strlen(str) + 1);
    }

    



    void appendString(const SkString& string) {
        this->append(string.c_str(), string.size() + 1);
    }

    




    SkDataTable* detachDataTable();

private:
    SkTDArray<SkDataTable::Dir> fDir;
    SkChunkAlloc*               fHeap;
    size_t                      fMinChunkSize;
};

#endif
