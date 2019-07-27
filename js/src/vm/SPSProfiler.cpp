





#include "vm/SPSProfiler.h"

#include "mozilla/DebugOnly.h"

#include "jsnum.h"
#include "jsprf.h"
#include "jsscript.h"

#include "jit/BaselineFrame.h"
#include "jit/BaselineJIT.h"
#include "jit/JitFrameIterator.h"
#include "jit/JitFrames.h"
#include "vm/StringBuffer.h"

using namespace js;

using mozilla::DebugOnly;

SPSProfiler::SPSProfiler(JSRuntime *rt)
  : rt(rt),
    stack_(nullptr),
    size_(nullptr),
    max_(0),
    slowAssertions(false),
    enabled_(false),
    lock_(nullptr),
    eventMarker_(nullptr)
{
    MOZ_ASSERT(rt != nullptr);
}

bool
SPSProfiler::init()
{
    lock_ = PR_NewLock();
    if (lock_ == nullptr)
        return false;

    return true;
}

SPSProfiler::~SPSProfiler()
{
    if (strings.initialized()) {
        for (ProfileStringMap::Enum e(strings); !e.empty(); e.popFront())
            js_free(const_cast<char *>(e.front().value()));
    }
    if (lock_)
        PR_DestroyLock(lock_);
}

void
SPSProfiler::setProfilingStack(ProfileEntry *stack, uint32_t *size, uint32_t max)
{
    AutoSPSLock lock(lock_);
    MOZ_ASSERT_IF(size_ && *size_ != 0, !enabled());
    if (!strings.initialized())
        strings.init();
    stack_ = stack;
    size_  = size;
    max_   = max;
}

void
SPSProfiler::setEventMarker(void (*fn)(const char *))
{
    eventMarker_ = fn;
}

void
SPSProfiler::enable(bool enabled)
{
    MOZ_ASSERT(installed());

    if (enabled_ == enabled)
        return;

    



    ReleaseAllJITCode(rt->defaultFreeOp());

    
    if (rt->jitActivation) {
        rt->jitActivation->setLastProfilingFrame(nullptr);
        rt->jitActivation->setLastProfilingCallSite(nullptr);
    }

    enabled_ = enabled;

    




    jit::ToggleBaselineProfiling(rt, enabled);

    


    if (rt->jitActivation) {
        void *lastProfilingFrame = GetTopProfilingJitFrame(rt->jitTop);
        rt->jitActivation->setLastProfilingFrame(lastProfilingFrame);
    }
}


const char*
SPSProfiler::profileString(JSScript *script, JSFunction *maybeFun)
{
    AutoSPSLock lock(lock_);
    MOZ_ASSERT(strings.initialized());
    ProfileStringMap::AddPtr s = strings.lookupForAdd(script);
    if (s)
        return s->value();
    const char *str = allocProfileString(script, maybeFun);
    if (str == nullptr)
        return nullptr;
    if (!strings.add(s, script, str)) {
        js_free(const_cast<char *>(str));
        return nullptr;
    }
    return str;
}

void
SPSProfiler::onScriptFinalized(JSScript *script)
{
    






    AutoSPSLock lock(lock_);
    if (!strings.initialized())
        return;
    if (ProfileStringMap::Ptr entry = strings.lookup(script)) {
        const char *tofree = entry->value();
        strings.remove(entry);
        js_free(const_cast<char *>(tofree));
    }
}

void
SPSProfiler::markEvent(const char *event)
{
    MOZ_ASSERT(enabled());
    if (eventMarker_) {
        JS::AutoSuppressGCAnalysis nogc;
        eventMarker_(event);
    }
}

bool
SPSProfiler::enter(JSScript *script, JSFunction *maybeFun)
{
    const char *str = profileString(script, maybeFun);
    if (str == nullptr)
        return false;

#ifdef DEBUG
    
    
    
    if (*size_ > 0 && *size_ - 1 < max_) {
        size_t start = (*size_ > 4) ? *size_ - 4 : 0;
        for (size_t i = start; i < *size_ - 1; i++)
            MOZ_ASSERT_IF(stack_[i].isJs(), stack_[i].pc() != nullptr);
    }
#endif

    push(str, nullptr, script, script->code(),  true);
    return true;
}

