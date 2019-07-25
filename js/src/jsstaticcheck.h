






































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




#ifndef HAVE_STATIC_ANNOTATIONS
#define HAVE_STATIC_ANNOTATIONS

#ifdef XGILL_PLUGIN

#define STATIC_PRECONDITION(COND)         __attribute__((precondition(#COND)))
#define STATIC_PRECONDITION_ASSUME(COND)  __attribute__((precondition_assume(#COND)))
#define STATIC_POSTCONDITION(COND)        __attribute__((postcondition(#COND)))
#define STATIC_POSTCONDITION_ASSUME(COND) __attribute__((postcondition_assume(#COND)))
#define STATIC_INVARIANT(COND)            __attribute__((invariant(#COND)))
#define STATIC_INVARIANT_ASSUME(COND)     __attribute__((invariant_assume(#COND)))


#define STATIC_PASTE2(X,Y) X ## Y
#define STATIC_PASTE1(X,Y) STATIC_PASTE2(X,Y)

#define STATIC_ASSERT(COND)                          \
  JS_BEGIN_MACRO                                     \
    __attribute__((assert_static(#COND), unused))    \
    int STATIC_PASTE1(assert_static_, __COUNTER__);  \
  JS_END_MACRO

#define STATIC_ASSUME(COND)                          \
  JS_BEGIN_MACRO                                     \
    __attribute__((assume_static(#COND), unused))    \
    int STATIC_PASTE1(assume_static_, __COUNTER__);  \
  JS_END_MACRO

#define STATIC_ASSERT_RUNTIME(COND)                         \
  JS_BEGIN_MACRO                                            \
    __attribute__((assert_static_runtime(#COND), unused))   \
    int STATIC_PASTE1(assert_static_runtime_, __COUNTER__); \
  JS_END_MACRO

#else 

#define STATIC_PRECONDITION(COND)
#define STATIC_PRECONDITION_ASSUME(COND)
#define STATIC_POSTCONDITION(COND)
#define STATIC_POSTCONDITION_ASSUME(COND)
#define STATIC_INVARIANT(COND)
#define STATIC_INVARIANT_ASSUME(COND)

#define STATIC_ASSERT(COND)          JS_BEGIN_MACRO /* nothing */ JS_END_MACRO
#define STATIC_ASSUME(COND)          JS_BEGIN_MACRO /* nothing */ JS_END_MACRO
#define STATIC_ASSERT_RUNTIME(COND)  JS_BEGIN_MACRO /* nothing */ JS_END_MACRO

#endif 

#define STATIC_SKIP_INFERENCE STATIC_INVARIANT(skip_inference())

#endif 

#endif 
