






#include "SkCanvas.h"
#include "SkClipStack.h"
#include "SkPath.h"
#include "SkThread.h"

#include <new>



static const int32_t kFirstUnreservedGenID = 3;
int32_t SkClipStack::gGenID = kFirstUnreservedGenID;

SkClipStack::Element::Element(const Element& that) {
    switch (that.getType()) {
        case kEmpty_Type:
            fPath.reset();
            break;
        case kRect_Type: 
        case kRRect_Type:
            fPath.reset();
            fRRect = that.fRRect;
            break;
        case kPath_Type:
            fPath.set(that.getPath());
            break;
    }

    fSaveCount = that.fSaveCount;
    fOp = that.fOp;
    fType = that.fType;
    fDoAA = that.fDoAA;
    fFiniteBoundType = that.fFiniteBoundType;
    fFiniteBound = that.fFiniteBound;
    fIsIntersectionOfRects = that.fIsIntersectionOfRects;
    fGenID = that.fGenID;
}

bool SkClipStack::Element::operator== (const Element& element) const {
    if (this == &element) {
        return true;
    }
    if (fOp != element.fOp ||
        fType != element.fType ||
        fDoAA != element.fDoAA ||
        fSaveCount != element.fSaveCount) {
        return false;
    }
    switch (fType) {
        case kPath_Type:
            return this->getPath() == element.getPath();
        case kRRect_Type:
            return fRRect == element.fRRect;
        case kRect_Type:
            return this->getRect() == element.getRect();
        case kEmpty_Type:
            return true;
        default:
            SkDEBUGFAIL("Unexpected type.");
            return false;
    }
}

void SkClipStack::Element::replay(SkCanvasClipVisitor* visitor) const {
    static const SkRect kEmptyRect = { 0, 0, 0, 0 };

    switch (fType) {
        case kPath_Type:
            visitor->clipPath(this->getPath(), this->getOp(), this->isAA());
            break;
        case kRRect_Type:
            visitor->clipRRect(this->getRRect(), this->getOp(), this->isAA());
            break;
        case kRect_Type:
            visitor->clipRect(this->getRect(), this->getOp(), this->isAA());
            break;
        case kEmpty_Type:
            visitor->clipRect(kEmptyRect, SkRegion::kIntersect_Op, false);
            break;
    }
}

void SkClipStack::Element::invertShapeFillType() {
    switch (fType) {
        case kRect_Type:
            fPath.init();
            fPath.get()->addRect(this->getRect());
            fPath.get()->setFillType(SkPath::kInverseEvenOdd_FillType);
            fType = kPath_Type;
            break;
        case kRRect_Type:
            fPath.init();
            fPath.get()->addRRect(fRRect);
            fPath.get()->setFillType(SkPath::kInverseEvenOdd_FillType);
            fType = kPath_Type;
            break;
        case kPath_Type:
            fPath.get()->toggleInverseFillType();
            break;
        case kEmpty_Type:
            
            break;
    }
}

void SkClipStack::Element::initPath(int saveCount, const SkPath& path, SkRegion::Op op,
                                    bool doAA) {
    if (!path.isInverseFillType()) {
        if (SkPath::kNone_PathAsRect != path.asRect()) {
            this->initRect(saveCount, path.getBounds(), op, doAA);
            return;
        }
        SkRect ovalRect;
        if (path.isOval(&ovalRect)) {
            SkRRect rrect;
            rrect.setOval(ovalRect);
            this->initRRect(saveCount, rrect, op, doAA);
            return;
        }
    }
    fPath.set(path);
    fType = kPath_Type;
    this->initCommon(saveCount, op, doAA);
}

void SkClipStack::Element::asPath(SkPath* path) const {
    switch (fType) {
        case kEmpty_Type:
            path->reset();
            break;
        case kRect_Type:
            path->reset();
            path->addRect(this->getRect());
            break;
        case kRRect_Type:
            path->reset();
            path->addRRect(fRRect);
            break;
        case kPath_Type:
            *path = *fPath.get();
            break;
    }
}

