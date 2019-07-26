















#ifndef MEDIA_BUFFER_GROUP_H_

#define MEDIA_BUFFER_GROUP_H_

#include <media/stagefright/MediaBuffer.h>
#include <utils/Errors.h>
#include <utils/threads.h>

namespace android {

class MediaBuffer;
class MetaData;

class MediaBufferGroup : public MediaBufferObserver {
public:
    MediaBufferGroup();
    ~MediaBufferGroup();

    void add_buffer(MediaBuffer *buffer);

    
    
    status_t acquire_buffer(MediaBuffer **buffer);

protected:
    virtual void signalBufferReturned(MediaBuffer *buffer);

private:
    friend class MediaBuffer;

    Mutex mLock;
    Condition mCondition;

    MediaBuffer *mFirstBuffer, *mLastBuffer;

    MediaBufferGroup(const MediaBufferGroup &);
    MediaBufferGroup &operator=(const MediaBufferGroup &);
};

}  

#endif  
