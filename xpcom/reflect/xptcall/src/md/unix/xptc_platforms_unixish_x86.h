




































































#if defined(LINUX) || (defined(__GLIBC__) && defined(__FreeBSD_kernel__))

#if (__GNUC__ == 2) && (__GNUC_MINOR__ <= 7)

#define CFRONT_STYLE_THIS_ADJUST
#else

#define THUNK_BASED_THIS_ADJUST
#endif

#elif defined(__FreeBSD__) 

















#if defined(__FreeBSD_cc_version) && \
    (__FreeBSD_cc_version < 500003) && \
    (__FreeBSD_cc_version < 400002 || __FreeBSD_cc_version > 400003)
#define CFRONT_STYLE_THIS_ADJUST
#else
#define THUNK_BASED_THIS_ADJUST
#endif

#elif defined(__NetBSD__) 
#define THUNK_BASED_THIS_ADJUST

#elif defined(__OpenBSD__) 
#if __GNUC__ >= 3
#define THUNK_BASED_THIS_ADJUST
#else

#include <sys/param.h>
#if OpenBSD <= 199905
#define THUNK_BASED_THIS_ADJUST
#else
#define CFRONT_STYLE_THIS_ADJUST
#endif
#endif

#elif defined(__bsdi__) 
#include <sys/param.h>
#if _BSDI_VERSION >= 199910

#define THUNK_BASED_THIS_ADJUST
#else
#define CFRONT_STYLE_THIS_ADJUST
#endif

#elif defined(NTO) 
#if (__GNUC__ == 2) && (__GNUC_MINOR__ <= 7)

#define CFRONT_STYLE_THIS_ADJUST
#else

#define THUNK_BASED_THIS_ADJUST
#endif

#elif defined(__BEOS__) 
#define CFRONT_STYLE_THIS_ADJUST

#elif defined(__sun__) || defined(__sun)
#if defined(__GXX_ABI_VERSION) && __GXX_ABI_VERSION >= 100 
#define THUNK_BASED_THIS_ADJUST
#else
#define CFRONT_STYLE_THIS_ADJUST
#endif

#elif defined(_WIN32)
#define THUNK_BASED_THIS_ADJUST

#elif defined(__OS2__)
#define THUNK_BASED_THIS_ADJUST

#elif defined (__APPLE__) && (__MACH__)
#define THUNK_BASED_THIS_ADJUST

#else
#error "need a platform define if using unixish x86 code"
#endif



#if !defined(THUNK_BASED_THIS_ADJUST) && !defined(CFRONT_STYLE_THIS_ADJUST)
#error "need to define 'this' adjust scheme"    
#endif

#if defined(THUNK_BASED_THIS_ADJUST) && defined(CFRONT_STYLE_THIS_ADJUST)
#error "need to define only ONE 'this' adjust scheme"    
#endif

#if defined(__QNXNTO__)


#define KEEP_STACK_16_BYTE_ALIGNED
#endif
