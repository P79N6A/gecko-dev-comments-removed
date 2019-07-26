


























#ifndef __WEBVTT_UTIL_H__
# define __WEBVTT_UTIL_H__

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

# if !defined(_MSC_VER) || _MSC_VER >= 1600




#   define WEBVTT_HAVE_STDINT 1
#   include <stdint.h>
# endif

# if defined(_WIN32) || defined(__WIN32__) || defined(__TOS_WIN__) ||\
     defined(__WINDOWS__)
#   define WEBVTT_OS_WIN32 1
#   if defined(_WIN64)
#     define WEBVTT_OS_WIN64 1
#   endif
# endif

# if defined(_MSC_VER)
#   define WEBVTT_CC_MSVC 1
#   define WEBVTT_CALLBACK __cdecl
#   if WEBVTT_BUILD_LIBRARY
#     define WEBVTT_EXPORT __declspec(dllexport)
#   elif !WEBVTT_STATIC
#     define WEBVTT_EXPORT __declspec(dllimport)
#   else
#     define WEBVTT_EXPORT
#   endif
# elif defined(__GNUC__)
#   define WEBVTT_CC_GCC 1
#   if WEBVTT_OS_WIN32
#     if WEBVTT_BUILD_LIBRARY
#       define WEBVTT_EXPORT __declspec(dllexport)
#     elif !WEBVTT_STATIC
#       define WEBVTT_EXPORT __declspec(dllimport)
#     else
#       define WEBVTT_EXPORT
#     endif
#   else
#     if __GNUC__ >= 4
#       define WEBVTT_EXPORT __attribute__((visibility("default")))
#       define WEBVTT_INTERN __attribute__((visibility("hidden")))
#     endif
#   endif
# else
#   define WEBVTT_CC_UNKNOWN 1
# endif

# ifndef WEBVTT_CALLBACK
#   define WEBVTT_CALLBACK
# endif
# ifndef WEBVTT_EXPORT
#   define WEBVTT_EXPORT
# endif
# ifndef WEBVTT_INTERN
#   define WEBVTT_INTERN
# endif

# if defined(__cplusplus) || defined(c_plusplus)
#   define WEBVTT_INLINE inline
# elif WEBVTT_CC_MSVC
#   define WEBVTT_INLINE __inline
# elif WEBVTT_CC_GCC
#   define WEBVTT_INLINE __inline__
# endif

# ifdef WEBVTT_HAVE_STDINT
  typedef int8_t webvtt_int8;
  typedef int16_t webvtt_int16;
  typedef int32_t webvtt_int32;
  typedef int64_t webvtt_int64;
  typedef uint8_t webvtt_uint8;
  typedef uint16_t webvtt_uint16;
  typedef uint32_t webvtt_uint32;
  typedef uint64_t webvtt_uint64;
# elif defined(_MSC_VER)
  typedef signed __int8 webvtt_int8;
  typedef signed __int16 webvtt_int16;
  typedef signed __int32 webvtt_int32;
  typedef signed __int64 webvtt_int64;
  typedef unsigned __int8 webvtt_uint8;
  typedef unsigned __int16 webvtt_uint16;
  typedef unsigned __int32 webvtt_uint32;
  typedef unsigned __int64 webvtt_uint64;
# elif WEBVTT_CC_UNKNOWN
#   warning "Unknown compiler. Compiler specific int-types probably broken!"
  typedef signed char webvtt_int8;
  typedef signed short webvtt_int16;
  typedef signed long webvtt_int32;
  typedef signed long long webvtt_int64;
  typedef unsigned char webvtt_uint8;
  typedef unsigned short webvtt_uint16;
  typedef unsigned long webvtt_uint32;
  typedef unsigned long long webvtt_uint64;
# endif

  typedef signed int webvtt_int;
  typedef signed char webvtt_char;
  typedef signed short webvtt_short;
  typedef signed long webvtt_long;
  typedef signed long long webvtt_longlong;
  typedef unsigned int webvtt_uint;
  typedef unsigned char webvtt_uchar;
  typedef unsigned short webvtt_ushort;
  typedef unsigned long webvtt_ulong;
  typedef unsigned long long webvtt_ulonglong;
  typedef webvtt_uint8 webvtt_byte;
  typedef webvtt_int webvtt_bool;
  typedef webvtt_uint32 webvtt_length;
  typedef webvtt_uint64 webvtt_timestamp;

  


  typedef void *(WEBVTT_CALLBACK *webvtt_alloc_fn_ptr)( void *userdata, webvtt_uint nbytes );
  typedef void (WEBVTT_CALLBACK *webvtt_free_fn_ptr)( void *userdata, void *pmem );

  















  WEBVTT_EXPORT void *webvtt_alloc( webvtt_uint nb );
  WEBVTT_EXPORT void *webvtt_alloc0( webvtt_uint nb );
  WEBVTT_EXPORT void webvtt_free( void *data );
  WEBVTT_EXPORT void webvtt_set_allocator( webvtt_alloc_fn_ptr alloc, webvtt_free_fn_ptr free, void *userdata );

  enum
  webvtt_status_t {
    WEBVTT_SUCCESS = 0,
    WEBVTT_UNFINISHED = -1,
    WEBVTT_PARSE_ERROR = -2,
    WEBVTT_OUT_OF_MEMORY = -3,
    WEBVTT_INVALID_PARAM = -4,
    WEBVTT_NOT_SUPPORTED = -5,
    WEBVTT_UNSUCCESSFUL = -6,
    WEBVTT_INVALID_TAG_NAME = -7,
    WEBVTT_INVALID_TOKEN_TYPE = -8,
    WEBVTT_INVALID_TOKEN_STATE = -9,
    WEBVTT_FAIL = -10, 

    

 
    WEBVTT_SKIP_CUE = -11,

    


    WEBVTT_NEXT_CUESETTING = -12,

    


     WEBVTT_NO_MATCH_FOUND = -13,

    



    WEBVTT_FAILED_ASSERTION = -14,
  };

  typedef enum webvtt_status_t webvtt_status;

  


# define WEBVTT_FAILED(status) ( (status) != WEBVTT_SUCCESS )

  struct
  webvtt_refcount_t {
# if WEBVTT_OS_WIN32
    



    long value;
# else
    int value;
# endif
  };

# ifdef WEBVTT_REF_INIT
#   undef WEBVTT_REF_INIT
# endif
# define WEBVTT_REF_INIT(Value) { (Value) }

  


# ifndef WEBVTT_ATOMIC_INC
#   define WEBVTT_ATOMIC_INC(x) ( ++(x) )
# endif
# ifndef WEBVTT_ATOMIC_DEC
#   define WEBVTT_ATOMIC_DEC(x) ( --(x) )
# endif

# if defined(WEBVTT_INLINE)
  static WEBVTT_INLINE int webvtt_ref( struct webvtt_refcount_t *ref )
  {
    return WEBVTT_ATOMIC_INC(ref->value);
  }
  static WEBVTT_INLINE int webvtt_deref( struct webvtt_refcount_t *ref )
  {
    return WEBVTT_ATOMIC_DEC(ref->value);
  }
# else
#   define webvtt_inc_ref(ref) ( WEBVTT_ATOMIC_INC((ref)->value) )
#   define webvtt_dec_ref(ref) ( WEBVTT_ATOMIC_DEC((ref)->value) )
# endif

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
