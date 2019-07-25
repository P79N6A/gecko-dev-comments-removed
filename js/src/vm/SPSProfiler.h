






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

    ProfileStringMap     strings;
    ProfileEntry         *stack_;
    uint32_t             *size_;
    uint32_t             max_;

    static const char *allocProfileString(JSContext *cx, JSScript *script,
                                          JSFunction *function);
  public:
    SPSProfiler() : stack_(NULL), size_(NULL), max_(0) {}
    ~SPSProfiler();

    uint32_t *size() { return size_; }
    uint32_t maxSize() { return max_; }
    ProfileEntry *stack() { return stack_; }

    bool enabled() { return stack_ != NULL; }
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
    bool pushed;
    DebugOnly<uint32_t> size_before;

  public:
    SPSEntryMarker(JSRuntime *rt);
    ~SPSEntryMarker();
};

} 

#endif 
