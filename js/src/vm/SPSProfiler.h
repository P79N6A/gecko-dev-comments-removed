





#ifndef vm_SPSProfiler_h
#define vm_SPSProfiler_h

#include "mozilla/DebugOnly.h"
#include "mozilla/GuardObjects.h"

#include <stddef.h>

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

    const char *allocProfileString(JSContext *cx, JSScript *script,
                                   JSFunction *function);
    void push(const char *string, void *sp, JSScript *script, jsbytecode *pc);
    void pop();

  public:
    SPSProfiler(JSRuntime *rt);
    ~SPSProfiler();

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

    








    bool enter(JSContext *cx, JSScript *script, JSFunction *maybeFun);
    void exit(JSContext *cx, JSScript *script, JSFunction *maybeFun);
    void updatePC(JSScript *script, jsbytecode *pc) {
        if (enabled() && *size_ - 1 < max_) {
            JS_ASSERT(*size_ > 0);
            JS_ASSERT(stack_[*size_ - 1].script() == script);
            stack_[*size_ - 1].setPC(pc);
        }
    }

    
    void enterNative(const char *string, void *sp);
    void exitNative() { pop(); }

    jsbytecode *ipToPC(JSScript *script, size_t ip) { return nullptr; }

    void setProfilingStack(ProfileEntry *stack, uint32_t *size, uint32_t max);
    const char *profileString(JSContext *cx, JSScript *script, JSFunction *maybeFun);
    void onScriptFinalized(JSScript *script);

    
    size_t stringsCount() { return strings.count(); }
    void stringsReset() { strings.clear(); }

    uint32_t *addressOfEnabled() {
        return &enabled_;
    }
};






class SPSEntryMarker
{
  public:
    SPSEntryMarker(JSRuntime *rt
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
        bool skipNext;    
        int  left;        
    };

    SPSProfiler *profiler_; 

    Vector<FrameState, 1, SystemAllocPolicy> frames;
    FrameState *frame;

  public:
    



    SPSInstrumentation(SPSProfiler *profiler)
      : profiler_(profiler), frame(nullptr)
    {
        enterInlineFrame();
    }

    
    bool enabled() { return profiler_ && profiler_->enabled(); }
    SPSProfiler *profiler() { JS_ASSERT(enabled()); return profiler_; }
    bool slowAssertions() { return enabled() && profiler_->slowAssertionsEnabled(); }

    
    void leaveInlineFrame() {
        if (!enabled())
            return;
        JS_ASSERT(frame->left == 0);
        JS_ASSERT(frame->script != nullptr);
        frames.shrinkBy(1);
        JS_ASSERT(frames.length() > 0);
        frame = &frames[frames.length() - 1];
    }

    
    bool enterInlineFrame() {
        if (!enabled())
            return true;
        JS_ASSERT_IF(frame != nullptr, frame->script != nullptr);
        JS_ASSERT_IF(frame != nullptr, frame->left == 1);
        if (!frames.growBy(1))
            return false;
        frame = &frames[frames.length() - 1];
        frame->script = nullptr;
        frame->skipNext = false;
        frame->left = 0;
        return true;
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

    



    bool push(JSContext *cx, JSScript *script, Assembler &masm, Register scratch) {
        if (!enabled())
            return true;
        const char *string = profiler_->profileString(cx, script,
                                                      script->function());
        if (string == nullptr)
            return false;
        masm.spsPushFrame(profiler_, string, script, scratch);
        setPushed(script);
        return true;
    }

    




    void pushManual(JSScript *script, Assembler &masm, Register scratch) {
        if (!enabled())
            return;
        masm.spsUpdatePCIdx(profiler_, ProfileEntry::NullPCIndex, scratch);
        setPushed(script);
    }

    






    void leave(jsbytecode *pc, Assembler &masm, Register scratch) {
        if (enabled() && frame->script && frame->left++ == 0) {
            JS_ASSERT(frame->script->code <= pc &&
                      pc < frame->script->code + frame->script->length);
            masm.spsUpdatePCIdx(profiler_, pc - frame->script->code, scratch);
        }
    }

    



    void reenter(Assembler &masm, Register scratch) {
        if (!enabled() || !frame->script || frame->left-- != 1)
            return;
        if (frame->skipNext)
            frame->skipNext = false;
        else
            masm.spsUpdatePCIdx(profiler_, ProfileEntry::NullPCIndex, scratch);
    }

    




    void pop(Assembler &masm, Register scratch) {
        if (enabled()) {
            JS_ASSERT(frame->left == 0);
            JS_ASSERT(frame->script);
            masm.spsPopFrame(profiler_, scratch);
        }
    }
};

} 

#endif
