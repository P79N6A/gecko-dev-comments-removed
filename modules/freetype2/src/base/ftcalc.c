
















  
  
  
  
  
  

  
  
  
  
  
  
  
  


#include <ft2build.h>
#include FT_GLYPH_H
#include FT_TRIGONOMETRY_H
#include FT_INTERNAL_CALC_H
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_OBJECTS_H


#ifdef FT_MULFIX_ASSEMBLER
#undef FT_MulFix
#endif



#ifndef FT_LONG64

  typedef struct  FT_Int64_
  {
    FT_UInt32  lo;
    FT_UInt32  hi;

  } FT_Int64;

#endif 


  
  
  
  
  
  
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_calc


  
#define FT_MOVE_SIGN( x, s ) \
  FT_BEGIN_STMNT             \
    if ( x < 0 )             \
    {                        \
      x = -x;                \
      s = -s;                \
    }                        \
  FT_END_STMNT

  
  

  

  FT_EXPORT_DEF( FT_Fixed )
  FT_RoundFix( FT_Fixed  a )
  {
    return a >= 0 ?   ( a + 0x8000L ) & ~0xFFFFL
                  : -((-a + 0x8000L ) & ~0xFFFFL );
  }


  

  FT_EXPORT_DEF( FT_Fixed )
  FT_CeilFix( FT_Fixed  a )
  {
    return a >= 0 ?   ( a + 0xFFFFL ) & ~0xFFFFL
                  : -((-a + 0xFFFFL ) & ~0xFFFFL );
  }


  

  FT_EXPORT_DEF( FT_Fixed )
  FT_FloorFix( FT_Fixed  a )
  {
    return a >= 0 ?   a & ~0xFFFFL
                  : -((-a) & ~0xFFFFL );
  }

#ifndef FT_MSB

  FT_BASE_DEF ( FT_Int )
  FT_MSB( FT_UInt32 z )
  {
    FT_Int  shift = 0;


    
    if ( z & 0xFFFF0000UL )
    {
      z     >>= 16;
      shift  += 16;
    }
    if ( z & 0x0000FF00UL )
    {
      z     >>= 8;
      shift  += 8;
    }
    if ( z & 0x000000F0UL )
    {
      z     >>= 4;
      shift  += 4;
    }
    if ( z & 0x0000000CUL )
    {
      z     >>= 2;
      shift  += 2;
    }
    if ( z & 0x00000002UL )
    {
   
      shift  += 1;
    }

    return shift;
  }

#endif 


  

  FT_BASE_DEF( FT_Fixed )
  FT_Hypot( FT_Fixed  x,
            FT_Fixed  y )
  {
    FT_Vector  v;


    v.x = x;
    v.y = y;

    return FT_Vector_Length( &v );
  }


#ifdef FT_LONG64


  

  FT_EXPORT_DEF( FT_Long )
  FT_MulDiv( FT_Long  a,
             FT_Long  b,
             FT_Long  c )
  {
    FT_Int   s = 1;
    FT_Long  d;


    FT_MOVE_SIGN( a, s );
    FT_MOVE_SIGN( b, s );
    FT_MOVE_SIGN( c, s );

    d = (FT_Long)( c > 0 ? ( (FT_Int64)a * b + ( c >> 1 ) ) / c
                         : 0x7FFFFFFFL );

    return s < 0 ? -d : d;
  }


  

  FT_BASE_DEF( FT_Long )
  FT_MulDiv_No_Round( FT_Long  a,
                      FT_Long  b,
                      FT_Long  c )
  {
    FT_Int   s = 1;
    FT_Long  d;


    FT_MOVE_SIGN( a, s );
    FT_MOVE_SIGN( b, s );
    FT_MOVE_SIGN( c, s );

    d = (FT_Long)( c > 0 ? (FT_Int64)a * b / c
                         : 0x7FFFFFFFL );

    return s < 0 ? -d : d;
  }


  

  FT_EXPORT_DEF( FT_Long )
  FT_MulFix( FT_Long  a,
             FT_Long  b )
  {
#ifdef FT_MULFIX_ASSEMBLER

    return FT_MULFIX_ASSEMBLER( a, b );

#else

    FT_Int   s = 1;
    FT_Long  c;


    FT_MOVE_SIGN( a, s );
    FT_MOVE_SIGN( b, s );

    c = (FT_Long)( ( (FT_Int64)a * b + 0x8000L ) >> 16 );

    return s < 0 ? -c : c;

#endif 
  }


  

  FT_EXPORT_DEF( FT_Long )
  FT_DivFix( FT_Long  a,
             FT_Long  b )
  {
    FT_Int   s = 1;
    FT_Long  q;


    FT_MOVE_SIGN( a, s );
    FT_MOVE_SIGN( b, s );

    q = (FT_Long)( b > 0 ? ( ( (FT_UInt64)a << 16 ) + ( b >> 1 ) ) / b
                         : 0x7FFFFFFFL );

    return s < 0 ? -q : q;
  }


