





#ifndef xrecore_h__
#define xrecore_h__

#include "nscore.h"




#ifdef XPCOM_GLUE
#define XRE_API(type, name, params) \
  typedef type (NS_FROZENCALL * name##Type) params; \
  extern name##Type name NS_HIDDEN;
#elif defined(IMPL_LIBXUL)
#define XRE_API(type, name, params) EXPORT_XPCOM_API(type) name params;
#else
#define XRE_API(type, name, params) IMPORT_XPCOM_API(type) name params;
#endif

#endif 
