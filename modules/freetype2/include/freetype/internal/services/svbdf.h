

















#ifndef __SVBDF_H__
#define __SVBDF_H__

#include FT_BDF_H
#include FT_INTERNAL_SERVICE_H


FT_BEGIN_HEADER


#define FT_SERVICE_ID_BDF  "bdf"

  typedef FT_Error
  (*FT_BDF_GetCharsetIdFunc)( FT_Face       face,
                              const char*  *acharset_encoding,
                              const char*  *acharset_registry );

  typedef FT_Error
  (*FT_BDF_GetPropertyFunc)( FT_Face           face,
                             const char*       prop_name,
                             BDF_PropertyRec  *aproperty );


  FT_DEFINE_SERVICE( BDF )
  {
    FT_BDF_GetCharsetIdFunc  get_charset_id;
    FT_BDF_GetPropertyFunc   get_property;
  };

#ifndef FT_CONFIG_OPTION_PIC

#define FT_DEFINE_SERVICE_BDFRec(class_, get_charset_id_, get_property_) \
  static const FT_Service_BDFRec class_ =                                \
  {                                                                      \
    get_charset_id_, get_property_                                       \
  };

#else  

#define FT_DEFINE_SERVICE_BDFRec(class_, get_charset_id_, get_property_) \
  void                                                                   \
  FT_Init_Class_##class_( FT_Service_BDFRec*  clazz )                    \
  {                                                                      \
    clazz->get_charset_id = get_charset_id_;                             \
    clazz->get_property = get_property_;                                 \
  } 

#endif  

  


FT_END_HEADER


#endif 



