




















#ifndef PNGCONF_H
#define PNGCONF_H

#ifndef PNG_BUILDING_SYMBOL_TABLE




#  ifndef PNG_NO_LIMITS_H
#    include <limits.h>
#  endif





#  ifdef BSD
#    include <strings.h>
#  else
#    include <string.h>
#  endif




#  ifdef PNG_STDIO_SUPPORTED
#    include <stdio.h>
#  endif
#endif







#ifndef PNG_READ_INT_FUNCTIONS_SUPPORTED
#  define PNG_USE_READ_MACROS
#endif
#if !defined(PNG_NO_USE_READ_MACROS) && !defined(PNG_USE_READ_MACROS)
#  if PNG_DEFAULT_READ_MACROS
#    define PNG_USE_READ_MACROS
#  endif
#endif
















#ifndef PNGARG

#  ifdef OF 
#    define PNGARG(arglist) OF(arglist)
#  else

#    ifdef _NO_PROTO
#      define PNGARG(arglist) ()
#    else
#      define PNGARG(arglist) arglist
#    endif 

#  endif 

#endif 























































































#if defined(_Windows) || defined(_WINDOWS) || defined(WIN32) ||\
    defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN__)
  




#  if PNG_API_RULE == 2
    




#    define PNGCAPI __watcall
#  endif

#  if defined(__GNUC__) || (defined (_MSC_VER) && (_MSC_VER >= 800))
#    define PNGCAPI __cdecl
#    if PNG_API_RULE == 1
       


#      define PNGAPI __stdcall
#    endif
#  else
    



#    ifndef PNGCAPI
#      define PNGCAPI _cdecl
#    endif
#    if PNG_API_RULE == 1 && !defined(PNGAPI)
#      define PNGAPI _stdcall
#    endif
#  endif 
  

#  if defined(PNGAPI) && !defined(PNG_USER_PRIVATEBUILD)
   ERROR: PNG_USER_PRIVATEBUILD must be defined if PNGAPI is changed
#  endif

#  if (defined(_MSC_VER) && _MSC_VER < 800) ||\
      (defined(__BORLANDC__) && __BORLANDC__ < 0x500)
    



#    ifndef PNG_EXPORT_TYPE
#      define PNG_EXPORT_TYPE(type) type PNG_IMPEXP
#    endif
#    define PNG_DLL_EXPORT __export
#  else 
#    define PNG_DLL_EXPORT __declspec(dllexport)
#    ifndef PNG_DLL_IMPORT
#      define PNG_DLL_IMPORT __declspec(dllimport)
#    endif
#  endif 

#else 
#  if (defined(__IBMC__) || defined(__IBMCPP__)) && defined(__OS2__)
#    define PNGAPI _System
#  else 
    


#  endif 
#endif 


#ifndef PNGCAPI
#  define PNGCAPI
#endif
#ifndef PNGCBAPI
#  define PNGCBAPI PNGCAPI
#endif
#ifndef PNGAPI
#  define PNGAPI PNGCAPI
#endif





#ifndef PNG_IMPEXP
#  if defined(PNG_USE_DLL) && defined(PNG_DLL_IMPORT)
     
#    define PNG_IMPEXP PNG_DLL_IMPORT
#  endif

#  ifndef PNG_IMPEXP
#    define PNG_IMPEXP
#  endif
#endif








#ifndef PNG_FUNCTION
#  define PNG_FUNCTION(type, name, args, attributes) attributes type name args
#endif

#ifndef PNG_EXPORT_TYPE
#  define PNG_EXPORT_TYPE(type) PNG_IMPEXP type
#endif

   



#ifndef PNG_EXPORTA

#  define PNG_EXPORTA(ordinal, type, name, args, attributes)\
      PNG_FUNCTION(PNG_EXPORT_TYPE(type),(PNGAPI name),PNGARG(args), \
        extern attributes)
#endif




#define PNG_EMPTY

