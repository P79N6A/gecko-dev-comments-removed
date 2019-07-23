

















#ifndef __CFFCMAP_H__
#define __CFFCMAP_H__

#include "cffobjs.h"

FT_BEGIN_HEADER


  
  
  
  
  
  
  

  
  typedef struct CFF_CMapStdRec_*  CFF_CMapStd;

  typedef struct  CFF_CMapStdRec_
  {
    FT_CMapRec  cmap;
    FT_UShort*  gids;   

  } CFF_CMapStdRec;


  FT_CALLBACK_TABLE const FT_CMap_ClassRec
  cff_cmap_encoding_class_rec;


  
  
  
  
  
  
  

  

  FT_CALLBACK_TABLE const FT_CMap_ClassRec
  cff_cmap_unicode_class_rec;


FT_END_HEADER

#endif 



