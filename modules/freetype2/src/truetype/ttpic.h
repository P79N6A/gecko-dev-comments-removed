

















#ifndef __TTPIC_H__
#define __TTPIC_H__

  
FT_BEGIN_HEADER

#ifndef FT_CONFIG_OPTION_PIC
#define FT_TT_SERVICES_GET                   tt_services
#define FT_TT_SERVICE_GX_MULTI_MASTERS_GET   tt_service_gx_multi_masters
#define FT_TT_SERVICE_TRUETYPE_GLYF_GET      tt_service_truetype_glyf

#else 

#include FT_MULTIPLE_MASTERS_H
#include FT_SERVICE_MULTIPLE_MASTERS_H
#include FT_SERVICE_TRUETYPE_GLYF_H

  typedef struct TTModulePIC_
  {
    FT_ServiceDescRec* tt_services;
#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
    FT_Service_MultiMastersRec tt_service_gx_multi_masters;
#endif
    FT_Service_TTGlyfRec tt_service_truetype_glyf;
  } TTModulePIC;

#define GET_PIC(lib)                         ((TTModulePIC*)((lib)->pic_container.truetype))
#define FT_TT_SERVICES_GET                   (GET_PIC(library)->tt_services)
#define FT_TT_SERVICE_GX_MULTI_MASTERS_GET   (GET_PIC(library)->tt_service_gx_multi_masters)
#define FT_TT_SERVICE_TRUETYPE_GLYF_GET      (GET_PIC(library)->tt_service_truetype_glyf)

#endif 

 

FT_END_HEADER

#endif 



