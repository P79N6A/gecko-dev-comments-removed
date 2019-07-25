





#ifndef _EXTENSION_BEHAVIOR_INCLUDED_
#define _EXTENSION_BEHAVIOR_INCLUDED_

#include "compiler/Common.h"

typedef enum {
    EBhRequire,
    EBhEnable,
    EBhWarn,
    EBhDisable,
    EBhUndefined,
} TBehavior;

inline const char* getBehaviorString(TBehavior b)
{
    switch(b) {
      case EBhRequire:
        return "require";
      case EBhEnable:
        return "enable";
      case EBhWarn:
        return "warn";
      case EBhDisable:
        return "disable";
      default:
        return NULL;
    }
}

typedef TMap<TString, TBehavior> TExtensionBehavior;

#endif 
