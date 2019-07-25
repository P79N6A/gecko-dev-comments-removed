



#ifndef mozilla_dom_ScreenOrientation_h
#define mozilla_dom_ScreenOrientation_h

namespace mozilla {
namespace dom {

enum ScreenOrientation {
  eScreenOrientation_Current            = 0,
  eScreenOrientation_PortraitPrimary    = 1,  
  eScreenOrientation_PortraitSecondary  = 2,  
  eScreenOrientation_Portrait           = 3,  
  eScreenOrientation_LandscapePrimary   = 4,  
  eScreenOrientation_LandscapeSecondary = 8,  
  eScreenOrientation_Landscape          = 12, 
  eScreenOrientation_EndGuard
};






class ScreenOrientationWrapper {
public:
  ScreenOrientationWrapper()
    : orientation(eScreenOrientation_Current)
  {}

  ScreenOrientationWrapper(ScreenOrientation aOrientation)
    : orientation(aOrientation)
  {}

  ScreenOrientation orientation;
};

} 
} 

namespace IPC {






template <>
struct ParamTraits<mozilla::dom::ScreenOrientation>
  : public EnumSerializer<mozilla::dom::ScreenOrientation,
                          mozilla::dom::eScreenOrientation_Current,
                          mozilla::dom::eScreenOrientation_EndGuard>
{};

} 

#endif 