#define PNG_EXPORT(ordinal, type, name, args)\
   PNG_EXPORTA(ordinal, type, name, args, PNG_EMPTY)


#ifndef PNG_REMOVED
#  define PNG_REMOVED(ordinal, type, name, args, attributes)
#endif

#ifndef PNG_CALLBACK
#  define PNG_CALLBACK(type, name, args) type (PNGCBAPI name) PNGARG(args)
#endif








#ifndef PNG_NO_PEDANTIC_WARNINGS
#  ifndef PNG_PEDANTIC_WARNINGS_SUPPORTED
#    define PNG_PEDANTIC_WARNINGS_SUPPORTED
#  endif
#endif

#ifdef PNG_PEDANTIC_WARNINGS_SUPPORTED
  




#  if defined(__GNUC__)
#    ifndef PNG_USE_RESULT
#      define PNG_USE_RESULT __attribute__((__warn_unused_result__))
#    endif
#    ifndef PNG_NORETURN
#      define PNG_NORETURN   __attribute__((__noreturn__))
#    endif
#    if __GNUC__ >= 3
#      ifndef PNG_ALLOCATED
#        define PNG_ALLOCATED  __attribute__((__malloc__))
#      endif
#      ifndef PNG_DEPRECATED
#        define PNG_DEPRECATED __attribute__((__deprecated__))
#      endif
#      ifndef PNG_PRIVATE
#        if 0 
#          define PNG_PRIVATE \
            __attribute__((warning("This function is not exported by libpng.")))
#        else
#          define PNG_PRIVATE \
            __attribute__((__deprecated__))
#        endif
#      endif
#    endif 
#  endif 

#  if defined(_MSC_VER)  && (_MSC_VER >= 1300)
#    ifndef PNG_USE_RESULT
#      define PNG_USE_RESULT
#    endif
#    ifndef PNG_NORETURN
#      define PNG_NORETURN __declspec(noreturn)
#    endif
#    ifndef PNG_ALLOCATED
#      if (_MSC_VER >= 1400)
#        define PNG_ALLOCATED __declspec(restrict)
#      endif
#    endif
#    ifndef PNG_DEPRECATED
#      define PNG_DEPRECATED __declspec(deprecated)
#    endif
#    ifndef PNG_PRIVATE
#      define PNG_PRIVATE __declspec(deprecated)
#    endif
#  endif 
#endif 

#ifndef PNG_DEPRECATED
#  define PNG_DEPRECATED
#endif
#ifndef PNG_USE_RESULT
#  define PNG_USE_RESULT
#endif
#ifndef PNG_NORETURN
#  define PNG_NORETURN
#endif
#ifndef PNG_ALLOCATED
#  define PNG_ALLOCATED
#endif
#ifndef PNG_PRIVATE
#  define PNG_PRIVATE
#endif
#ifndef PNG_FP_EXPORT     
#  ifdef PNG_FLOATING_POINT_SUPPORTED
#     define PNG_FP_EXPORT(ordinal, type, name, args)\
         PNG_EXPORT(ordinal, type, name, args);
#  else                   
#     define PNG_FP_EXPORT(ordinal, type, name, args)
#  endif
#endif
#ifndef PNG_FIXED_EXPORT  
#  ifdef PNG_FIXED_POINT_SUPPORTED
#     define PNG_FIXED_EXPORT(ordinal, type, name, args)\
         PNG_EXPORT(ordinal, type, name, args);
#  else                   
#     define PNG_FIXED_EXPORT(ordinal, type, name, args)
#  endif
#endif








#ifndef PNG_CONST
#  ifndef PNG_NO_CONST
#    define PNG_CONST const
#  else
#    define PNG_CONST
#  endif
#endif









#if defined(INT_MAX) && (INT_MAX > 0x7ffffffeL)
typedef unsigned int png_uint_32;
typedef int png_int_32;
#else
typedef unsigned long png_uint_32;
typedef long png_int_32;
#endif
typedef unsigned short png_uint_16;
typedef short png_int_16;
typedef unsigned char png_byte;