#else 


  static void
  ft_multo64( FT_UInt32  x,
              FT_UInt32  y,
              FT_Int64  *z )
  {
    FT_UInt32  lo1, hi1, lo2, hi2, lo, hi, i1, i2;


    lo1 = x & 0x0000FFFFU;  hi1 = x >> 16;
    lo2 = y & 0x0000FFFFU;  hi2 = y >> 16;

    lo = lo1 * lo2;
    i1 = lo1 * hi2;
    i2 = lo2 * hi1;
    hi = hi1 * hi2;

    
    i1 += i2;
    hi += (FT_UInt32)( i1 < i2 ) << 16;

    hi += i1 >> 16;
    i1  = i1 << 16;

    
    lo += i1;
    hi += ( lo < i1 );

    z->lo = lo;
    z->hi = hi;
  }


  static FT_UInt32
  ft_div64by32( FT_UInt32  hi,
                FT_UInt32  lo,
                FT_UInt32  y )
  {
    FT_UInt32  r, q;
    FT_Int     i;


    if ( hi >= y )
      return (FT_UInt32)0x7FFFFFFFL;

    
    
    
    

    i = 31 - FT_MSB( hi );
    r = ( hi << i ) | ( lo >> ( 32 - i ) ); lo <<= i; 
    q = r / y;
    r -= q * y;   

    i = 32 - i;   
    do
    {
      q <<= 1;
      r   = ( r << 1 ) | ( lo >> 31 ); lo <<= 1;

      if ( r >= y )
      {
        r -= y;
        q |= 1;
      }
    } while ( --i );

    return q;
  }


  static void
  FT_Add64( FT_Int64*  x,
            FT_Int64*  y,
            FT_Int64  *z )
  {
    FT_UInt32  lo, hi;


    lo = x->lo + y->lo;
    hi = x->hi + y->hi + ( lo < x->lo );

    z->lo = lo;
    z->hi = hi;
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  

  FT_EXPORT_DEF( FT_Long )
  FT_MulDiv( FT_Long  a,
             FT_Long  b,
             FT_Long  c )
  {
    FT_Int  s = 1;


    
    if ( a == 0 || b == c )
      return a;

    FT_MOVE_SIGN( a, s );
    FT_MOVE_SIGN( b, s );
    FT_MOVE_SIGN( c, s );

    if ( c == 0 )
      a = 0x7FFFFFFFL;

    else if ( (FT_ULong)a + b <= 129894UL - ( c >> 17 ) )
      a = ( (FT_ULong)a * b + ( c >> 1 ) ) / c;

    else
    {
      FT_Int64  temp, temp2;


      ft_multo64( a, b, &temp );

      temp2.hi = 0;
      temp2.lo = c >> 1;

      FT_Add64( &temp, &temp2, &temp );

      
      a = temp.hi == 0 ? temp.lo / c
                       : ft_div64by32( temp.hi, temp.lo, c );
    }

    return s < 0 ? -a : a;
  }


  FT_BASE_DEF( FT_Long )
  FT_MulDiv_No_Round( FT_Long  a,
                      FT_Long  b,
                      FT_Long  c )
  {
    FT_Int  s = 1;


    if ( a == 0 || b == c )
      return a;

    FT_MOVE_SIGN( a, s );
    FT_MOVE_SIGN( b, s );
    FT_MOVE_SIGN( c, s );

    if ( c == 0 )
      a = 0x7FFFFFFFL;

    else if ( (FT_ULong)a + b <= 131071UL )
      a = (FT_ULong)a * b / c;

    else
    {
      FT_Int64  temp;


      ft_multo64( a, b, &temp );

      
      a = temp.hi == 0 ? temp.lo / c
                       : ft_div64by32( temp.hi, temp.lo, c );
    }

    return s < 0 ? -a : a;
  }


  

  FT_EXPORT_DEF( FT_Long )
  FT_MulFix( FT_Long  a,
             FT_Long  b )
  {
#ifdef FT_MULFIX_ASSEMBLER

    return FT_MULFIX_ASSEMBLER( a, b );

#elif 0

    






    FT_Long   sa, sb;
    FT_ULong  ua, ub;


    if ( a == 0 || b == 0x10000L )
      return a;

    

















    sa = ( a >> ( sizeof ( a ) * 8 - 1 ) );
    a  = ( a ^ sa ) - sa;
    sb = ( b >> ( sizeof ( b ) * 8 - 1 ) );
    b  = ( b ^ sb ) - sb;

    ua = (FT_ULong)a;
    ub = (FT_ULong)b;

    if ( ua + ( ub >> 8 ) <= 8190UL )
      ua = ( ua * ub + 0x8000U ) >> 16;
    else
    {
      FT_ULong  al = ua & 0xFFFFU;


      ua = ( ua >> 16 ) * ub +  al * ( ub >> 16 ) +
           ( ( al * ( ub & 0xFFFFU ) + 0x8000U ) >> 16 );
    }

    sa ^= sb,
    ua  = (FT_ULong)(( ua ^ sa ) - sa);

    return (FT_Long)ua;

#else 

    FT_Int    s = 1;
    FT_ULong  ua, ub;


    if ( a == 0 || b == 0x10000L )
      return a;

    FT_MOVE_SIGN( a, s );
    FT_MOVE_SIGN( b, s );

    ua = (FT_ULong)a;
    ub = (FT_ULong)b;

    if ( ua + ( ub >> 8 ) <= 8190UL )
      ua = ( ua * ub + 0x8000UL ) >> 16;
    else
    {
      FT_ULong  al = ua & 0xFFFFUL;


      ua = ( ua >> 16 ) * ub +  al * ( ub >> 16 ) +
           ( ( al * ( ub & 0xFFFFUL ) + 0x8000UL ) >> 16 );
    }

    return s < 0 ? -(FT_Long)ua : (FT_Long)ua;

#endif 

  }


  

  FT_EXPORT_DEF( FT_Long )
  FT_DivFix( FT_Long  a,
             FT_Long  b )
  {
    FT_Int   s = 1;
    FT_Long  q;


    

    FT_MOVE_SIGN( a, s );
    FT_MOVE_SIGN( b, s );

    if ( b == 0 )
    {
      
      q = 0x7FFFFFFFL;
    }
    else if ( a <= 65535L - ( b >> 17 ) )
    {
      
      q = (FT_Long)( ( ( (FT_ULong)a << 16 ) + ( b >> 1 ) ) / b );
    }
    else
    {
      
      FT_Int64  temp, temp2;


      temp.hi  = a >> 16;
      temp.lo  = a << 16;
      temp2.hi = 0;
      temp2.lo = b >> 1;

      FT_Add64( &temp, &temp2, &temp );
      q = (FT_Long)ft_div64by32( temp.hi, temp.lo, b );
    }

    return s < 0 ? -q : q;
  }


#endif 


  

  FT_EXPORT_DEF( void )
  FT_Matrix_Multiply( const FT_Matrix*  a,
                      FT_Matrix        *b )
  {
    FT_Fixed  xx, xy, yx, yy;


    if ( !a || !b )
      return;

    xx = FT_MulFix( a->xx, b->xx ) + FT_MulFix( a->xy, b->yx );
    xy = FT_MulFix( a->xx, b->xy ) + FT_MulFix( a->xy, b->yy );
    yx = FT_MulFix( a->yx, b->xx ) + FT_MulFix( a->yy, b->yx );
    yy = FT_MulFix( a->yx, b->xy ) + FT_MulFix( a->yy, b->yy );

    b->xx = xx;  b->xy = xy;
    b->yx = yx;  b->yy = yy;
  }


  

  FT_EXPORT_DEF( FT_Error )
  FT_Matrix_Invert( FT_Matrix*  matrix )
  {
    FT_Pos  delta, xx, yy;


    if ( !matrix )
      return FT_THROW( Invalid_Argument );

    
    delta = FT_MulFix( matrix->xx, matrix->yy ) -
            FT_MulFix( matrix->xy, matrix->yx );

    if ( !delta )
      return FT_THROW( Invalid_Argument );  

    matrix->xy = - FT_DivFix( matrix->xy, delta );
    matrix->yx = - FT_DivFix( matrix->yx, delta );

    xx = matrix->xx;
    yy = matrix->yy;

    matrix->xx = FT_DivFix( yy, delta );
    matrix->yy = FT_DivFix( xx, delta );

    return FT_Err_Ok;
  }


  

  FT_BASE_DEF( void )
  FT_Matrix_Multiply_Scaled( const FT_Matrix*  a,
                             FT_Matrix        *b,
                             FT_Long           scaling )
  {
    FT_Fixed  xx, xy, yx, yy;

    FT_Long   val = 0x10000L * scaling;


    if ( !a || !b )
      return;

    xx = FT_MulDiv( a->xx, b->xx, val ) + FT_MulDiv( a->xy, b->yx, val );
    xy = FT_MulDiv( a->xx, b->xy, val ) + FT_MulDiv( a->xy, b->yy, val );
    yx = FT_MulDiv( a->yx, b->xx, val ) + FT_MulDiv( a->yy, b->yx, val );
    yy = FT_MulDiv( a->yx, b->xy, val ) + FT_MulDiv( a->yy, b->yy, val );

    b->xx = xx;  b->xy = xy;
    b->yx = yx;  b->yy = yy;
  }


  

  FT_BASE_DEF( void )
  FT_Vector_Transform_Scaled( FT_Vector*        vector,
                              const FT_Matrix*  matrix,
                              FT_Long           scaling )
  {
    FT_Pos   xz, yz;

    FT_Long  val = 0x10000L * scaling;


    if ( !vector || !matrix )
      return;

    xz = FT_MulDiv( vector->x, matrix->xx, val ) +
         FT_MulDiv( vector->y, matrix->xy, val );

    yz = FT_MulDiv( vector->x, matrix->yx, val ) +
         FT_MulDiv( vector->y, matrix->yy, val );

    vector->x = xz;
    vector->y = yz;
  }


#if 0

  

  FT_BASE_DEF( FT_Int32 )
  FT_SqrtFixed( FT_Int32  x )
  {
    FT_UInt32  root, rem_hi, rem_lo, test_div;
    FT_Int     count;


    root = 0;

    if ( x > 0 )
    {
      rem_hi = 0;
      rem_lo = x;
      count  = 24;
      do
      {
        rem_hi   = ( rem_hi << 2 ) | ( rem_lo >> 30 );
        rem_lo <<= 2;
        root   <<= 1;
        test_div = ( root << 1 ) + 1;

        if ( rem_hi >= test_div )
        {
          rem_hi -= test_div;
          root   += 1;
        }
      } while ( --count );
    }

    return (FT_Int32)root;
  }

#endif 


  

  FT_BASE_DEF( FT_Int )
  ft_corner_orientation( FT_Pos  in_x,
                         FT_Pos  in_y,
                         FT_Pos  out_x,
                         FT_Pos  out_y )
  {
    FT_Long  result; 


    
    if ( in_y == 0 )
    {
      if ( in_x >= 0 )
        result = out_y;
      else
        result = -out_y;
    }
    else if ( in_x == 0 )
    {
      if ( in_y >= 0 )
        result = -out_x;
      else
        result = out_x;
    }
    else if ( out_y == 0 )
    {
      if ( out_x >= 0 )
        result = in_y;
      else
        result = -in_y;
    }
    else if ( out_x == 0 )
    {
      if ( out_y >= 0 )
        result = -in_x;
      else
        result =  in_x;
    }
    else 
    {
#ifdef FT_LONG64

      FT_Int64  delta = (FT_Int64)in_x * out_y - (FT_Int64)in_y * out_x;


      if ( delta == 0 )
        result = 0;
      else
        result = 1 - 2 * ( delta < 0 );

#else

      FT_Int64  z1, z2;


      
      ft_multo64( (FT_Int32)in_x, (FT_Int32)out_y, &z1 );
      ft_multo64( (FT_Int32)in_y, (FT_Int32)out_x, &z2 );

      if ( z1.hi > z2.hi )
        result = +1;
      else if ( z1.hi < z2.hi )
        result = -1;
      else if ( z1.lo > z2.lo )
        result = +1;
      else if ( z1.lo < z2.lo )
        result = -1;
      else
        result = 0;

#endif
    }

    
    return (FT_Int)result;
  }


  

  FT_BASE_DEF( FT_Int )
  ft_corner_is_flat( FT_Pos  in_x,
                     FT_Pos  in_y,
                     FT_Pos  out_x,
                     FT_Pos  out_y )
  {
    FT_Pos  ax = in_x + out_x;
    FT_Pos  ay = in_y + out_y;

    FT_Pos  d_in, d_out, d_hypot;


    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    d_in    = FT_HYPOT(  in_x,  in_y );
    d_out   = FT_HYPOT( out_x, out_y );
    d_hypot = FT_HYPOT(    ax,    ay );

    
    
    

    return ( d_in + d_out - d_hypot ) < ( d_hypot >> 4 );
  }



