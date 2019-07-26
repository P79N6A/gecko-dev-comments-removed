




#ifndef _nsEventShell_H_
#define _nsEventShell_H_

#include "AccEvent.h"

class nsIPersistentProperties;




class nsEventShell
{
public:

  


  static void FireEvent(mozilla::a11y::AccEvent* aEvent);

  





  static void FireEvent(uint32_t aEventType,
                        mozilla::a11y::Accessible* aAccessible,
                        mozilla::a11y::EIsFromUserInput aIsFromUserInput = mozilla::a11y::eAutoDetect);

  






  static void GetEventAttributes(nsINode *aNode,
                                 nsIPersistentProperties *aAttributes);

private:
  static nsCOMPtr<nsINode> sEventTargetNode;
  static bool sEventFromUserInput;
};

#endif
