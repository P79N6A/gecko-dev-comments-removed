






#ifndef pcscriptcache_inl_h__
#define pcscriptcache_inl_h__

#include "PcScriptCache.h"

namespace js {
namespace ion {


bool
PcScriptCache::get(JSRuntime *rt, uint32_t hash, uint8_t *addr,
                   JSScript **scriptRes, jsbytecode **pcRes)
{
    
    if (gcNumber != rt->gcNumber) {
        clear(rt->gcNumber);
        return false;
    }

    if (entries[hash].returnAddress != addr)
        return false;

    *scriptRes = entries[hash].script;
    if (pcRes)
        *pcRes = entries[hash].pc;

    return true;
}

} 
} 

#endif 
