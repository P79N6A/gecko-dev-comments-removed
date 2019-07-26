









#ifndef WEBRTC_MODULES_AUDIO_CONFERENCE_MIXER_SOURCE_MEMORY_POOL_WINDOWS_H_
#define WEBRTC_MODULES_AUDIO_CONFERENCE_MIXER_SOURCE_MEMORY_POOL_WINDOWS_H_

#include <assert.h>
#include <windows.h>

#include "aligned_malloc.h"
#include "atomic32.h"
#include "typedefs.h"

namespace webrtc {
template<class MemoryType> struct MemoryPoolItem;

template<class MemoryType>
struct MemoryPoolItemPayload
{
    MemoryPoolItemPayload()
        : memoryType(),
          base(NULL)
    {
    }
    MemoryType                  memoryType;
    MemoryPoolItem<MemoryType>* base;
};

template<class MemoryType>
struct MemoryPoolItem
{
    
    SLIST_ENTRY itemEntry;
    
    MemoryPoolItemPayload<MemoryType>* payload;
};

template<class MemoryType>
class MemoryPoolImpl
{
public:
    
    WebRtc_Word32 PopMemory(MemoryType*&  memory);
    WebRtc_Word32 PushMemory(MemoryType*& memory);

    MemoryPoolImpl(WebRtc_Word32 );
    ~MemoryPoolImpl();

    
    WebRtc_Word32 Terminate();
    bool Initialize();
private:
    
    MemoryPoolItem<MemoryType>* CreateMemory();

    
    

    
    PSLIST_HEADER _pListHead;

    Atomic32 _createdMemory;
    Atomic32 _outstandingMemory;
};

template<class MemoryType>
MemoryPoolImpl<MemoryType>::MemoryPoolImpl(
    WebRtc_Word32 )
    : _pListHead(NULL),
      _createdMemory(0),
      _outstandingMemory(0)
{
}

template<class MemoryType>
MemoryPoolImpl<MemoryType>::~MemoryPoolImpl()
{
    Terminate();
    if(_pListHead != NULL)
    {
        AlignedFree(reinterpret_cast<void*>(_pListHead));
        _pListHead = NULL;
    }
    
    assert(_createdMemory.Value() == 0);
    assert(_outstandingMemory.Value() == 0);
}

template<class MemoryType>
WebRtc_Word32 MemoryPoolImpl<MemoryType>::PopMemory(MemoryType*& memory)
{
    PSLIST_ENTRY pListEntry = InterlockedPopEntrySList(_pListHead);
    if(pListEntry == NULL)
    {
        MemoryPoolItem<MemoryType>* item = CreateMemory();
        if(item == NULL)
        {
            return -1;
        }
        pListEntry = &(item->itemEntry);
    }
    ++_outstandingMemory;
    memory = &((MemoryPoolItem<MemoryType>*)pListEntry)->payload->memoryType;
    return 0;
}

template<class MemoryType>
WebRtc_Word32 MemoryPoolImpl<MemoryType>::PushMemory(MemoryType*& memory)
{
    if(memory == NULL)
    {
        return -1;
    }

    MemoryPoolItem<MemoryType>* item =
        ((MemoryPoolItemPayload<MemoryType>*)memory)->base;

    const WebRtc_Word32 usedItems  = --_outstandingMemory;
    const WebRtc_Word32 totalItems = _createdMemory.Value();
    const WebRtc_Word32 freeItems  = totalItems - usedItems;
    if(freeItems < 0)
    {
        assert(false);
        delete item->payload;
        AlignedFree(item);
        return -1;
    }
    if(freeItems >= totalItems>>1)
    {
        delete item->payload;
        AlignedFree(item);
        --_createdMemory;
        return 0;
    }
    InterlockedPushEntrySList(_pListHead,&(item->itemEntry));
    return 0;
}

template<class MemoryType>
bool MemoryPoolImpl<MemoryType>::Initialize()
{
    _pListHead = (PSLIST_HEADER)AlignedMalloc(sizeof(SLIST_HEADER),
                                              MEMORY_ALLOCATION_ALIGNMENT);
    if(_pListHead == NULL)
    {
        return false;
    }
    InitializeSListHead(_pListHead);
    return true;
}

template<class MemoryType>
WebRtc_Word32 MemoryPoolImpl<MemoryType>::Terminate()
{
    WebRtc_Word32 itemsFreed = 0;
    PSLIST_ENTRY pListEntry = InterlockedPopEntrySList(_pListHead);
    while(pListEntry != NULL)
    {
        MemoryPoolItem<MemoryType>* item = ((MemoryPoolItem<MemoryType>*)pListEntry);
        delete item->payload;
        AlignedFree(item);
        --_createdMemory;
        itemsFreed++;
        pListEntry = InterlockedPopEntrySList(_pListHead);
    }
    return itemsFreed;
}

template<class MemoryType>
MemoryPoolItem<MemoryType>* MemoryPoolImpl<MemoryType>::CreateMemory()
{
    MemoryPoolItem<MemoryType>* returnValue = (MemoryPoolItem<MemoryType>*)
        AlignedMalloc(sizeof(MemoryPoolItem<MemoryType>),
                      MEMORY_ALLOCATION_ALIGNMENT);
    if(returnValue == NULL)
    {
        return NULL;
    }

    returnValue->payload = new MemoryPoolItemPayload<MemoryType>();
    if(returnValue->payload == NULL)
    {
        delete returnValue;
        return NULL;
    }
    returnValue->payload->base = returnValue;
    ++_createdMemory;
    return returnValue;
}
} 

#endif 
