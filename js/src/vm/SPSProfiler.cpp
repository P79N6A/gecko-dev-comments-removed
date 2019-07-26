





#include "vm/SPSProfiler.h"

#include "mozilla/DebugOnly.h"

#include "jsnum.h"
#include "jsscript.h"

#include "jit/BaselineJIT.h"
#include "vm/StringBuffer.h"

using namespace js;

using mozilla::DebugOnly;

SPSProfiler::SPSProfiler(JSRuntime *rt)
  : rt(rt),
    stack_(nullptr),
    size_(nullptr),
    max_(0),
    slowAssertions(false),
    enabled_(false)
{
    JS_ASSERT(rt != nullptr);
}

SPSProfiler::~SPSProfiler()
{
    if (strings.initialized()) {
        for (ProfileStringMap::Enum e(strings); !e.empty(); e.popFront())
            js_free(const_cast<char *>(e.front().value));
    }
}

void
SPSProfiler::setProfilingStack(ProfileEntry *stack, uint32_t *size, uint32_t max)
{
    JS_ASSERT_IF(size_ && *size_ != 0, !enabled());
    if (!strings.initialized())
        strings.init();
    stack_ = stack;
    size_  = size;
    max_   = max;
}

void
SPSProfiler::enable(bool enabled)
{
    JS_ASSERT(installed());
    enabled_ = enabled;
    



    ReleaseAllJITCode(rt->defaultFreeOp());

#ifdef JS_ION
    




    jit::ToggleBaselineSPS(rt, enabled);
#endif
}


const char*
SPSProfiler::profileString(JSContext *cx, JSScript *script, JSFunction *maybeFun)
{
    JS_ASSERT(strings.initialized());
    ProfileStringMap::AddPtr s = strings.lookupForAdd(script);
    if (s)
        return s->value;
    const char *str = allocProfileString(cx, script, maybeFun);
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
    






    if (!strings.initialized())
        return;
    if (ProfileStringMap::Ptr entry = strings.lookup(script)) {
        const char *tofree = entry->value;
        strings.remove(entry);
        js_free(const_cast<char *>(tofree));
    }
}

bool
SPSProfiler::enter(JSContext *cx, JSScript *script, JSFunction *maybeFun)
{
    const char *str = profileString(cx, script, maybeFun);
    if (str == nullptr)
        return false;

    JS_ASSERT_IF(*size_ > 0 && *size_ - 1 < max_ && stack_[*size_ - 1].js(),
                 stack_[*size_ - 1].pc() != nullptr);
    push(str, nullptr, script, script->code);
    return true;
}

void
SPSProfiler::exit(JSContext *cx, JSScript *script, JSFunction *maybeFun)
{
    pop();

#ifdef DEBUG
    
    if (*size_ < max_) {
        const char *str = profileString(cx, script, maybeFun);
        
        JS_ASSERT(str != nullptr);

        
        if (!stack_[*size_].js()) {
            fprintf(stderr, "--- ABOUT TO FAIL ASSERTION ---\n");
            fprintf(stderr, " stack=%p size=%d/%d\n", (void*) stack_, *size_, max_);
            for (int32_t i = *size_; i >= 0; i--) {
                if (stack_[i].js())
                    fprintf(stderr, "  [%d] JS %s\n", i, stack_[i].label());
                else
                    fprintf(stderr, "  [%d] C line %d %s\n", i, stack_[i].line(), stack_[i].label());
            }
        }

        JS_ASSERT(stack_[*size_].js());
        JS_ASSERT(stack_[*size_].script() == script);
        JS_ASSERT(strcmp((const char*) stack_[*size_].label(), str) == 0);
        stack_[*size_].setLabel(nullptr);
        stack_[*size_].setPC(nullptr);
    }
#endif
}

void
SPSProfiler::enterNative(const char *string, void *sp)
{
    
    volatile ProfileEntry *stack = stack_;
    volatile uint32_t *size = size_;
    uint32_t current = *size;

    JS_ASSERT(enabled());
    if (current < max_) {
        stack[current].setLabel(string);
        stack[current].setStackAddress(sp);
        stack[current].setScript(nullptr);
        stack[current].setLine(0);
    }
    *size = current + 1;
}

void
SPSProfiler::push(const char *string, void *sp, JSScript *script, jsbytecode *pc)
{
    
    volatile ProfileEntry *stack = stack_;
    volatile uint32_t *size = size_;
    uint32_t current = *size;

    JS_ASSERT(enabled());
    if (current < max_) {
        stack[current].setLabel(string);
        stack[current].setStackAddress(sp);
        stack[current].setScript(script);
        stack[current].setPC(pc);
    }
    *size = current + 1;
}

void
SPSProfiler::pop()
{
    JS_ASSERT(installed());
    (*size_)--;
    JS_ASSERT(*(int*)size_ >= 0);
}







const char*
SPSProfiler::allocProfileString(JSContext *cx, JSScript *script, JSFunction *maybeFun)
{
    
    
    DebugOnly<uint64_t> gcBefore = cx->runtime()->gcNumber;
    StringBuffer buf(cx);
    bool hasAtom = maybeFun != nullptr && maybeFun->displayAtom() != nullptr;
    if (hasAtom) {
        if (!buf.append(maybeFun->displayAtom()))
            return nullptr;
        if (!buf.append(" ("))
            return nullptr;
    }
    if (script->filename()) {
        if (!buf.appendInflated(script->filename(), strlen(script->filename())))
            return nullptr;
    } else if (!buf.append("<unknown>")) {
        return nullptr;
    }
    if (!buf.append(":"))
        return nullptr;
    if (!NumberValueToStringBuffer(cx, NumberValue(script->lineno), buf))
        return nullptr;
    if (hasAtom && !buf.append(")"))
        return nullptr;

    size_t len = buf.length();
    char *cstr = js_pod_malloc<char>(len + 1);
    if (cstr == nullptr)
        return nullptr;

    const jschar *ptr = buf.begin();
    for (size_t i = 0; i < len; i++)
        cstr[i] = ptr[i];
    cstr[len] = 0;

    JS_ASSERT(gcBefore == cx->runtime()->gcNumber);
    return cstr;
}

SPSEntryMarker::SPSEntryMarker(JSRuntime *rt
                               MOZ_GUARD_OBJECT_NOTIFIER_PARAM_IN_IMPL)
    : profiler(&rt->spsProfiler)
{
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    if (!profiler->enabled()) {
        profiler = nullptr;
        return;
    }
    size_before = *profiler->size_;
    profiler->push("js::RunScript", this, nullptr, nullptr);
}

SPSEntryMarker::~SPSEntryMarker()
{
    if (profiler != nullptr) {
        profiler->pop();
        JS_ASSERT(size_before == *profiler->size_);
    }
}

JS_FRIEND_API(jsbytecode*)
ProfileEntry::pc() volatile
{
    JS_ASSERT_IF(idx != NullPCIndex, idx >= 0 && uint32_t(idx) < script()->length);
    return idx == NullPCIndex ? nullptr : script()->code + idx;
}

JS_FRIEND_API(void)
ProfileEntry::setPC(jsbytecode *pc) volatile
{
    JS_ASSERT_IF(pc != nullptr, script()->code <= pc && pc < script()->code + script()->length);
    idx = pc == nullptr ? NullPCIndex : pc - script()->code;
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

JS_FRIEND_API(jsbytecode*)
js::ProfilingGetPC(JSRuntime *rt, JSScript *script, void *ip)
{
    return rt->spsProfiler.ipToPC(script, size_t(ip));
}
