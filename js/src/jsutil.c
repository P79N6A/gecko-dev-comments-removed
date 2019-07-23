










































#include "jsstddef.h"
#include <stdio.h>
#include <stdlib.h>
#include "jstypes.h"
#include "jsutil.h"

#ifdef WIN32
#    include <windows.h>
#endif

JS_PUBLIC_API(void) JS_Assert(const char *s, const char *file, JSIntn ln)
{
    fprintf(stderr, "Assertion failure: %s, at %s:%d\n", s, file, ln);
#if defined(WIN32)
    DebugBreak();
    exit(3);
#elif defined(XP_OS2) || (defined(__GNUC__) && defined(__i386))
    asm("int $3");
#endif
    abort();
}

#if defined DEBUG_notme && defined XP_UNIX

#define __USE_GNU 1
#include <dlfcn.h>
#include <string.h>
#include "jshash.h"
#include "jsprf.h"

JSCallsite js_calltree_root = {0, NULL, NULL, 0, NULL, NULL, NULL, NULL};

static JSCallsite *
CallTree(void **bp)
{
    void **bpup, **bpdown, *pc;
    JSCallsite *parent, *site, **csp;
    Dl_info info;
    int ok, offset;
    const char *symbol;
    char *method;

    
    bpup = NULL;
    for (;;) {
        bpdown = (void**) bp[0];
        bp[0] = (void*) bpup;
        if ((void**) bpdown[0] < bpdown)
            break;
        bpup = bp;
        bp = bpdown;
    }

    
    parent = &js_calltree_root;
    do {
        bpup = (void**) bp[0];
        bp[0] = (void*) bpdown;
        pc = bp[1];

        csp = &parent->kids;
        while ((site = *csp) != NULL) {
            if (site->pc == pc) {
                
                *csp = site->siblings;
                site->siblings = parent->kids;
                parent->kids = site;

                
                goto upward;
            }
            csp = &site->siblings;
        }

        
        for (site = parent; site; site = site->parent) {
            if (site->pc == pc)
                goto upward;
        }

        



        info.dli_fname = info.dli_sname = NULL;
        ok = dladdr(pc, &info);
        if (ok < 0) {
            fprintf(stderr, "dladdr failed!\n");
            return NULL;
        }


        symbol = info.dli_sname;
        offset = (char*)pc - (char*)info.dli_fbase;
        method = symbol
                 ? strdup(symbol)
                 : JS_smprintf("%s+%X",
                               info.dli_fname ? info.dli_fname : "main",
                               offset);
        if (!method)
            return NULL;

        
        site = (JSCallsite *) malloc(sizeof(JSCallsite));
        if (!site)
            return NULL;

        
        site->pc = pc;
        site->name = method;
        site->library = info.dli_fname;
        site->offset = offset;
        site->parent = parent;
        site->siblings = parent->kids;
        parent->kids = site;
        site->kids = NULL;

      upward:
        parent = site;
        bpdown = bp;
        bp = bpup;
    } while (bp);

    return site;
}

JSCallsite *
JS_Backtrace(int skip)
{
    void **bp, **bpdown;

    
#if defined(__i386)
    __asm__( "movl %%ebp, %0" : "=g"(bp));
#elif defined(__x86_64__)
    __asm__( "movq %%rbp, %0" : "=g"(bp));
#else
    




    bp = (void**) __builtin_frame_address(0);
#endif
    while (--skip >= 0) {
        bpdown = (void**) *bp++;
        if (bpdown < bp)
            break;
        bp = bpdown;
    }

    return CallTree(bp);
}

#endif 
