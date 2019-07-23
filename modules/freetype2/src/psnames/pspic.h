

















#ifndef __PSPIC_H__
#define __PSPIC_H__

  
FT_BEGIN_HEADER

#include FT_INTERNAL_PIC_H

#ifndef FT_CONFIG_OPTION_PIC
#define FT_PSCMAPS_SERVICES_GET     pscmaps_services
#define FT_PSCMAPS_INTERFACE_GET    pscmaps_interface

#else 

#include FT_SERVICE_POSTSCRIPT_CMAPS_H

  typedef struct PSModulePIC_
  {
    FT_ServiceDescRec* pscmaps_services;
    FT_Service_PsCMapsRec pscmaps_interface;
  } PSModulePIC;

#define GET_PIC(lib)                ((PSModulePIC*)((lib)->pic_container.psnames))
#define FT_PSCMAPS_SERVICES_GET     (GET_PIC(library)->pscmaps_services)
#define FT_PSCMAPS_INTERFACE_GET    (GET_PIC(library)->pscmaps_interface)

#endif 

 

FT_END_HEADER

#endif 



