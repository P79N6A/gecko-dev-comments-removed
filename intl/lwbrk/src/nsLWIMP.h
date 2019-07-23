



































#ifndef nsLWIMP_h__
#define nsLWIMP_h__

#ifdef MOZ_ENABLE_PANGO
# include "nsPangoLineBreaker.h"
# define LINEBREAKER nsPangoLineBreaker
#else
# include "nsJISx4501LineBreaker.h"
# define LINEBREAKER nsJISx4501LineBreaker
#endif

#include "nsSampleWordBreaker.h"

#define WORDBREAKER nsSampleWordBreaker

#endif  
