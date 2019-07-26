







#ifndef SkBBoxHierarchy_DEFINED
#define SkBBoxHierarchy_DEFINED

#include "SkRect.h"
#include "SkTDArray.h"
#include "SkRefCnt.h"






class SkBBoxHierarchyClient {
public:
    virtual ~SkBBoxHierarchyClient() {}

    




    virtual bool shouldRewind(void* data) = 0;
};





class SkBBoxHierarchy : public SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(SkBBoxHierarchy)

    SkBBoxHierarchy() : fClient(NULL) {}

    








    virtual void insert(void* data, const SkIRect& bounds, bool defer = false) = 0;

    


    virtual void flushDeferredInserts() = 0;

    


    virtual void search(const SkIRect& query, SkTDArray<void*>* results) = 0;

    virtual void clear() = 0;

    


    virtual int getCount() const = 0;

    





    virtual void rewindInserts() = 0;

    void setClient(SkBBoxHierarchyClient* client) { fClient = client; }

protected:
    SkBBoxHierarchyClient* fClient;

private:
    typedef SkRefCnt INHERITED;
};

#endif
