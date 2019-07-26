









#ifndef WEBRTC_MODULES_AUDIO_CONFERENCE_MIXER_SOURCE_MEMORY_POOL_GENERIC_H_
#define WEBRTC_MODULES_AUDIO_CONFERENCE_MIXER_SOURCE_MEMORY_POOL_GENERIC_H_

#include <assert.h>
#include <list>

#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/typedefs.h"

namespace webrtc {
template<class MemoryType>
class MemoryPoolImpl
{
public:
    
    int32_t PopMemory(MemoryType*&  memory);
    int32_t PushMemory(MemoryType*& memory);

    MemoryPoolImpl(int32_t initialPoolSize);
    ~MemoryPoolImpl();

    
    int32_t Terminate();
    bool Initialize();
private:
    
    int32_t CreateMemory(uint32_t amountToCreate);

    CriticalSectionWrapper* _crit;

    bool _terminate;

    std::list<MemoryType*> _memoryPool;

    uint32_t _initialPoolSize;
    uint32_t _createdMemory;
    uint32_t _outstandingMemory;
};

template<class MemoryType>
MemoryPoolImpl<MemoryType>::MemoryPoolImpl(int32_t initialPoolSize)
    : _crit(CriticalSectionWrapper::CreateCriticalSection()),
      _terminate(false),
      _initialPoolSize(initialPoolSize),
      _createdMemory(0),
      _outstandingMemory(0)
{
}

template<class MemoryType>
MemoryPoolImpl<MemoryType>::~MemoryPoolImpl()
{
    
    assert(_createdMemory == 0);
    assert(_outstandingMemory == 0);
    delete _crit;
}

template<class MemoryType>
int32_t MemoryPoolImpl<MemoryType>::PopMemory(MemoryType*& memory)
{
    CriticalSectionScoped cs(_crit);
    if(_terminate)
    {
        memory = NULL;
        return -1;
    }
    if (_memoryPool.empty()) {
        
        CreateMemory(_initialPoolSize);
        if(_memoryPool.empty())
        {
            memory = NULL;
            return -1;
        }
    }
    memory = _memoryPool.front();
    _memoryPool.pop_front();
    _outstandingMemory++;
    return 0;
}

template<class MemoryType>
int32_t MemoryPoolImpl<MemoryType>::PushMemory(MemoryType*& memory)
{
    if(memory == NULL)
    {
        return -1;
    }
    CriticalSectionScoped cs(_crit);
    _outstandingMemory--;
    if(_memoryPool.size() > (_initialPoolSize << 1))
    {
        
        _createdMemory--;
        delete memory;
        memory = NULL;
        return 0;
    }
    _memoryPool.push_back(memory);
    memory = NULL;
    return 0;
}

template<class MemoryType>
bool MemoryPoolImpl<MemoryType>::Initialize()
{
    CriticalSectionScoped cs(_crit);
    return CreateMemory(_initialPoolSize) == 0;
}

template<class MemoryType>
int32_t MemoryPoolImpl<MemoryType>::Terminate()
{
    CriticalSectionScoped cs(_crit);
    assert(_createdMemory == _outstandingMemory + _memoryPool.size());

    _terminate = true;
    
    while(_createdMemory > 0)
    {
        MemoryType* memory = _memoryPool.front();
        _memoryPool.pop_front();
        delete memory;
        _createdMemory--;
    }
    return 0;
}

template<class MemoryType>
int32_t MemoryPoolImpl<MemoryType>::CreateMemory(
    uint32_t amountToCreate)
{
    for(uint32_t i = 0; i < amountToCreate; i++)
    {
        MemoryType* memory = new MemoryType();
        if(memory == NULL)
        {
            return -1;
        }
        _memoryPool.push_back(memory);
        _createdMemory++;
    }
    return 0;
}
}  

#endif 
