

















#include <ft2build.h>
#include FT_INTERNAL_POSTSCRIPT_AUX_H
#include FT_INTERNAL_DEBUG_H

#include "psconv.h"
#include "psobjs.h"
#include "psauxerr.h"


  
  

#if 'A' == 65
  

  static const FT_Char  ft_char_table[128] =
  {
    
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1,
    -1, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
    25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, -1, -1, -1, -1, -1,
    -1, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
    25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, -1, -1, -1, -1, -1,
  };

  
#define OP  >=

#endif 

#if 'A' == 193
  

  static const FT_Char  ft_char_table[128] =
  {
    
    -1, 10, 11, 12, 13, 14, 15, 16, 17, 18, -1, -1, -1, -1, -1, -1,
    -1, 19, 20, 21, 22, 23, 24, 25, 26, 27, -1, -1, -1, -1, -1, -1,
    -1, -1, 28, 29, 30, 31, 32, 33, 34, 35, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, 10, 11, 12, 13, 14, 15, 16, 17, 18, -1, -1, -1, -1, -1, -1,
    -1, 19, 20, 21, 22, 23, 24, 25, 26, 27, -1, -1, -1, -1, -1, -1,
    -1, -1, 28, 29, 30, 31, 32, 33, 34, 35, -1, -1, -1, -1, -1, -1,
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1,
  };

  
#define OP  <

#endif 


  FT_LOCAL_DEF( FT_Int )
  PS_Conv_Strtol( FT_Byte**  cursor,
                  FT_Byte*   limit,
                  FT_Int     base )
  {
    FT_Byte*  p = *cursor;
    FT_Int    num = 0;
    FT_Bool   sign = 0;


    if ( p == limit || base < 2 || base > 36 )
      return 0;

    if ( *p == '-' || *p == '+' )
    {
      sign = FT_BOOL( *p == '-' );

      p++;
      if ( p == limit )
        return 0;
    }

    for ( ; p < limit; p++ )
    {
      FT_Char  c;


      if ( IS_PS_SPACE( *p ) || *p OP 0x80 )
        break;

      c = ft_char_table[*p & 0x7f];

      if ( c < 0 || c >= base )
        break;

      num = num * base + c;
    }

    if ( sign )
      num = -num;

    *cursor = p;

    return num;
  }


  FT_LOCAL_DEF( FT_Int )
  PS_Conv_ToInt( FT_Byte**  cursor,
                 FT_Byte*   limit )

  {
    FT_Byte*  p;
    FT_Int    num;


    num = PS_Conv_Strtol( cursor, limit, 10 );
    p   = *cursor;

    if ( p < limit && *p == '#' )
    {
      *cursor = p + 1;

      return PS_Conv_Strtol( cursor, limit, num );
    }
    else
      return num;
  }


  FT_LOCAL_DEF( FT_Fixed )
  PS_Conv_ToFixed( FT_Byte**  cursor,
                   FT_Byte*   limit,
                   FT_Int     power_ten )
  {
    FT_Byte*  p = *cursor;
    FT_Fixed  integral;
    FT_Long   decimal = 0, divider = 1;
    FT_Bool   sign = 0;


    if ( p == limit )
      return 0;

    if ( *p == '-' || *p == '+' )
    {
      sign = FT_BOOL( *p == '-' );

      p++;
      if ( p == limit )
        return 0;
    }

    if ( *p != '.' )
      integral = PS_Conv_ToInt( &p, limit ) << 16;
    else
      integral = 0;

    
    if ( p < limit && *p == '.' )
    {
      p++;

      for ( ; p < limit; p++ )
      {
        FT_Char  c;


        if ( IS_PS_SPACE( *p ) || *p OP 0x80 )
          break;

        c = ft_char_table[*p & 0x7f];

        if ( c < 0 || c >= 10 )
          break;

        if ( !integral && power_ten > 0 )
        {
          power_ten--;
          decimal = decimal * 10 + c;
        }
        else
        {
          if ( divider < 10000000L )
          {
            decimal = decimal * 10 + c;
            divider *= 10;
          }
        }
      }
    }

    
    if ( p + 1 < limit && ( *p == 'e' || *p == 'E' ) )
    {
      p++;
      power_ten += PS_Conv_ToInt( &p, limit );
    }

    while ( power_ten > 0 )
    {
      integral *= 10;
      decimal  *= 10;
      power_ten--;
    }

    while ( power_ten < 0 )
    {
      integral /= 10;
      divider  *= 10;
      power_ten++;
    }

    if ( decimal )
      integral += FT_DivFix( decimal, divider );

    if ( sign )
      integral = -integral;

    *cursor = p;

    return integral;
  }


