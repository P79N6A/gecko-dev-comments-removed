

















#ifndef __TTBDF_H__
#define __TTBDF_H__


#include <ft2build.h>
#include "ttload.h"
#include FT_BDF_H


FT_BEGIN_HEADER


  FT_LOCAL( void )
  tt_face_free_bdf_props( TT_Face  face );


  FT_LOCAL( FT_Error )
  tt_face_find_bdf_prop( TT_Face           face,
                         const char*       property_name,
                         BDF_PropertyRec  *aprop );


FT_END_HEADER

#endif 



