





#ifndef js_ProfilingStack_h
#define js_ProfilingStack_h

#include "mozilla/NullPtr.h"
#include "mozilla/TypedEnum.h"

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

    
    uint32_t volatile flags_;

  public:
    
    enum Flags {
        
        
        
        IS_CPP_ENTRY = 0x01,

        
        
        FRAME_LABEL_COPY = 0x02,

        
        ASMJS = 0x04,

        
        CATEGORY_MASK = ~IS_CPP_ENTRY & ~FRAME_LABEL_COPY & ~ASMJS
    };

    
    MOZ_BEGIN_NESTED_ENUM_CLASS(Category, uint32_t)
        OTHER    = 0x08,
        CSS      = 0x10,
        JS       = 0x20,
        GC       = 0x40,
        CC       = 0x80,
        NETWORK  = 0x100,
        GRAPHICS = 0x200,
        STORAGE  = 0x400,
        EVENTS   = 0x800,

        FIRST    = OTHER,
        LAST     = EVENTS
    MOZ_END_NESTED_ENUM_CLASS(Category)

    
    
    
    

    bool isCpp() const volatile { return hasFlag(IS_CPP_ENTRY); }
    bool isJs() const volatile { return !isCpp(); }

    bool isCopyLabel() const volatile { return hasFlag(FRAME_LABEL_COPY); }

    void setLabel(const char *aString) volatile { string = aString; }
    const char *label() const volatile { return string; }

    void setJsFrame(JSScript *aScript, jsbytecode *aPc) volatile {
        flags_ = 0;
        spOrScript = aScript;
        setPC(aPc);
    }
    void setCppFrame(void *aSp, uint32_t aLine) volatile {
        flags_ = IS_CPP_ENTRY;
        spOrScript = aSp;
        lineOrPc = static_cast<int32_t>(aLine);
    }

    void setFlag(uint32_t flag) volatile {
        MOZ_ASSERT(flag != IS_CPP_ENTRY);
        flags_ |= flag;
    }
    void unsetFlag(uint32_t flag) volatile {
        MOZ_ASSERT(flag != IS_CPP_ENTRY);
        flags_ &= ~flag;
    }
    bool hasFlag(uint32_t flag) const volatile {
        return bool(flags_ & flag);
    }

    uint32_t flags() const volatile {
        return flags_;
    }
    uint32_t category() const volatile {
        return flags_ & CATEGORY_MASK;
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
        return static_cast<uint32_t>(lineOrPc);
    }

    
    JS_FRIEND_API(jsbytecode *) pc() const volatile;
    JS_FRIEND_API(void) setPC(jsbytecode *pc) volatile;

    
    
    
    static const int32_t NullPCOffset = -1;

    static size_t offsetOfLabel() { return offsetof(ProfileEntry, string); }
    static size_t offsetOfSpOrScript() { return offsetof(ProfileEntry, spOrScript); }
    static size_t offsetOfLineOrPc() { return offsetof(ProfileEntry, lineOrPc); }
    static size_t offsetOfFlags() { return offsetof(ProfileEntry, flags_); }
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
