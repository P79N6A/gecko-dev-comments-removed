




































#ifndef nsColorNames_h___
#define nsColorNames_h___

#include "nsColor.h"
#include "gfxCore.h"
#include "nsStringFwd.h"








#define GFX_COLOR(_name, _value) eColorName_##_name,
enum nsColorName {
  eColorName_UNKNOWN = -1,
#include "nsColorNameList.h"
  eColorName_COUNT
};
#undef GFX_COLOR

class NS_GFX nsColorNames {
public:
  static void AddRefTable(void);
  static void ReleaseTable(void);

  
  
  static nsColorName LookupName(const nsAString& aName);
  static nsColorName LookupName(const nsACString& aName);

  static const nsAFlatCString& GetStringValue(nsColorName aColorName);

  
  static NS_GFX_STATIC_MEMBER_(const nscolor) kColors[];
};

#endif 