#ifdef PNG_NO_SIZE_T
typedef unsigned int png_size_t;
#else
typedef size_t png_size_t;
#endif
#define png_sizeof(x) (sizeof (x))













#ifdef __BORLANDC__
#  if defined(__LARGE__) || defined(__HUGE__) || defined(__COMPACT__)
#    define LDATA 1
#  else
#    define LDATA 0
#  endif
  
#  if !defined(__WIN32__) && !defined(__FLAT__) && !defined(__CYGWIN__)
#    define PNG_MAX_MALLOC_64K
#    if (LDATA != 1)
#      ifndef FAR
#        define FAR __far
#      endif
#      define USE_FAR_KEYWORD
#    endif   
         




#  endif  
#endif   








#ifdef FAR
#  ifdef M_I86MM
#    define USE_FAR_KEYWORD
#    define FARDATA FAR
#    include <dos.h>
#  endif
#endif


#ifndef FAR
#  define FAR
#endif


#ifndef FARDATA
#  define FARDATA
#endif




typedef png_int_32 png_fixed_point;


typedef void                      FAR * png_voidp;
typedef PNG_CONST void            FAR * png_const_voidp;
typedef png_byte                  FAR * png_bytep;
typedef PNG_CONST png_byte        FAR * png_const_bytep;
typedef png_uint_32               FAR * png_uint_32p;
typedef PNG_CONST png_uint_32     FAR * png_const_uint_32p;
typedef png_int_32                FAR * png_int_32p;
typedef PNG_CONST png_int_32      FAR * png_const_int_32p;
typedef png_uint_16               FAR * png_uint_16p;
typedef PNG_CONST png_uint_16     FAR * png_const_uint_16p;
typedef png_int_16                FAR * png_int_16p;
typedef PNG_CONST png_int_16      FAR * png_const_int_16p;
typedef char                      FAR * png_charp;
typedef PNG_CONST char            FAR * png_const_charp;
typedef png_fixed_point           FAR * png_fixed_point_p;
typedef PNG_CONST png_fixed_point FAR * png_const_fixed_point_p;
typedef png_size_t                FAR * png_size_tp;
typedef PNG_CONST png_size_t      FAR * png_const_size_tp;

#ifdef PNG_STDIO_SUPPORTED
typedef FILE            * png_FILE_p;
#endif

#ifdef PNG_FLOATING_POINT_SUPPORTED
typedef double           FAR * png_doublep;
typedef PNG_CONST double FAR * png_const_doublep;
#endif


typedef png_byte        FAR * FAR * png_bytepp;
typedef png_uint_32     FAR * FAR * png_uint_32pp;
typedef png_int_32      FAR * FAR * png_int_32pp;
typedef png_uint_16     FAR * FAR * png_uint_16pp;
typedef png_int_16      FAR * FAR * png_int_16pp;
typedef PNG_CONST char  FAR * FAR * png_const_charpp;
typedef char            FAR * FAR * png_charpp;
typedef png_fixed_point FAR * FAR * png_fixed_point_pp;
#ifdef PNG_FLOATING_POINT_SUPPORTED
typedef double          FAR * FAR * png_doublepp;
#endif


typedef char            FAR * FAR * FAR * png_charppp;










#if defined(__TURBOC__) && !defined(__FLAT__)
   typedef unsigned long png_alloc_size_t;
#else
#  if defined(_MSC_VER) && defined(MAXSEG_64K)
     typedef unsigned long    png_alloc_size_t;
#  else
     



#    if (defined(_Windows) || defined(_WINDOWS) || defined(_WINDOWS_)) && \
        (!defined(INT_MAX) || INT_MAX <= 0x7ffffffeL)
       typedef DWORD         png_alloc_size_t;
#    else
       typedef png_size_t    png_alloc_size_t;
#    endif
#  endif
#endif

#endif 
