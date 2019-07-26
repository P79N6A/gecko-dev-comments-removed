

















  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

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

#elif defined( _MSC_VER ) && _MSC_VER >= 900  

  
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


  
  
  
  
  
  
  
#if defined( FT_LONG64 ) && !defined( FT_CONFIG_OPTION_FORCE_INT64 )

#ifdef __STDC__

  
#undef FT_LONG64
#undef FT_INT64

#endif 

#endif 

#ifdef FT_LONG64
  typedef FT_INT64   FT_Int64;
  typedef FT_UINT64  FT_UInt64;
#endif


#define FT_BEGIN_STMNT  do {
#define FT_END_STMNT    } while ( 0 )
#define FT_DUMMY_STMNT  FT_BEGIN_STMNT FT_END_STMNT


#ifndef  FT_CONFIG_OPTION_NO_ASSEMBLER
  
  

#if defined( __CC_ARM ) || defined( __ARMCC__ )  

#define FT_MULFIX_ASSEMBLER  FT_MulFix_arm

  

  static __inline FT_Int32
  FT_MulFix_arm( FT_Int32  a,
                 FT_Int32  b )
  {
    register FT_Int32  t, t2;


    __asm
    {
      smull t2, t,  b,  a           
      mov   a,  t,  asr #31         
      add   a,  a,  #0x8000         
      adds  t2, t2, a               
      adc   t,  t,  #0              
      mov   a,  t2, lsr #16         
      orr   a,  a,  t,  lsl #16     
    }
    return a;
  }

#endif 


#ifdef __GNUC__

#if defined( __arm__ )                                 && \
    ( !defined( __thumb__ ) || defined( __thumb2__ ) ) && \
    !( defined( __CC_ARM ) || defined( __ARMCC__ ) )

#define FT_MULFIX_ASSEMBLER  FT_MulFix_arm

  

  static __inline__ FT_Int32
  FT_MulFix_arm( FT_Int32  a,
                 FT_Int32  b )
  {
    register FT_Int32  t, t2;


    __asm__ __volatile__ (
      "smull  %1, %2, %4, %3\n\t"       
      "mov    %0, %2, asr #31\n\t"      
      "add    %0, %0, #0x8000\n\t"      
      "adds   %1, %1, %0\n\t"           
      "adc    %2, %2, #0\n\t"           
      "mov    %0, %1, lsr #16\n\t"      
      "orr    %0, %0, %2, lsl #16\n\t"  
      : "=r"(a), "=&r"(t2), "=&r"(t)
      : "r"(a), "r"(b)
      : "cc" );
    return a;
  }

#endif 
       
       


#if defined( __i386__ )

#define FT_MULFIX_ASSEMBLER  FT_MulFix_i386

  

  static __inline__ FT_Int32
  FT_MulFix_i386( FT_Int32  a,
                  FT_Int32  b )
  {
    register FT_Int32  result;


    __asm__ __volatile__ (
      "imul  %%edx\n"
      "movl  %%edx, %%ecx\n"
      "sarl  $31, %%ecx\n"
      "addl  $0x8000, %%ecx\n"
      "addl  %%ecx, %%eax\n"
      "adcl  $0, %%edx\n"
      "shrl  $16, %%eax\n"
      "shll  $16, %%edx\n"
      "addl  %%edx, %%eax\n"
      : "=a"(result), "=d"(b)
      : "a"(a), "d"(b)
      : "%ecx", "cc" );
    return result;
  }

#endif 

#endif 


#ifdef _MSC_VER 

#ifdef _M_IX86

#define FT_MULFIX_ASSEMBLER  FT_MulFix_i386

  

  static __inline FT_Int32
  FT_MulFix_i386( FT_Int32  a,
                  FT_Int32  b )
  {
    register FT_Int32  result;

    __asm
    {
      mov eax, a
      mov edx, b
      imul edx
      mov ecx, edx
      sar ecx, 31
      add ecx, 8000h
      add eax, ecx
      adc edx, 0
      shr eax, 16
      shl edx, 16
      add eax, edx
      mov result, eax
    }
    return result;
  }

#endif 

#endif 


#if defined( __GNUC__ ) && defined( __x86_64__ )

#define FT_MULFIX_ASSEMBLER  FT_MulFix_x86_64

  static __inline__ FT_Int32
  FT_MulFix_x86_64( FT_Int32  a,
                    FT_Int32  b )
  {
    
    
#if ( __GNUC__ > 4 ) || ( ( __GNUC__ == 4 ) && ( __GNUC_MINOR__ >= 6 ) )
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wlong-long"
#endif

#if 1
    
    
    long long  ret, tmp;


    ret  = (long long)a * b;
    tmp  = ret >> 63;
    ret += 0x8000 + tmp;

    return (FT_Int32)( ret >> 16 );
#else

    
    
    
    
    
    
    long long  wide_a = (long long)a;
    long long  wide_b = (long long)b;
    long long  result;


    __asm__ __volatile__ (
      "imul %2, %1\n"
      "mov %1, %0\n"
      "sar $63, %0\n"
      "lea 0x8000(%1, %0), %0\n"
      "sar $16, %0\n"
      : "=&r"(result), "=&r"(wide_a)
      : "r"(wide_b)
      : "cc" );

    return (FT_Int32)result;
#endif

#if ( __GNUC__ > 4 ) || ( ( __GNUC__ == 4 ) && ( __GNUC_MINOR__ >= 6 ) )
#pragma GCC diagnostic pop
#endif
  }

#endif 

#endif 


#ifdef FT_CONFIG_OPTION_INLINE_MULFIX
#ifdef FT_MULFIX_ASSEMBLER
#define FT_MULFIX_INLINED  FT_MULFIX_ASSEMBLER
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



