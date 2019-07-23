






































#ifndef jsstaticcheck_h___
#define jsstaticcheck_h___

#ifdef NS_STATIC_CHECKING



inline __attribute__ ((unused)) void MUST_FLOW_THROUGH(const char *label) {
}


#define MUST_FLOW_LABEL(label) goto label; label:

inline JS_FORCES_STACK void VOUCH_DOES_NOT_REQUIRE_STACK() {}

inline JS_FORCES_STACK void
JS_ASSERT_NOT_ON_TRACE(JSContext *cx)
{
    JS_ASSERT(!JS_ON_TRACE(cx));
}

#else
#define MUST_FLOW_THROUGH(label)            ((void) 0)
#define MUST_FLOW_LABEL(label)
#define VOUCH_DOES_NOT_REQUIRE_STACK()      ((void) 0)
#define JS_ASSERT_NOT_ON_TRACE(cx)          JS_ASSERT(!JS_ON_TRACE(cx))
#endif
#define VOUCH_HAVE_STACK                    VOUCH_DOES_NOT_REQUIRE_STACK

#endif 
