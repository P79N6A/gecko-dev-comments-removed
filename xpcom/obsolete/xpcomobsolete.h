





































#ifndef xpcomobsolete_h___
#define xpcomobsolete_h___

#include "nscore.h"

#ifdef _IMPL_NS_COM_OBSOLETE
#define NS_COM_OBSOLETE NS_EXPORT
#else
#define NS_COM_OBSOLETE NS_IMPORT
#endif

#endif 
