




































#ifndef nsLWBrkConstructors_h__
#define nsLWBrkConstructors_h__

#include "nsLWBrkCIID.h"
#include "nsILineBreaker.h"
#include "nsIWordBreaker.h"
#ifdef MOZ_ENABLE_PANGO
# include "nsPangoLineBreaker.h"
#else
# include "nsJISx4501LineBreaker.h"
#endif
#include "nsSampleWordBreaker.h"
#include "nsLWBRKDll.h"

#ifdef MOZ_ENABLE_PANGO
NS_GENERIC_FACTORY_CONSTRUCTOR(nsPangoLineBreaker)
#else
NS_GENERIC_FACTORY_CONSTRUCTOR(nsJISx4051LineBreaker)
#endif

NS_GENERIC_FACTORY_CONSTRUCTOR(nsSampleWordBreaker)

#endif
     
