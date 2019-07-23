


























#ifndef __BDFDRIVR_H__
#define __BDFDRIVR_H__

#include <ft2build.h>
#include FT_INTERNAL_DRIVER_H

#include "bdf.h"


FT_BEGIN_HEADER


  typedef struct  BDF_encoding_el_
  {
    FT_ULong   enc;
    FT_UShort  glyph;

  } BDF_encoding_el;


  typedef struct  BDF_FaceRec_
  {
    FT_FaceRec        root;

    char*             charset_encoding;
    char*             charset_registry;

    bdf_font_t*       bdffont;

    BDF_encoding_el*  en_table;

    FT_CharMap        charmap_handle;
    FT_CharMapRec     charmap;  

    FT_UInt           default_glyph;

  } BDF_FaceRec, *BDF_Face;


  FT_EXPORT_VAR( const FT_Driver_ClassRec )  bdf_driver_class;


FT_END_HEADER


#endif 



