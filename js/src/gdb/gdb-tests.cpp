



#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gdb-tests.h"

using namespace JS;


JSClass global_class = {
    "global", JSCLASS_GLOBAL_FLAGS,
    JS_PropertyStub,  JS_PropertyStub, JS_PropertyStub,  JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub
};

template<typename T>
inline T *
checkPtr(T *ptr)
{
  if (! ptr)
    abort();
  return ptr;
}

void
checkBool(JSBool success)
{
  if (! success)
    abort();
}


void reportError(JSContext *cx, const char *message, JSErrorReport *report)
{
    fprintf(stderr, "%s:%u: %s\n",
            report->filename ? report->filename : "<no filename>",
            (unsigned int) report->lineno,
            message);
}



void breakpoint() {
    
    
    
    
    fprintf(stderr, "Called " __FILE__ ":breakpoint\n");
}

GDBFragment *GDBFragment::allFragments = NULL;

int
main (int argc, const char **argv)
{
    JSRuntime *runtime = checkPtr(JS_NewRuntime(1024 * 1024, JS_USE_HELPER_THREADS));
    JS_SetGCParameter(runtime, JSGC_MAX_BYTES, 0xffffffff);
    JS_SetNativeStackQuota(runtime, 5000000);

    JSContext *cx = checkPtr(JS_NewContext(runtime, 8192));
    JS_SetVersion(cx, JSVERSION_LATEST);
    JS_SetErrorReporter(cx, reportError);

    JSAutoRequest ar(cx);

    
    js::RootedObject global(cx, checkPtr(JS_NewGlobalObject(cx, &global_class, NULL)));
    JS_SetGlobalObject(cx, global);

    JSAutoCompartment ac(cx, global);

    

    checkBool(JS_InitStandardClasses(cx, global));

    argv++;
    while (*argv) {
        const char *name = *argv++;
        GDBFragment *fragment;
        for (fragment = GDBFragment::allFragments; fragment; fragment = fragment->next) {
            if (strcmp(fragment->name(), name) == 0) {
                fragment->run(cx, argv);
                break;
            }
        }
        if (!fragment) {
            fprintf(stderr, "Unrecognized fragment name: %s\n", name);
            exit(1);
        }
    }

    return 0;
}
