






#ifndef SkQuadTree_DEFINED
#define SkQuadTree_DEFINED

#include "SkRect.h"
#include "SkTDArray.h"
#include "SkBBoxHierarchy.h"
#include "SkTInternalSList.h"
#include "SkTObjectPool.h"









class SkQuadTree : public SkBBoxHierarchy {
public:
    SK_DECLARE_INST_COUNT(SkQuadTree)

    





    SkQuadTree(const SkIRect& bounds);

    virtual ~SkQuadTree();

    







    virtual void insert(void* data, const SkIRect& bounds, bool defer = false) SK_OVERRIDE;

    


    virtual void flushDeferredInserts() SK_OVERRIDE;

    


    virtual void search(const SkIRect& query, SkTDArray<void*>* results) SK_OVERRIDE;

    virtual void clear() SK_OVERRIDE;

    


    virtual int getDepth() const SK_OVERRIDE;

    


    virtual int getCount() const SK_OVERRIDE {
        return fEntryPool.allocated() - fEntryPool.available();
    }

    virtual void rewindInserts() SK_OVERRIDE;

private:
    struct Entry {
        Entry() : fData(NULL) {}
        SkIRect fBounds;
        void* fData;
        SK_DECLARE_INTERNAL_SLIST_INTERFACE(Entry);
    };

    static const int kChildCount = 4;

    struct Node {
        Node() {
            for (int index=0; index<kChildCount; ++index) {
                fChildren[index] = NULL;
            }
        }
        SkTInternalSList<Entry> fEntries;
        SkIRect fBounds;
        SkIPoint fSplitPoint; 
        Node* fChildren[kChildCount];
        SK_DECLARE_INTERNAL_SLIST_ADAPTER(Node, fChildren[0]);
    };

    SkTObjectPool<Entry> fEntryPool;
    SkTObjectPool<Node> fNodePool;
    Node* fRoot;
    SkIRect fRootBounds;
    SkTInternalSList<Entry> fDeferred;

    void insert(Node* node, Entry* entry);
    void split(Node* node);
    void search(Node* node, const SkIRect& query, SkTDArray<void*>* results) const;
    void clear(Node* node);
    int getDepth(Node* node) const;

    typedef SkBBoxHierarchy INHERITED;
};

#endif
