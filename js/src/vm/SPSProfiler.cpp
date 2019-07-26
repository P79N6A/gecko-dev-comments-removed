





#include "vm/SPSProfiler.h"

#include "mozilla/DebugOnly.h"

#include "jsnum.h"
#include "jsprf.h"
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
    enabled_(false),
    lock_(nullptr)
{
    JS_ASSERT(rt != nullptr);
}

bool
SPSProfiler::init()
{
#ifdef JS_THREADSAFE
    lock_ = PR_NewLock();
    if (lock_ == nullptr)
        return false;
#endif
    return true;
}

SPSProfiler::~SPSProfiler()
{
    if (strings.initialized()) {
        for (ProfileStringMap::Enum e(strings); !e.empty(); e.popFront())
            js_free(const_cast<char *>(e.front().value()));
    }
#ifdef JS_THREADSAFE
    if (lock_)
        PR_DestroyLock(lock_);
#endif
}

void
SPSProfiler::setProfilingStack(ProfileEntry *stack, uint32_t *size, uint32_t max)
{
    AutoSPSLock lock(lock_);
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

    if (enabled_ == enabled)
        return;

    



    ReleaseAllJITCode(rt->defaultFreeOp());

    enabled_ = enabled;

#ifdef JS_ION
    




    jit::ToggleBaselineSPS(rt, enabled);
#endif
}


const char*
SPSProfiler::profileString(JSScript *script, JSFunction *maybeFun)
{
    AutoSPSLock lock(lock_);
    JS_ASSERT(strings.initialized());
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
            MOZ_ASSERT_IF(stack_[i].js(), stack_[i].pc() != nullptr);
    }
#endif

    push(str, nullptr, script, script->code());
    return true;
}

void
SPSProfiler::exit(JSScript *script, JSFunction *maybeFun)
{
    pop();

#ifdef DEBUG
    
    if (*size_ < max_) {
        const char *str = profileString(script, maybeFun);
        
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

    JS_ASSERT(installed());
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







const char *
SPSProfiler::allocProfileString(JSScript *script, JSFunction *maybeFun)
{
    
    

    
    bool hasAtom = maybeFun && maybeFun->displayAtom();

    
    const jschar *atom = nullptr;
    size_t lenAtom = 0;
    if (hasAtom) {
        atom = maybeFun->displayAtom()->charsZ();
        lenAtom = maybeFun->displayAtom()->length();
    }

    
    const char *filename = script->filename();
    if (filename == nullptr)
        filename = "<unknown>";
    size_t lenFilename = strlen(filename);

    
    uint64_t lineno = script->lineno();
    size_t lenLineno = 1;
    for (uint64_t i = lineno; i /= 10; lenLineno++);

    
    size_t len = lenFilename + lenLineno + 1; 
    if (hasAtom)
        len += lenAtom + 3; 

    
    char *cstr = js_pod_malloc<char>(len + 1);
    if (cstr == nullptr)
        return nullptr;

    
    size_t ret;
    if (hasAtom)
        ret = JS_snprintf(cstr, len + 1, "%hs (%s:%llu)", atom, filename, lineno);
    else
        ret = JS_snprintf(cstr, len + 1, "%s:%llu", filename, lineno);

    MOZ_ASSERT(ret == len, "Computed length should match actual length!");

    return cstr;
}

SPSEntryMarker::SPSEntryMarker(JSRuntime *rt
                               MOZ_GUARD_OBJECT_NOTIFIER_PARAM_IN_IMPL)
    : profiler(&rt->spsProfiler)
{
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    if (!profiler->installed()) {
        profiler = nullptr;
        return;
    }
    size_before = *profiler->size_;
    profiler->pushNoCopy("js::RunScript", this, nullptr, nullptr);
}

SPSEntryMarker::~SPSEntryMarker()
{
    if (profiler != nullptr) {
        profiler->pop();
        JS_ASSERT(size_before == *profiler->size_);
    }
}

JS_FRIEND_API(jsbytecode*)
ProfileEntry::pc() const volatile
{
    return idx == NullPCIndex ? nullptr : script()->offsetToPC(idx);
}

JS_FRIEND_API(void)
ProfileEntry::setPC(jsbytecode *pc) volatile
{
    idx = pc == nullptr ? NullPCIndex : script()->pcToOffset(pc);
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
