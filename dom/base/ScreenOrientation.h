



#ifndef mozilla_dom_ScreenOrientation_h
#define mozilla_dom_ScreenOrientation_h

namespace mozilla {
namespace dom {



typedef uint32_t ScreenOrientation;

static const ScreenOrientation eScreenOrientation_None               = 0;
static const ScreenOrientation eScreenOrientation_PortraitPrimary    = 1u << 0;
static const ScreenOrientation eScreenOrientation_PortraitSecondary  = 1u << 1;
static const ScreenOrientation eScreenOrientation_LandscapePrimary   = 1u << 2;
static const ScreenOrientation eScreenOrientation_LandscapeSecondary = 1u << 3;

} 
} 

#endif 
