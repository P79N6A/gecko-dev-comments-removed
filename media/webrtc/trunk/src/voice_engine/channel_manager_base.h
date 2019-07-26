









#ifndef WEBRTC_VOICE_ENGINE_CHANNEL_MANAGER_BASE_H
#define WEBRTC_VOICE_ENGINE_CHANNEL_MANAGER_BASE_H

#include "typedefs.h"
#include "map_wrapper.h"
#include "voice_engine_defines.h"

namespace webrtc
{
class CriticalSectionWrapper;
class RWLockWrapper;

namespace voe
{

class ScopedChannel;
class Channel;

class ChannelManagerBase
{
public:
    enum {KMaxNumberOfItems = kVoiceEngineMaxNumOfChannels};

protected:
    bool CreateItem(WebRtc_Word32& itemId);

    void InsertItem(WebRtc_Word32 itemId, void* item);

    void* RemoveItem(WebRtc_Word32 itemId);

    void* GetItem(WebRtc_Word32 itemId) const;

    void* GetFirstItem(void*& iterator) const ;

    void* GetNextItem(void*& iterator) const;

    void ReleaseItem();

    void AddFreeItemId(WebRtc_Word32 itemId);

    bool GetFreeItemId(WebRtc_Word32& itemId);

    void RemoveFreeItemIds();

    void DestroyAllItems();

    WebRtc_Word32 NumOfItems() const;

    WebRtc_Word32 MaxNumOfItems() const;

    void GetItemIds(WebRtc_Word32* channelsArray,
                    WebRtc_Word32& numOfChannels) const;

    void GetChannels(MapWrapper& channels) const;

    virtual void* NewItem(WebRtc_Word32 itemId) = 0;

    virtual void DeleteItem(void* item) = 0;

    ChannelManagerBase();

    virtual ~ChannelManagerBase();

private:
    
    CriticalSectionWrapper* _itemsCritSectPtr;

    MapWrapper _items;

    bool _freeItemIds[KMaxNumberOfItems];

    
    RWLockWrapper* _itemsRWLockPtr;
};

} 

} 

#endif
