









#include "ChannelPool.h"
#include "map_wrapper.h"
#include <string.h>
#include <assert.h>
#include "critical_section_wrapper.h"

ChannelPool::ChannelPool():
_critSect(*webrtc::CriticalSectionWrapper::CreateCriticalSection())
{
}

ChannelPool::~ChannelPool(void)
{
    assert(_channelMap.Size()==0);    
    delete &_critSect;
}

WebRtc_Word32 ChannelPool::AddChannel(int channel)
{
    return _channelMap.Insert(channel,(void*) channel);
}
WebRtc_Word32 ChannelPool::RemoveChannel(int channel)
{
    return _channelMap.Erase(channel);
}

webrtc::MapWrapper& ChannelPool::ChannelMap()
{
    return _channelMap;
}
