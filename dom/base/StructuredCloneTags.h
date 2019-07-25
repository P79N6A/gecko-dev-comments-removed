



































#ifndef StructuredCloneTags_h__
#define StructuredCloneTags_h__

#include "jsapi.h"

namespace mozilla {
namespace dom {

enum StructuredCloneTags {
  SCTAG_BASE = JS_SCTAG_USER_MIN,
  SCTAG_DOM_BLOB,
  SCTAG_DOM_FILE,
  SCTAG_DOM_FILELIST,
  SCTAG_DOM_MAX
};

} 
} 

#endif 
