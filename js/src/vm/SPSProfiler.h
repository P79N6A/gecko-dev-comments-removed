





#ifndef vm_SPSProfiler_h
#define vm_SPSProfiler_h

#include "mozilla/DebugOnly.h"
#include "mozilla/GuardObjects.h"

#include <stddef.h>

#include "jslock.h"
#include "jsscript.h"

#include "js/ProfilingStack.h"
























































































namespace js {

class ProfileEntry;

typedef HashMap<JSScript*, const char*, DefaultHasher<JSScript*>, SystemAllocPolicy>
        ProfileStringMap;

class SPSEntryMarker;

class SPSProfiler
{
    friend class SPSEntryMarker;

    JSRuntime            *rt;
    ProfileStringMap     strings;
    ProfileEntry         *stack_;
    uint32_t             *size_;
    uint32_t             max_;
    bool                 slowAssertions;
    uint32_t             enabled_;
    PRLock               *lock_;
    void                (*eventMarker_)(const char *);

    const char *allocProfileString(JSScript *script, JSFunction *function);
    void push(const char *string, void *sp, JSScript *script, jsbytecode *pc, bool copy);
    void pop();

  public:
    explicit SPSProfiler(JSRuntime *rt);
    ~SPSProfiler();

    bool init();

    uint32_t **addressOfSizePointer() {
        return &size_;
    }

    uint32_t *addressOfMaxSize() {
        return &max_;
    }

    ProfileEntry **addressOfStack() {
        return &stack_;
    }

    uint32_t *sizePointer() { return size_; }
    uint32_t maxSize() { return max_; }
    ProfileEntry *stack() { return stack_; }

    
    bool enabled() { MOZ_ASSERT_IF(enabled_, installed()); return enabled_; }
    bool installed() { return stack_ != nullptr && size_ != nullptr; }
    void enable(bool enabled);
    void enableSlowAssertions(bool enabled) { slowAssertions = enabled; }
    bool slowAssertionsEnabled() { return slowAssertions; }

    








    bool enter(JSScript *script, JSFunction *maybeFun);
    void exit(JSScript *script, JSFunction *maybeFun);
    void updatePC(JSScript *script, jsbytecode *pc) {
        if (enabled() && *size_ - 1 < max_) {
            MOZ_ASSERT(*size_ > 0);
            MOZ_ASSERT(stack_[*size_ - 1].script() == script);
            stack_[*size_ - 1].setPC(pc);
        }
    }

    
    void enterAsmJS(const char *string, void *sp);
    void exitAsmJS() { pop(); }

    jsbytecode *ipToPC(JSScript *script, size_t ip) { return nullptr; }

    void setProfilingStack(ProfileEntry *stack, uint32_t *size, uint32_t max);
    void setEventMarker(void (*fn)(const char *));
    const char *profileString(JSScript *script, JSFunction *maybeFun);
    void onScriptFinalized(JSScript *script);

    void markEvent(const char *event);

    
    size_t stringsCount();
    void stringsReset();

    uint32_t *addressOfEnabled() {
        return &enabled_;
    }
};





class AutoSPSLock
{
  public:
    explicit AutoSPSLock(PRLock *lock)
    {
        MOZ_ASSERT(lock, "Parameter should not be null!");
        lock_ = lock;
        PR_Lock(lock);
    }
    ~AutoSPSLock() { PR_Unlock(lock_); }

  private:
    PRLock *lock_;
};





class AutoSuppressProfilerSampling
{
  public:
    explicit AutoSuppressProfilerSampling(JSContext *cx MOZ_GUARD_OBJECT_NOTIFIER_PARAM);
    explicit AutoSuppressProfilerSampling(JSRuntime *rt MOZ_GUARD_OBJECT_NOTIFIER_PARAM);

    ~AutoSuppressProfilerSampling();

  private:
    JSRuntime *rt_;
    bool previouslyEnabled_;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

inline size_t
SPSProfiler::stringsCount()
{
    AutoSPSLock lock(lock_);
    return strings.count();
}

inline void
SPSProfiler::stringsReset()
{
    AutoSPSLock lock(lock_);
    strings.clear();
}






class SPSEntryMarker
{
  public:
    explicit SPSEntryMarker(JSRuntime *rt,
                            JSScript *script
                            MOZ_GUARD_OBJECT_NOTIFIER_PARAM);
    ~SPSEntryMarker();

  private:
    SPSProfiler *profiler;
    mozilla::DebugOnly<uint32_t> size_before;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};














template<class Assembler, class Register>
class SPSInstrumentation
{
    SPSProfiler *profiler_; 

  public:
    



    explicit SPSInstrumentation(SPSProfiler *profiler) : profiler_(profiler) {}

    
    bool enabled() { return profiler_ && profiler_->enabled(); }
    SPSProfiler *profiler() { MOZ_ASSERT(enabled()); return profiler_; }
    void disable() { profiler_ = nullptr; }
};



void *GetTopProfilingJitFrame(uint8_t *exitFramePtr);

} 

#endif
