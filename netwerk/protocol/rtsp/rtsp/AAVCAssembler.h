















#ifndef A_AVC_ASSEMBLER_H_

#define A_AVC_ASSEMBLER_H_

#include "mozilla/Types.h"
#include "ARTPAssembler.h"

#include <utils/List.h>
#include <utils/RefBase.h>

namespace android {

struct MOZ_EXPORT ABuffer;
struct MOZ_EXPORT AMessage;

struct AAVCAssembler : public ARTPAssembler {
    AAVCAssembler(const sp<AMessage> &notify);

protected:
    virtual ~AAVCAssembler();

    virtual AssemblyStatus assembleMore(const sp<ARTPSource> &source);
    virtual void onByeReceived();
    virtual void packetLost();

private:
    sp<AMessage> mNotifyMsg;

    uint32_t mAccessUnitRTPTime;
    bool mNextExpectedSeqNoValid;
    uint32_t mNextExpectedSeqNo;
    bool mAccessUnitDamaged;
    List<sp<ABuffer> > mNALUnits;

    AssemblyStatus addNALUnit(const sp<ARTPSource> &source);
    void addSingleNALUnit(const sp<ABuffer> &buffer);
    AssemblyStatus addFragmentedNALUnit(List<sp<ABuffer> > *queue);
    bool addSingleTimeAggregationPacket(const sp<ABuffer> &buffer);

    void submitAccessUnit();

    DISALLOW_EVIL_CONSTRUCTORS(AAVCAssembler);
};

}  

#endif  