void SkClipStack::Element::setEmpty() {
    fType = kEmpty_Type;
    fFiniteBound.setEmpty();
    fFiniteBoundType = kNormal_BoundsType;
    fIsIntersectionOfRects = false;
    fRRect.setEmpty();
    fPath.reset();
    fGenID = kEmptyGenID;
    SkDEBUGCODE(this->checkEmpty();)
}

void SkClipStack::Element::checkEmpty() const {
    SkASSERT(fFiniteBound.isEmpty());
    SkASSERT(kNormal_BoundsType == fFiniteBoundType);
    SkASSERT(!fIsIntersectionOfRects);
    SkASSERT(kEmptyGenID == fGenID);
    SkASSERT(!fPath.isValid());
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

    if (!SkRect::Intersects(this->getRect(), newR)) {
        
        return true;
    }

    if (this->getRect().contains(newR)) {
        
        
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
                this->setEmpty();
            } else {
                fFiniteBoundType = kNormal_BoundsType;
            }
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
                this->setEmpty();
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
                this->setEmpty();
            } else {
                fFiniteBoundType = kNormal_BoundsType;
            }
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
    switch (fType) {
        case kRect_Type:
            fFiniteBound = this->getRect();
            fFiniteBoundType = kNormal_BoundsType;

            if (SkRegion::kReplace_Op == fOp ||
                (SkRegion::kIntersect_Op == fOp && NULL == prior) ||
                (SkRegion::kIntersect_Op == fOp && prior->fIsIntersectionOfRects &&
                    prior->rectRectIntersectAllowed(this->getRect(), fDoAA))) {
                fIsIntersectionOfRects = true;
            }
            break;
        case kRRect_Type:
            fFiniteBound = fRRect.getBounds();
            fFiniteBoundType = kNormal_BoundsType;
            break;
        case kPath_Type:
            fFiniteBound = fPath.get()->getBounds();

            if (fPath.get()->isInverseFillType()) {
                fFiniteBoundType = kInsideOut_BoundsType;
            } else {
                fFiniteBoundType = kNormal_BoundsType;
            }
            break;
        case kEmpty_Type:
            SkDEBUGFAIL("We shouldn't get here with an empty element.");
            break;
    }

    if (!fDoAA) {
        
        
        
        
        
        
        fFiniteBound.set(SkScalarFloorToScalar(fFiniteBound.fLeft+0.45f),
                         SkScalarRoundToScalar(fFiniteBound.fTop),
                         SkScalarRoundToScalar(fFiniteBound.fRight),
                         SkScalarRoundToScalar(fFiniteBound.fBottom));
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
            SkDebugf("SkRegion::Op error\n");
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
    if (this->getTopmostGenID() == b.getTopmostGenID()) {
        return true;
    }
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
    restoreTo(fSaveCount);
}

