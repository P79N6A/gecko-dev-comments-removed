








































#ifndef jsion_frames_inl_h__
#define jsion_frames_inl_h__

#include "ion/IonFrames.h"
#include "ion/Snapshots.h"
#include "ion/IonFrameIterator.h"

namespace js {
namespace ion {

bool
IonFrameInfo::hasSnapshotOffset() const
{
    return snapshotOffset_ != INVALID_SNAPSHOT_OFFSET;
}

SnapshotOffset
IonFrameInfo::snapshotOffset() const
{
    JS_ASSERT(hasSnapshotOffset());
    return snapshotOffset_;
}

static inline size_t
SizeOfFramePrefix(FrameType type)
{
    switch (type) {
      case IonFrame_Entry:
        return sizeof(IonEntryFrameLayout);
      case IonFrame_JS:
        return sizeof(IonJSFrameLayout);
      case IonFrame_Rectifier:
        return sizeof(IonRectifierFrameLayout);
      case IonFrame_Exit:
        return sizeof(IonExitFrameLayout);
      default:
        JS_NOT_REACHED("unknown frame type");
    }
    return 0;
}

} 
} 

#endif 

