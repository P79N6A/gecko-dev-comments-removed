



#ifndef mozilla_dom_ScreenOrientation_h
#define mozilla_dom_ScreenOrientation_h

#include "ipc/IPCMessageUtils.h"

namespace mozilla {
namespace dom {




typedef uint32_t ScreenOrientation;

static const ScreenOrientation eScreenOrientation_None               = 0;
static const ScreenOrientation eScreenOrientation_PortraitPrimary    = 1;  
static const ScreenOrientation eScreenOrientation_PortraitSecondary  = 2;  
static const ScreenOrientation eScreenOrientation_Portrait           = 3;  
static const ScreenOrientation eScreenOrientation_LandscapePrimary   = 4;  
static const ScreenOrientation eScreenOrientation_LandscapeSecondary = 8;  
static const ScreenOrientation eScreenOrientation_Landscape          = 12; 

} 
} 

#endif 
