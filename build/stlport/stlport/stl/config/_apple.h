





#define _STLP_COMPILER "spec me!"

#if defined(__MRC__) && __MRC__ < 0x500
# error Apple's MPW MrCpp v.5.0.0 or better compiler required
#endif
#if defined(__SC__) && __SC__ < 0x890
# error Apple's MPW SCpp v.8.9.0 or better compiler required
#endif




#define _STLP_NO_CONTAINERS_EXTENSION

#ifdef qMacApp
# ifndef __CONDITIONALMACROS__ 
# include <CoreSwitches_AC.h>
# include <ConditionalMacros_AC.h>
# include <Types_AC.h>
# define _STLP_FILE__ _FILE_AC
# define _STLP_DEBUG_MESSAGE
# define __stl_debug_message ProgramBreak_AC
# include <ConditionalMacros.h>
# endif
# include <Types.h>
#else
# include <ConditionalMacros.h>
# include <Types.h>
#endif

#define _STLP_UINT32_T UInt32
typedef int wint_t;

#ifndef TYPE_BOOL
# error <ConditionalMacros.h> must be included. (TYPE_BOOL)
#endif
#if !TYPE_BOOL
# define _STLP_NO_BOOL
# define _STLP_DONT_USE_BOOL_TYPEDEF
#endif

#ifndef TYPE_LONGLONG
# error <ConditionalMacros.h> must be included. (TYPE_LONGLONG)
#endif
#if TYPE_LONGLONG
# define _STLP_LONG_LONG long long
#endif

#if !__option(exceptions)
# define _STLP_HAS_NO_EXCEPTIONS
#endif

#define _STLP_DEBUG_MESSAGE_POST DebugStr("\pSTL diagnosis issued. See 'stderr' for detail.");
#define _STLP_ASSERT_MSG_TRAILER " "

#ifdef _STLP_DEBUG
#   define _STLP_THROW(x) (DebugStr("\pSTL is about to throw exception: "#x),throw x)
#endif

#if defined(__MRC__)
# ifndef __spillargs
#  define __spillargs 1 // MrCpp requires this symbol to be defined as 1 to properly handle va_start; ref.[ file stdarg.h; line 26 ]
# endif
#endif

#if defined(__SC__)
#define _STLP_VENDOR_LONG_DOUBLE_MATH
#endif

#ifndef _STLP_NATIVE_INCLUDE_PATH
# if __option(unix_includes)
#  define _STLP_NATIVE_INCLUDE_PATH ../CIncludes   // expects the alias to {CIncludes} under the same folder as {STL}
# else
#  define _STLP_NATIVE_INCLUDE_PATH ::CIncludes   // expects the alias to {CIncludes} under the same folder as {STL}
# endif
#endif
#if !defined(_STLP_MAKE_HEADER)
# if !__option(unix_includes)
#  define _STLP_MAKE_HEADER(path, header) <path:header> // Mac uses ":" for directory delimiter
# endif
#endif

# define _STLD _DBG  // to keep the length of generated symbols within the compiler limitation

#define _STLP_USE_STDIO_IO 1       //*TY 02/24/2000 - see also ; ref.[ file _fstream.h; line 36 ]
#define _STLP_NO_THREADS
#undef _REENTRANT                  //*ty 11/24/2001 - to make sure no thread facility is activated
#define _NOTHREADS


#define _STLP_VENDOR_GLOBAL_STD
#define _STLP_NO_BAD_ALLOC
#define _STLP_HAS_NO_NEW_C_HEADERS
#define _STLP_NO_NEW_NEW_HEADER
#define _STLP_NO_NATIVE_MBSTATE_T
#define _STLP_NO_NATIVE_WIDE_FUNCTIONS
#define _STLP_NO_NATIVE_WIDE_STREAMS
#define _STLP_NO_UNCAUGHT_EXCEPT_SUPPORT
#define _STLP_BROKEN_EXCEPTION_CLASS


# define _STLP_DONT_SIMULATE_PARTIAL_SPEC_FOR_TYPE_TRAITS

# define _STLP_MPWFIX_TRY try{                      //*TY 06/01/2000 - exception handling bug workaround
# define _STLP_MPWFIX_CATCH }catch(...){throw;}              //*TY 06/01/2000 - exception handling bug workaround
# define _STLP_MPWFIX_CATCH_ACTION(action) }catch(...){action;throw;}  //*TY 06/01/2000 - exception handling bug workaround
# define _STLP_THROW_RETURN_BUG
# define _STLP_NO_CLASS_PARTIAL_SPECIALIZATION
# define _STLP_NO_PARTIAL_SPECIALIZATION_SYNTAX
# define _STLP_NO_FUNCTION_TMPL_PARTIAL_ORDER
# define _STLP_NO_RELOPS_NAMESPACE

