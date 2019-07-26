







#ifndef SkRTree_DEFINED
#define SkRTree_DEFINED

#include "SkRect.h"
#include "SkTDArray.h"
#include "SkChunkAlloc.h"
#include "SkBBoxHierarchy.h"



























class SkRTree : public SkBBoxHierarchy {
public:
    SK_DECLARE_INST_COUNT(SkRTree)

    










    static SkRTree* Create(int minChildren, int maxChildren, SkScalar aspectRatio = 1);
    virtual ~SkRTree();

    







    virtual void insert(void* data, const SkIRect& bounds, bool defer = false);

    


    virtual void flushDeferredInserts();

    


    virtual void search(const SkIRect& query, SkTDArray<void*>* results);

    virtual void clear();
    bool isEmpty() const { return 0 == fCount; }
    int getDepth() const { return this->isEmpty() ? 0 : fRoot.fChild.subtree->fLevel + 1; }

    


    virtual int getCount() const { return fCount; }

private:

    struct Node;

    


    struct Branch {
        union {
            Node* subtree;
            void* data;
        } fChild;
        SkIRect fBounds;
    };

    


    struct Node {
        uint16_t fNumChildren;
        uint16_t fLevel;
        bool isLeaf() { return 0 == fLevel; }
        
        
        Branch* child(size_t index) {
            return reinterpret_cast<Branch*>(this + 1) + index;
        }
    };

    typedef int32_t SkIRect::*SortSide;

    
    static bool RectLessThan(SortSide const& side, const Branch lhs, const Branch rhs) {
        return lhs.fBounds.*side < rhs.fBounds.*side;
    }

    static bool RectLessX(int&, const Branch lhs, const Branch rhs) {
        return ((lhs.fBounds.fRight - lhs.fBounds.fLeft) >> 1) <
               ((rhs.fBounds.fRight - lhs.fBounds.fLeft) >> 1);
    }

    static bool RectLessY(int&, const Branch lhs, const Branch rhs) {
        return ((lhs.fBounds.fBottom - lhs.fBounds.fTop) >> 1) <
               ((rhs.fBounds.fBottom - lhs.fBounds.fTop) >> 1);
    }

    SkRTree(int minChildren, int maxChildren, SkScalar aspectRatio);

    



    Branch* insert(Node* root, Branch* branch, uint16_t level = 0);

    int chooseSubtree(Node* root, Branch* branch);
    SkIRect computeBounds(Node* n);
    int distributeChildren(Branch* children);
    void search(Node* root, const SkIRect query, SkTDArray<void*>* results) const;

    










    Branch bulkLoad(SkTDArray<Branch>* branches, int level = 0);

    void validate();
    int validateSubtree(Node* root, SkIRect bounds, bool isRoot = false);

    const int fMinChildren;
    const int fMaxChildren;
    const size_t fNodeSize;

    
    size_t fCount;

    Branch fRoot;
    SkChunkAlloc fNodes;
    SkTDArray<Branch> fDeferredInserts;
    SkScalar fAspectRatio;

    Node* allocateNode(uint16_t level);

    typedef SkBBoxHierarchy INHERITED;
};

#endif

