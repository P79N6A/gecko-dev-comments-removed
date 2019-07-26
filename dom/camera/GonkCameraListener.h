















#ifndef GONK_CAMERA_LISTENER_H
#define GONK_CAMERA_LISTENER_H

#include <utils/Timers.h>
#include <camera/Camera.h>

namespace android {


class GonkCameraListener: virtual public RefBase
{
public:
    virtual void notify(int32_t msgType, int32_t ext1, int32_t ext2) = 0;
    virtual void postData(int32_t msgType, const sp<IMemory>& dataPtr,
                          camera_frame_metadata_t *metadata) = 0;
    virtual void postDataTimestamp(nsecs_t timestamp, int32_t msgType, const sp<IMemory>& dataPtr) = 0;
};

}; 

#endif
