






















#ifndef __SVTTCMAP_H__
#define __SVTTCMAP_H__

#include FT_INTERNAL_SERVICE_H
#include FT_TRUETYPE_TABLES_H


FT_BEGIN_HEADER


#define FT_SERVICE_ID_TT_CMAP "tt-cmaps"


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  typedef struct  TT_CMapInfo_
  {
    FT_ULong language;
    FT_Long  format;

  } TT_CMapInfo;


  typedef FT_Error
  (*TT_CMap_Info_GetFunc)( FT_CharMap    charmap,
                           TT_CMapInfo  *cmap_info );


  FT_DEFINE_SERVICE( TTCMaps )
  {
    TT_CMap_Info_GetFunc  get_cmap_info;
  };

  


FT_END_HEADER

#endif 



