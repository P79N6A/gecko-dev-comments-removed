















#ifndef A_H263_ASSEMBLER_H_

#define A_H263_ASSEMBLER_H_

#include "mozilla/Types.h"
#include "ARTPAssembler.h"

#include <utils/List.h>

#include <stdint.h>

namespace android {

struct MOZ_EXPORT AMessage;

struct AH263Assembler : public ARTPAssembler {
    AH263Assembler(const sp<AMessage> &notify);

protected:
    virtual ~AH263Assembler();

    virtual AssemblyStatus assembleMore(const sp<ARTPSource> &source);
    virtual void onByeReceived();
    virtual void packetLost();

private:
    sp<AMessage> mNotifyMsg;
    uint32_t mAccessUnitRTPTime;
    bool mNextExpectedSeqNoValid;
    uint32_t mNextExpectedSeqNo;
    bool mAccessUnitDamaged;
    List<sp<ABuffer> > mPackets;

    AssemblyStatus addPacket(const sp<ARTPSource> &source);
    void submitAccessUnit();

    DISALLOW_EVIL_CONSTRUCTORS(AH263Assembler);
};

}  

#endif  
