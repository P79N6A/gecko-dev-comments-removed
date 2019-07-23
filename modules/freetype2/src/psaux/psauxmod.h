

















#ifndef __PSAUXMOD_H__
#define __PSAUXMOD_H__


#include <ft2build.h>
#include FT_MODULE_H


FT_BEGIN_HEADER

#ifdef FT_CONFIG_OPTION_PIC
#error "this module does not support PIC yet"
#endif 


  FT_EXPORT_VAR( const FT_Module_Class )  psaux_driver_class;


FT_END_HEADER

#endif 



