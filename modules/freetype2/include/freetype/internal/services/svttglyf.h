















#ifndef __SVTTGLYF_H__
#define __SVTTGLYF_H__

#include FT_INTERNAL_SERVICE_H
#include FT_TRUETYPE_TABLES_H


FT_BEGIN_HEADER


#define FT_SERVICE_ID_TT_GLYF "tt-glyf"


  typedef FT_ULong
  (*TT_Glyf_GetLocationFunc)( FT_Face    face,
                              FT_UInt    gindex,
                              FT_ULong  *psize );

  FT_DEFINE_SERVICE( TTGlyf )
  {
    TT_Glyf_GetLocationFunc  get_location;
  };

  


FT_END_HEADER

#endif 