void
SPSProfiler::exit(JSScript *script, JSFunction *maybeFun)
{
    pop();

#ifdef DEBUG
    
    if (*size_ < max_) {
        const char *str = profileString(script, maybeFun);
        
        MOZ_ASSERT(str != nullptr);

        
        if (!stack_[*size_].isJs()) {
            fprintf(stderr, "--- ABOUT TO FAIL ASSERTION ---\n");
            fprintf(stderr, " stack=%p size=%d/%d\n", (void*) stack_, *size_, max_);
            for (int32_t i = *size_; i >= 0; i--) {
                if (stack_[i].isJs())
                    fprintf(stderr, "  [%d] JS %s\n", i, stack_[i].label());
                else
                    fprintf(stderr, "  [%d] C line %d %s\n", i, stack_[i].line(), stack_[i].label());
            }
        }

        MOZ_ASSERT(stack_[*size_].isJs());
        MOZ_ASSERT(stack_[*size_].script() == script);
        MOZ_ASSERT(strcmp((const char*) stack_[*size_].label(), str) == 0);
        stack_[*size_].setLabel(nullptr);
        stack_[*size_].setPC(nullptr);
    }
#endif
}

void
SPSProfiler::beginPseudoJS(const char *string, void *sp)
{
    
    volatile ProfileEntry *stack = stack_;
    volatile uint32_t *size = size_;
    uint32_t current = *size;

    MOZ_ASSERT(installed());
    if (current < max_) {
        stack[current].setLabel(string);
        stack[current].setCppFrame(sp, 0);
        stack[current].setFlag(ProfileEntry::BEGIN_PSEUDO_JS);
    }
    *size = current + 1;
}

void
SPSProfiler::push(const char *string, void *sp, JSScript *script, jsbytecode *pc, bool copy)
{
    MOZ_ASSERT_IF(sp != nullptr, script == nullptr && pc == nullptr);
    MOZ_ASSERT_IF(sp == nullptr, script != nullptr && pc != nullptr);

    
    volatile ProfileEntry *stack = stack_;
    volatile uint32_t *size = size_;
    uint32_t current = *size;

    MOZ_ASSERT(installed());
    if (current < max_) {
        volatile ProfileEntry &entry = stack[current];
        entry.setLabel(string);

        if (sp != nullptr) {
            entry.setCppFrame(sp, 0);
            MOZ_ASSERT(entry.flags() == js::ProfileEntry::IS_CPP_ENTRY);
        }
        else {
            entry.setJsFrame(script, pc);
            MOZ_ASSERT(entry.flags() == 0);
        }

        
        if (copy)
            entry.setFlag(js::ProfileEntry::FRAME_LABEL_COPY);
        else
            entry.unsetFlag(js::ProfileEntry::FRAME_LABEL_COPY);
    }
    *size = current + 1;
}

void
SPSProfiler::pop()
{
    MOZ_ASSERT(installed());
    (*size_)--;
    MOZ_ASSERT(*(int*)size_ >= 0);
}







const char *
SPSProfiler::allocProfileString(JSScript *script, JSFunction *maybeFun)
{
    
    

    
    JSAtom *atom = maybeFun ? maybeFun->displayAtom() : nullptr;

    
    const char *filename = script->filename();
    if (filename == nullptr)
        filename = "<unknown>";
    size_t lenFilename = strlen(filename);

    
    uint64_t lineno = script->lineno();
    size_t lenLineno = 1;
    for (uint64_t i = lineno; i /= 10; lenLineno++);

    
    size_t len = lenFilename + lenLineno + 1; 
    if (atom)
        len += atom->length() + 3; 

    
    char *cstr = js_pod_malloc<char>(len + 1);
    if (cstr == nullptr)
        return nullptr;

    
    DebugOnly<size_t> ret;
    if (atom) {
        JS::AutoCheckCannotGC nogc;
        if (atom->hasLatin1Chars())
            ret = JS_snprintf(cstr, len + 1, "%s (%s:%llu)", atom->latin1Chars(nogc), filename, lineno);
        else
            ret = JS_snprintf(cstr, len + 1, "%hs (%s:%llu)", atom->twoByteChars(nogc), filename, lineno);
    } else {
        ret = JS_snprintf(cstr, len + 1, "%s:%llu", filename, lineno);
    }

    MOZ_ASSERT(ret == len, "Computed length should match actual length!");

    return cstr;
}

