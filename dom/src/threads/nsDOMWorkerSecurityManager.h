





































#ifndef __NSDOMWORKERSECURITYMANAGER_H__
#define __NSDOMWORKERSECURITYMANAGER_H__

#include "nsIXPCSecurityManager.h"

class nsDOMWorkerSecurityManager : public nsIXPCSecurityManager
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIXPCSECURITYMANAGER
};

#endif
