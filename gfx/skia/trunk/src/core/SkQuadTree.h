






#ifndef SkQuadTree_DEFINED
#define SkQuadTree_DEFINED

#include "SkRect.h"
#include "SkTDArray.h"
#include "SkBBoxHierarchy.h"









class SkQuadTree : public SkBBoxHierarchy {
public:
    SK_DECLARE_INST_COUNT(SkQuadTree)

    


    static SkQuadTree* Create(const SkIRect& bounds);
    virtual ~SkQuadTree();

    







    virtual void insert(void* data, const SkIRect& bounds, bool defer = false) SK_OVERRIDE;

    


    virtual void flushDeferredInserts() SK_OVERRIDE {}

    


    virtual void search(const SkIRect& query, SkTDArray<void*>* results) SK_OVERRIDE;

    virtual void clear() SK_OVERRIDE;

    


    virtual int getDepth() const SK_OVERRIDE;

    


    virtual int getCount() const SK_OVERRIDE { return fCount; }

    virtual void rewindInserts() SK_OVERRIDE;

private:
    class QuadTreeNode;

    SkQuadTree(const SkIRect& bounds);

    
    int fCount;

    QuadTreeNode* fRoot;

    typedef SkBBoxHierarchy INHERITED;
};

#endif
