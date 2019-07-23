















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

#ifndef FT_CONFIG_OPTION_PIC

#define FT_DEFINE_SERVICE_TTGLYFREC(class_, get_location_ )   \
  static const FT_Service_TTGlyfRec class_ =                  \
  {                                                           \
    get_location_                                             \
  };

#else  

#define FT_DEFINE_SERVICE_TTGLYFREC(class_, get_location_ )   \
  void                                                        \
  FT_Init_Class_##class_( FT_Service_TTGlyfRec*  clazz )      \
  {                                                           \
    clazz->get_location = get_location_;                      \
  } 

#endif  

  


FT_END_HEADER

#endif 



