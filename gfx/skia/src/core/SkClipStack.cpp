






#include "SkClipStack.h"
#include "SkPath.h"
#include "SkThread.h"

#include <new>



static const int32_t kFirstUnreservedGenID = 3;
int32_t SkClipStack::gGenID = kFirstUnreservedGenID;

void SkClipStack::Element::invertShapeFillType() {
    switch (fType) {
        case kRect_Type:
            fPath.reset();
            fPath.addRect(fRect);
            fPath.setFillType(SkPath::kInverseWinding_FillType);
            fType = kPath_Type;
            break;
        case kPath_Type:
            fPath.toggleInverseFillType();
        case kEmpty_Type:
            break;
    }
}

void SkClipStack::Element::checkEmpty() const {
    SkASSERT(fFiniteBound.isEmpty());
    SkASSERT(kNormal_BoundsType == fFiniteBoundType);
    SkASSERT(!fIsIntersectionOfRects);
    SkASSERT(kEmptyGenID == fGenID);
    SkASSERT(fPath.isEmpty());
}

bool SkClipStack::Element::canBeIntersectedInPlace(int saveCount, SkRegion::Op op) const {
    if (kEmpty_Type == fType &&
        (SkRegion::kDifference_Op == op || SkRegion::kIntersect_Op == op)) {
        return true;
    }
    
    
    return  fSaveCount == saveCount &&
            SkRegion::kIntersect_Op == op &&
            (SkRegion::kIntersect_Op == fOp || SkRegion::kReplace_Op == fOp);
}

bool SkClipStack::Element::rectRectIntersectAllowed(const SkRect& newR, bool newAA) const {
    SkASSERT(kRect_Type == fType);

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


void SkClipStack::Element::combineBoundsDiff(FillCombo combination, const SkRect& prevFinite) {
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
                fGenID = kEmptyGenID;
            }
            fFiniteBoundType = kNormal_BoundsType;
            break;
        case kPrev_Cur_FillCombo:
            
            
            
            
            
            
            
            fFiniteBound = prevFinite;
            break;
        default:
            SkDEBUGFAIL("SkClipStack::Element::combineBoundsDiff Invalid fill combination");
            break;
    }
}

void SkClipStack::Element::combineBoundsXOR(int combination, const SkRect& prevFinite) {

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
            SkDEBUGFAIL("SkClipStack::Element::combineBoundsXOR Invalid fill combination");
            break;
    }
}


void SkClipStack::Element::combineBoundsUnion(int combination, const SkRect& prevFinite) {

    switch (combination) {
        case kInvPrev_InvCur_FillCombo:
            if (!fFiniteBound.intersect(prevFinite)) {
                fFiniteBound.setEmpty();
                fGenID = kWideOpenGenID;
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
            SkDEBUGFAIL("SkClipStack::Element::combineBoundsUnion Invalid fill combination");
            break;
    }
}


void SkClipStack::Element::combineBoundsIntersection(int combination, const SkRect& prevFinite) {

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
                fGenID = kEmptyGenID;
            }
            break;
        default:
            SkDEBUGFAIL("SkClipStack::Element::combineBoundsIntersection Invalid fill combination");
            break;
    }
}


