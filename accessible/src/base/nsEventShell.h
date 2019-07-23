





































#ifndef _nsEventShell_H_
#define _nsEventShell_H_

#include "nsAccEvent.h"

class nsEventShell
{
public:

  


  static void FireEvent(nsAccEvent *aEvent);

  







  static void FireEvent(PRUint32 aEventType, nsIAccessible *aAccessible,
                        PRBool aIsAsynch = PR_FALSE);
};

#endif
