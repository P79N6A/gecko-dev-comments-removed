









#ifndef WEBRTC_VOICE_ENGINE_DTMF_INBAND_QUEUE_H
#define WEBRTC_VOICE_ENGINE_DTMF_INBAND_QUEUE_H

#include "critical_section_wrapper.h"
#include "typedefs.h"
#include "voice_engine_defines.h"


namespace webrtc {

class DtmfInbandQueue
{
public:

    DtmfInbandQueue(const WebRtc_Word32 id);

    virtual ~DtmfInbandQueue();

    int AddDtmf(WebRtc_UWord8 DtmfKey,
                WebRtc_UWord16 len,
                WebRtc_UWord8 level);

    WebRtc_Word8 NextDtmf(WebRtc_UWord16* len, WebRtc_UWord8* level);

    bool PendingDtmf();

    void ResetDtmf();

private:
    enum {kDtmfInbandMax = 20};

    WebRtc_Word32 _id;
    CriticalSectionWrapper& _DtmfCritsect;
    WebRtc_UWord8 _nextEmptyIndex;
    WebRtc_UWord8 _DtmfKey[kDtmfInbandMax];
    WebRtc_UWord16 _DtmfLen[kDtmfInbandMax];
    WebRtc_UWord8 _DtmfLevel[kDtmfInbandMax];
};

}   

#endif
