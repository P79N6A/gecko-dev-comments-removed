











#if defined (__GNUC__)
#  define FUNC     ((const char*) (__PRETTY_FUNCTION__))
#elif defined (__sun) || (defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L)
#  define FUNC     ((const char*) (__func__))
#else
#  define FUNC     ((const char*) ("???"))
#endif

#ifndef INT16_MIN
# define INT16_MIN              (-32767-1)
#endif

#ifndef INT16_MAX
# define INT16_MAX              (32767)
#endif

#ifndef INT32_MIN
# define INT32_MIN              (-2147483647-1)
#endif

#ifndef INT32_MAX
# define INT32_MAX              (2147483647)
#endif

#ifndef UINT32_MIN
# define UINT32_MIN             (0)
#endif

#ifndef UINT32_MAX
# define UINT32_MAX             (4294967295U)
#endif

#ifndef M_PI
# define M_PI			3.14159265358979323846
#endif

#ifdef _MSC_VER

#   define inline __inline
#   define force_inline __forceinline
#elif defined __GNUC__ || (defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590))
#   define inline __inline__
#   define force_inline __inline__ __attribute__ ((__always_inline__))
#else
#   ifndef force_inline
#      define force_inline inline
#   endif
#endif


#if defined(__GNUC__) && __GNUC__ >= 4
#   define PIXMAN_EXPORT __attribute__ ((visibility("default")))

#elif defined(__SUNPRO_C) && (__SUNPRO_C >= 0x550)
#   define PIXMAN_EXPORT __global
#else
#   define PIXMAN_EXPORT
#endif


#if defined(PIXMAN_NO_TLS)

#   define PIXMAN_DEFINE_THREAD_LOCAL(type, name)            \
    static type name
#   define PIXMAN_GET_THREAD_LOCAL(name)                \
    (&name)

#elif defined(TOOLCHAIN_SUPPORTS__THREAD)

#   define PIXMAN_DEFINE_THREAD_LOCAL(type, name)			\
    static __thread type name
#   define PIXMAN_GET_THREAD_LOCAL(name)				\
    (&name)

#elif defined(_MSC_VER)

#   define PIXMAN_DEFINE_THREAD_LOCAL(type, name)			\
    static __declspec(thread) type name
#   define PIXMAN_GET_THREAD_LOCAL(name)				\
    (&name)

#elif defined(HAVE_PTHREAD_SETSPECIFIC)

#include <pthread.h>

#  define PIXMAN_DEFINE_THREAD_LOCAL(type, name)			\
    static pthread_once_t tls_ ## name ## _once_control = PTHREAD_ONCE_INIT; \
    static pthread_key_t tls_ ## name ## _key;				\
									\
    static void								\
    tls_ ## name ## _make_key (void)					\
    {									\
	pthread_key_create (&tls_ ## name ## _key, NULL);		\
    }									\
									\
    static type *							\
    tls_ ## name ## _alloc (void)					\
    {									\
	type *value = calloc (1, sizeof (type));			\
	if (value)							\
	    pthread_setspecific (tls_ ## name ## _key, value);		\
	return value;							\
    }									\
									\
    static force_inline type *						\
    tls_ ## name ## _get (void)						\
    {									\
	type *value = NULL;						\
	if (pthread_once (&tls_ ## name ## _once_control,		\
			  tls_ ## name ## _make_key) == 0)		\
	{								\
	    value = pthread_getspecific (tls_ ## name ## _key);		\
	    if (!value)							\
		value = tls_ ## name ## _alloc ();			\
	}								\
	return value;							\
    }

#   define PIXMAN_GET_THREAD_LOCAL(name)				\
    tls_ ## name ## _get ()

#else

#    error "Unknown thread local support for this system. Pixman will not work with multiple threads. Define PIXMAN_NO_TLS to acknowledge and accept this limitation and compile pixman without thread-safety support."

#endif
