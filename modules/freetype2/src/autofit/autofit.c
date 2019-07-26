

















#define FT_MAKE_OPTION_SINGLE_OBJECT
#include <ft2build.h>
#include "afpic.c"
#include "afangles.c"
#include "afblue.c"
#include "afglobal.c"
#include "afhints.c"

#include "afranges.c"

#include "afdummy.c"
#include "aflatin.c"
#ifdef FT_OPTION_AUTOFIT2
#include "aflatin2.c"
#endif
#include "afcjk.c"
#include "afindic.c"

#include "hbshim.c"

#include "afloader.c"
#include "afmodule.c"

#ifdef AF_CONFIG_OPTION_USE_WARPER
#include "afwarp.c"
#endif


