






#include "SkClipStack.h"
#include "SkPath.h"
#include "SkThread.h"

#include <new>



int32_t SkClipStack::gGenID = 3;

struct SkClipStack::Rec {
    enum State {
        kEmpty_State,
        kRect_State,
        kPath_State
    };

    SkPath          fPath;
    SkRect          fRect;
    int             fSaveCount;
    SkRegion::Op    fOp;
    State           fState;
    bool            fDoAA;

    
    
    
    
    
    
    
    
    
    
    
    
    
    SkClipStack::BoundsType fFiniteBoundType;
    SkRect                  fFiniteBound;
    bool                    fIsIntersectionOfRects;

    int                     fGenID;

    Rec(int saveCount)
    : fGenID(kInvalidGenID) {
        fSaveCount = saveCount;
        this->setEmpty();
    }

    Rec(int saveCount, const SkRect& rect, SkRegion::Op op, bool doAA)
        : fRect(rect)
        , fGenID(kInvalidGenID) {
        fSaveCount = saveCount;
        fOp = op;
        fState = kRect_State;
        fDoAA = doAA;
        
    }

    Rec(int saveCount, const SkPath& path, SkRegion::Op op, bool doAA)
        : fPath(path)
        , fGenID(kInvalidGenID) {
        fRect.setEmpty();
        fSaveCount = saveCount;
        fOp = op;
        fState = kPath_State;
        fDoAA = doAA;
        
    }

    void setEmpty() {
        fState = kEmpty_State;
        fFiniteBound.setEmpty();
        fFiniteBoundType = kNormal_BoundsType;
        fIsIntersectionOfRects = false;
        fGenID = kEmptyGenID;
    }

    void checkEmpty() {
        SkASSERT(fFiniteBound.isEmpty());
        SkASSERT(kNormal_BoundsType == fFiniteBoundType);
        SkASSERT(!fIsIntersectionOfRects);
        SkASSERT(kEmptyGenID == fGenID);
    }

    bool operator==(const Rec& b) const {
        if (fSaveCount != b.fSaveCount ||
            fOp != b.fOp ||
            fState != b.fState ||
            fDoAA != b.fDoAA) {
            return false;
        }
        switch (fState) {
            case kEmpty_State:
                return true;
            case kRect_State:
                return fRect == b.fRect;
            case kPath_State:
                return fPath == b.fPath;
        }
        return false;  
    }

    bool operator!=(const Rec& b) const {
        return !(*this == b);
    }


    


    bool canBeIntersectedInPlace(int saveCount, SkRegion::Op op) const {
        if (kEmpty_State == fState && (
                    SkRegion::kDifference_Op == op ||
                    SkRegion::kIntersect_Op == op)) {
            return true;
        }
        
        
        return  fSaveCount == saveCount &&
                SkRegion::kIntersect_Op == op &&
                (SkRegion::kIntersect_Op == fOp || SkRegion::kReplace_Op == fOp);
    }

    




    bool rectRectIntersectAllowed(const SkRect& newR, bool newAA) const {
        SkASSERT(kRect_State == fState);

        if (fDoAA == newAA) {
            
            return true;
        }

        if (!SkRect::Intersects(fRect, newR)) {
            
            return true;
        }

        if (fRect.contains(newR)) {
            
            
            return true;
        }

        
        
        
        
        
        return false;
    }


    



