






#ifndef SkPathHeap_DEFINED
#define SkPathHeap_DEFINED

#include "SkRefCnt.h"
#include "SkChunkAlloc.h"
#include "SkTDArray.h"

class SkPath;
class SkFlattenableReadBuffer;
class SkFlattenableWriteBuffer;

class SkPathHeap : public SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(SkPathHeap)

    SkPathHeap();
    SkPathHeap(SkFlattenableReadBuffer&);
    virtual ~SkPathHeap();

    



    int append(const SkPath&);

    
    int count() const { return fPaths.count(); }
    const SkPath& operator[](int index) const {
        return *fPaths[index];
    }

    void flatten(SkFlattenableWriteBuffer&) const;

private:
    
    SkChunkAlloc        fHeap;
    
    SkTDArray<SkPath*>  fPaths;

    typedef SkRefCnt INHERITED;
};

#endif

