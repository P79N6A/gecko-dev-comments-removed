

















#ifndef __SVPROP_H__
#define __SVPROP_H__


FT_BEGIN_HEADER


#define FT_SERVICE_ID_PROPERTIES  "properties"


  typedef FT_Error
  (*FT_Properties_SetFunc)( FT_Module    module,
                            const char*  property_name,
                            const void*  value );

  typedef FT_Error
  (*FT_Properties_GetFunc)( FT_Module    module,
                            const char*  property_name,
                            void*        value );


  FT_DEFINE_SERVICE( Properties )
  {
    FT_Properties_SetFunc  set_property;
    FT_Properties_GetFunc  get_property;
  };


#ifndef FT_CONFIG_OPTION_PIC

#define FT_DEFINE_SERVICE_PROPERTIESREC( class_,          \
                                         set_property_,   \
                                         get_property_ )  \
  static const FT_Service_PropertiesRec  class_ =         \
  {                                                       \
    set_property_,                                        \
    get_property_                                         \
  };

#else 

#define FT_DEFINE_SERVICE_PROPERTIESREC( class_,                \
                                         set_property_,         \
                                         get_property_ )        \
  void                                                          \
  FT_Init_Class_ ## class_( FT_Service_PropertiesRec*  clazz )  \
  {                                                             \
    clazz->set_property = set_property_;                        \
    clazz->get_property = get_property_;                        \
  }

#endif 

  


FT_END_HEADER


#endif 



