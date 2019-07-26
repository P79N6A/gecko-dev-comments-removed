




#ifndef _nsEventShell_H_
#define _nsEventShell_H_

#include "AccEvent.h"

namespace mozilla {
template<typename T> class StaticRefPtr;
}
class nsIPersistentProperties;




class nsEventShell
{
public:

  


  static void FireEvent(mozilla::a11y::AccEvent* aEvent);

  





  static void FireEvent(uint32_t aEventType,
                        mozilla::a11y::Accessible* aAccessible,
                        mozilla::a11y::EIsFromUserInput aIsFromUserInput = mozilla::a11y::eAutoDetect);

  


  static void FireEvent(mozilla::a11y::Accessible* aTarget, uint64_t aState,
                        bool aIsEnabled, bool aIsFromUserInput)
  {
    nsRefPtr<mozilla::a11y::AccStateChangeEvent> stateChangeEvent =
      new mozilla::a11y::AccStateChangeEvent(aTarget, aState, aIsEnabled,
                                             (aIsFromUserInput ?
                                               mozilla::a11y::eFromUserInput :
                                               mozilla::a11y::eNoUserInput));
    FireEvent(stateChangeEvent);
  }

  






  static void GetEventAttributes(nsINode *aNode,
                                 nsIPersistentProperties *aAttributes);

private:
  static mozilla::StaticRefPtr<nsINode> sEventTargetNode;
  static bool sEventFromUserInput;
};

#endif
