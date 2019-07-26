















#ifndef A_BUFFER_H_

#define A_BUFFER_H_

#include <sys/types.h>
#include <stdint.h>

#include <media/stagefright/foundation/ABase.h>
#include <utils/RefBase.h>

namespace android {

struct AMessage;

struct ABuffer : public RefBase {
    ABuffer(size_t capacity);
    ABuffer(void *data, size_t capacity);

    void setFarewellMessage(const sp<AMessage> msg);

    uint8_t *base() { return (uint8_t *)mData; }
    uint8_t *data() { return (uint8_t *)mData + mRangeOffset; }
    size_t capacity() const { return mCapacity; }
    size_t size() const { return mRangeLength; }
    size_t offset() const { return mRangeOffset; }

    void setRange(size_t offset, size_t size);

    void setInt32Data(int32_t data) { mInt32Data = data; }
    int32_t int32Data() const { return mInt32Data; }

    sp<AMessage> meta();

protected:
    virtual ~ABuffer();

private:
    sp<AMessage> mFarewell;
    sp<AMessage> mMeta;

    void *mData;
    size_t mCapacity;
    size_t mRangeOffset;
    size_t mRangeLength;

    int32_t mInt32Data;

    bool mOwnsData;

    DISALLOW_EVIL_CONSTRUCTORS(ABuffer);
};

}  

#endif  
