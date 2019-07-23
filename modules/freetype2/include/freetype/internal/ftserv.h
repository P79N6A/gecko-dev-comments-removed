
















  
  
  
  
  
  
  
  
  
  
  


#ifndef __FTSERV_H__
#define __FTSERV_H__


FT_BEGIN_HEADER

#if defined( _MSC_VER )      

  
  
#pragma warning( disable : 4127 )

#endif 

  





















#ifdef __cplusplus

#define FT_FACE_FIND_SERVICE( face, ptr, id )                               \
  FT_BEGIN_STMNT                                                            \
    FT_Module    module = FT_MODULE( FT_FACE( face )->driver );             \
    FT_Pointer   _tmp_  = NULL;                                             \
    FT_Pointer*  _pptr_ = (FT_Pointer*)&(ptr);                              \
                                                                            \
                                                                            \
    if ( module->clazz->get_interface )                                     \
      _tmp_ = module->clazz->get_interface( module, FT_SERVICE_ID_ ## id ); \
    *_pptr_ = _tmp_;                                                        \
  FT_END_STMNT

#else 

#define FT_FACE_FIND_SERVICE( face, ptr, id )                               \
  FT_BEGIN_STMNT                                                            \
    FT_Module   module = FT_MODULE( FT_FACE( face )->driver );              \
    FT_Pointer  _tmp_  = NULL;                                              \
                                                                            \
    if ( module->clazz->get_interface )                                     \
      _tmp_ = module->clazz->get_interface( module, FT_SERVICE_ID_ ## id ); \
    ptr = _tmp_;                                                            \
  FT_END_STMNT

#endif 

  





















#ifdef __cplusplus

#define FT_FACE_FIND_GLOBAL_SERVICE( face, ptr, id )               \
  FT_BEGIN_STMNT                                                   \
    FT_Module    module = FT_MODULE( FT_FACE( face )->driver );    \
    FT_Pointer   _tmp_;                                            \
    FT_Pointer*  _pptr_ = (FT_Pointer*)&(ptr);                     \
                                                                   \
                                                                   \
    _tmp_ = ft_module_get_service( module, FT_SERVICE_ID_ ## id ); \
    *_pptr_ = _tmp_;                                               \
  FT_END_STMNT

#else 

#define FT_FACE_FIND_GLOBAL_SERVICE( face, ptr, id )               \
  FT_BEGIN_STMNT                                                   \
    FT_Module   module = FT_MODULE( FT_FACE( face )->driver );     \
    FT_Pointer  _tmp_;                                             \
                                                                   \
                                                                   \
    _tmp_ = ft_module_get_service( module, FT_SERVICE_ID_ ## id ); \
    ptr   = _tmp_;                                                 \
  FT_END_STMNT

#endif 


  
  
  
  
  
  
  

  



  typedef struct  FT_ServiceDescRec_
  {
    const char*  serv_id;     
    const void*  serv_data;   

  } FT_ServiceDescRec;

  typedef const FT_ServiceDescRec*  FT_ServiceDesc;


  








  FT_BASE( FT_Pointer )
  ft_service_list_lookup( FT_ServiceDesc  service_descriptors,
                          const char*     service_id );


  
  
  
  
  
  
  

  












  typedef struct  FT_ServiceCacheRec_
  {
    FT_Pointer  service_POSTSCRIPT_FONT_NAME;
    FT_Pointer  service_MULTI_MASTERS;
    FT_Pointer  service_GLYPH_DICT;
    FT_Pointer  service_PFR_METRICS;
    FT_Pointer  service_WINFNT;

  } FT_ServiceCacheRec, *FT_ServiceCache;


  


#define FT_SERVICE_UNAVAILABLE  ((FT_Pointer)-2)  /* magic number */


  





















#ifdef __cplusplus

#define FT_FACE_LOOKUP_SERVICE( face, ptr, id )                \
  FT_BEGIN_STMNT                                               \
    FT_Pointer   svc;                                          \
    FT_Pointer*  Pptr = (FT_Pointer*)&(ptr);                   \
                                                               \
                                                               \
    svc = FT_FACE( face )->internal->services. service_ ## id; \
    if ( svc == FT_SERVICE_UNAVAILABLE )                       \
      svc = NULL;                                              \
    else if ( svc == NULL )                                    \
    {                                                          \
      FT_FACE_FIND_SERVICE( face, svc, id );                   \
                                                               \
      FT_FACE( face )->internal->services. service_ ## id =    \
        (FT_Pointer)( svc != NULL ? svc                        \
                                  : FT_SERVICE_UNAVAILABLE );  \
    }                                                          \
    *Pptr = svc;                                               \
  FT_END_STMNT

#else 

#define FT_FACE_LOOKUP_SERVICE( face, ptr, id )                \
  FT_BEGIN_STMNT                                               \
    FT_Pointer  svc;                                           \
                                                               \
                                                               \
    svc = FT_FACE( face )->internal->services. service_ ## id; \
    if ( svc == FT_SERVICE_UNAVAILABLE )                       \
      svc = NULL;                                              \
    else if ( svc == NULL )                                    \
    {                                                          \
      FT_FACE_FIND_SERVICE( face, svc, id );                   \
                                                               \
      FT_FACE( face )->internal->services. service_ ## id =    \
        (FT_Pointer)( svc != NULL ? svc                        \
                                  : FT_SERVICE_UNAVAILABLE );  \
    }                                                          \
    ptr = svc;                                                 \
  FT_END_STMNT

#endif 

  



#define FT_DEFINE_SERVICE( name )            \
  typedef struct FT_Service_ ## name ## Rec_ \
    FT_Service_ ## name ## Rec ;             \
  typedef struct FT_Service_ ## name ## Rec_ \
    const * FT_Service_ ## name ;            \
  struct FT_Service_ ## name ## Rec_

  

  



#define FT_SERVICE_BDF_H                <freetype/internal/services/svbdf.h>
#define FT_SERVICE_CID_H                <freetype/internal/services/svcid.h>
#define FT_SERVICE_GLYPH_DICT_H         <freetype/internal/services/svgldict.h>
#define FT_SERVICE_GX_VALIDATE_H        <freetype/internal/services/svgxval.h>
#define FT_SERVICE_KERNING_H            <freetype/internal/services/svkern.h>
#define FT_SERVICE_MULTIPLE_MASTERS_H   <freetype/internal/services/svmm.h>
#define FT_SERVICE_OPENTYPE_VALIDATE_H  <freetype/internal/services/svotval.h>
#define FT_SERVICE_PFR_H                <freetype/internal/services/svpfr.h>
#define FT_SERVICE_POSTSCRIPT_CMAPS_H   <freetype/internal/services/svpscmap.h>
#define FT_SERVICE_POSTSCRIPT_INFO_H    <freetype/internal/services/svpsinfo.h>
#define FT_SERVICE_POSTSCRIPT_NAME_H    <freetype/internal/services/svpostnm.h>
#define FT_SERVICE_SFNT_H               <freetype/internal/services/svsfnt.h>
#define FT_SERVICE_TRUETYPE_ENGINE_H    <freetype/internal/services/svtteng.h>
#define FT_SERVICE_TT_CMAP_H            <freetype/internal/services/svttcmap.h>
#define FT_SERVICE_WINFNT_H             <freetype/internal/services/svwinfnt.h>
#define FT_SERVICE_XFREE86_NAME_H       <freetype/internal/services/svxf86nm.h>
#define FT_SERVICE_TRUETYPE_GLYF_H      <freetype/internal/services/svttglyf.h>

 

FT_END_HEADER

#endif 



