

















#ifndef __AFPIC_H__
#define __AFPIC_H__


FT_BEGIN_HEADER

#include FT_INTERNAL_PIC_H


#ifndef FT_CONFIG_OPTION_PIC

#define AF_SERVICES_GET                af_services
#define AF_SERVICE_PROPERTIES_GET      af_service_properties

#define AF_WRITING_SYSTEM_CLASSES_GET  af_writing_system_classes
#define AF_SCRIPT_CLASSES_GET          af_script_classes
#define AF_INTERFACE_GET               af_autofitter_interface

#else 

  
#include FT_SERVICE_PROPERTIES_H

#include "aftypes.h"


  typedef struct  AFModulePIC_
  {
    FT_ServiceDescRec*          af_services;
    FT_Service_PropertiesRec    af_service_properties;

    AF_WritingSystemClass       af_writing_system_classes
                                  [AF_WRITING_SYSTEM_MAX];
    AF_WritingSystemClassRec    af_writing_system_classes_rec
                                  [AF_WRITING_SYSTEM_MAX - 1];

    AF_ScriptClass              af_script_classes
                                  [AF_SCRIPT_MAX];
    AF_ScriptClassRec           af_script_classes_rec
                                  [AF_SCRIPT_MAX - 1];

    FT_AutoHinter_InterfaceRec  af_autofitter_interface;

  } AFModulePIC;


#define GET_PIC( lib )  \
          ( (AFModulePIC*)((lib)->pic_container.autofit) )

#define AF_SERVICES_GET  \
          ( GET_PIC( library )->af_services )
#define AF_SERVICE_PROPERTIES_GET  \
          ( GET_PIC( library )->af_service_properties )

#define AF_WRITING_SYSTEM_CLASSES_GET  \
          ( GET_PIC( FT_FACE_LIBRARY( globals->face ) )->af_writing_system_classes )
#define AF_SCRIPT_CLASSES_GET  \
          ( GET_PIC( FT_FACE_LIBRARY( globals->face ) )->af_script_classes )
#define AF_INTERFACE_GET  \
          ( GET_PIC( library )->af_autofitter_interface )


  
  void
  autofit_module_class_pic_free( FT_Library  library );

  FT_Error
  autofit_module_class_pic_init( FT_Library  library );

#endif 

 

FT_END_HEADER

#endif 



