

















#define FT_MAKE_OPTION_SINGLE_OBJECT

#include <ft2build.h>
#include "ttload.c"
#include "ttmtx.c"
#include "ttcmap.c"
#include "ttkern.c"
#include "sfobjs.c"
#include "sfdriver.c"

#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS
#include "ttsbit.c"
#endif

#ifdef TT_CONFIG_OPTION_POSTSCRIPT_NAMES
#include "ttpost.c"
#endif

#ifdef TT_CONFIG_OPTION_BDF
#include "ttbdf.c"
#endif


