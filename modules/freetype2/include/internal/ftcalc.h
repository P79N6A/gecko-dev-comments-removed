

















#ifndef __FTCALC_H__
#define __FTCALC_H__


#include <ft2build.h>
#include FT_FREETYPE_H


FT_BEGIN_HEADER


  
  
  
  
  

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
#if defined( __clang__ ) && defined( __thumb2__ )
      "add.w  %0, %0, #0x8000\n\t"      
#else
      "add    %0, %0, #0x8000\n\t"      
#endif
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
    
    
#if __GNUC__ > 4 || ( __GNUC__ == 4 && __GNUC_MINOR__ >= 6 )
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

#if __GNUC__ > 4 || ( __GNUC__ == 4 && __GNUC_MINOR__ >= 6 )
#pragma GCC diagnostic pop
#endif
  }

#endif 

#endif


#ifdef FT_CONFIG_OPTION_INLINE_MULFIX
#ifdef FT_MULFIX_ASSEMBLER
#define FT_MulFix( a, b )  FT_MULFIX_ASSEMBLER( (FT_Int32)(a), (FT_Int32)(b) )
#endif
#endif


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_BASE( FT_Long )
  FT_MulDiv_No_Round( FT_Long  a,
                      FT_Long  b,
                      FT_Long  c );


  







  FT_BASE( void )
  FT_Matrix_Multiply_Scaled( const FT_Matrix*  a,
                             FT_Matrix        *b,
                             FT_Long           scaling );


  



  FT_BASE( void )
  FT_Vector_Transform_Scaled( FT_Vector*        vector,
                              const FT_Matrix*  matrix,
                              FT_Long           scaling );


  





  FT_BASE( FT_Int )
  ft_corner_orientation( FT_Pos  in_x,
                         FT_Pos  in_y,
                         FT_Pos  out_x,
                         FT_Pos  out_y );


  




  FT_BASE( FT_Int )
  ft_corner_is_flat( FT_Pos  in_x,
                     FT_Pos  in_y,
                     FT_Pos  out_x,
                     FT_Pos  out_y );


  



#ifndef  FT_CONFIG_OPTION_NO_ASSEMBLER
#if defined( __GNUC__ )                                          && \
    ( __GNUC__ > 3 || ( __GNUC__ == 3 && __GNUC_MINOR__ >= 4 ) )

#if FT_SIZEOF_INT == 4

#define FT_MSB( x )  ( 31 - __builtin_clz( x ) )

#elif FT_SIZEOF_LONG == 4

#define FT_MSB( x )  ( 31 - __builtin_clzl( x ) )

#endif

#endif 
#endif 

#ifndef FT_MSB

  FT_BASE( FT_Int )
  FT_MSB( FT_UInt32  z );

#endif


  



  FT_BASE( FT_Fixed )
  FT_Hypot( FT_Fixed  x,
            FT_Fixed  y );


#if 0

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  FT_BASE( FT_Int32 )
  FT_SqrtFixed( FT_Int32  x );

#endif 


#define INT_TO_F26DOT6( x )    ( (FT_Long)(x) << 6  )
#define INT_TO_F2DOT14( x )    ( (FT_Long)(x) << 14 )
#define INT_TO_FIXED( x )      ( (FT_Long)(x) << 16 )
#define F2DOT14_TO_FIXED( x )  ( (FT_Long)(x) << 2  )
#define FLOAT_TO_FIXED( x )    ( (FT_Long)( x * 65536.0 ) )
#define FIXED_TO_INT( x )      ( FT_RoundFix( x ) >> 16 )

#define ROUND_F26DOT6( x )     ( x >= 0 ? (    ( (x) + 32 ) & -64 )     \
                                        : ( -( ( 32 - (x) ) & -64 ) ) )


FT_END_HEADER

#endif



