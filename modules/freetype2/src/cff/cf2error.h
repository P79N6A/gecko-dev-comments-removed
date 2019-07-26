





































#ifndef __CF2ERROR_H__
#define __CF2ERROR_H__


#include FT_MODULE_ERRORS_H

#undef __FTERRORS_H__

#undef  FT_ERR_PREFIX
#define FT_ERR_PREFIX  CF2_Err_
#define FT_ERR_BASE    FT_Mod_Err_CF2


#include FT_ERRORS_H
#include "cf2ft.h"


FT_BEGIN_HEADER


  




































  
  FT_LOCAL( void )
  cf2_setError( FT_Error*  error,
                FT_Error   value );


  






#define CF2_SET_ERROR( error, e )              \
          cf2_setError( error, FT_THROW( e ) )


FT_END_HEADER


#endif 



