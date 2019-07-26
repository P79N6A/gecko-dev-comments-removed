






#include "SkQuadTree.h"
#include "SkTSort.h"
#include <stdio.h>
#include <vector>

class SkQuadTree::QuadTreeNode {
public:
    struct Data {
        Data(const SkIRect& bounds, void* data) : fBounds(bounds), fInnerBounds(bounds), fData(data) {}
        SkIRect fBounds;
        SkIRect fInnerBounds;
        void* fData;
    };

    QuadTreeNode(const SkIRect& bounds)
     : fBounds(bounds)
     , fTopLeft(NULL)
     , fTopRight(NULL)
     , fBottomLeft(NULL)
     , fBottomRight(NULL)
     , fCanSubdivide((fBounds.width() * fBounds.height()) > 0) {}

    ~QuadTreeNode() {
        clear();
    }

    void clear() {
        SkDELETE(fTopLeft);
        fTopLeft = NULL;
        SkDELETE(fTopRight);
        fTopRight = NULL;
        SkDELETE(fBottomLeft);
        fBottomLeft = NULL;
        SkDELETE(fBottomRight);
        fBottomRight = NULL;
        fData.reset();
    }

    const SkIRect& getBounds() const { return fBounds; }

    
    bool insert(Data& data) {
        
        return data.fInnerBounds.intersect(fBounds) && doInsert(data);
    }

    
    void queryRange(const SkIRect& range, SkTDArray<void*>* dataInRange) const {
        
        if (!SkIRect::Intersects(fBounds, range)) {
            return; 
        }

        
        for (int i = 0; i < fData.count(); ++i) {
            if (SkIRect::Intersects(fData[i].fBounds, range)) {
                dataInRange->push(fData[i].fData);
            }
        }

        
        if (!hasChildren()) {
            return;
        }

        
        fTopLeft->queryRange(range, dataInRange);
        fTopRight->queryRange(range, dataInRange);
        fBottomLeft->queryRange(range, dataInRange);
        fBottomRight->queryRange(range, dataInRange);
    }

    int getDepth(int i = 1) const {
        if (hasChildren()) {
            int depthTL = fTopLeft->getDepth(++i);
            int depthTR = fTopRight->getDepth(i);
            int depthBL = fBottomLeft->getDepth(i);
            int depthBR = fBottomRight->getDepth(i);
            return SkTMax(SkTMax(depthTL, depthTR), SkTMax(depthBL, depthBR));
        }
        return i;
    }

    void rewindInserts(SkBBoxHierarchyClient* client) {
        for (int i = fData.count() - 1; i >= 0; --i) {
            if (client->shouldRewind(fData[i].fData)) {
                fData.remove(i);
            }
        }
        if (hasChildren()) {
            fTopLeft->rewindInserts(client);
            fTopRight->rewindInserts(client);
            fBottomLeft->rewindInserts(client);
            fBottomRight->rewindInserts(client);
        }
    }

private:
    
    void subdivide() {
        if (!hasChildren() && fCanSubdivide) {
            SkIPoint center = SkIPoint::Make(fBounds.centerX(), fBounds.centerY());
            fTopLeft = SkNEW_ARGS(QuadTreeNode, (SkIRect::MakeLTRB(
                fBounds.fLeft, fBounds.fTop, center.fX, center.fY)));
            fTopRight = SkNEW_ARGS(QuadTreeNode, (SkIRect::MakeLTRB(
                center.fX, fBounds.fTop, fBounds.fRight, center.fY)));
            fBottomLeft = SkNEW_ARGS(QuadTreeNode, (SkIRect::MakeLTRB(
                fBounds.fLeft, center.fY, center.fX, fBounds.fBottom)));
            fBottomRight = SkNEW_ARGS(QuadTreeNode, (SkIRect::MakeLTRB(
                center.fX, center.fY, fBounds.fRight, fBounds.fBottom)));

            
            for (int i = fData.count() - 1; i >= 0; --i) {
                
                
                if (fTopLeft->doInsert(fData[i]) ||
                    fTopRight->doInsert(fData[i]) ||
                    fBottomLeft->doInsert(fData[i]) ||
                    fBottomRight->doInsert(fData[i])) {
                    fData.remove(i);
                }
            }
        }
    }

    bool doInsert(const Data& data) {
        if (!fBounds.contains(data.fInnerBounds)) {
            return false;
        }

        if (fData.count() > kQuadTreeNodeCapacity) {
            subdivide();
        }

        
        
        if ((fData.count() <= kQuadTreeNodeCapacity) || !fCanSubdivide) {
            if (fData.isEmpty()) {
                fData.setReserve(kQuadTreeNodeCapacity);
            }
            fData.push(data);
        } else if (!fTopLeft->doInsert(data) && !fTopRight->doInsert(data) &&
                   !fBottomLeft->doInsert(data) && !fBottomRight->doInsert(data)) {
            
            fData.push(data);
        }

        return true;
    }

    bool hasChildren() const {
        return (NULL != fTopLeft);
    }

    
    enum { kQuadTreeNodeCapacity = 4 };

    
    SkIRect fBounds;

    
    SkTDArray<Data> fData;

    
    QuadTreeNode* fTopLeft;
    QuadTreeNode* fTopRight;
    QuadTreeNode* fBottomLeft;
    QuadTreeNode* fBottomRight;

    
    bool fCanSubdivide;
};



SkQuadTree* SkQuadTree::Create(const SkIRect& bounds) {
    return new SkQuadTree(bounds);
}

SkQuadTree::SkQuadTree(const SkIRect& bounds)
    : fCount(0)
    , fRoot(SkNEW_ARGS(QuadTreeNode, (bounds))) {
    SkASSERT((bounds.width() * bounds.height()) > 0);
}

SkQuadTree::~SkQuadTree() {
    SkDELETE(fRoot);
}

void SkQuadTree::insert(void* data, const SkIRect& bounds, bool) {
    if (bounds.isEmpty()) {
        SkASSERT(false);
        return;
    }

    QuadTreeNode::Data quadTreeData(bounds, data);
    fRoot->insert(quadTreeData);
    ++fCount;
}

void SkQuadTree::search(const SkIRect& query, SkTDArray<void*>* results) {
    SkASSERT(NULL != results);
    fRoot->queryRange(query, results);
}

void SkQuadTree::clear() {
    fCount = 0;
    fRoot->clear();
}

int SkQuadTree::getDepth() const {
    return fRoot->getDepth();
}

void SkQuadTree::rewindInserts() {
    SkASSERT(fClient);
    fRoot->rewindInserts(fClient);
}
