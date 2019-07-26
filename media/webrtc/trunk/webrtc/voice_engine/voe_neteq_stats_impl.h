









#ifndef WEBRTC_VOICE_ENGINE_VOE_NETEQ_STATS_IMPL_H
#define WEBRTC_VOICE_ENGINE_VOE_NETEQ_STATS_IMPL_H

#include "voe_neteq_stats.h"

#include "shared_data.h"

namespace webrtc {

class VoENetEqStatsImpl : public VoENetEqStats
{
public:
    virtual int GetNetworkStatistics(int channel,
                                     NetworkStatistics& stats);

protected:
    VoENetEqStatsImpl(voe::SharedData* shared);
    virtual ~VoENetEqStatsImpl();

private:
    voe::SharedData* _shared;
};

}  

#endif    
