


















#include "jsapi.h"

void breakpoint();

struct GDBFragment {
    GDBFragment() {
        next = allFragments;
        allFragments = this;
    }

    
    
    virtual const char *name() = 0;

    
    
    
    
    virtual void run(JSContext *cx, const char **&argv) = 0;

    
    
    
    static GDBFragment *allFragments;

    
    GDBFragment *next;
};











#define FRAGMENT(category, subname)                                                             \
class FRAGMENT_CLASS_NAME(category, subname): public GDBFragment {                              \
    void run(JSContext *cx, const char **&argv);                                                \
    const char *name() { return FRAGMENT_STRING_NAME(category, subname); }                      \
    static FRAGMENT_CLASS_NAME(category, subname) singleton;                                    \
};                                                                                              \
FRAGMENT_CLASS_NAME(category, subname) FRAGMENT_CLASS_NAME(category, subname)::singleton;       \
void FRAGMENT_CLASS_NAME(category, subname)::run(JSContext *cx, const char **&argv)

#define FRAGMENT_STRING_NAME(category, subname) (#category "." #subname)
#define FRAGMENT_CLASS_NAME(category, subname) Fragment_ ## category ## _ ## subname

