









#ifndef WEBRTC_VOICE_ENGINE_CHANNEL_MANAGER_H
#define WEBRTC_VOICE_ENGINE_CHANNEL_MANAGER_H

#include "channel_manager_base.h"
#include "typedefs.h"

namespace webrtc
{

namespace voe
{

class ScopedChannel;
class Channel;

class ChannelManager: private ChannelManagerBase
{
    friend class ScopedChannel;

public:
    bool CreateChannel(int32_t& channelId);

    int32_t DestroyChannel(const int32_t channelId);

    int32_t MaxNumOfChannels() const;

    int32_t NumOfChannels() const;

    void GetChannelIds(int32_t* channelsArray,
                       int32_t& numOfChannels) const;

    ChannelManager(const uint32_t instanceId);

    ~ChannelManager();

private:
    ChannelManager(const ChannelManager&);

    ChannelManager& operator=(const ChannelManager&);

    Channel* GetChannel(const int32_t channelId) const;

    void GetChannels(MapWrapper& channels) const;

    void ReleaseChannel();

    virtual void* NewItem(int32_t itemID);

    virtual void DeleteItem(void* item);

    uint32_t _instanceId;
};

class ScopedChannel
{
public:
    
    ScopedChannel(ChannelManager& chManager);

    ScopedChannel(ChannelManager& chManager, int32_t channelId);

    Channel* ChannelPtr();

    Channel* GetFirstChannel(void*& iterator) const;

    Channel* GetNextChannel(void*& iterator) const;

    ~ScopedChannel();
private:
    ChannelManager& _chManager;
    Channel* _channelPtr;
    MapWrapper _channels;
};

} 

} 

#endif  
