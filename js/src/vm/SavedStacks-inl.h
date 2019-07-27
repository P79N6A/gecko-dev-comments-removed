





#ifndef vm_SavedStacksInl_h
#define vm_SavedStacksInl_h

#include "vm/SavedStacks.h"











inline void
js::AssertObjectIsSavedFrameOrWrapper(JSContext* cx, HandleObject stack)
{
#ifdef DEBUG
    if (stack) {
        RootedObject savedFrameObj(cx, CheckedUnwrap(stack));
        MOZ_ASSERT(savedFrameObj);
        MOZ_ASSERT(js::SavedFrame::isSavedFrameAndNotProto(*savedFrameObj));
    }
#endif
}

#endif 
