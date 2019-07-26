



#define _STLP_COMPILER "aCC"


#if defined (_XOPEN_SOURCE) && (_XOPEN_VERSION - 0 >= 4)
#  define _STLP_RAND48 1
#endif


#define _STLP_HPACC_BROKEN_BUFEND       1
#define _STLP_WCHAR_HPACC_EXCLUDE      1


#define _STLP_INLINE_STRING_LITERAL_BUG


#define _STLP_HAS_SPECIFIC_PROLOG_EPILOG


#ifdef __HPACC_NOEH
#  define _STLP_HAS_NO_EXCEPTIONS 1
#endif

#define _STLP_NO_FORCE_INSTANTIATE
#define _STLP_LONG_LONG long long
#define _STLP_NO_VENDOR_STDLIB_L





#if ((__HP_aCC > 60000) && (__HP_aCC < 61200))
#  define __EDG__
#  define __EDG_VERSION__ 306
#endif 

#if (__HP_aCC >= 32500 )
#  define _STLP_USE_NEW_C_HEADERS

#  define _STLP_FORCE_ALLOCATORS(t,a) \
  typedef typename _Alloc_traits<t,a>::_Orig _STLP_dummy_type1;\
  typedef typename _STLP_dummy_type1:: _STLP_TEMPLATE rebind<t>::other _STLP_dummy_type2;

#  if defined (_HP_NAMESPACE_STD) 






#    define _STLP_NATIVE_INCLUDE_PATH ../include_std
#  else 
#    define _STLP_VENDOR_GLOBAL_STD         1
#    define _STLP_VENDOR_GLOBAL_CSTD        1
#    define _STLP_DONT_THROW_RANGE_ERRORS   1
#  endif
#endif

#if (__HP_aCC >= 31400 && __HP_aCC < 32500)
#  define _STLP_FORCE_ALLOCATORS(t,a) \
typedef typename _Alloc_traits<t,a>::_Orig _STLP_dummy_type1;\
typedef typename _STLP_dummy_type1:: _STLP_TEMPLATE rebind<t>::other _STLP_dummy_type2;
#  define _STLP_NO_CWCHAR
#  if defined (_NAMESPACE_STD) 

#    define _STLP_NATIVE_INCLUDE_PATH       ../include_std
#  else 
#    define _STLP_VENDOR_GLOBAL_STD         1
#    define _STLP_VENDOR_GLOBAL_CSTD        1
#    define _STLP_DONT_THROW_RANGE_ERRORS   1
#    define _STLP_NO_ROPE                   1
#  endif
#endif 

#if ((__HP_aCC >= 30000 && __HP_aCC < 31400) || (__HP_aCC == 1)) 

#  if (__HP_aCC != 1)
#    define _STLP_HAS_NO_NEW_C_HEADERS 1
#  endif

#  define _STLP_NO_QUALIFIED_FRIENDS       1



#  define _STLP_PARTIAL_SPEC_NEEDS_TEMPLATE_ARGS 1

#  define _STLP_NO_MEMBER_TEMPLATE_CLASSES 1
#  define _STLP_NO_MEMBER_TEMPLATE_KEYWORD 1

#  define _STLP_VENDOR_GLOBAL_EXCEPT_STD

#  define _STLP_VENDOR_GLOBAL_CSTD        1

#  define _XPG4
#  define _INCLUDE_XOPEN_SOURCE
#  define _INCLUDE_AES_SOURCE
#endif

#if (__HP_aCC <= 30000 && __HP_aCC >= 12100)

#  ifdef _STLP_DEBUG
static void _STLP_dummy_literal() { const char *p = "x";}
static void _STLP_dummy_literal_2() { const char *p = "123456789"; }
static void _STLP_dummy_literal_3() { const char *p = "123456700000000000000089";}
#  endif

#  define _STLP_VENDOR_GLOBAL_STD         1
#  define _STLP_VENDOR_GLOBAL_CSTD        1
#  define _STLP_DONT_THROW_RANGE_ERRORS   1
#  define _STLP_STATIC_CONST_INIT_BUG 1
#  if (__HP_aCC  < 12700)

#    define _STLP_NO_CWCHAR
#  endif

#  define _STLP_FORCE_ALLOCATORS(t,a) \
  typedef typename _Alloc_traits<t,a>::_Orig _STLP_dummy_type1;\
  typedef typename _STLP_dummy_type1:: _STLP_TEMPLATE rebind<t>::other _STLP_dummy_type2;
#endif

#if __HP_aCC == 1
#  define _STLP_BROKEN_USING_IN_CLASS
#  define _STLP_USING_BASE_MEMBER
#  define _STLP_NO_CWCHAR

#endif
