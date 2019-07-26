









#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_DTMF_QUEUE_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_DTMF_QUEUE_H_

#include "typedefs.h"
#include "rtp_rtcp_config.h"

#include "critical_section_wrapper.h"

namespace webrtc {
class DTMFqueue
{
public:
    DTMFqueue();
    virtual ~DTMFqueue();

    int32_t AddDTMF(uint8_t DTMFKey, uint16_t len, uint8_t level);
    int8_t NextDTMF(uint8_t* DTMFKey, uint16_t * len, uint8_t * level);
    bool PendingDTMF();
    void ResetDTMF();

private:
    CriticalSectionWrapper* _DTMFCritsect;
    uint8_t        _nextEmptyIndex;
    uint8_t        _DTMFKey[DTMF_OUTBAND_MAX];
    uint16_t       _DTMFLen[DTMF_OUTBAND_MAX];
    uint8_t        _DTMFLevel[DTMF_OUTBAND_MAX];
};
} 

#endif 
