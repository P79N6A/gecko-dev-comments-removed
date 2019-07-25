






#ifndef SPSProfiler_h__
#define SPSProfiler_h__

#include "mozilla/HashFunctions.h"

#include <stddef.h>

#include "jsfriendapi.h"
#include "jsfun.h"
#include "jsscript.h"



































































namespace js {

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

    static const char *allocProfileString(JSContext *cx, JSScript *script,
                                          JSFunction *function);
    void push(const char *string, void *sp);
    void pop();

  public:
    SPSProfiler(JSRuntime *rt)
        : rt(rt),
          stack_(NULL),
          size_(NULL),
          max_(0),
          slowAssertions(false),
          enabled_(false)
    {}
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
