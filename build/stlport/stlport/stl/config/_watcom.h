


#define _STLP_COMPILER "Watcom"

#if (__WATCOMC__ < 1250)
#  error Not supported!
#endif

#ifndef _CPPRTTI
#  define _STLP_NO_RTTI 1
#endif


#if !(defined (__SW_XS) || defined (__SW_XSS) || defined(__SW_XST))
#  define _STLP_HAS_NO_EXCEPTIONS 1
#endif

#if defined (_MT) && !defined (_NOTHREADS)
#  define _STLP_THREADS 1
#endif

#define _STLP_NO_VENDOR_STDLIB_L
#define _STLP_NO_VENDOR_MATH_F
#define _STLP_NO_VENDOR_MATH_L

#define _STLP_LONG_LONG long long

#define _STLP_CALL __cdecl
#define _STLP_IMPORT_DECLSPEC __declspec(dllimport)

#define _STLP_NO_CONST_IN_PAIR




#define _STLP_DONT_SUPPORT_REBIND_MEMBER_TEMPLATE

#define _STLP_NO_RELOPS_NAMESPACE

#define _STLP_NEEDS_EXTRA_TEMPLATE_CONSTRUCTORS
#define _STLP_NO_STATIC_CONST_DEFINITION


#define _STLP_DONT_SIMULATE_PARTIAL_SPEC_FOR_TYPE_TRAITS


#define _STLP_NO_FUNCTION_TMPL_PARTIAL_ORDER 1
#define _STLP_NO_CLASS_PARTIAL_SPECIALIZATION 1








#define _STLP_NO_EXPLICIT_FUNCTION_TMPL_ARGS 1





#define _STLP_BASE_TYPEDEF_OUTSIDE_BUG 1

#define _STLP_NO_DEFAULT_NON_TYPE_PARAM 1
#define _STLP_NON_TYPE_TMPL_PARAM_BUG 1



#define _STLP_NO_BAD_ALLOC 1






#define _STLP_NO_ARROW_OPERATOR 1




#undef min
#undef max



#ifndef __QNX__
#  define _STLP_NATIVE_INCLUDE_PATH ../h
#else

#  define _STLP_NO_TYPEINFO 1
#endif





#define _STLP_NO_WATCOM_INLINE_INTERLOCK
#if !defined(_STLP_NO_WATCOM_INLINE_INTERLOCK)

long    __stl_InterlockedIncrement( long *var );
long    __stl_InterlockedDecrement( long *var );

#ifdef _STL_MULTIPROCESSOR

#pragma aux __stl_InterlockedIncrement parm [ ecx ] = \
        ".586"                  \
        "mov eax, 1"            \
        "lock xadd [ecx], eax"       \
        "inc eax"               \
        value [eax];


#pragma aux __stl_InterlockedDecrement parm [ ecx ] = \
        ".586"                  \
        "mov eax, 0FFFFFFFFh"   \
        "lock xadd [ecx], eax"       \
        "dec eax"               \
        value [eax];
#else

#pragma aux __stl_InterlockedIncrement parm [ ecx ] = \
        ".586"                  \
        "mov eax, 1"            \
        "xadd [ecx], eax"       \
        "inc eax"               \
        value [eax];

#pragma aux __stl_InterlockedDecrement parm [ ecx ] = \
        ".586"                  \
        "mov eax, 0FFFFFFFFh"   \
        "xadd [ecx], eax"       \
        "dec eax"               \
        value [eax];
#endif 

long    __stl_InterlockedExchange( long *Destination, long Value );


#pragma aux __stl_InterlockedExchange parm [ecx] [eax] = \
        ".586"                  \
        "xchg eax, [ecx]"       \
        value [eax];

#  define _STLP_ATOMIC_INCREMENT(__x) __stl_InterlockedIncrement((long*)__x)
#  define _STLP_ATOMIC_DECREMENT(__x) __stl_InterlockedDecrement((long*)__x)
#  define _STLP_ATOMIC_EXCHANGE(__x, __y) __stl_InterlockedExchange((long*)__x, (long)__y)
#  define _STLP_ATOMIC_EXCHANGE_PTR(__x, __y) __stl_InterlockedExchange((long*)__x, (long)__y)
#endif 

