





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

    
    bool enabled() { JS_ASSERT_IF(enabled_, installed()); return enabled_; }
    bool installed() { return stack_ != nullptr && size_ != nullptr; }
    void enable(bool enabled);
    void enableSlowAssertions(bool enabled) { slowAssertions = enabled; }
    bool slowAssertionsEnabled() { return slowAssertions; }

    








    bool enter(JSScript *script, JSFunction *maybeFun);
    void exit(JSScript *script, JSFunction *maybeFun);
    void updatePC(JSScript *script, jsbytecode *pc) {
        if (enabled() && *size_ - 1 < max_) {
            JS_ASSERT(*size_ > 0);
            JS_ASSERT(stack_[*size_ - 1].script() == script);
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
    
    struct FrameState {
        JSScript *script; 
        jsbytecode *pc;   
        bool skipNext;    
        int  left;        
    };

    SPSProfiler *profiler_; 

    Vector<FrameState, 1, SystemAllocPolicy> frames;
    FrameState *frame;

    static void clearFrame(FrameState *frame) {
        frame->script = nullptr;
        frame->pc = nullptr;
        frame->skipNext = false;
        frame->left = 0;
    }

  public:
    



    explicit SPSInstrumentation(SPSProfiler *profiler)
      : profiler_(profiler), frame(nullptr)
    {
        enterInlineFrame(nullptr);
    }

    
    bool enabled() { return profiler_ && profiler_->enabled(); }
    SPSProfiler *profiler() { JS_ASSERT(enabled()); return profiler_; }
    void disable() { profiler_ = nullptr; }

    
    void leaveInlineFrame() {
        if (!enabled())
            return;
        JS_ASSERT(frame->left == 0);
        JS_ASSERT(frame->script != nullptr);
        frames.shrinkBy(1);
        JS_ASSERT(frames.length() > 0);
        frame = &frames[frames.length() - 1];
    }

    
    bool enterInlineFrame(jsbytecode *callerPC) {
        if (!enabled())
            return true;
        JS_ASSERT_IF(frames.empty(), callerPC == nullptr);

        JS_ASSERT_IF(frame != nullptr, frame->script != nullptr);
        JS_ASSERT_IF(frame != nullptr, frame->left == 1);
        if (!frames.empty()) {
            JS_ASSERT(frame == &frames[frames.length() - 1]);
            frame->pc = callerPC;
        }
        if (!frames.growBy(1))
            return false;
        frame = &frames[frames.length() - 1];
        clearFrame(frame);
        return true;
    }

    






    bool prepareForOOL() {
        if (!enabled())
            return true;
        JS_ASSERT(!frames.empty());
        if (frames.length() >= 2) {
            frames.shrinkBy(frames.length() - 2);

        } else { 
            if (!frames.growBy(1))
                return false;
        }
        frames[0].pc = frames[0].script->code();
        frame = &frames[1];
        clearFrame(frame);
        return true;
    }
    void finishOOL() {
        if (!enabled())
            return;
        JS_ASSERT(!frames.empty());
        frames.shrinkBy(frames.length() - 1);
    }

    
    unsigned inliningDepth() {
        return frames.length() - 1;
    }

    







    void skipNextReenter() {
        
        if (!enabled() || frame->left != 0)
            return;
        JS_ASSERT(frame->script);
        JS_ASSERT(!frame->skipNext);
        frame->skipNext = true;
    }

    




    void setPushed(JSScript *script) {
        if (!enabled())
            return;
        JS_ASSERT(frame->left == 0);
        frame->script = script;
    }

    JSScript *getPushed() {
        if (!enabled())
            return nullptr;
        return frame->script;
    }

    



    bool push(JSScript *script, Assembler &masm, Register scratch, bool inlinedFunction = false) {
        if (!enabled())
            return true;
        if (!inlinedFunction) {
            const char *string = profiler_->profileString(script, script->functionNonDelazifying());
            if (string == nullptr)
                return false;
            masm.spsPushFrame(profiler_, string, script, scratch);
        }
        setPushed(script);
        return true;
    }

    




    void pushManual(JSScript *script, Assembler &masm, Register scratch,
                    bool inlinedFunction = false)
    {
        if (!enabled())
            return;

        if (!inlinedFunction)
            masm.spsUpdatePCIdx(profiler_, ProfileEntry::NullPCOffset, scratch);

        setPushed(script);
    }

    






    void leave(jsbytecode *pc, Assembler &masm, Register scratch, bool inlinedFunction = false) {
        if (enabled() && frame->script && frame->left++ == 0) {
            jsbytecode *updatePC = pc;
            JSScript *script = frame->script;
            if (!inlinedFunction) {
                
                
                if (inliningDepth() > 0) {
                    JS_ASSERT(frames[0].pc);
                    updatePC = frames[0].pc;
                    script = frames[0].script;
                }
            }

            if (!inlinedFunction)
                masm.spsUpdatePCIdx(profiler_, script->pcToOffset(updatePC), scratch);
        }
    }

    



    void reenter(Assembler &masm, Register scratch, bool inlinedFunction = false) {
        if (!enabled() || !frame->script || frame->left-- != 1)
            return;
        if (frame->skipNext) {
            frame->skipNext = false;
        } else {
            if (!inlinedFunction)
                masm.spsUpdatePCIdx(profiler_, ProfileEntry::NullPCOffset, scratch);
        }
    }

    




    void pop(Assembler &masm, Register scratch, bool inlinedFunction = false) {
        if (enabled()) {
            JS_ASSERT(frame->left == 0);
            JS_ASSERT(frame->script);
            if (!inlinedFunction)
                masm.spsPopFrame(profiler_, scratch);
        }
    }
};

} 

#endif 
