



#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gdb-tests.h"
#include "jsapi.h"
#include "jsfriendapi.h"

using namespace JS;


const JSClass global_class = {
    "global", JSCLASS_GLOBAL_FLAGS,
    nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr,
    JS_GlobalObjectTraceHook
};

template<typename T>
static inline T*
checkPtr(T* ptr)
{
  if (! ptr)
    abort();
  return ptr;
}

static void
checkBool(bool success)
{
  if (! success)
    abort();
}


void reportError(JSContext* cx, const char* message, JSErrorReport* report)
{
    fprintf(stderr, "%s:%u: %s\n",
            report->filename ? report->filename : "<no filename>",
            (unsigned int) report->lineno,
            message);
}



void breakpoint() {
    
    
    
    
    fprintf(stderr, "Called " __FILE__ ":breakpoint\n");
}

GDBFragment* GDBFragment::allFragments = nullptr;

int
main (int argc, const char** argv)
{
    if (!JS_Init()) return 1;
    JSRuntime* runtime = checkPtr(JS_NewRuntime(1024 * 1024));
    JS_SetGCParameter(runtime, JSGC_MAX_BYTES, 0xffffffff);
    JS_SetNativeStackQuota(runtime, 5000000);

    JSContext* cx = checkPtr(JS_NewContext(runtime, 8192));
    JS_SetErrorReporter(runtime, reportError);

    JSAutoRequest ar(cx);

    
    JS::CompartmentOptions options;
    options.setVersion(JSVERSION_LATEST);
    RootedObject global(cx, checkPtr(JS_NewGlobalObject(cx, &global_class,
                        nullptr, JS::FireOnNewGlobalHook, options)));
    JSAutoCompartment ac(cx, global);

    

    checkBool(JS_InitStandardClasses(cx, global));

    argv++;
    while (*argv) {
        const char* name = *argv++;
        GDBFragment* fragment;
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
