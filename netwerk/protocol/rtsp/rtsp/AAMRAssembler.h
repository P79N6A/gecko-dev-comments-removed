















#ifndef A_AMR_ASSEMBLER_H_

#define A_AMR_ASSEMBLER_H_

#include "mozilla/Types.h"
#include "ARTPAssembler.h"

#include <utils/List.h>

#include <stdint.h>

namespace android {

struct MOZ_EXPORT AMessage;
struct MOZ_EXPORT AString;

struct AAMRAssembler : public ARTPAssembler {
    AAMRAssembler(
            const sp<AMessage> &notify, bool isWide,
            const AString &params);

protected:
    virtual ~AAMRAssembler();

    virtual AssemblyStatus assembleMore(const sp<ARTPSource> &source);
    virtual void onByeReceived();
    virtual void packetLost();

private:
    bool mIsWide;

    sp<AMessage> mNotifyMsg;
    bool mNextExpectedSeqNoValid;
    uint32_t mNextExpectedSeqNo;

    AssemblyStatus addPacket(const sp<ARTPSource> &source);

    DISALLOW_EVIL_CONSTRUCTORS(AAMRAssembler);
};

}  

#endif  

