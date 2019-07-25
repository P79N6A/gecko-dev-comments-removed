





































#ifndef _nsEventShell_H_
#define _nsEventShell_H_

#include "AccEvent.h"

class nsIPersistentProperties;




class nsEventShell
{
public:

  


  static void FireEvent(AccEvent* aEvent);

  





  static void FireEvent(PRUint32 aEventType, nsAccessible *aAccessible,
                        EIsFromUserInput aIsFromUserInput = eAutoDetect);

  






  static void GetEventAttributes(nsINode *aNode,
                                 nsIPersistentProperties *aAttributes);

private:
  static nsCOMPtr<nsINode> sEventTargetNode;
  static bool sEventFromUserInput;
};

#endif
