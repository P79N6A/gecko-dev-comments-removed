















#ifndef A_RAW_AUDIO_ASSEMBLER_H_

#define A_RAW_AUDIO_ASSEMBLER_H_

#include "mozilla/Types.h"
#include "ARTPAssembler.h"

namespace android {

struct MOZ_EXPORT AMessage;
struct MOZ_EXPORT AString;
struct MOZ_EXPORT MetaData;

struct ARawAudioAssembler : public ARTPAssembler {
    ARawAudioAssembler(
            const sp<AMessage> &notify,
            const char *desc, const AString &params);

    static bool Supports(const char *desc);

    static void MakeFormat(
            const char *desc, const sp<MetaData> &format);

protected:
    virtual ~ARawAudioAssembler();

    virtual AssemblyStatus assembleMore(const sp<ARTPSource> &source);
    virtual void onByeReceived();
    virtual void packetLost();

private:
    bool mIsWide;

    sp<AMessage> mNotifyMsg;
    bool mNextExpectedSeqNoValid;
    uint32_t mNextExpectedSeqNo;

    AssemblyStatus addPacket(const sp<ARTPSource> &source);

    DISALLOW_EVIL_CONSTRUCTORS(ARawAudioAssembler);
};

}  

#endif  
