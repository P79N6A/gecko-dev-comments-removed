

















#define FT_MAKE_OPTION_SINGLE_OBJECT

#include <ft2build.h>
#include "ttdriver.c"   
#include "ttpload.c"    
#include "ttgload.c"    
#include "ttobjs.c"     

#ifdef TT_USE_BYTECODE_INTERPRETER
#include "ttinterp.c"
#endif

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
#include "ttgxvar.c"    
#endif



