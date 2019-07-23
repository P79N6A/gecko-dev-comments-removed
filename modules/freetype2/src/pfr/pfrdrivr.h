

















#ifndef __PFRDRIVR_H__
#define __PFRDRIVR_H__


#include <ft2build.h>
#include FT_INTERNAL_DRIVER_H


FT_BEGIN_HEADER

#ifdef FT_CONFIG_OPTION_PIC
#error "this module does not support PIC yet"
#endif 


  FT_EXPORT_VAR( const FT_Driver_ClassRec )  pfr_driver_class;


FT_END_HEADER


#endif 



