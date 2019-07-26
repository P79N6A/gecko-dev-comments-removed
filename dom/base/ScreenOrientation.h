



#ifndef mozilla_dom_ScreenOrientation_h
#define mozilla_dom_ScreenOrientation_h

#include "ipc/IPCMessageUtils.h"

namespace mozilla {
namespace dom {




typedef uint32_t ScreenOrientation;

static const ScreenOrientation eScreenOrientation_None               = 0;
static const ScreenOrientation eScreenOrientation_PortraitPrimary    = PR_BIT(0);
static const ScreenOrientation eScreenOrientation_PortraitSecondary  = PR_BIT(1);
static const ScreenOrientation eScreenOrientation_LandscapePrimary   = PR_BIT(2);
static const ScreenOrientation eScreenOrientation_LandscapeSecondary = PR_BIT(3);

} 
} 

#endif 