void SkClipStack::Element::combineBoundsRevDiff(int combination, const SkRect& prevFinite) {

    switch (combination) {
        case kInvPrev_InvCur_FillCombo:
            
            
            
            fFiniteBound = prevFinite;
            fFiniteBoundType = kNormal_BoundsType;
            break;
        case kInvPrev_Cur_FillCombo:
            if (!fFiniteBound.intersect(prevFinite)) {
                fFiniteBound.setEmpty();
                fGenID = kEmptyGenID;
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
            SkDEBUGFAIL("SkClipStack::Element::combineBoundsRevDiff Invalid fill combination");
            break;
    }
}

void SkClipStack::Element::updateBoundAndGenID(const Element* prior) {
    
    
    fGenID = GetNextGenID();

    
    
    fIsIntersectionOfRects = false;
    if (kRect_Type == fType) {
        fFiniteBound = fRect;
        fFiniteBoundType = kNormal_BoundsType;

        if (SkRegion::kReplace_Op == fOp ||
            (SkRegion::kIntersect_Op == fOp && NULL == prior) ||
            (SkRegion::kIntersect_Op == fOp && prior->fIsIntersectionOfRects &&
                prior->rectRectIntersectAllowed(fRect, fDoAA))) {
            fIsIntersectionOfRects = true;
        }

    } else {
        SkASSERT(kPath_Type == fType);

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
            this->combineBoundsDiff(combination, prevFinite);
            break;
        case SkRegion::kXOR_Op:
            this->combineBoundsXOR(combination, prevFinite);
            break;
        case SkRegion::kUnion_Op:
            this->combineBoundsUnion(combination, prevFinite);
            break;
        case SkRegion::kIntersect_Op:
            this->combineBoundsIntersection(combination, prevFinite);
            break;
        case SkRegion::kReverseDifference_Op:
            this->combineBoundsRevDiff(combination, prevFinite);
            break;
        case SkRegion::kReplace_Op:
            
            
            
            break;
        default:
            SkDebugf("SkRegion::Op error/n");
            SkASSERT(0);
            break;
    }
}





static const int kDefaultElementAllocCnt = 8;

SkClipStack::SkClipStack()
    : fDeque(sizeof(Element), kDefaultElementAllocCnt)
    , fSaveCount(0) {
}

SkClipStack::SkClipStack(const SkClipStack& b)
    : fDeque(sizeof(Element), kDefaultElementAllocCnt) {
    *this = b;
}

SkClipStack::SkClipStack(const SkRect& r)
    : fDeque(sizeof(Element), kDefaultElementAllocCnt)
    , fSaveCount(0) {
    if (!r.isEmpty()) {
        this->clipDevRect(r, SkRegion::kReplace_Op, false);
    }
}

SkClipStack::SkClipStack(const SkIRect& r)
    : fDeque(sizeof(Element), kDefaultElementAllocCnt)
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
    for (const Element* element = (const Element*)recIter.next();
         element != NULL;
         element = (const Element*)recIter.next()) {
        new (fDeque.push_back()) Element(*element);
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
    const Element* myElement = (const Element*)myIter.next();
    const Element* bElement = (const Element*)bIter.next();

    while (myElement != NULL && bElement != NULL) {
        if (*myElement != *bElement) {
            return false;
        }
        myElement = (const Element*)myIter.next();
        bElement = (const Element*)bIter.next();
    }
    return myElement == NULL && bElement == NULL;
}

void SkClipStack::reset() {
    
    
    while (!fDeque.empty()) {
        Element* element = (Element*)fDeque.back();
        element->~Element();
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
        Element* element = (Element*)fDeque.back();
        if (element->fSaveCount <= fSaveCount) {
            break;
        }
        this->purgeClip(element);
        element->~Element();
        fDeque.pop_back();
    }
}

void SkClipStack::getBounds(SkRect* canvFiniteBound,
                            BoundsType* boundType,
                            bool* isIntersectionOfRects) const {
    SkASSERT(NULL != canvFiniteBound && NULL != boundType);

    Element* element = (Element*)fDeque.back();

    if (NULL == element) {
        
        canvFiniteBound->setEmpty();
        *boundType = kInsideOut_BoundsType;
        if (NULL != isIntersectionOfRects) {
            *isIntersectionOfRects = false;
        }
        return;
    }

    *canvFiniteBound = element->fFiniteBound;
    *boundType = element->fFiniteBoundType;
    if (NULL != isIntersectionOfRects) {
        *isIntersectionOfRects = element->fIsIntersectionOfRects;
    }
}

bool SkClipStack::intersectRectWithClip(SkRect* rect) const {
    SkASSERT(NULL != rect);

    SkRect bounds;
    SkClipStack::BoundsType bt;
    this->getBounds(&bounds, &bt);
    if (bt == SkClipStack::kInsideOut_BoundsType) {
        if (bounds.contains(*rect)) {
            return false;
        } else {
            
            
            return true;
        }
    } else {
        return rect->intersect(bounds);
    }
}

bool SkClipStack::quickContains(const SkRect& rect) const {

    Iter iter(*this, Iter::kTop_IterStart);
    const Element* element = iter.prev();
    while (element != NULL) {
        if (SkRegion::kIntersect_Op != element->getOp() && SkRegion::kReplace_Op != element->getOp())
            return false;
        if (element->isInverseFilled()) {
            
            if (SkRect::Intersects(element->getBounds(), rect)) {
                return false;
            }
        } else {
            if (!element->contains(rect)) {
                return false;
            }
        }
        if (SkRegion::kReplace_Op == element->getOp()) {
            break;
        }
        element = iter.prev();
    }
    return true;
}

void SkClipStack::clipDevRect(const SkRect& rect, SkRegion::Op op, bool doAA) {

    
    SkDeque::Iter iter(fDeque, SkDeque::Iter::kBack_IterStart);
    Element* element = (Element*) iter.prev();

    if (element && element->canBeIntersectedInPlace(fSaveCount, op)) {
        switch (element->fType) {
            case Element::kEmpty_Type:
                element->checkEmpty();
                return;
            case Element::kRect_Type:
                if (element->rectRectIntersectAllowed(rect, doAA)) {
                    this->purgeClip(element);
                    if (!element->fRect.intersect(rect)) {
                        element->setEmpty();
                        return;
                    }

                    element->fDoAA = doAA;
                    Element* prev = (Element*) iter.prev();
                    element->updateBoundAndGenID(prev);
                    return;
                }
                break;
            case Element::kPath_Type:
                if (!SkRect::Intersects(element->fPath.getBounds(), rect)) {
                    this->purgeClip(element);
                    element->setEmpty();
                    return;
                }
                break;
        }
    }
    new (fDeque.push_back()) Element(fSaveCount, rect, op, doAA);
    ((Element*) fDeque.back())->updateBoundAndGenID(element);

    if (element && element->fSaveCount == fSaveCount) {
        this->purgeClip(element);
    }
}

void SkClipStack::clipDevPath(const SkPath& path, SkRegion::Op op, bool doAA) {
    SkRect alt;
    if (path.isRect(&alt) && !path.isInverseFillType()) {
        return this->clipDevRect(alt, op, doAA);
    }

    Element* element = (Element*)fDeque.back();
    if (element && element->canBeIntersectedInPlace(fSaveCount, op)) {
        const SkRect& pathBounds = path.getBounds();
        switch (element->fType) {
            case Element::kEmpty_Type:
                element->checkEmpty();
                return;
            case Element::kRect_Type:
                if (!SkRect::Intersects(element->fRect, pathBounds)) {
                    this->purgeClip(element);
                    element->setEmpty();
                    return;
                }
                break;
            case Element::kPath_Type:
                if (!SkRect::Intersects(element->fPath.getBounds(), pathBounds)) {
                    this->purgeClip(element);
                    element->setEmpty();
                    return;
                }
                break;
        }
    }
    new (fDeque.push_back()) Element(fSaveCount, path, op, doAA);
    ((Element*) fDeque.back())->updateBoundAndGenID(element);

    if (element && element->fSaveCount == fSaveCount) {
        this->purgeClip(element);
    }
}

void SkClipStack::clipEmpty() {

    Element* element = (Element*) fDeque.back();

    if (element && element->canBeIntersectedInPlace(fSaveCount, SkRegion::kIntersect_Op)) {
        switch (element->fType) {
            case Element::kEmpty_Type:
                element->checkEmpty();
                return;
            case Element::kRect_Type:
            case Element::kPath_Type:
                this->purgeClip(element);
                element->setEmpty();
                return;
        }
    }
    new (fDeque.push_back()) Element(fSaveCount);

    if (element && element->fSaveCount == fSaveCount) {
        this->purgeClip(element);
    }
    ((Element*)fDeque.back())->fGenID = kEmptyGenID;
}

bool SkClipStack::isWideOpen() const {
    if (0 == fDeque.count()) {
        return true;
    }

    const Element* back = (const Element*) fDeque.back();
    return kWideOpenGenID == back->fGenID ||
           (kInsideOut_BoundsType == back->fFiniteBoundType && back->fFiniteBound.isEmpty());
}



SkClipStack::Iter::Iter() : fStack(NULL) {
}

SkClipStack::Iter::Iter(const SkClipStack& stack, IterStart startLoc)
    : fStack(&stack) {
    this->reset(stack, startLoc);
}

const SkClipStack::Element* SkClipStack::Iter::next() {
    return (const SkClipStack::Element*)fIter.next();
}

const SkClipStack::Element* SkClipStack::Iter::prev() {
    return (const SkClipStack::Element*)fIter.prev();
}

const SkClipStack::Element* SkClipStack::Iter::skipToTopmost(SkRegion::Op op) {

    if (NULL == fStack) {
        return NULL;
    }

    fIter.reset(fStack->fDeque, SkDeque::Iter::kBack_IterStart);

    const SkClipStack::Element* element = NULL;

    for (element = (const SkClipStack::Element*) fIter.prev();
         NULL != element;
         element = (const SkClipStack::Element*) fIter.prev()) {

        if (op == element->fOp) {
            
            
            
            
            
            
            if (NULL == fIter.next()) {
                
                
                
                fIter.reset(fStack->fDeque, SkDeque::Iter::kFront_IterStart);
            }
            break;
        }
    }

    if (NULL == element) {
        
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


void SkClipStack::purgeClip(Element* element) {
    SkASSERT(NULL != element);
    if (element->fGenID >= 0 && element->fGenID < kFirstUnreservedGenID) {
        return;
    }

    for (int i = 0; i < fCallbackData.count(); ++i) {
        (*fCallbackData[i].fCallback)(element->fGenID, fCallbackData[i].fData);
    }

    
    element->fGenID = kInvalidGenID;
}

int32_t SkClipStack::GetNextGenID() {
    
    return sk_atomic_inc(&gGenID);
}

int32_t SkClipStack::getTopmostGenID() const {

    if (fDeque.empty()) {
        return kInvalidGenID;
    }

    Element* element = (Element*)fDeque.back();
    return element->fGenID;
}