void SkClipStack::restoreTo(int saveCount) {
    while (!fDeque.empty()) {
        Element* element = (Element*)fDeque.back();
        if (element->fSaveCount <= saveCount) {
            break;
        }
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

void SkClipStack::pushElement(const Element& element) {
    
    SkDeque::Iter iter(fDeque, SkDeque::Iter::kBack_IterStart);
    Element* prior = (Element*) iter.prev();

    if (NULL != prior) {
        if (prior->canBeIntersectedInPlace(fSaveCount, element.getOp())) {
            switch (prior->fType) {
                case Element::kEmpty_Type:
                    SkDEBUGCODE(prior->checkEmpty();)
                    return;
                case Element::kRect_Type:
                    if (Element::kRect_Type == element.getType()) {
                        if (prior->rectRectIntersectAllowed(element.getRect(), element.isAA())) {
                            SkRect isectRect;
                            if (!isectRect.intersect(prior->getRect(), element.getRect())) {
                                prior->setEmpty();
                                return;
                            }

                            prior->fRRect.setRect(isectRect);
                            prior->fDoAA = element.isAA();
                            Element* priorPrior = (Element*) iter.prev();
                            prior->updateBoundAndGenID(priorPrior);
                            return;
                        }
                        break;
                    }
                    
                default:
                    if (!SkRect::Intersects(prior->getBounds(), element.getBounds())) {
                        prior->setEmpty();
                        return;
                    }
                    break;
            }
        } else if (SkRegion::kReplace_Op == element.getOp()) {
            this->restoreTo(fSaveCount - 1);
            prior = (Element*) fDeque.back();
        }
    }
    Element* newElement = SkNEW_PLACEMENT_ARGS(fDeque.push_back(), Element, (element));
    newElement->updateBoundAndGenID(prior);
}

void SkClipStack::clipDevRRect(const SkRRect& rrect, SkRegion::Op op, bool doAA) {
    Element element(fSaveCount, rrect, op, doAA);
    this->pushElement(element);
}

void SkClipStack::clipDevRect(const SkRect& rect, SkRegion::Op op, bool doAA) {
    Element element(fSaveCount, rect, op, doAA);
    this->pushElement(element);
}

void SkClipStack::clipDevPath(const SkPath& path, SkRegion::Op op, bool doAA) {
    Element element(fSaveCount, path, op, doAA);
    this->pushElement(element);
}

void SkClipStack::clipEmpty() {
    Element* element = (Element*) fDeque.back();

    if (element && element->canBeIntersectedInPlace(fSaveCount, SkRegion::kIntersect_Op)) {
        element->setEmpty();
    }
    new (fDeque.push_back()) Element(fSaveCount);

    ((Element*)fDeque.back())->fGenID = kEmptyGenID;
}

bool SkClipStack::isWideOpen() const {
    return this->getTopmostGenID() == kWideOpenGenID;
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

int32_t SkClipStack::GetNextGenID() {
    
    return sk_atomic_inc(&gGenID);
}

int32_t SkClipStack::getTopmostGenID() const {
    if (fDeque.empty()) {
        return kWideOpenGenID;
    }

    const Element* back = static_cast<const Element*>(fDeque.back());
    if (kInsideOut_BoundsType == back->fFiniteBoundType && back->fFiniteBound.isEmpty()) {
        return kWideOpenGenID;
    }

    return back->getGenID();
}

#ifdef SK_DEVELOPER
void SkClipStack::Element::dump() const {
    static const char* kTypeStrings[] = {
        "empty",
        "rect",
        "rrect",
        "path"
    };
    SK_COMPILE_ASSERT(0 == kEmpty_Type, type_str);
    SK_COMPILE_ASSERT(1 == kRect_Type, type_str);
    SK_COMPILE_ASSERT(2 == kRRect_Type, type_str);
    SK_COMPILE_ASSERT(3 == kPath_Type, type_str);
    SK_COMPILE_ASSERT(SK_ARRAY_COUNT(kTypeStrings) == kTypeCnt, type_str);

    static const char* kOpStrings[] = {
        "difference",
        "intersect",
        "union",
        "xor",
        "reverse-difference",
        "replace",
    };
    SK_COMPILE_ASSERT(0 == SkRegion::kDifference_Op, op_str);
    SK_COMPILE_ASSERT(1 == SkRegion::kIntersect_Op, op_str);
    SK_COMPILE_ASSERT(2 == SkRegion::kUnion_Op, op_str);
    SK_COMPILE_ASSERT(3 == SkRegion::kXOR_Op, op_str);
    SK_COMPILE_ASSERT(4 == SkRegion::kReverseDifference_Op, op_str);
    SK_COMPILE_ASSERT(5 == SkRegion::kReplace_Op, op_str);
    SK_COMPILE_ASSERT(SK_ARRAY_COUNT(kOpStrings) == SkRegion::kOpCnt, op_str);

    SkDebugf("Type: %s, Op: %s, AA: %s, Save Count: %d\n", kTypeStrings[fType],
             kOpStrings[fOp], (fDoAA ? "yes" : "no"), fSaveCount);
    switch (fType) {
        case kEmpty_Type:
            SkDebugf("\n");
            break;
        case kRect_Type:
            this->getRect().dump();
            SkDebugf("\n");
            break;
        case kRRect_Type:
            this->getRRect().dump();
            SkDebugf("\n");
            break;
        case kPath_Type:
            this->getPath().dump(NULL, true);
            break;
    }
}

void SkClipStack::dump() const {
    B2TIter iter(*this);
    const Element* e;
    while ((e = iter.next())) {
        e->dump();
        SkDebugf("\n");
    }
}
#endif
