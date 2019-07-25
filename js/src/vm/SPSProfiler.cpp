






#include "jsnum.h"

#include "vm/SPSProfiler.h"
#include "vm/StringBuffer.h"

using namespace js;

SPSProfiler::~SPSProfiler()
{
    if (strings.initialized()) {
        for (ProfileStringMap::Enum e(strings); !e.empty(); e.popFront())
            js_free((void*) e.front().value);
    }
}

void
SPSProfiler::setProfilingStack(ProfileEntry *stack, uint32_t *size, uint32_t max)
{
    if (!strings.initialized())
        strings.init(max);
    stack_ = stack;
    size_  = size;
    max_   = max;
}


const char*
SPSProfiler::profileString(JSContext *cx, JSScript *script, JSFunction *maybeFun)
{
    JS_ASSERT(enabled());
    JS_ASSERT(strings.initialized());
    ProfileStringMap::AddPtr s = strings.lookupForAdd(script);
    if (s)
        return s->value;
    const char *str = allocProfileString(cx, script, maybeFun);
    if (str == NULL)
        return NULL;
    if (!strings.add(s, script, str)) {
        js_free((void*) str);
        return NULL;
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
        js_free((void*) tofree);
    }
}

bool
SPSProfiler::enter(JSContext *cx, JSScript *script, JSFunction *maybeFun)
{
    JS_ASSERT(enabled());
    const char *str = profileString(cx, script, maybeFun);
    if (str == NULL)
        return false;

    if (*size_ < max_) {
        stack_[*size_].string = str;
        stack_[*size_].sp     = NULL;
    }
    (*size_)++;
    return true;
}

void
SPSProfiler::exit(JSContext *cx, JSScript *script, JSFunction *maybeFun)
{
    JS_ASSERT(enabled());
    (*size_)--;
    JS_ASSERT(*(int*)size_ >= 0);

#ifdef DEBUG
    
    if (*size_ < max_) {
        const char *str = profileString(cx, script, maybeFun);
        
        JS_ASSERT(str != NULL);
        JS_ASSERT(strcmp((const char*) stack_[*size_].string, str) == 0);
        stack_[*size_].string = NULL;
        stack_[*size_].sp     = NULL;
    }
#endif
}







const char*
SPSProfiler::allocProfileString(JSContext *cx, JSScript *script, JSFunction *maybeFun)
{
    DebugOnly<uint64_t> gcBefore = cx->runtime->gcNumber;
    StringBuffer buf(cx);
    bool hasAtom = maybeFun != NULL && maybeFun->atom != NULL;
    if (hasAtom) {
        if (!buf.append(maybeFun->atom))
            return NULL;
        if (!buf.append(" ("))
            return NULL;
    }
    if (script->filename) {
        if (!buf.appendInflated(script->filename, strlen(script->filename)))
            return NULL;
    } else if (!buf.append("<unknown>")) {
        return NULL;
    }
    if (!buf.append(":"))
        return NULL;
    if (!NumberValueToStringBuffer(cx, NumberValue(script->lineno), buf))
        return NULL;
    if (hasAtom && !buf.append(")"))
        return NULL;

    size_t len = buf.length();
    char *cstr = (char*) js_malloc(len + 1);
    if (cstr == NULL)
        return NULL;

    const jschar *ptr = buf.begin();
    for (size_t i = 0; i < len; i++)
        cstr[i] = ptr[i];
    cstr[len] = 0;

    JS_ASSERT(gcBefore == cx->runtime->gcNumber);
    return cstr;
}

SPSEntryMarker::SPSEntryMarker(JSRuntime *rt) : profiler(&rt->spsProfiler), pushed(false)
{
    if (!profiler->enabled())
        return;
    uint32_t *size = profiler->size_;
    size_before = *size;
    if (*size < profiler->max_) {
        profiler->stack_[*size].string = "js::RunScript";
        profiler->stack_[*size].sp = this;
    }
    (*size)++;
    pushed = true;
}

SPSEntryMarker::~SPSEntryMarker()
{
    if (!pushed || !profiler->enabled())
        return;
    (*profiler->size_)--;
    JS_ASSERT(*profiler->size_ == size_before);
}