    enum FillCombo {
        kPrev_Cur_FillCombo,
        kPrev_InvCur_FillCombo,
        kInvPrev_Cur_FillCombo,
        kInvPrev_InvCur_FillCombo
    };

    
    void CombineBoundsDiff(FillCombo combination, const SkRect& prevFinite) {
        switch (combination) {
            case kInvPrev_InvCur_FillCombo:
                
                
                
                
                fFiniteBoundType = kNormal_BoundsType;
                break;
            case kInvPrev_Cur_FillCombo:
                
                
                
                fFiniteBound.join(prevFinite);
                fFiniteBoundType = kInsideOut_BoundsType;
                break;
            case kPrev_InvCur_FillCombo:
                
                
                
                if (!fFiniteBound.intersect(prevFinite)) {
                    fFiniteBound.setEmpty();
                }
                fFiniteBoundType = kNormal_BoundsType;
                break;
            case kPrev_Cur_FillCombo:
                
                
                
                
                
                
                
                fFiniteBound = prevFinite;
                break;
            default:
                SkDEBUGFAIL("SkClipStack::Rec::CombineBoundsDiff Invalid fill combination");
                break;
        }
    }

    void CombineBoundsXOR(int combination, const SkRect& prevFinite) {

        switch (combination) {
            case kInvPrev_Cur_FillCombo:       
            case kPrev_InvCur_FillCombo:
                
                
                
                fFiniteBound.join(prevFinite);
                fFiniteBoundType = kInsideOut_BoundsType;
                break;
            case kInvPrev_InvCur_FillCombo:
                
                
                
                
            case kPrev_Cur_FillCombo:
                
                
                
                
                
                
                fFiniteBound.join(prevFinite);
                fFiniteBoundType = kNormal_BoundsType;
                break;
            default:
                SkDEBUGFAIL("SkClipStack::Rec::CombineBoundsXOR Invalid fill combination");
                break;
        }
    }

    
    void CombineBoundsUnion(int combination, const SkRect& prevFinite) {

        switch (combination) {
            case kInvPrev_InvCur_FillCombo:
                if (!fFiniteBound.intersect(prevFinite)) {
                    fFiniteBound.setEmpty();
                }
                fFiniteBoundType = kInsideOut_BoundsType;
                break;
            case kInvPrev_Cur_FillCombo:
                
                
                fFiniteBound = prevFinite;
                fFiniteBoundType = kInsideOut_BoundsType;
                break;
            case kPrev_InvCur_FillCombo:
                
                
                break;
            case kPrev_Cur_FillCombo:
                fFiniteBound.join(prevFinite);
                break;
            default:
                SkDEBUGFAIL("SkClipStack::Rec::CombineBoundsUnion Invalid fill combination");
                break;
        }
    }

    
    void CombineBoundsIntersection(int combination, const SkRect& prevFinite) {

        switch (combination) {
            case kInvPrev_InvCur_FillCombo:
                
                
                fFiniteBound.join(prevFinite);
                fFiniteBoundType = kInsideOut_BoundsType;
                break;
            case kInvPrev_Cur_FillCombo:
                
                
                break;
            case kPrev_InvCur_FillCombo:
                
                
                fFiniteBound = prevFinite;
                fFiniteBoundType = kNormal_BoundsType;
                break;
            case kPrev_Cur_FillCombo:
                if (!fFiniteBound.intersect(prevFinite)) {
                    fFiniteBound.setEmpty();
                }
                break;
            default:
                SkDEBUGFAIL("SkClipStack::Rec::CombineBoundsIntersection Invalid fill combination");
                break;
        }
    }

    
    void CombineBoundsRevDiff(int combination, const SkRect& prevFinite) {

        switch (combination) {
            case kInvPrev_InvCur_FillCombo:
                
                
                
                fFiniteBound = prevFinite;
                fFiniteBoundType = kNormal_BoundsType;
                break;
            case kInvPrev_Cur_FillCombo:
                if (!fFiniteBound.intersect(prevFinite)) {
                    fFiniteBound.setEmpty();
                }
                fFiniteBoundType = kNormal_BoundsType;
                break;
            case kPrev_InvCur_FillCombo:
                fFiniteBound.join(prevFinite);
                fFiniteBoundType = kInsideOut_BoundsType;
                break;
            case kPrev_Cur_FillCombo:
                
                
                
                
                
                break;
            default:
                SkDEBUGFAIL("SkClipStack::Rec::CombineBoundsRevDiff Invalid fill combination");
                break;
        }
    }

