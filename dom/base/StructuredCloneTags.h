



#ifndef StructuredCloneTags_h__
#define StructuredCloneTags_h__

#include "js/StructuredClone.h"

namespace mozilla {
namespace dom {



enum StructuredCloneTags {
  SCTAG_BASE = JS_SCTAG_USER_MIN,

  
  SCTAG_DOM_BLOB,

  
  
  SCTAG_DOM_FILE_WITHOUT_LASTMODIFIEDDATE,

  SCTAG_DOM_FILELIST,
  SCTAG_DOM_MUTABLEFILE,
  SCTAG_DOM_FILE,

  
  SCTAG_DOM_IMAGEDATA,
  SCTAG_DOM_MAP_MESSAGEPORT,

  SCTAG_DOM_FUNCTION,

  
  SCTAG_DOM_WEBCRYPTO_KEY,

  SCTAG_DOM_NULL_PRINCIPAL,
  SCTAG_DOM_SYSTEM_PRINCIPAL,
  SCTAG_DOM_CONTENT_PRINCIPAL,

  SCTAG_DOM_MAX
};

} 
} 

#endif 
