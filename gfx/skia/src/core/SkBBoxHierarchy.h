







#ifndef SkBBoxHierarchy_DEFINED
#define SkBBoxHierarchy_DEFINED

#include "SkRect.h"
#include "SkTDArray.h"
#include "SkRefCnt.h"





class SkBBoxHierarchy : public SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(SkBBoxHierarchy)

    








    virtual void insert(void* data, const SkIRect& bounds, bool defer = false) = 0;

    


    virtual void flushDeferredInserts() = 0;

    


    virtual void search(const SkIRect& query, SkTDArray<void*>* results) = 0;

    virtual void clear() = 0;

    


    virtual int getCount() const = 0;

private:
    typedef SkRefCnt INHERITED;
};

#endif

