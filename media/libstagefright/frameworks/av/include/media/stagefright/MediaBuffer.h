















#ifndef MEDIA_BUFFER_H_

#define MEDIA_BUFFER_H_

#include <pthread.h>

#include <utils/Errors.h>
#include <utils/RefBase.h>
#include "nsTArray.h"

namespace stagefright {

struct ABuffer;
class GraphicBuffer;
class MediaBuffer;
class MediaBufferObserver;
class MetaData;

class MediaBufferObserver {
public:
    MediaBufferObserver() {}
    virtual ~MediaBufferObserver() {}

    virtual void signalBufferReturned(MediaBuffer *buffer) = 0;

private:
    MediaBufferObserver(const MediaBufferObserver &);
    MediaBufferObserver &operator=(const MediaBufferObserver &);
};

class MediaBuffer {
public:
    
    MediaBuffer(void *data, size_t size);

    MediaBuffer(size_t size);

    MediaBuffer(const sp<GraphicBuffer>& graphicBuffer);

    MediaBuffer(const sp<ABuffer> &buffer);

    
    
    void release();

    
    void add_ref();

    void *data() const;
    size_t size() const;

    size_t range_offset() const;
    size_t range_length() const;

    void set_range(size_t offset, size_t length);

    sp<GraphicBuffer> graphicBuffer() const;

    sp<MetaData> meta_data();

    
    void reset();

    void setObserver(MediaBufferObserver *group);

    
    
    
    MediaBuffer *clone();

    int refcount() const;

    bool ensuresize(size_t length);

protected:
    virtual ~MediaBuffer();

private:
    friend class MediaBufferGroup;
    friend class OMXDecoder;

    
    
    void claim();

    MediaBufferObserver *mObserver;
    MediaBuffer *mNextBuffer;
    int mRefCount;

    void *mData;
    size_t mSize, mRangeOffset, mRangeLength;
    sp<GraphicBuffer> mGraphicBuffer;
    sp<ABuffer> mBuffer;

    bool mOwnsData;

    sp<MetaData> mMetaData;

    MediaBuffer *mOriginal;

    void setNextBuffer(MediaBuffer *buffer);
    MediaBuffer *nextBuffer();

    MediaBuffer(const MediaBuffer &);
    MediaBuffer &operator=(const MediaBuffer &);

    FallibleTArray<uint8_t> mBufferBackend;
};

}  

#endif  