    void updateBound(const Rec* prior) {

        
        
        fIsIntersectionOfRects = false;
        if (kRect_State == fState) {
            fFiniteBound = fRect;
            fFiniteBoundType = kNormal_BoundsType;

            if (SkRegion::kReplace_Op == fOp ||
                (SkRegion::kIntersect_Op == fOp && NULL == prior) ||
                (SkRegion::kIntersect_Op == fOp && prior->fIsIntersectionOfRects &&
                 prior->rectRectIntersectAllowed(fRect, fDoAA))) {
                fIsIntersectionOfRects = true;
            }

        } else {
            SkASSERT(kPath_State == fState);

            fFiniteBound = fPath.getBounds();

            if (fPath.isInverseFillType()) {
                fFiniteBoundType = kInsideOut_BoundsType;
            } else {
                fFiniteBoundType = kNormal_BoundsType;
            }
        }

        if (!fDoAA) {
            
            
            
            
            
            
            fFiniteBound.set(SkIntToScalar(SkScalarFloorToInt(fFiniteBound.fLeft+0.45f)),
                             SkIntToScalar(SkScalarRound(fFiniteBound.fTop)),
                             SkIntToScalar(SkScalarRound(fFiniteBound.fRight)),
                             SkIntToScalar(SkScalarRound(fFiniteBound.fBottom)));
        }

        
        
        SkRect prevFinite;
        SkClipStack::BoundsType prevType;

        if (NULL == prior) {
            
            prevFinite.setEmpty();   
            prevType = kInsideOut_BoundsType;
        } else {
            prevFinite = prior->fFiniteBound;
            prevType = prior->fFiniteBoundType;
        }

        FillCombo combination = kPrev_Cur_FillCombo;
        if (kInsideOut_BoundsType == fFiniteBoundType) {
            combination = (FillCombo) (combination | 0x01);
        }
        if (kInsideOut_BoundsType == prevType) {
            combination = (FillCombo) (combination | 0x02);
        }

        SkASSERT(kInvPrev_InvCur_FillCombo == combination ||
                 kInvPrev_Cur_FillCombo == combination ||
                 kPrev_InvCur_FillCombo == combination ||
                 kPrev_Cur_FillCombo == combination);

        
        switch (fOp) {
            case SkRegion::kDifference_Op:
                this->CombineBoundsDiff(combination, prevFinite);
                break;
            case SkRegion::kXOR_Op:
                this->CombineBoundsXOR(combination, prevFinite);
                break;
            case SkRegion::kUnion_Op:
                this->CombineBoundsUnion(combination, prevFinite);
                break;
            case SkRegion::kIntersect_Op:
                this->CombineBoundsIntersection(combination, prevFinite);
                break;
            case SkRegion::kReverseDifference_Op:
                this->CombineBoundsRevDiff(combination, prevFinite);
                break;
            case SkRegion::kReplace_Op:
                
                
                
                break;
            default:
                SkDebugf("SkRegion::Op error/n");
                SkASSERT(0);
                break;
        }
    }
};





static const int kDefaultRecordAllocCnt = 8;

SkClipStack::SkClipStack()
    : fDeque(sizeof(Rec), kDefaultRecordAllocCnt)
    , fSaveCount(0) {
}

SkClipStack::SkClipStack(const SkClipStack& b)
    : fDeque(sizeof(Rec), kDefaultRecordAllocCnt) {
    *this = b;
}

SkClipStack::SkClipStack(const SkRect& r)
    : fDeque(sizeof(Rec), kDefaultRecordAllocCnt)
    , fSaveCount(0) {
    if (!r.isEmpty()) {
        this->clipDevRect(r, SkRegion::kReplace_Op, false);
    }
}

SkClipStack::SkClipStack(const SkIRect& r)
    : fDeque(sizeof(Rec), kDefaultRecordAllocCnt)
    , fSaveCount(0) {
    if (!r.isEmpty()) {
        SkRect temp;
        temp.set(r);
        this->clipDevRect(temp, SkRegion::kReplace_Op, false);
    }
}

