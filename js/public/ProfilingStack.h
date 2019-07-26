





#ifndef js_ProfilingStack_h
#define js_ProfilingStack_h

#include "mozilla/NullPtr.h"
 
#include "jsbytecode.h"
#include "jstypes.h"

#include "js/Utility.h"

struct JSRuntime;

namespace js {






class ProfileEntry
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    const char * volatile string; 
    void * volatile sp;           
                                  
                                  
    JSScript * volatile script_;  
                                  
    int32_t volatile idx;         

  public:
    static const uintptr_t SCRIPT_OPT_STACKPOINTER = 0x1;

    
    
    
    

    bool js() const volatile {
        MOZ_ASSERT_IF(uintptr_t(sp) <= SCRIPT_OPT_STACKPOINTER, script_ != nullptr);
        return uintptr_t(sp) <= SCRIPT_OPT_STACKPOINTER;
    }

    uint32_t line() const volatile { MOZ_ASSERT(!js()); return idx; }
    JSScript *script() const volatile { MOZ_ASSERT(js()); return script_; }
    bool scriptIsOptimized() const volatile {
        MOZ_ASSERT(js());
        return uintptr_t(sp) <= SCRIPT_OPT_STACKPOINTER;
    }
    void *stackAddress() const volatile {
        if (js())
            return nullptr;
        return sp;
    }
    const char *label() const volatile { return string; }

    void setLine(uint32_t aLine) volatile { MOZ_ASSERT(!js()); idx = aLine; }
    void setLabel(const char *aString) volatile { string = aString; }
    void setStackAddress(void *aSp) volatile { sp = aSp; }
    void setScript(JSScript *aScript) volatile { script_ = aScript; }

    
    JS_FRIEND_API(jsbytecode *) pc() const volatile;
    JS_FRIEND_API(void) setPC(jsbytecode *pc) volatile;

    static size_t offsetOfString() { return offsetof(ProfileEntry, string); }
    static size_t offsetOfStackAddress() { return offsetof(ProfileEntry, sp); }
    static size_t offsetOfPCIdx() { return offsetof(ProfileEntry, idx); }
    static size_t offsetOfScript() { return offsetof(ProfileEntry, script_); }

    
    
    
    static const int32_t NullPCIndex = -1;

    
    
    static const uintptr_t NoCopyBit = 1;
};

JS_FRIEND_API(void)
SetRuntimeProfilingStack(JSRuntime *rt, ProfileEntry *stack, uint32_t *size,
                         uint32_t max);

JS_FRIEND_API(void)
EnableRuntimeProfilingStack(JSRuntime *rt, bool enabled);

JS_FRIEND_API(void)
RegisterRuntimeProfilingEventMarker(JSRuntime *rt, void (*fn)(const char *));

JS_FRIEND_API(jsbytecode*)
ProfilingGetPC(JSRuntime *rt, JSScript *script, void *ip);

} 

#endif  
