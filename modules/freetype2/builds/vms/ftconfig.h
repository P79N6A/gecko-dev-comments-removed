

















  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

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


  
  
#ifndef FT_UNUSED
#define FT_UNUSED( arg )  ( (arg) = (arg) )
#endif


  
  
  
  
  
  
  
  
  


  
  
  
  
  
  
  
#if defined( __APPLE__ ) || ( defined( __MWERKS__ ) && defined( macintosh ) )
  
  
  
#include <errno.h>
#ifdef ECANCELED 
#include "AvailabilityMacros.h"
#endif
#if defined( __LP64__ ) && \
    ( MAC_OS_X_VERSION_MIN_REQUIRED <= MAC_OS_X_VERSION_10_4 )
#undef FT_MACINTOSH
#endif

#elif defined( __SC__ ) || defined( __MRC__ )
  
#include "ConditionalMacros.h"
#if TARGET_OS_MAC
#define FT_MACINTOSH 1
#endif

#endif


  
  
  
  
  
  


  
  
  
  
  
  
  
  
  typedef signed short  FT_Int16;


  
  
  
  
  
  
  
  
  typedef unsigned short  FT_UInt16;

  


  
#if 0

  
  
  
  
  
  
  
  
  
  typedef signed XXX  FT_Int32;


  
  
  
  
  
  
  
  
  typedef unsigned XXX  FT_UInt32;


  
  
  
  
  
  
  
  
  
  typedef signed XXX  FT_Int64;


  
  
  
  
  
  
  
  
  
  typedef unsigned XXX  FT_UInt64;

  

#endif

#if FT_SIZEOF_INT == (32 / FT_CHAR_BIT)

  typedef signed int      FT_Int32;
  typedef unsigned int    FT_UInt32;

#elif FT_SIZEOF_LONG == (32 / FT_CHAR_BIT)

  typedef signed long     FT_Int32;
  typedef unsigned long   FT_UInt32;

#else
#error "no 32bit type found -- please check your configuration files"
#endif


  
#if FT_SIZEOF_INT >= (32 / FT_CHAR_BIT)

  typedef int            FT_Fast;
  typedef unsigned int   FT_UFast;

#elif FT_SIZEOF_LONG >= (32 / FT_CHAR_BIT)

  typedef long           FT_Fast;
  typedef unsigned long  FT_UFast;

#endif


  
  
#if FT_SIZEOF_LONG == (64 / FT_CHAR_BIT)

  
#define FT_LONG64
#define FT_INT64   long
#define FT_UINT64  unsigned long

  
  
  
  
  
  
  
#elif !defined( __STDC__ ) || defined( FT_CONFIG_OPTION_FORCE_INT64 )

#if defined( _MSC_VER ) && _MSC_VER >= 900  

  
#define FT_LONG64
#define FT_INT64   __int64
#define FT_UINT64  unsigned __int64

#elif defined( __BORLANDC__ )  

  
  

  
#define FT_LONG64
#define FT_INT64   __int64
#define FT_UINT64  unsigned __int64

#elif defined( __WATCOMC__ )   

  

#elif defined( __MWERKS__ )    

#define FT_LONG64
#define FT_INT64   long long int
#define FT_UINT64  unsigned long long int

#elif defined( __GNUC__ )

  
#define FT_LONG64
#define FT_INT64   long long int
#define FT_UINT64  unsigned long long int

#endif 

#endif 

#ifdef FT_LONG64
  typedef FT_INT64   FT_Int64;
  typedef FT_UINT64  FT_UInt64;
#endif


#define FT_BEGIN_STMNT  do {
#define FT_END_STMNT    } while ( 0 )
#define FT_DUMMY_STMNT  FT_BEGIN_STMNT FT_END_STMNT


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

#define FT_LOCAL_ARRAY( x )      extern const  x
#define FT_LOCAL_ARRAY_DEF( x )  const  x


#ifndef FT_BASE

#ifdef __cplusplus
#define FT_BASE( x )  extern "C"  x
#else
#define FT_BASE( x )  extern  x
#endif

#endif 


#ifndef FT_BASE_DEF

#ifdef __cplusplus
#define FT_BASE_DEF( x )  x
#else
#define FT_BASE_DEF( x )  x
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



