






#ifndef SPSProfiler_h__
#define SPSProfiler_h__

#include <stddef.h>

#include "mozilla/DebugOnly.h"
#include "mozilla/GuardObjects.h"
#include "mozilla/HashFunctions.h"

#include "js/Utility.h"
#include "jsscript.h"
























































































class JSFunction;

namespace js {

class ProfileEntry;

#ifdef JS_METHODJIT
namespace mjit {
    struct JITChunk;
    struct JITScript;
    struct JSActiveFrame;
    struct PCLengthEntry;
}
#endif

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
    bool                 enabled_;

    const char *allocProfileString(JSContext *cx, RawScript script,
                                   RawFunction function);
    void push(const char *string, void *sp, RawScript script, jsbytecode *pc);
    void pop();

  public:
    SPSProfiler(JSRuntime *rt);
    ~SPSProfiler();

    uint32_t *sizePointer() { return size_; }
    uint32_t maxSize() { return max_; }
    ProfileEntry *stack() { return stack_; }

    
    bool enabled() { JS_ASSERT_IF(enabled_, installed()); return enabled_; }
    bool installed() { return stack_ != NULL && size_ != NULL; }
    void enable(bool enabled);
    void enableSlowAssertions(bool enabled) { slowAssertions = enabled; }
    bool slowAssertionsEnabled() { return slowAssertions; }

    








    bool enter(JSContext *cx, RawScript script, RawFunction maybeFun);
    void exit(JSContext *cx, RawScript script, RawFunction maybeFun);
    void updatePC(RawScript script, jsbytecode *pc) {
        if (enabled() && *size_ - 1 < max_) {
            JS_ASSERT(*size_ > 0);
            JS_ASSERT(stack_[*size_ - 1].script() == script);
            stack_[*size_ - 1].setPC(pc);
        }
    }

#ifdef JS_METHODJIT
    struct ICInfo
    {
        size_t base;
        size_t size;
        jsbytecode *pc;

        ICInfo(void *base, size_t size, jsbytecode *pc)
          : base(size_t(base)), size(size), pc(pc)
        {}
    };

    struct JMChunkInfo
    {
        size_t mainStart;               
        size_t mainEnd;
        size_t stubStart;               
        size_t stubEnd;
        mjit::PCLengthEntry *pcLengths; 
        mjit::JITChunk *chunk;          

        JMChunkInfo(mjit::JSActiveFrame *frame,
                    mjit::PCLengthEntry *pcLengths,
                    mjit::JITChunk *chunk);

        jsbytecode *convert(RawScript script, size_t ip);
    };

    struct JMScriptInfo
    {
        Vector<ICInfo, 0, SystemAllocPolicy> ics;
        Vector<JMChunkInfo, 1, SystemAllocPolicy> chunks;
    };

    typedef HashMap<JSScript*, JMScriptInfo*, DefaultHasher<JSScript*>,
                    SystemAllocPolicy> JITInfoMap;

    













    JITInfoMap jminfo;

    bool registerMJITCode(mjit::JITChunk *chunk,
                          mjit::JSActiveFrame *outerFrame,
                          mjit::JSActiveFrame **inlineFrames);
    void discardMJITCode(mjit::JITScript *jscr,
                         mjit::JITChunk *chunk, void* address);
    bool registerICCode(mjit::JITChunk *chunk, RawScript script, jsbytecode* pc,
                        void *start, size_t size);
    jsbytecode *ipToPC(RawScript script, size_t ip);

  private:
    JMChunkInfo *registerScript(mjit::JSActiveFrame *frame,
                                mjit::PCLengthEntry *lenths,
                                mjit::JITChunk *chunk);
    void unregisterScript(RawScript script, mjit::JITChunk *chunk);
  public:
#else
    jsbytecode *ipToPC(RawScript script, size_t ip) { return NULL; }
#endif

    void setProfilingStack(ProfileEntry *stack, uint32_t *size, uint32_t max);
    const char *profileString(JSContext *cx, RawScript script, RawFunction maybeFun);
    void onScriptFinalized(RawScript script);

    
    size_t stringsCount() { return strings.count(); }
    void stringsReset() { strings.clear(); }
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
      : profiler_(profiler), frame(NULL)
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
        JS_ASSERT(frame->script != NULL);
        frames.shrinkBy(1);
        JS_ASSERT(frames.length() > 0);
        frame = &frames[frames.length() - 1];
    }

    
    bool enterInlineFrame() {
        if (!enabled())
            return true;
        JS_ASSERT_IF(frame != NULL, frame->script != NULL);
        JS_ASSERT_IF(frame != NULL, frame->left == 1);
        if (!frames.growBy(1))
            return false;
        frame = &frames[frames.length() - 1];
        frame->script = NULL;
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

    




    void setPushed(RawScript script) {
        if (!enabled())
            return;
        JS_ASSERT(frame->left == 0);
        frame->script = script;
    }

    



    bool push(JSContext *cx, RawScript script, Assembler &masm, Register scratch) {
        if (!enabled())
            return true;
        const char *string = profiler_->profileString(cx, script,
                                                      script->function());
        if (string == NULL)
            return false;
        masm.spsPushFrame(profiler_, string, script, scratch);
        setPushed(script);
        return true;
    }

    




    void pushManual(RawScript script, Assembler &masm, Register scratch) {
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
