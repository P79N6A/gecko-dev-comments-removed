









#pragma once
#include "StdAfx.h"
#include "common_types.h"

#include "vie_base.h"
#include "map_wrapper.h"

namespace webrtc {
class CriticalSectionWrapper;
}

class ChannelPool
{
public:
    ChannelPool();
    ~ChannelPool(void);
    WebRtc_Word32 AddChannel(int channel);
    WebRtc_Word32 RemoveChannel(int channel);    

    webrtc::MapWrapper& ChannelMap();

    private:     
        webrtc::CriticalSectionWrapper& _critSect;        
        webrtc::MapWrapper _channelMap;

};
