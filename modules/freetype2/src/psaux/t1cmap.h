

















#ifndef __T1CMAP_H__
#define __T1CMAP_H__

#include <ft2build.h>
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_TYPE1_TYPES_H

FT_BEGIN_HEADER


  
  
  
  
  
  
  

  
  typedef struct T1_CMapStdRec_*  T1_CMapStd;

  typedef struct  T1_CMapStdRec_
  {
    FT_CMapRec                cmap;

    const FT_UShort*          code_to_sid;
    PS_Adobe_Std_StringsFunc  sid_to_string;

    FT_UInt                   num_glyphs;
    const char* const*        glyph_names;

  } T1_CMapStdRec;


  FT_CALLBACK_TABLE const FT_CMap_ClassRec
  t1_cmap_standard_class_rec;

  FT_CALLBACK_TABLE const FT_CMap_ClassRec
  t1_cmap_expert_class_rec;


  
  
  
  
  
  
  

  typedef struct T1_CMapCustomRec_*  T1_CMapCustom;

  typedef struct  T1_CMapCustomRec_
  {
    FT_CMapRec  cmap;
    FT_UInt     first;
    FT_UInt     count;
    FT_UShort*  indices;

  } T1_CMapCustomRec;


  FT_CALLBACK_TABLE const FT_CMap_ClassRec
  t1_cmap_custom_class_rec;


  
  
  
  
  
  
  

  

  FT_CALLBACK_TABLE const FT_CMap_ClassRec
  t1_cmap_unicode_class_rec;

 


FT_END_HEADER

#endif 