SkClipStack::~SkClipStack() {
    reset();
}

SkClipStack& SkClipStack::operator=(const SkClipStack& b) {
    if (this == &b) {
        return *this;
    }
    reset();

    fSaveCount = b.fSaveCount;
    SkDeque::F2BIter recIter(b.fDeque);
    for (const Rec* rec = (const Rec*)recIter.next();
         rec != NULL;
         rec = (const Rec*)recIter.next()) {
        new (fDeque.push_back()) Rec(*rec);
    }

    return *this;
}

bool SkClipStack::operator==(const SkClipStack& b) const {
    if (fSaveCount != b.fSaveCount ||
        fDeque.count() != b.fDeque.count()) {
        return false;
    }
    SkDeque::F2BIter myIter(fDeque);
    SkDeque::F2BIter bIter(b.fDeque);
    const Rec* myRec = (const Rec*)myIter.next();
    const Rec* bRec = (const Rec*)bIter.next();

    while (myRec != NULL && bRec != NULL) {
        if (*myRec != *bRec) {
            return false;
        }
        myRec = (const Rec*)myIter.next();
        bRec = (const Rec*)bIter.next();
    }
    return myRec == NULL && bRec == NULL;
}

void SkClipStack::reset() {
    
    
    while (!fDeque.empty()) {
        Rec* rec = (Rec*)fDeque.back();
        rec->~Rec();
        fDeque.pop_back();
    }

    fSaveCount = 0;
}

void SkClipStack::save() {
    fSaveCount += 1;
}

void SkClipStack::restore() {
    fSaveCount -= 1;
    while (!fDeque.empty()) {
        Rec* rec = (Rec*)fDeque.back();
        if (rec->fSaveCount <= fSaveCount) {
            break;
        }
        this->purgeClip(rec);
        rec->~Rec();
        fDeque.pop_back();
    }
}

void SkClipStack::getBounds(SkRect* canvFiniteBound,
                            BoundsType* boundType,
                            bool* isIntersectionOfRects) const {
    SkASSERT(NULL != canvFiniteBound && NULL != boundType);

    Rec* rec = (Rec*)fDeque.back();

    if (NULL == rec) {
        
        canvFiniteBound->setEmpty();
        *boundType = kInsideOut_BoundsType;
        if (NULL != isIntersectionOfRects) {
            *isIntersectionOfRects = false;
        }
        return;
    }

    *canvFiniteBound = rec->fFiniteBound;
    *boundType = rec->fFiniteBoundType;
    if (NULL != isIntersectionOfRects) {
        *isIntersectionOfRects = rec->fIsIntersectionOfRects;
    }
}

void SkClipStack::clipDevRect(const SkRect& rect, SkRegion::Op op, bool doAA) {

    int32_t genID = GetNextGenID();

    
    SkDeque::Iter iter(fDeque, SkDeque::Iter::kBack_IterStart);
    Rec* rec = (Rec*) iter.prev();

    if (rec && rec->canBeIntersectedInPlace(fSaveCount, op)) {
        switch (rec->fState) {
            case Rec::kEmpty_State:
                rec->checkEmpty();
                return;
            case Rec::kRect_State:
                if (rec->rectRectIntersectAllowed(rect, doAA)) {
                    this->purgeClip(rec);
                    if (!rec->fRect.intersect(rect)) {
                        rec->setEmpty();
                        return;
                    }

                    rec->fDoAA = doAA;
                    Rec* prev = (Rec*) iter.prev();
                    rec->updateBound(prev);
                    rec->fGenID = genID;
                    return;
                }
                break;
            case Rec::kPath_State:
                if (!SkRect::Intersects(rec->fPath.getBounds(), rect)) {
                    this->purgeClip(rec);
                    rec->setEmpty();
                    return;
                }
                break;
        }
    }
    new (fDeque.push_back()) Rec(fSaveCount, rect, op, doAA);
    ((Rec*) fDeque.back())->updateBound(rec);
    ((Rec*) fDeque.back())->fGenID = genID;

    if (rec && rec->fSaveCount == fSaveCount) {
        this->purgeClip(rec);
    }
}

