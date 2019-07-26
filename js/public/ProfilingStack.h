





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

    
    void * volatile spOrScript;

    
    int32_t volatile lineOrPc;

    
    uint32_t volatile flags;

  public:
    ProfileEntry(void) : flags(0) {}

    
    enum Flags {
        
        
        
        IS_CPP_ENTRY = 0x01,

        
        
        FRAME_LABEL_COPY = 0x02
    };

    
    
    
    

    bool isCpp() const volatile { return hasFlag(IS_CPP_ENTRY); }
    bool isJs() const volatile { return !isCpp(); }

    bool isCopyLabel() const volatile { return hasFlag(FRAME_LABEL_COPY); };

    void setLabel(const char *aString) volatile { string = aString; }
    const char *label() const volatile { return string; }

    void setJsFrame(JSScript *aScript, jsbytecode *aPc) volatile {
        flags &= ~IS_CPP_ENTRY;
        spOrScript = aScript;
        setPC(aPc);
    }
    void setCppFrame(void *aSp, uint32_t aLine) volatile {
        flags |= IS_CPP_ENTRY;
        spOrScript = aSp;
        lineOrPc = aLine;
    }

    void setFlag(Flags flag) volatile {
        MOZ_ASSERT(flag != IS_CPP_ENTRY);
        flags |= flag;
    }
    void unsetFlag(Flags flag) volatile {
        MOZ_ASSERT(flag != IS_CPP_ENTRY);
        flags &= ~flag;
    }
    bool hasFlag(Flags flag) const volatile {
        return bool(flags & uint32_t(flag));
    }

    void *stackAddress() const volatile {
        MOZ_ASSERT(!isJs());
        return spOrScript;
    }
    JSScript *script() const volatile {
        MOZ_ASSERT(isJs());
        return (JSScript *)spOrScript;
    }
    uint32_t line() const volatile {
        MOZ_ASSERT(!isJs());
        return lineOrPc;
    }

    
    JS_FRIEND_API(jsbytecode *) pc() const volatile;
    JS_FRIEND_API(void) setPC(jsbytecode *pc) volatile;

    
    
    
    static const int32_t NullPCOffset = -1;

    static size_t offsetOfLabel() { return offsetof(ProfileEntry, string); }
    static size_t offsetOfSpOrScript() { return offsetof(ProfileEntry, spOrScript); }
    static size_t offsetOfLineOrPc() { return offsetof(ProfileEntry, lineOrPc); }
    static size_t offsetOfFlags() { return offsetof(ProfileEntry, flags); }
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
