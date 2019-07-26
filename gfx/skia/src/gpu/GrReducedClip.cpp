







#include "GrReducedClip.h"

typedef SkClipStack::Element Element;


namespace GrReducedClip {


void reduced_stack_walker(const SkClipStack& stack,
                          const SkRect& queryBounds,
                          ElementList* result,
                          InitialState* initialState,
                          bool* requiresAA);








void ReduceClipStack(const SkClipStack& stack,
                     const SkIRect& queryBounds,
                     ElementList* result,
                     InitialState* initialState,
                     SkIRect* tighterBounds,
                     bool* requiresAA) {
    result->reset();

    if (stack.isWideOpen()) {
        *initialState = kAllIn_InitialState;
        return;
    }


    
    

    SkClipStack::BoundsType stackBoundsType;
    SkRect stackBounds;
    bool iior;
    stack.getBounds(&stackBounds, &stackBoundsType, &iior);

    const SkIRect* bounds = &queryBounds;

    SkRect scalarQueryBounds = SkRect::MakeFromIRect(queryBounds);

    if (iior) {
        SkASSERT(SkClipStack::kNormal_BoundsType == stackBoundsType);
        SkRect isectRect;
        if (stackBounds.contains(scalarQueryBounds)) {
            *initialState = kAllIn_InitialState;
            if (NULL != tighterBounds) {
                *tighterBounds = queryBounds;
            }
            if (NULL != requiresAA) {
               *requiresAA = false;
            }
        } else if (isectRect.intersect(stackBounds, scalarQueryBounds)) {
            if (NULL != tighterBounds) {
                isectRect.roundOut(tighterBounds);
                SkRect scalarTighterBounds = SkRect::MakeFromIRect(*tighterBounds);
                if (scalarTighterBounds == isectRect) {
                    
                    *requiresAA = false;
                    *initialState = kAllIn_InitialState;
                    return;
                }
                *initialState = kAllOut_InitialState;
                
                SkClipStack::Iter iter(stack, SkClipStack::Iter::kTop_IterStart);
                bool doAA = iter.prev()->isAA();
                SkNEW_INSERT_AT_LLIST_HEAD(result, Element, (isectRect, SkRegion::kReplace_Op, doAA));
                if (NULL != requiresAA) {
                    *requiresAA = doAA;
                }
            }
        } else {
            *initialState = kAllOut_InitialState;
             if (NULL != requiresAA) {
                *requiresAA = false;
             }
        }
        return;
    } else {
        if (SkClipStack::kNormal_BoundsType == stackBoundsType) {
            if (!SkRect::Intersects(stackBounds, scalarQueryBounds)) {
                *initialState = kAllOut_InitialState;
                if (NULL != requiresAA) {
                   *requiresAA = false;
                }
                return;
            }
            if (NULL != tighterBounds) {
                SkIRect stackIBounds;
                stackBounds.roundOut(&stackIBounds);
                tighterBounds->intersect(queryBounds, stackIBounds);
                bounds = tighterBounds;
            }
        } else {
            if (stackBounds.contains(scalarQueryBounds)) {
                *initialState = kAllOut_InitialState;
                if (NULL != requiresAA) {
                   *requiresAA = false;
                }
                return;
            }
            if (NULL != tighterBounds) {
                *tighterBounds = queryBounds;
            }
        }
    }

    SkRect scalarBounds = SkRect::MakeFromIRect(*bounds);

    
    
    reduced_stack_walker(stack, scalarBounds, result, initialState, requiresAA);
}

void reduced_stack_walker(const SkClipStack& stack,
                          const SkRect& queryBounds,
                          ElementList* result,
                          InitialState* initialState,
                          bool* requiresAA) {

    
    
    
    

    static const InitialState kUnknown_InitialState = static_cast<InitialState>(-1);
    *initialState = kUnknown_InitialState;

    
    
    bool embiggens = false;
    bool emsmallens = false;

    SkClipStack::Iter iter(stack, SkClipStack::Iter::kTop_IterStart);
    int numAAElements = 0;
    while ((kUnknown_InitialState == *initialState)) {
        const Element* element = iter.prev();
        if (NULL == element) {
            *initialState = kAllIn_InitialState;
            break;
        }
        if (SkClipStack::kEmptyGenID == element->getGenID()) {
            *initialState = kAllOut_InitialState;
            break;
        }
        if (SkClipStack::kWideOpenGenID == element->getGenID()) {
            *initialState = kAllIn_InitialState;
            break;
        }

        bool skippable = false;
        bool isFlip = false; 

        switch (element->getOp()) {
            case SkRegion::kDifference_Op:
                
                
                if (element->isInverseFilled()) {
                    if (element->contains(queryBounds)) {
                        skippable = true;
                    } else if (!SkRect::Intersects(element->getBounds(), queryBounds)) {
                        *initialState = kAllOut_InitialState;
                        skippable = true;
                    }
                } else {
                    if (element->contains(queryBounds)) {
                        *initialState = kAllOut_InitialState;
                        skippable = true;
                    } else if (!SkRect::Intersects(element->getBounds(), queryBounds)) {
                        skippable = true;
                    }
                }
                if (!skippable) {
                    emsmallens = true;
                }
                break;
            case SkRegion::kIntersect_Op:
                
                
                
                if (element->isInverseFilled()) {
                    if (element->contains(queryBounds)) {
                        *initialState = kAllOut_InitialState;
                        skippable = true;
                    } else if (!SkRect::Intersects(element->getBounds(), queryBounds)) {
                        skippable = true;
                    }
                } else {
                    if (element->contains(queryBounds)) {
                        skippable = true;
                    } else if (!SkRect::Intersects(element->getBounds(), queryBounds)) {
                        *initialState = kAllOut_InitialState;
                        skippable = true;
                    }
                }
                if (!skippable) {
                    emsmallens = true;
                }
                break;
            case SkRegion::kUnion_Op:
                
                
                
                if (element->isInverseFilled()) {
                    if (element->contains(queryBounds)) {
                        skippable = true;
                    } else if (!SkRect::Intersects(element->getBounds(), queryBounds)) {
                        *initialState = kAllIn_InitialState;
                        skippable = true;
                    }
                } else {
                    if (element->contains(queryBounds)) {
                        *initialState = kAllIn_InitialState;
                        skippable = true;
                    } else if (!SkRect::Intersects(element->getBounds(), queryBounds)) {
                        skippable = true;
                    }
                }
                if (!skippable) {
                    embiggens = true;
                }
                break;
            case SkRegion::kXOR_Op:
                
                
                
                
                if (element->isInverseFilled()) {
                    if (element->contains(queryBounds)) {
                        skippable = true;
                    } else if (!SkRect::Intersects(element->getBounds(), queryBounds)) {
                        isFlip = true;
                    }
                } else {
                    if (element->contains(queryBounds)) {
                        isFlip = true;
                    } else if (!SkRect::Intersects(element->getBounds(), queryBounds)) {
                        skippable = true;
                    }
                }
                if (!skippable) {
                    emsmallens = embiggens = true;
                }
                break;
            case SkRegion::kReverseDifference_Op:
                
                
                
                
                if (element->isInverseFilled()) {
                    if (element->contains(queryBounds)) {
                        *initialState = kAllOut_InitialState;
                        skippable = true;
                    } else if (!SkRect::Intersects(element->getBounds(), queryBounds)) {
                        isFlip = true;
                    }
                } else {
                    if (element->contains(queryBounds)) {
                        isFlip = true;
                    } else if (!SkRect::Intersects(element->getBounds(), queryBounds)) {
                        *initialState = kAllOut_InitialState;
                        skippable = true;
                    }
                }
                if (!skippable) {
                    emsmallens = embiggens = true;
                }
                break;
            case SkRegion::kReplace_Op:
                
                
                
                
                if (element->isInverseFilled()) {
                    if (element->contains(queryBounds)) {
                        *initialState = kAllOut_InitialState;
                        skippable = true;
                    } else if (!SkRect::Intersects(element->getBounds(), queryBounds)) {
                        *initialState = kAllIn_InitialState;
                        skippable = true;
                    }
                } else {
                    if (element->contains(queryBounds)) {
                        *initialState = kAllIn_InitialState;
                        skippable = true;
                    } else if (!SkRect::Intersects(element->getBounds(), queryBounds)) {
                        *initialState = kAllOut_InitialState;
                        skippable = true;
                    }
                }
                if (!skippable) {
                    *initialState = kAllOut_InitialState;
                    embiggens = emsmallens = true;
                }
                break;
            default:
                SkDEBUGFAIL("Unexpected op.");
                break;
        }
        if (!skippable) {
            
            if (isFlip) {
                SkASSERT(SkRegion::kXOR_Op == element->getOp() ||
                         SkRegion::kReverseDifference_Op == element->getOp());
                SkNEW_INSERT_AT_LLIST_HEAD(result,
                                           Element,
                                           (queryBounds, SkRegion::kReverseDifference_Op, false));
            } else {
                Element* newElement = result->addToHead(*element);
                if (newElement->isAA()) {
                    ++numAAElements;
                }
                
                
                
                bool isReplace = SkRegion::kReplace_Op == newElement->getOp();
                if (newElement->isInverseFilled() &&
                    (SkRegion::kIntersect_Op == newElement->getOp() || isReplace)) {
                    newElement->invertShapeFillType();
                    newElement->setOp(SkRegion::kDifference_Op);
                    if (isReplace) {
                        SkASSERT(kAllOut_InitialState == *initialState);
                        *initialState = kAllIn_InitialState;
                    }
                }
            }
        }
    }

    if ((kAllOut_InitialState == *initialState && !embiggens) ||
        (kAllIn_InitialState == *initialState && !emsmallens)) {
        result->reset();
    } else {
        Element* element = result->headIter().get();
        while (NULL != element) {
            bool skippable = false;
            switch (element->getOp()) {
                case SkRegion::kDifference_Op:
                    
                    skippable = kAllOut_InitialState == *initialState;
                    break;
                case SkRegion::kIntersect_Op:
                    
                    skippable = kAllOut_InitialState == *initialState;
                    break;
                case SkRegion::kUnion_Op:
                    if (kAllIn_InitialState == *initialState) {
                        
                        skippable = true;
                    } else {
                        
                        element->setOp(SkRegion::kReplace_Op);
                    }
                    break;
                case SkRegion::kXOR_Op:
                    if (kAllOut_InitialState == *initialState) {
                        
                        element->setOp(SkRegion::kReplace_Op);
                    }
                    break;
                case SkRegion::kReverseDifference_Op:
                    if (kAllIn_InitialState == *initialState) {
                        
                        skippable = true;
                        *initialState = kAllOut_InitialState;
                    } else {
                        
                        skippable = element->isInverseFilled() ?
                            !SkRect::Intersects(element->getBounds(), queryBounds) :
                            element->contains(queryBounds);
                        if (skippable) {
                            *initialState = kAllIn_InitialState;
                        } else {
                            element->setOp(SkRegion::kReplace_Op);
                        }
                    }
                    break;
                case SkRegion::kReplace_Op:
                    skippable = false; 
                                       
                    break;
                default:
                    SkDEBUGFAIL("Unexpected op.");
                    break;
            }
            if (!skippable) {
                break;
            } else {
                if (element->isAA()) {
                    --numAAElements;
                }
                result->popHead();
                element = result->headIter().get();
            }
        }
    }
    if (NULL != requiresAA) {
        *requiresAA = numAAElements > 0;
    }
}
} 