void SkClipStack::clipDevPath(const SkPath& path, SkRegion::Op op, bool doAA) {
    SkRect alt;
    if (path.isRect(&alt)) {
        return this->clipDevRect(alt, op, doAA);
    }

    int32_t genID = GetNextGenID();

    Rec* rec = (Rec*)fDeque.back();
    if (rec && rec->canBeIntersectedInPlace(fSaveCount, op)) {
        const SkRect& pathBounds = path.getBounds();
        switch (rec->fState) {
            case Rec::kEmpty_State:
                rec->checkEmpty();
                return;
            case Rec::kRect_State:
                if (!SkRect::Intersects(rec->fRect, pathBounds)) {
                    this->purgeClip(rec);
                    rec->setEmpty();
                    return;
                }
                break;
            case Rec::kPath_State:
                if (!SkRect::Intersects(rec->fPath.getBounds(), pathBounds)) {
                    this->purgeClip(rec);
                    rec->setEmpty();
                    return;
                }
                break;
        }
    }
    new (fDeque.push_back()) Rec(fSaveCount, path, op, doAA);
    ((Rec*) fDeque.back())->updateBound(rec);
    ((Rec*) fDeque.back())->fGenID = genID;

    if (rec && rec->fSaveCount == fSaveCount) {
        this->purgeClip(rec);
    }
}

void SkClipStack::clipEmpty() {

    Rec* rec = (Rec*) fDeque.back();

    if (rec && rec->canBeIntersectedInPlace(fSaveCount, SkRegion::kIntersect_Op)) {
        switch (rec->fState) {
            case Rec::kEmpty_State:
                rec->checkEmpty();
                return;
            case Rec::kRect_State:
            case Rec::kPath_State:
                this->purgeClip(rec);
                rec->setEmpty();
                return;
        }
    }
    new (fDeque.push_back()) Rec(fSaveCount);

    if (rec && rec->fSaveCount == fSaveCount) {
        this->purgeClip(rec);
    }
}

bool SkClipStack::isWideOpen() const {
    if (0 == fDeque.count()) {
        return true;
    }

    const Rec* back = (const Rec*) fDeque.back();
    return kInsideOut_BoundsType == back->fFiniteBoundType &&
           back->fFiniteBound.isEmpty();
}



SkClipStack::Iter::Iter() : fStack(NULL) {
}

bool operator==(const SkClipStack::Iter::Clip& a,
                const SkClipStack::Iter::Clip& b) {
    return a.fOp == b.fOp && a.fDoAA == b.fDoAA &&
           ((a.fRect == NULL && b.fRect == NULL) ||
               (a.fRect != NULL && b.fRect != NULL && *a.fRect == *b.fRect)) &&
           ((a.fPath == NULL && b.fPath == NULL) ||
               (a.fPath != NULL && b.fPath != NULL && *a.fPath == *b.fPath));
}

bool operator!=(const SkClipStack::Iter::Clip& a,
                const SkClipStack::Iter::Clip& b) {
    return !(a == b);
}

SkClipStack::Iter::Iter(const SkClipStack& stack, IterStart startLoc)
    : fStack(&stack) {
    this->reset(stack, startLoc);
}

const SkClipStack::Iter::Clip* SkClipStack::Iter::updateClip(
                        const SkClipStack::Rec* rec) {
    switch (rec->fState) {
        case SkClipStack::Rec::kEmpty_State:
            fClip.fRect = NULL;
            fClip.fPath = NULL;
            break;
        case SkClipStack::Rec::kRect_State:
            fClip.fRect = &rec->fRect;
            fClip.fPath = NULL;
            break;
        case SkClipStack::Rec::kPath_State:
            fClip.fRect = NULL;
            fClip.fPath = &rec->fPath;
            break;
    }
    fClip.fOp = rec->fOp;
    fClip.fDoAA = rec->fDoAA;
    fClip.fGenID = rec->fGenID;
    return &fClip;
}

