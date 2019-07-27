







#ifndef js_Principals_h
#define js_Principals_h

#include "mozilla/Atomics.h"

#include <stdint.h>

#include "jspubtd.h"

struct JSPrincipals {
    
    mozilla::Atomic<int32_t> refcount;

#ifdef JS_DEBUG
    
    uint32_t    debugToken;
#endif

    JSPrincipals() : refcount(0) {}

    void setDebugToken(uint32_t token) {
# ifdef JS_DEBUG
        debugToken = token;
# endif
    }

    



    JS_PUBLIC_API(void) dump();
};

extern JS_PUBLIC_API(void)
JS_HoldPrincipals(JSPrincipals *principals);

extern JS_PUBLIC_API(void)
JS_DropPrincipals(JSRuntime *rt, JSPrincipals *principals);




typedef bool
(* JSSubsumesOp)(JSPrincipals *first, JSPrincipals *second);





typedef bool
(* JSCSPEvalChecker)(JSContext *cx);

struct JSSecurityCallbacks {
    JSCSPEvalChecker           contentSecurityPolicyAllows;
    JSSubsumesOp               subsumes;
};

extern JS_PUBLIC_API(void)
JS_SetSecurityCallbacks(JSRuntime *rt, const JSSecurityCallbacks *callbacks);

extern JS_PUBLIC_API(const JSSecurityCallbacks *)
JS_GetSecurityCallbacks(JSRuntime *rt);













extern JS_PUBLIC_API(void)
JS_SetTrustedPrincipals(JSRuntime *rt, const JSPrincipals *prin);

typedef void
(* JSDestroyPrincipalsOp)(JSPrincipals *principals);






extern JS_PUBLIC_API(void)
JS_InitDestroyPrincipalsCallback(JSRuntime *rt, JSDestroyPrincipalsOp destroyPrincipals);

#endif  