SPSEntryMarker::SPSEntryMarker(JSRuntime *rt,
                               JSScript *script
                               MOZ_GUARD_OBJECT_NOTIFIER_PARAM_IN_IMPL)
    : profiler(&rt->spsProfiler)
{
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    if (!profiler->installed()) {
        profiler = nullptr;
        return;
    }
    size_before = *profiler->size_;
    
    profiler->beginPseudoJS("js::RunScript", this);
    profiler->push("js::RunScript", nullptr, script, script->code(),  false);
}

SPSEntryMarker::~SPSEntryMarker()
{
    if (profiler == nullptr)
        return;

    profiler->pop();
    profiler->endPseudoJS();
    MOZ_ASSERT(size_before == *profiler->size_);
}

SPSBaselineOSRMarker::SPSBaselineOSRMarker(JSRuntime *rt, bool hasSPSFrame
                                           MOZ_GUARD_OBJECT_NOTIFIER_PARAM_IN_IMPL)
    : profiler(&rt->spsProfiler)
{
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    if (!hasSPSFrame || !profiler->enabled()) {
        profiler = nullptr;
        return;
    }

    size_before = profiler->size();
    if (profiler->size() == 0)
        return;

    ProfileEntry &entry = profiler->stack()[profiler->size() - 1];
    MOZ_ASSERT(entry.isJs());
    entry.setOSR();
}

SPSBaselineOSRMarker::~SPSBaselineOSRMarker()
{
    if (profiler == nullptr)
        return;

    MOZ_ASSERT(size_before == *profiler->size_);
    if (profiler->size() == 0)
        return;

    ProfileEntry &entry = profiler->stack()[profiler->size() - 1];
    MOZ_ASSERT(entry.isJs());
    entry.unsetOSR();
}

JS_FRIEND_API(jsbytecode*)
ProfileEntry::pc() const volatile
{
    MOZ_ASSERT(isJs());
    return lineOrPc == NullPCOffset ? nullptr : script()->offsetToPC(lineOrPc);
}

JS_FRIEND_API(void)
ProfileEntry::setPC(jsbytecode *pc) volatile
{
    MOZ_ASSERT(isJs());
    lineOrPc = pc == nullptr ? NullPCOffset : script()->pcToOffset(pc);
}

JS_FRIEND_API(void)
js::SetRuntimeProfilingStack(JSRuntime *rt, ProfileEntry *stack, uint32_t *size, uint32_t max)
{
    rt->spsProfiler.setProfilingStack(stack, size, max);
}

JS_FRIEND_API(void)
js::EnableRuntimeProfilingStack(JSRuntime *rt, bool enabled)
{
    rt->spsProfiler.enable(enabled);
}

JS_FRIEND_API(void)
js::RegisterRuntimeProfilingEventMarker(JSRuntime *rt, void (*fn)(const char *))
{
    MOZ_ASSERT(rt->spsProfiler.enabled());
    rt->spsProfiler.setEventMarker(fn);
}

JS_FRIEND_API(jsbytecode*)
js::ProfilingGetPC(JSRuntime *rt, JSScript *script, void *ip)
{
    return rt->spsProfiler.ipToPC(script, size_t(ip));
}

AutoSuppressProfilerSampling::AutoSuppressProfilerSampling(JSContext *cx
                                                           MOZ_GUARD_OBJECT_NOTIFIER_PARAM_IN_IMPL)
  : rt_(cx->runtime()),
    previouslyEnabled_(rt_->isProfilerSamplingEnabled())
{
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    if (previouslyEnabled_)
        rt_->disableProfilerSampling();
}

AutoSuppressProfilerSampling::AutoSuppressProfilerSampling(JSRuntime *rt
                                                           MOZ_GUARD_OBJECT_NOTIFIER_PARAM_IN_IMPL)
  : rt_(rt),
    previouslyEnabled_(rt_->isProfilerSamplingEnabled())
{
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    if (previouslyEnabled_)
        rt_->disableProfilerSampling();
}

AutoSuppressProfilerSampling::~AutoSuppressProfilerSampling()
{
        if (previouslyEnabled_)
            rt_->enableProfilerSampling();
}

void *
js::GetTopProfilingJitFrame(uint8_t *exitFramePtr)
{
    
    if (!exitFramePtr)
        return nullptr;

    jit::JitProfilingFrameIterator iter(exitFramePtr);
    MOZ_ASSERT(!iter.done());
    return iter.fp();
}