const SkClipStack::Iter::Clip* SkClipStack::Iter::next() {
    const SkClipStack::Rec* rec = (const SkClipStack::Rec*)fIter.next();
    if (NULL == rec) {
        return NULL;
    }

    return this->updateClip(rec);
}

const SkClipStack::Iter::Clip* SkClipStack::Iter::prev() {
    const SkClipStack::Rec* rec = (const SkClipStack::Rec*)fIter.prev();
    if (NULL == rec) {
        return NULL;
    }

    return this->updateClip(rec);
}

const SkClipStack::Iter::Clip* SkClipStack::Iter::skipToTopmost(SkRegion::Op op) {

    if (NULL == fStack) {
        return NULL;
    }

    fIter.reset(fStack->fDeque, SkDeque::Iter::kBack_IterStart);

    const SkClipStack::Rec* rec = NULL;

    for (rec = (const SkClipStack::Rec*) fIter.prev();
         NULL != rec;
         rec = (const SkClipStack::Rec*) fIter.prev()) {

        if (op == rec->fOp) {
            
            
            
            
            
            
            if (NULL == fIter.next()) {
                
                
                
                fIter.reset(fStack->fDeque, SkDeque::Iter::kFront_IterStart);
            }
            break;
        }
    }

    if (NULL == rec) {
        
        fIter.reset(fStack->fDeque, SkDeque::Iter::kFront_IterStart);
    }

    return this->next();
}

void SkClipStack::Iter::reset(const SkClipStack& stack, IterStart startLoc) {
    fStack = &stack;
    fIter.reset(stack.fDeque, static_cast<SkDeque::Iter::IterStart>(startLoc));
}


void SkClipStack::getConservativeBounds(int offsetX,
                                        int offsetY,
                                        int maxWidth,
                                        int maxHeight,
                                        SkRect* devBounds,
                                        bool* isIntersectionOfRects) const {
    SkASSERT(NULL != devBounds);

    devBounds->setLTRB(0, 0,
                       SkIntToScalar(maxWidth), SkIntToScalar(maxHeight));

    SkRect temp;
    SkClipStack::BoundsType boundType;

    
    this->getBounds(&temp, &boundType, isIntersectionOfRects);
    if (SkClipStack::kInsideOut_BoundsType == boundType) {
        return;
    }

    
    temp.offset(SkIntToScalar(offsetX), SkIntToScalar(offsetY));

    if (!devBounds->intersect(temp)) {
        devBounds->setEmpty();
    }
}

void SkClipStack::addPurgeClipCallback(PFPurgeClipCB callback, void* data) const {
    ClipCallbackData temp = { callback, data };
    fCallbackData.append(1, &temp);
}

void SkClipStack::removePurgeClipCallback(PFPurgeClipCB callback, void* data) const {
    ClipCallbackData temp = { callback, data };
    int index = fCallbackData.find(temp);
    if (index >= 0) {
        fCallbackData.removeShuffle(index);
    }
}


void SkClipStack::purgeClip(Rec* rec) {
    SkASSERT(NULL != rec);

    for (int i = 0; i < fCallbackData.count(); ++i) {
        (*fCallbackData[i].fCallback)(rec->fGenID, fCallbackData[i].fData);
    }

    
    rec->fGenID = kInvalidGenID;
}

int32_t SkClipStack::GetNextGenID() {
    return sk_atomic_inc(&gGenID);
}

int32_t SkClipStack::getTopmostGenID() const {

    if (fDeque.empty()) {
        return kInvalidGenID;
    }

    Rec* rec = (Rec*)fDeque.back();
    return rec->fGenID;
}
