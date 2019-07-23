


























#ifndef __PCFUTIL_H__
#define __PCFUTIL_H__


#include <ft2build.h>
#include FT_CONFIG_CONFIG_H


FT_BEGIN_HEADER

  FT_LOCAL( void )
  BitOrderInvert( unsigned char*  buf,
                  int             nbytes );

  FT_LOCAL( void )
  TwoByteSwap( unsigned char*  buf,
               int             nbytes );

  FT_LOCAL( void )
  FourByteSwap( unsigned char*  buf,
                int             nbytes );

FT_END_HEADER

#endif 



