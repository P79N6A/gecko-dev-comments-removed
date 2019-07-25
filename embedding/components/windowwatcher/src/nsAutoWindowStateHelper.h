




































#ifndef __nsAutoWindowStateHelper_h
#define __nsAutoWindowStateHelper_h

#include "nsCOMPtr.h"






class nsIDOMWindow;

class nsAutoWindowStateHelper
{
public:
  nsAutoWindowStateHelper(nsIDOMWindow *aWindow);
  ~nsAutoWindowStateHelper();

  bool DefaultEnabled()
  {
    return mDefaultEnabled;
  }

protected:
  bool DispatchCustomEvent(const char *aEventName);

  nsIDOMWindow *mWindow;
  nsCOMPtr<nsIDOMWindow> mCallerWindow;
  bool mDefaultEnabled;
};


#endif
