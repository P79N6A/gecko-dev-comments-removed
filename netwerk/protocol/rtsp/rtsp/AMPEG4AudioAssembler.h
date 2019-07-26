















#ifndef A_MPEG4_AUDIO_ASSEMBLER_H_

#define A_MPEG4_AUDIO_ASSEMBLER_H_

#include "mozilla/Types.h"
#include "ARTPAssembler.h"

#include <utils/List.h>

#include <stdint.h>

namespace android {

struct MOZ_EXPORT AMessage;
struct MOZ_EXPORT AString;

struct AMPEG4AudioAssembler : public ARTPAssembler {
    AMPEG4AudioAssembler(
            const sp<AMessage> &notify, const AString &params);

protected:
    virtual ~AMPEG4AudioAssembler();

    virtual AssemblyStatus assembleMore(const sp<ARTPSource> &source);
    virtual void onByeReceived();
    virtual void packetLost();

private:
    sp<AMessage> mNotifyMsg;

    bool mMuxConfigPresent;
    unsigned mNumSubFrames;
    unsigned mFrameLengthType;
    ssize_t mFixedFrameLength;
    bool mOtherDataPresent;
    unsigned mOtherDataLenBits;

    uint32_t mAccessUnitRTPTime;
    bool mNextExpectedSeqNoValid;
    uint32_t mNextExpectedSeqNo;
    bool mAccessUnitDamaged;
    List<sp<ABuffer> > mPackets;

    AssemblyStatus addPacket(const sp<ARTPSource> &source);
    void submitAccessUnit();

    sp<ABuffer> removeLATMFraming(const sp<ABuffer> &buffer);

    DISALLOW_EVIL_CONSTRUCTORS(AMPEG4AudioAssembler);
};

}  

#endif  
