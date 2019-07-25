




































#ifndef __nsAutoWindowStateHelper_h
#define __nsAutoWindowStateHelper_h

#include "nsCOMPtr.h"






class nsIDOMWindow;

class nsAutoWindowStateHelper
{
public:
  nsAutoWindowStateHelper(nsIDOMWindow *aWindow);
  ~nsAutoWindowStateHelper();

  PRBool DefaultEnabled()
  {
    return mDefaultEnabled;
  }

protected:
  PRBool DispatchCustomEvent(const char *aEventName);

  nsIDOMWindow *mWindow;
  nsCOMPtr<nsIDOMWindow> mCallerWindow;
  PRBool mDefaultEnabled;
};


#endif
