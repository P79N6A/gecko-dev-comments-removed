




#ifndef __nsAutoWindowStateHelper_h
#define __nsAutoWindowStateHelper_h

#include "nsCOMPtr.h"
#include "nsPIDOMWindow.h"






class nsPIDOMWindow;

class nsAutoWindowStateHelper
{
public:
  explicit nsAutoWindowStateHelper(nsPIDOMWindow *aWindow);
  ~nsAutoWindowStateHelper();

  bool DefaultEnabled()
  {
    return mDefaultEnabled;
  }

protected:
  bool DispatchEventToChrome(const char *aEventName);

  nsCOMPtr<nsPIDOMWindow> mWindow;
  bool mDefaultEnabled;
};


#endif
