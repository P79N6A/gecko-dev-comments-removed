





































#ifndef __CF2FIXED_H__
#define __CF2FIXED_H__


FT_BEGIN_HEADER


  

#define   CF2_Fixed  CF2_F16Dot16
  typedef FT_Int32   CF2_Frac;   


#define CF2_FIXED_MAX      ( (CF2_Fixed)0x7FFFFFFFL )
#define CF2_FIXED_MIN      ( (CF2_Fixed)0x80000000L )
#define CF2_FIXED_ONE      0x10000L
#define CF2_FIXED_EPSILON  0x0001

  
  

#define cf2_intToFixed( i )                                    \
          ( (CF2_Fixed)( (FT_UInt32)(i) << 16 ) )
#define cf2_fixedToInt( x )                                    \
          ( (FT_Short)( ( (FT_UInt32)(x) + 0x8000U ) >> 16 ) )
#define cf2_fixedRound( x )                                    \
          ( (CF2_Fixed)( ( (x) + 0x8000 ) & 0xFFFF0000L ) )
#define cf2_floatToFixed( f )                                  \
          ( (CF2_Fixed)( (f) * 65536.0 + 0.5 ) )
#define cf2_fixedAbs( x )                                      \
          ( (x) < 0 ? -(x) : (x) )
#define cf2_fixedFloor( x )                                    \
          ( (CF2_Fixed)( (x) & 0xFFFF0000L ) )
#define cf2_fixedFraction( x )                                 \
          ( (x) - cf2_fixedFloor( x ) )
#define cf2_fracToFixed( x )                                   \
          ( (x) < 0 ? -( ( -(x) + 0x2000 ) >> 14 )             \
                    :  ( (  (x) + 0x2000 ) >> 14 ) )


  
  typedef enum  CF2_NumberType_
  {
    CF2_NumberFixed,    
    CF2_NumberFrac,     
    CF2_NumberInt       

  } CF2_NumberType;


FT_END_HEADER


#endif 



