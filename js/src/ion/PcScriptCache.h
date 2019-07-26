






#ifndef pcscriptcache_h__
#define pcscriptcache_h__




struct JSRuntime;

namespace js {
namespace ion {

struct PcScriptCacheEntry
{
    uint8_t *returnAddress; 
    jsbytecode *pc;         
    RawScript script;       
};

struct PcScriptCache
{
    static const uint32_t Length = 73;

    
    
    
    uint64_t gcNumber;

    
    PcScriptCacheEntry entries[Length];

    void clear(uint64_t gcNumber) {
        for (uint32_t i = 0; i < Length; i++)
            entries[i].returnAddress = NULL;
        this->gcNumber = gcNumber;
    }

    
    
    bool get(JSRuntime *rt, uint32_t hash, uint8_t *addr,
             JSScript **scriptRes, jsbytecode **pcRes);

    void add(uint32_t hash, uint8_t *addr, jsbytecode *pc, RawScript script) {
        entries[hash].returnAddress = addr;
        entries[hash].pc = pc;
        entries[hash].script = script;
    }

    static uint32_t Hash(uint8_t *addr) {
        uint32_t key = (uint32_t)((uintptr_t)addr);
        return ((key >> 3) * 2654435761u) % Length;
    }
};

} 
} 

#endif 
