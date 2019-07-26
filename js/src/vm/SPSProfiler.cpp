






#include "mozilla/DebugOnly.h"

#include "jsnum.h"
#include "jsscript.h"

#include "methodjit/MethodJIT.h"
#include "methodjit/Compiler.h"

#include "vm/SPSProfiler.h"
#include "vm/StringBuffer.h"

using namespace js;

using mozilla::DebugOnly;

SPSProfiler::SPSProfiler(JSRuntime *rt)
  : rt(rt),
    stack_(NULL),
    size_(NULL),
    max_(0),
    slowAssertions(false),
    enabled_(false)
{
    JS_ASSERT(rt != NULL);
}

SPSProfiler::~SPSProfiler()
{
    if (strings.initialized()) {
        for (ProfileStringMap::Enum e(strings); !e.empty(); e.popFront())
            js_free(const_cast<char *>(e.front().value));
    }
#ifdef JS_METHODJIT
    if (jminfo.initialized()) {
        for (JITInfoMap::Enum e(jminfo); !e.empty(); e.popFront())
            js_delete(e.front().value);
    }
#endif
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
}


const char*
SPSProfiler::profileString(JSContext *cx, RawScript script, RawFunction maybeFun)
{
    JS_ASSERT(strings.initialized());
    ProfileStringMap::AddPtr s = strings.lookupForAdd(script);
    if (s)
        return s->value;
    const char *str = allocProfileString(cx, script, maybeFun);
    if (str == NULL)
        return NULL;
    if (!strings.add(s, script, str)) {
        js_free(const_cast<char *>(str));
        return NULL;
    }
    return str;
}

void
SPSProfiler::onScriptFinalized(RawScript script)
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
SPSProfiler::enter(JSContext *cx, RawScript script, RawFunction maybeFun)
{
    const char *str = profileString(cx, script, maybeFun);
    if (str == NULL)
        return false;

    JS_ASSERT_IF(*size_ > 0 && *size_ - 1 < max_ && stack_[*size_ - 1].js(),
                 stack_[*size_ - 1].pc() != NULL);
    push(str, NULL, script, script->code);
    return true;
}

void
SPSProfiler::exit(JSContext *cx, RawScript script, RawFunction maybeFun)
{
    pop();

#ifdef DEBUG
    
    if (*size_ < max_) {
        const char *str = profileString(cx, script, maybeFun);
        
        JS_ASSERT(str != NULL);

        
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
        stack_[*size_].setLabel(NULL);
        stack_[*size_].setPC(NULL);
    }
#endif
}

