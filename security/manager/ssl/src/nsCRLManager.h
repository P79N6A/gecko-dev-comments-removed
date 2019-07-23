



































#ifndef _NSCRLMANAGER_H_
#define _NSCRLMANAGER_H_

#include "nsICRLManager.h"

class nsCRLManager : public nsICRLManager
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICRLMANAGER
  
  nsCRLManager();
  virtual ~nsCRLManager();
};

#endif
