









#ifndef WEBRTC_MODULES_AUDIO_CONFERENCE_MIXER_SOURCE_MEMORY_POOL_H_
#define WEBRTC_MODULES_AUDIO_CONFERENCE_MIXER_SOURCE_MEMORY_POOL_H_

#include <assert.h>

#include "webrtc/typedefs.h"

#if _WIN32
#include "webrtc/modules/audio_conference_mixer/source/memory_pool_win.h"
#else
#include "webrtc/modules/audio_conference_mixer/source/memory_pool_posix.h"
#endif

namespace webrtc {

template<class MemoryType>
class MemoryPool
{
public:
    
    static int32_t CreateMemoryPool(MemoryPool*& memoryPool,
                                    uint32_t initialPoolSize);

    
    
    static int32_t DeleteMemoryPool(
        MemoryPool*& memoryPool);

    
    int32_t PopMemory(MemoryType*&  memory);
    int32_t PushMemory(MemoryType*& memory);
private:
    MemoryPool(int32_t initialPoolSize);
    ~MemoryPool();

    MemoryPoolImpl<MemoryType>* _ptrImpl;
};

template<class MemoryType>
MemoryPool<MemoryType>::MemoryPool(int32_t initialPoolSize)
{
    _ptrImpl = new MemoryPoolImpl<MemoryType>(initialPoolSize);
}

template<class MemoryType>
MemoryPool<MemoryType>::~MemoryPool()
{
    delete _ptrImpl;
}

template<class MemoryType> int32_t
MemoryPool<MemoryType>::CreateMemoryPool(MemoryPool*&   memoryPool,
                                         uint32_t initialPoolSize)
{
    memoryPool = new MemoryPool(initialPoolSize);
    if(memoryPool == NULL)
    {
        return -1;
    }
    if(memoryPool->_ptrImpl == NULL)
    {
        delete memoryPool;
        memoryPool = NULL;
        return -1;
    }
    if(!memoryPool->_ptrImpl->Initialize())
    {
        delete memoryPool;
        memoryPool = NULL;
        return -1;
    }
    return 0;
}

template<class MemoryType>
int32_t MemoryPool<MemoryType>::DeleteMemoryPool(MemoryPool*& memoryPool)
{
    if(memoryPool == NULL)
    {
        return -1;
    }
    if(memoryPool->_ptrImpl == NULL)
    {
        return -1;
    }
    if(memoryPool->_ptrImpl->Terminate() == -1)
    {
        return -1;
    }
    delete memoryPool;
    memoryPool = NULL;
    return 0;
}

template<class MemoryType>
int32_t MemoryPool<MemoryType>::PopMemory(MemoryType*& memory)
{
    return _ptrImpl->PopMemory(memory);
}

template<class MemoryType>
int32_t MemoryPool<MemoryType>::PushMemory(MemoryType*& memory)
{
    if(memory == NULL)
    {
        return -1;
    }
    return _ptrImpl->PushMemory(memory);
}
}  

#endif 
