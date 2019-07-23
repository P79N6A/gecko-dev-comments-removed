





































#ifndef __NSDOMWORKERNAVIGATOR_H__
#define __NSDOMWORKERNAVIGATOR_H__

#include "nsIClassInfo.h"
#include "nsIDOMWorkers.h"
#include "nsIXPCScriptable.h"

class nsDOMWorkerNavigator : public nsIWorkerNavigator,
                             public nsIClassInfo,
                             public nsIXPCScriptable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIWORKERNAVIGATOR
  NS_DECL_NSICLASSINFO
  NS_DECL_NSIXPCSCRIPTABLE
};

#endif
