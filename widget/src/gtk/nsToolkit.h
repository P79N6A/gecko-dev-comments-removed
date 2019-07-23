




































#ifndef TOOLKIT_H      
#define TOOLKIT_H

#include "nsIToolkit.h"
#include <gtk/gtk.h>





 

class nsToolkit : public nsIToolkit
{

public:
  nsToolkit();
  virtual ~nsToolkit();
  
  NS_DECL_ISUPPORTS

  NS_IMETHOD    Init(PRThread *aThread);

  void          CreateSharedGC(void);
  GdkGC         *GetSharedGC(void);
  
private:
  GdkGC         *mSharedGC;
};



#endif  
