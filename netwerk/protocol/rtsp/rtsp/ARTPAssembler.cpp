















#include "ARTPAssembler.h"

#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AMessage.h>

#include <stdint.h>

namespace android {

static int64_t getNowUs() {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return (int64_t)tv.tv_usec + tv.tv_sec * 1000000ll;
}

ARTPAssembler::ARTPAssembler()
    : mFirstFailureTimeUs(-1) {
}

void ARTPAssembler::onPacketReceived(const sp<ARTPSource> &source) {
    AssemblyStatus status;
    for (;;) {
        status = assembleMore(source);

        if (status == WRONG_SEQUENCE_NUMBER) {
            if (mFirstFailureTimeUs >= 0) {
                if (getNowUs() - mFirstFailureTimeUs > 10000ll) {
                    mFirstFailureTimeUs = -1;

                    
                    packetLost();
                    continue;
                }
            } else {
                mFirstFailureTimeUs = getNowUs();
            }
            break;
        } else {
            mFirstFailureTimeUs = -1;

            if (status == NOT_ENOUGH_DATA) {
                break;
            }
        }
    }
}


void ARTPAssembler::CopyTimes(const sp<ABuffer> &to, const sp<ABuffer> &from) {
    uint32_t rtpTime;
    CHECK(from->meta()->findInt32("rtp-time", (int32_t *)&rtpTime));

    to->meta()->setInt32("rtp-time", rtpTime);

    
    to->setInt32Data(from->int32Data());
}

}  
