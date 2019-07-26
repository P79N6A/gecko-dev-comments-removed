





#ifndef jit_PcScriptCache_h
#define jit_PcScriptCache_h




struct JSRuntime;

namespace js {
namespace jit {

struct PcScriptCacheEntry
{
    uint8_t *returnAddress; 
    jsbytecode *pc;         
    JSScript *script;       
};

struct PcScriptCache
{
    static const uint32_t Length = 73;

    
    
    
    uint64_t gcNumber;

    
    PcScriptCacheEntry entries[Length];

    void clear(uint64_t gcNumber) {
        for (uint32_t i = 0; i < Length; i++)
            entries[i].returnAddress = nullptr;
        this->gcNumber = gcNumber;
    }

    
    bool get(JSRuntime *rt, uint32_t hash, uint8_t *addr,
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

    void add(uint32_t hash, uint8_t *addr, jsbytecode *pc, JSScript *script) {
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
