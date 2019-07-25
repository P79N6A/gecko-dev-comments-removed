








































#ifndef jsion_frames_inl_h__
#define jsion_frames_inl_h__

#include "ion/IonFrames.h"
#include "ion/Snapshots.h"

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

} 
} 

#endif 