void
SPSProfiler::push(const char *string, void *sp, RawScript script, jsbytecode *pc)
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
SPSProfiler::allocProfileString(JSContext *cx, RawScript script, RawFunction maybeFun)
{
    DebugOnly<uint64_t> gcBefore = cx->runtime->gcNumber;
    StringBuffer buf(cx);
    bool hasAtom = maybeFun != NULL && maybeFun->displayAtom() != NULL;
    if (hasAtom) {
        if (!buf.append(maybeFun->displayAtom()))
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
    char *cstr = js_pod_malloc<char>(len + 1);
    if (cstr == NULL)
        return NULL;

    const jschar *ptr = buf.begin();
    for (size_t i = 0; i < len; i++)
        cstr[i] = ptr[i];
    cstr[len] = 0;

    JS_ASSERT(gcBefore == cx->runtime->gcNumber);
    return cstr;
}

#ifdef JS_METHODJIT
typedef SPSProfiler::JMChunkInfo JMChunkInfo;

JMChunkInfo::JMChunkInfo(mjit::JSActiveFrame *frame,
                         mjit::PCLengthEntry *pcLengths,
                         mjit::JITChunk *chunk)
  : mainStart(frame->mainCodeStart),
    mainEnd(frame->mainCodeEnd),
    stubStart(frame->stubCodeStart),
    stubEnd(frame->stubCodeEnd),
    pcLengths(pcLengths),
    chunk(chunk)
{}

jsbytecode*
SPSProfiler::ipToPC(RawScript script, size_t ip)
{
    if (!jminfo.initialized())
        return NULL;

    JITInfoMap::Ptr ptr = jminfo.lookup(script);
    if (!ptr)
        return NULL;
    JMScriptInfo *info = ptr->value;

    
    for (unsigned i = 0; i < info->ics.length(); i++) {
        ICInfo &ic = info->ics[i];
        if (ic.base <= ip && ip < ic.base + ic.size)
            return ic.pc;
    }

    
    for (unsigned i = 0; i < info->chunks.length(); i++) {
        jsbytecode *pc = info->chunks[i].convert(script, ip);
        if (pc != NULL)
            return pc;
    }

    return NULL;
}

jsbytecode*
JMChunkInfo::convert(RawScript script, size_t ip)
{
    if (mainStart <= ip && ip < mainEnd) {
        size_t offset = 0;
        uint32_t i;
        for (i = 0; i < script->length - 1; i++) {
            offset += (uint32_t) pcLengths[i].inlineLength;
            if (mainStart + offset > ip)
                break;
        }
        return &script->code[i];
    } else if (stubStart <= ip && ip < stubEnd) {
        size_t offset = 0;
        uint32_t i;
        for (i = 0; i < script->length - 1; i++) {
            offset += (uint32_t) pcLengths[i].stubLength;
            if (stubStart + offset > ip)
                break;
        }
        return &script->code[i];
    }

    return NULL;
}

bool
SPSProfiler::registerMJITCode(mjit::JITChunk *chunk,
                              mjit::JSActiveFrame *outerFrame,
                              mjit::JSActiveFrame **inlineFrames)
{
    if (!jminfo.initialized() && !jminfo.init(100))
        return false;

    JS_ASSERT(chunk->pcLengths != NULL);

    JMChunkInfo *info = registerScript(outerFrame, chunk->pcLengths, chunk);
    if (!info)
        return false;

    









    mjit::PCLengthEntry *pcLengths = chunk->pcLengths + outerFrame->script->length;
    for (unsigned i = 0; i < chunk->nInlineFrames; i++) {
        JMChunkInfo *child = registerScript(inlineFrames[i], pcLengths, chunk);
        if (!child)
            return false;
        





        child->mainStart += info->mainStart;
        child->mainEnd   += info->mainStart;
        child->stubStart += info->stubStart;
        child->stubEnd   += info->stubStart;

        pcLengths += inlineFrames[i]->script->length;
    }

    return true;
}

JMChunkInfo*
SPSProfiler::registerScript(mjit::JSActiveFrame *frame,
                            mjit::PCLengthEntry *entries,
                            mjit::JITChunk *chunk)
{
    




    JITInfoMap::AddPtr ptr = jminfo.lookupForAdd(frame->script);
    JMScriptInfo *info;
    if (ptr) {
        info = ptr->value;
        JS_ASSERT(info->chunks.length() > 0);
    } else {
        info = rt->new_<JMScriptInfo>();
        if (info == NULL || !jminfo.add(ptr, frame->script, info))
            return NULL;
    }
    if (!info->chunks.append(JMChunkInfo(frame, entries, chunk)))
        return NULL;
    return info->chunks.end() - 1;
}

bool
SPSProfiler::registerICCode(mjit::JITChunk *chunk,
                            RawScript script, jsbytecode *pc,
                            void *base, size_t size)
{
    JS_ASSERT(jminfo.initialized());
    JITInfoMap::Ptr ptr = jminfo.lookup(script);
    JS_ASSERT(ptr);
    return ptr->value->ics.append(ICInfo(base, size, pc));
}

void
SPSProfiler::discardMJITCode(mjit::JITScript *jscr,
                             mjit::JITChunk *chunk, void* address)
{
    if (!jminfo.initialized())
        return;

    unregisterScript(jscr->script, chunk);
    for (unsigned i = 0; i < chunk->nInlineFrames; i++)
        unregisterScript(chunk->inlineFrames()[i].fun->nonLazyScript(), chunk);
}

void
SPSProfiler::unregisterScript(RawScript script, mjit::JITChunk *chunk)
{
    JITInfoMap::Ptr ptr = jminfo.lookup(script);
    if (!ptr)
        return;
    JMScriptInfo *info = ptr->value;
    for (unsigned i = 0; i < info->chunks.length(); i++) {
        if (info->chunks[i].chunk == chunk) {
            info->chunks.erase(&info->chunks[i]);
            break;
        }
    }
    if (info->chunks.length() == 0) {
        jminfo.remove(ptr);
        js_delete(info);
    }
}
#endif

SPSEntryMarker::SPSEntryMarker(JSRuntime *rt
                               MOZ_GUARD_OBJECT_NOTIFIER_PARAM_IN_IMPL)
    : profiler(&rt->spsProfiler)
{
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    if (!profiler->enabled()) {
        profiler = NULL;
        return;
    }
    size_before = *profiler->size_;
    profiler->push("js::RunScript", this, NULL, NULL);
}

SPSEntryMarker::~SPSEntryMarker()
{
    if (profiler != NULL) {
        profiler->pop();
        JS_ASSERT(size_before == *profiler->size_);
    }
}

JS_FRIEND_API(jsbytecode*)
ProfileEntry::pc() volatile {
    JS_ASSERT_IF(idx != NullPCIndex, idx >= 0 && uint32_t(idx) < script()->length);
    return idx == NullPCIndex ? NULL : script()->code + idx;
}

JS_FRIEND_API(void)
ProfileEntry::setPC(jsbytecode *pc) volatile {
    JS_ASSERT_IF(pc != NULL, script()->code <= pc &&
                             pc < script()->code + script()->length);
    idx = pc == NULL ? NullPCIndex : pc - script()->code;
}
