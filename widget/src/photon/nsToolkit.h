




































#ifndef TOOLKIT_H      
#define TOOLKIT_H

#include <Pt.h>
#include "nsIToolkit.h"

class nsToolkit : public nsIToolkit
{

public:
  nsToolkit();
  virtual ~nsToolkit();

  NS_DECL_ISUPPORTS

  NS_IMETHOD   Init( PRThread *aThread );
};


#endif  
