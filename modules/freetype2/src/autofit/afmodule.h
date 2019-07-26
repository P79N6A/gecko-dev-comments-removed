

















#ifndef __AFMODULE_H__
#define __AFMODULE_H__

#include <ft2build.h>
#include FT_INTERNAL_OBJECTS_H
#include FT_MODULE_H

#include "afloader.h"


FT_BEGIN_HEADER


  






  typedef struct  AF_ModuleRec_
  {
    FT_ModuleRec  root;

    FT_UInt       fallback_script;

    AF_LoaderRec  loader[1];

  } AF_ModuleRec;


FT_DECLARE_MODULE(autofit_module_class)


FT_END_HEADER

#endif 



