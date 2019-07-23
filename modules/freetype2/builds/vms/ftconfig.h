

















  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  


#ifndef __FTCONFIG_H__
#define __FTCONFIG_H__



#include <ft2build.h>
#include FT_CONFIG_OPTIONS_H
#include FT_CONFIG_STANDARD_LIBRARY_H


FT_BEGIN_HEADER

  
  
  
  
  
  
  
  
  
  


#define HAVE_UNISTD_H  1
#define HAVE_FCNTL_H   1

#define SIZEOF_INT   4
#define SIZEOF_LONG  4

#define FT_SIZEOF_INT   4
#define FT_SIZEOF_LONG  4

#define FT_CHAR_BIT  8


  
#define FT_ALIGNMENT  8


  
  
#ifndef FT_UNUSED
#define FT_UNUSED( arg )  ( (arg) = (arg) )
#endif


  
  
  
  
  
  
  
  
  


  
  
  
  
  
  
  
#if ( defined( __APPLE__ ) && !defined( DARWIN_NO_CARBON ) ) || \
    ( defined( __MWERKS__ ) && defined( macintosh )        )
  
#include "AvailabilityMacros.h"
#if defined( __LP64__ ) && \
    ( MAC_OS_X_VERSION_MIN_REQUIRED <= MAC_OS_X_VERSION_10_4 )
#define DARWIN_NO_CARBON 1
#else
#define FT_MACINTOSH 1
#endif
#endif


  
  
  
  
  
  
  typedef signed short    FT_Int16;
  typedef unsigned short  FT_UInt16;

#if FT_SIZEOF_INT == 4

  typedef signed int      FT_Int32;
  typedef unsigned int    FT_UInt32;

#elif FT_SIZEOF_LONG == 4

  typedef signed long     FT_Int32;
  typedef unsigned long   FT_UInt32;

#else
#error "no 32bit type found -- please check your configuration files"
#endif

  
#if FT_SIZEOF_INT >= 4

  typedef int            FT_Fast;
  typedef unsigned int   FT_UFast;

#elif FT_SIZEOF_LONG >= 4

  typedef long           FT_Fast;
  typedef unsigned long  FT_UFast;

#endif


  
  
#if FT_SIZEOF_LONG == 8

  
#define FT_LONG64
#define FT_INT64  long

#elif defined( _MSC_VER ) && _MSC_VER >= 900  

  
#define FT_LONG64
#define FT_INT64  __int64

#elif defined( __BORLANDC__ )  

  
  

  
#define FT_LONG64
#define FT_INT64  __int64

#elif defined( __WATCOMC__ )   

  

#elif defined( __MWERKS__ )    

#define FT_LONG64
#define FT_INT64  long long int

#elif defined( __GNUC__ )

  
#define FT_LONG64
#define FT_INT64  long long int

#endif 


#define FT_BEGIN_STMNT  do {
#define FT_END_STMNT    } while ( 0 )
#define FT_DUMMY_STMNT  FT_BEGIN_STMNT FT_END_STMNT


  
  
  
  
  
  
  
#if defined( FT_LONG64 ) && !defined( FT_CONFIG_OPTION_FORCE_INT64 )

#ifdef __STDC__

  
#undef FT_LONG64
#undef FT_INT64

#endif 

#endif 


#ifdef FT_MAKE_OPTION_SINGLE_OBJECT

#define FT_LOCAL( x )      static  x
#define FT_LOCAL_DEF( x )  static  x

#else

#ifdef __cplusplus
#define FT_LOCAL( x )      extern "C"  x
#define FT_LOCAL_DEF( x )  extern "C"  x
#else
#define FT_LOCAL( x )      extern  x
#define FT_LOCAL_DEF( x )  x
#endif

#endif 


#ifndef FT_BASE

#ifdef __cplusplus
#define FT_BASE( x )  extern "C"  x
#else
#define FT_BASE( x )  extern  x
#endif

#endif 


#ifndef FT_BASE_DEF

#ifdef __cplusplus
#define FT_BASE_DEF( x )  extern "C"  x
#else
#define FT_BASE_DEF( x )  extern  x
#endif

#endif 


#ifndef FT_EXPORT

#ifdef __cplusplus
#define FT_EXPORT( x )  extern "C"  x
#else
#define FT_EXPORT( x )  extern  x
#endif

#endif 


#ifndef FT_EXPORT_DEF

#ifdef __cplusplus
#define FT_EXPORT_DEF( x )  extern "C"  x
#else
#define FT_EXPORT_DEF( x )  extern  x
#endif

#endif 


#ifndef FT_EXPORT_VAR

#ifdef __cplusplus
#define FT_EXPORT_VAR( x )  extern "C"  x
#else
#define FT_EXPORT_VAR( x )  extern  x
#endif

#endif 

  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
#ifndef FT_CALLBACK_DEF
#ifdef __cplusplus
#define FT_CALLBACK_DEF( x )  extern "C"  x
#else
#define FT_CALLBACK_DEF( x )  static  x
#endif
#endif 

#ifndef FT_CALLBACK_TABLE
#ifdef __cplusplus
#define FT_CALLBACK_TABLE      extern "C"
#define FT_CALLBACK_TABLE_DEF  extern "C"
#else
#define FT_CALLBACK_TABLE      extern
#define FT_CALLBACK_TABLE_DEF
#endif
#endif 


FT_END_HEADER


#endif 



