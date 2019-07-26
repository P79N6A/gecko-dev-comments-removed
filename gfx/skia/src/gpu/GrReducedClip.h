







#ifndef GrReducedClip_DEFINED
#define GrReducedClip_DEFINED

#include "SkClipStack.h"
#include "SkTLList.h"

namespace GrReducedClip {

typedef SkTLList<SkClipStack::Element> ElementList;

enum InitialState {
    kAllIn_InitialState,
    kAllOut_InitialState,
};













void ReduceClipStack(const SkClipStack& stack,
                     const SkIRect& queryBounds,
                     ElementList* result,
                     InitialState* initialState,
                     SkIRect* tighterBounds = NULL,
                     bool* requiresAA = NULL);

} 

#endif
