















#ifndef A_MPEG4_ELEM_ASSEMBLER_H_

#define A_MPEG4_ELEM_ASSEMBLER_H_

#include "mozilla/Types.h"
#include "ARTPAssembler.h"

#include <media/stagefright/foundation/AString.h>

#include <utils/List.h>
#include <utils/RefBase.h>

namespace android {

struct MOZ_EXPORT ABuffer;
struct MOZ_EXPORT AMessage;

struct AMPEG4ElementaryAssembler : public ARTPAssembler {
    AMPEG4ElementaryAssembler(
            const sp<AMessage> &notify, const AString &desc,
            const AString &params);

protected:
    virtual ~AMPEG4ElementaryAssembler();

    virtual AssemblyStatus assembleMore(const sp<ARTPSource> &source);
    virtual void onByeReceived();
    virtual void packetLost();

private:
    sp<AMessage> mNotifyMsg;
    bool mIsGeneric;
    AString mParams;

    unsigned mSizeLength;
    unsigned mIndexLength;
    unsigned mIndexDeltaLength;
    unsigned mCTSDeltaLength;
    unsigned mDTSDeltaLength;
    bool mRandomAccessIndication;
    unsigned mStreamStateIndication;
    unsigned mAuxiliaryDataSizeLength;
    unsigned mConstantDuration;
    unsigned mPreviousAUCount;
    bool mHasAUHeader;

    uint32_t mAccessUnitRTPTime;
    bool mNextExpectedSeqNoValid;
    uint32_t mNextExpectedSeqNo;
    bool mAccessUnitDamaged;
    List<sp<ABuffer> > mPackets;

    AssemblyStatus addPacket(const sp<ARTPSource> &source);
    void submitAccessUnit();

    DISALLOW_EVIL_CONSTRUCTORS(AMPEG4ElementaryAssembler);
};

}  

#endif  
