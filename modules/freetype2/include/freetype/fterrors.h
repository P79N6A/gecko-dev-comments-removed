

















  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  


#ifndef __FTERRORS_H__
#define __FTERRORS_H__



#include FT_MODULE_ERRORS_H


  
  
  
  
  
  
  


#undef  FT_NEED_EXTERN_C

#undef  FT_ERR_XCAT
#undef  FT_ERR_CAT

#define FT_ERR_XCAT( x, y )  x ## y
#define FT_ERR_CAT( x, y )   FT_ERR_XCAT( x, y )


  
  
  
#ifndef FT_ERR_PREFIX
#define FT_ERR_PREFIX  FT_Err_
#endif


  
  
#ifdef FT_CONFIG_OPTION_USE_MODULE_ERRORS

#ifndef FT_ERR_BASE
#define FT_ERR_BASE  FT_Mod_Err_Base
#endif

#else

#undef FT_ERR_BASE
#define FT_ERR_BASE  0

#endif 


  
  
  
#ifndef FT_ERRORDEF

#define FT_ERRORDEF( e, v, s )  e = v,
#define FT_ERROR_START_LIST     enum {
#define FT_ERROR_END_LIST       FT_ERR_CAT( FT_ERR_PREFIX, Max ) };

#ifdef __cplusplus
#define FT_NEED_EXTERN_C
  extern "C" {
#endif

#endif 


  
#define FT_ERRORDEF_( e, v, s )   \
          FT_ERRORDEF( FT_ERR_CAT( FT_ERR_PREFIX, e ), v + FT_ERR_BASE, s )

  
#define FT_NOERRORDEF_( e, v, s ) \
          FT_ERRORDEF( FT_ERR_CAT( FT_ERR_PREFIX, e ), v, s )


#ifdef FT_ERROR_START_LIST
  FT_ERROR_START_LIST
#endif


  
#include FT_ERROR_DEFINITIONS_H


#ifdef FT_ERROR_END_LIST
  FT_ERROR_END_LIST
#endif


  
  
  
  
  
  
  

#ifdef FT_NEED_EXTERN_C
  }
#endif

#undef FT_ERROR_START_LIST
#undef FT_ERROR_END_LIST

#undef FT_ERRORDEF
#undef FT_ERRORDEF_
#undef FT_NOERRORDEF_

#undef FT_NEED_EXTERN_C
#undef FT_ERR_CONCAT
#undef FT_ERR_BASE

  
#ifndef FT_KEEP_ERR_PREFIX
#undef FT_ERR_PREFIX
#endif

#endif 



