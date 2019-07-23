




































#ifndef nsToolkit_h__
#define nsToolkit_h__

#include "nsIToolkit.h"
#include "nsGCCache.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>





 

class nsToolkit : public nsIToolkit
{
private:
  Display *mDisplay;
  void CreateSharedGC();
  xGC *mGC;

public:
  nsToolkit();
  virtual ~nsToolkit();
  
  NS_DECL_ISUPPORTS
  NS_IMETHOD Init(PRThread *aThread);
  xGC *GetSharedGC();
};

#endif
