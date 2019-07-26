















#ifndef A_RTP_ASSEMBLER_H_

#define A_RTP_ASSEMBLER_H_

#include "mozilla/Types.h"
#include <media/stagefright/foundation/ABase.h>
#include <utils/RefBase.h>

namespace android {

struct MOZ_EXPORT ABuffer;
struct ARTPSource;

struct ARTPAssembler : public RefBase {
    enum AssemblyStatus {
        MALFORMED_PACKET,
        WRONG_SEQUENCE_NUMBER,
        NOT_ENOUGH_DATA,
        OK
    };

    ARTPAssembler();

    void onPacketReceived(const sp<ARTPSource> &source);
    virtual void onByeReceived() = 0;

protected:
    virtual AssemblyStatus assembleMore(const sp<ARTPSource> &source) = 0;
    virtual void packetLost() = 0;

    static void CopyTimes(const sp<ABuffer> &to, const sp<ABuffer> &from);

private:
    int64_t mFirstFailureTimeUs;

    DISALLOW_EVIL_CONSTRUCTORS(ARTPAssembler);
};

}  

#endif  
