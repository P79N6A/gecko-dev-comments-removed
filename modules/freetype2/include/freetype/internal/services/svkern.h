

















#ifndef __SVKERN_H__
#define __SVKERN_H__

#include FT_INTERNAL_SERVICE_H
#include FT_TRUETYPE_TABLES_H


FT_BEGIN_HEADER

#define FT_SERVICE_ID_KERNING  "kerning"


  typedef FT_Error
  (*FT_Kerning_TrackGetFunc)( FT_Face    face,
                              FT_Fixed   point_size,
                              FT_Int     degree,
                              FT_Fixed*  akerning );

  FT_DEFINE_SERVICE( Kerning )
  {
    FT_Kerning_TrackGetFunc  get_track;
  };

  


FT_END_HEADER


#endif 



