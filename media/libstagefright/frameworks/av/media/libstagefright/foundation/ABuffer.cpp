















#include "ABuffer.h"

#include "ADebug.h"
#include "ALooper.h"
#include "AMessage.h"

namespace stagefright {

ABuffer::ABuffer(size_t capacity)
    : mData(malloc(capacity)),
      mCapacity(capacity),
      mRangeOffset(0),
      mRangeLength(capacity),
      mInt32Data(0),
      mOwnsData(true) {
}

ABuffer::ABuffer(void *data, size_t capacity)
    : mData(data),
      mCapacity(capacity),
      mRangeOffset(0),
      mRangeLength(capacity),
      mInt32Data(0),
      mOwnsData(false) {
}

ABuffer::~ABuffer() {
    if (mOwnsData) {
        if (mData != NULL) {
            free(mData);
            mData = NULL;
        }
    }

    if (mFarewell != NULL) {
        mFarewell->post();
    }
}

void ABuffer::setRange(size_t offset, size_t size) {
    CHECK_LE(offset, mCapacity);
    CHECK_LE(offset + size, mCapacity);

    mRangeOffset = offset;
    mRangeLength = size;
}

void ABuffer::setFarewellMessage(const sp<AMessage> msg) {
    mFarewell = msg;
}

sp<AMessage> ABuffer::meta() {
    if (mMeta == NULL) {
        mMeta = new AMessage;
    }
    return mMeta;
}

}  

