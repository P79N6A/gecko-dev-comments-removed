






#ifndef SPSProfiler_h__
#define SPSProfiler_h__

#include <stddef.h>

#include "mozilla/HashFunctions.h"
#include "js/Utility.h"
























































































struct JSFunction;
struct JSScript;

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
    bool                 enabled_;

    const char *allocProfileString(JSContext *cx, JSScript *script,
                                   JSFunction *function);
    void push(const char *string, void *sp, JSScript *script, jsbytecode *pc);
    void pop();

  public:
    SPSProfiler(JSRuntime *rt);
    ~SPSProfiler();

    uint32_t *size() { return size_; }
    uint32_t maxSize() { return max_; }
    ProfileEntry *stack() { return stack_; }

    
    bool enabled() { JS_ASSERT_IF(enabled_, installed()); return enabled_; }
    bool installed() { return stack_ != NULL && size_ != NULL; }
    void enable(bool enabled);
    void enableSlowAssertions(bool enabled) { slowAssertions = enabled; }
    bool slowAssertionsEnabled() { return slowAssertions; }

    








    bool enter(JSContext *cx, JSScript *script, JSFunction *maybeFun);
    void exit(JSContext *cx, JSScript *script, JSFunction *maybeFun);
    void updatePC(JSScript *script, jsbytecode *pc) {
        if (enabled() && *size_ - 1 < max_) {
            JS_ASSERT(*size_ > 0);
            stack_[*size_ - 1].setPC(pc);
        }
    }

    void setProfilingStack(ProfileEntry *stack, uint32_t *size, uint32_t max);
    const char *profileString(JSContext *cx, JSScript *script, JSFunction *maybeFun);
    void onScriptFinalized(JSScript *script);

    
    size_t stringsCount() { return strings.count(); }
    void stringsReset() { strings.clear(); }
};






class SPSEntryMarker
{
    SPSProfiler *profiler;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
  public:
    SPSEntryMarker(JSRuntime *rt JS_GUARD_OBJECT_NOTIFIER_PARAM);
    ~SPSEntryMarker();
};

} 

#endif 
