





#ifndef _EXTENSION_BEHAVIOR_INCLUDED_
#define _EXTENSION_BEHAVIOR_INCLUDED_

#include "compiler/Common.h"

typedef enum {
    EBhRequire,
    EBhEnable,
    EBhWarn,
    EBhDisable
} TBehavior;

typedef TMap<TString, TBehavior> TExtensionBehavior;

#endif 
