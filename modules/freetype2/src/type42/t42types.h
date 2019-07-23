
















#ifndef __T42TYPES_H__
#define __T42TYPES_H__


#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_TYPE1_TABLES_H
#include FT_INTERNAL_TYPE1_TYPES_H
#include FT_INTERNAL_POSTSCRIPT_HINTS_H


FT_BEGIN_HEADER


  typedef struct  T42_FaceRec_
  {
    FT_FaceRec      root;
    T1_FontRec      type1;
    const void*     psnames;
    const void*     psaux;
    const void*     afm_data;
    FT_Byte*        ttf_data;
    FT_ULong        ttf_size;
    FT_Face         ttf_face;
    FT_CharMapRec   charmaprecs[2];
    FT_CharMap      charmaps[2];
    PS_UnicodesRec  unicode_map;

  } T42_FaceRec, *T42_Face;


FT_END_HEADER

#endif 



