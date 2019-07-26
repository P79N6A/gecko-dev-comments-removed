





































#ifndef __CF2INTRP_H__
#define __CF2INTRP_H__


#include "cf2ft.h"
#include "cf2hints.h"


FT_BEGIN_HEADER


  FT_LOCAL( void )
  cf2_hintmask_init( CF2_HintMask  hintmask,
                     FT_Error*     error );
  FT_LOCAL( FT_Bool )
  cf2_hintmask_isValid( const CF2_HintMask  hintmask );
  FT_LOCAL( FT_Bool )
  cf2_hintmask_isNew( const CF2_HintMask  hintmask );
  FT_LOCAL( void )
  cf2_hintmask_setNew( CF2_HintMask  hintmask,
                       FT_Bool       val );
  FT_LOCAL( FT_Byte* )
  cf2_hintmask_getMaskPtr( CF2_HintMask  hintmask );
  FT_LOCAL( void )
  cf2_hintmask_setAll( CF2_HintMask  hintmask,
                       size_t        bitCount );

  FT_LOCAL( void )
  cf2_interpT2CharString( CF2_Font              font,
                          CF2_Buffer            charstring,
                          CF2_OutlineCallbacks  callbacks,
                          const FT_Vector*      translation,
                          FT_Bool               doingSeac,
                          CF2_Fixed             curX,
                          CF2_Fixed             curY,
                          CF2_Fixed*            width );


FT_END_HEADER


#endif 



