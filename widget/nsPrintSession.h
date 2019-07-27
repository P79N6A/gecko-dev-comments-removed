




#ifndef nsPrintSession_h__
#define nsPrintSession_h__

#include "nsIPrintSession.h" 
#include "nsWeakReference.h"
#include "gfxCore.h"





class nsPrintSession : public nsIPrintSession,
                       public nsSupportsWeakReference
{
  virtual ~nsPrintSession();

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPRINTSESSION

  nsPrintSession();
  
  virtual nsresult Init();
};

#endif 