#if 0
  FT_LOCAL_DEF( FT_UInt )
  PS_Conv_StringDecode( FT_Byte**  cursor,
                        FT_Byte*   limit,
                        FT_Byte*   buffer,
                        FT_UInt    n )
  {
    FT_Byte*  p;
    FT_UInt   r = 0;


    for ( p = *cursor; r < n && p < limit; p++ )
    {
      FT_Byte  b;


      if ( *p != '\\' )
      {
        buffer[r++] = *p;

        continue;
      }

      p++;

      switch ( *p )
      {
      case 'n':
        b = '\n';
        break;
      case 'r':
        b = '\r';
        break;
      case 't':
        b = '\t';
        break;
      case 'b':
        b = '\b';
        break;
      case 'f':
        b = '\f';
        break;
      case '\r':
        p++;
        if ( *p != '\n' )
        {
          b = *p;

          break;
        }
        
      case '\n':
        continue;
        break;
      default:
        if ( IS_PS_DIGIT( *p ) )
        {
          b = *p - '0';

          p++;

          if ( IS_PS_DIGIT( *p ) )
          {
            b = b * 8 + *p - '0';

            p++;

            if ( IS_PS_DIGIT( *p ) )
              b = b * 8 + *p - '0';
            else
            {
              buffer[r++] = b;
              b = *p;
            }
          }
          else
          {
            buffer[r++] = b;
            b = *p;
          }
        }
        else
          b = *p;
        break;
      }

      buffer[r++] = b;
    }

    *cursor = p;

    return r;
  }
#endif 


  FT_LOCAL_DEF( FT_UInt )
  PS_Conv_ASCIIHexDecode( FT_Byte**  cursor,
                          FT_Byte*   limit,
                          FT_Byte*   buffer,
                          FT_UInt    n )
  {
    FT_Byte*  p;
    FT_UInt   r   = 0;
    FT_UInt   w   = 0;
    FT_UInt   pad = 0x01;


    n *= 2;

#if 1

    p  = *cursor;
    if ( n > (FT_UInt)( limit - p ) )
      n = (FT_UInt)( limit - p );

    
    for ( ; r < n; r++ )
    {
      FT_UInt  c = p[r];


      if ( IS_PS_SPACE( c ) )
        continue;

      if ( c OP 0x80 )
        break;

      c = ft_char_table[c & 0x7F];
      if ( (unsigned)c >= 16 )
        break;

      pad = ( pad << 4 ) | c;
      if ( pad & 0x100 )
      {
        buffer[w++] = (FT_Byte)pad;
        pad         = 0x01;
      }
    }

    if ( pad != 0x01 )
      buffer[w++] = (FT_Byte)( pad << 4 );

    *cursor = p + r;

    return w;

#else 

    for ( r = 0; r < n; r++ )
    {
      FT_Char  c;


      if ( IS_PS_SPACE( *p ) )
        continue;

      if ( *p OP 0x80 )
        break;

      c = ft_char_table[*p & 0x7f];

      if ( (unsigned)c >= 16 )
        break;

      if ( r & 1 )
      {
        *buffer = (FT_Byte)(*buffer + c);
        buffer++;
      }
      else
        *buffer = (FT_Byte)(c << 4);

      r++;
    }

    *cursor = p;

    return ( r + 1 ) / 2;

#endif 

  }


  FT_LOCAL_DEF( FT_UInt )
  PS_Conv_EexecDecode( FT_Byte**   cursor,
                       FT_Byte*    limit,
                       FT_Byte*    buffer,
                       FT_UInt     n,
                       FT_UShort*  seed )
  {
    FT_Byte*  p;
    FT_UInt   r;
    FT_UInt   s = *seed;


#if 1

    p = *cursor;
    if ( n > (FT_UInt)(limit - p) )
      n = (FT_UInt)(limit - p);

    for ( r = 0; r < n; r++ )
    {
      FT_UInt  val = p[r];
      FT_UInt  b   = ( val ^ ( s >> 8 ) );


      s         = ( (val + s)*52845U + 22719 ) & 0xFFFFU;
      buffer[r] = (FT_Byte) b;
    }

    *cursor = p + n;
    *seed   = (FT_UShort)s;

#else 

    for ( r = 0, p = *cursor; r < n && p < limit; r++, p++ )
    {
      FT_Byte  b = (FT_Byte)( *p ^ ( s >> 8 ) );


      s = (FT_UShort)( ( *p + s ) * 52845U + 22719 );
      *buffer++ = b;
    }
    *cursor = p;
    *seed   = s;

#endif 

    return r;
  }



