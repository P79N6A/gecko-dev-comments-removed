

















#ifndef __SVPOSTNM_H__
#define __SVPOSTNM_H__

#include FT_INTERNAL_SERVICE_H


FT_BEGIN_HEADER

  









#define FT_SERVICE_ID_POSTSCRIPT_FONT_NAME  "postscript-font-name"


  typedef const char*
  (*FT_PsName_GetFunc)( FT_Face  face );


  FT_DEFINE_SERVICE( PsFontName )
  {
    FT_PsName_GetFunc  get_ps_font_name;
  };

#ifndef FT_CONFIG_OPTION_PIC

#define FT_DEFINE_SERVICE_PSFONTNAMEREC(class_, get_ps_font_name_) \
  static const FT_Service_PsFontNameRec class_ =                   \
  {                                                                \
    get_ps_font_name_                                              \
  };

#else  

#define FT_DEFINE_SERVICE_PSFONTNAMEREC(class_, get_ps_font_name_) \
  void                                                             \
  FT_Init_Class_##class_( FT_Library library,                      \
                          FT_Service_PsFontNameRec* clazz)         \
  {                                                                \
    FT_UNUSED(library);                                            \
    clazz->get_ps_font_name = get_ps_font_name_;                   \
  } 

#endif  

  


FT_END_HEADER


#endif 



