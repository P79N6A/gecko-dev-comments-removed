

















  
  
  
  
  
  

#ifndef __OTVERROR_H__
#define __OTVERROR_H__

#include FT_MODULE_ERRORS_H

#undef __FTERRORS_H__

#undef  FT_ERR_PREFIX
#define FT_ERR_PREFIX  OTV_Err_
#define FT_ERR_BASE    FT_Mod_Err_OTvalid

#include FT_ERRORS_H

#endif 



