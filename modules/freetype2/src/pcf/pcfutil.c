

































#include <ft2build.h>
#include "pcfutil.h"


  



  FT_LOCAL_DEF( void )
  BitOrderInvert( unsigned char*  buf,
                  size_t          nbytes )
  {
    for ( ; nbytes > 0; nbytes--, buf++ )
    {
      unsigned int  val = *buf;


      val = ( ( val >> 1 ) & 0x55 ) | ( ( val << 1 ) & 0xAA );
      val = ( ( val >> 2 ) & 0x33 ) | ( ( val << 2 ) & 0xCC );
      val = ( ( val >> 4 ) & 0x0F ) | ( ( val << 4 ) & 0xF0 );

      *buf = (unsigned char)val;
    }
  }


  



  FT_LOCAL_DEF( void )
  TwoByteSwap( unsigned char*  buf,
               size_t          nbytes )
  {
    for ( ; nbytes >= 2; nbytes -= 2, buf += 2 )
    {
      unsigned char  c;


      c      = buf[0];
      buf[0] = buf[1];
      buf[1] = c;
    }
  }

  



  FT_LOCAL_DEF( void )
  FourByteSwap( unsigned char*  buf,
                size_t          nbytes )
  {
    for ( ; nbytes >= 4; nbytes -= 4, buf += 4 )
    {
      unsigned char  c;


      c      = buf[0];
      buf[0] = buf[3];
      buf[3] = c;

      c      = buf[1];
      buf[1] = buf[2];
      buf[2] = c;
    }
  }



