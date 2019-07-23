



































#include "ojiapitests.h"

#include <nsIJVMManager.h>
#include <nsILiveConnectManager.h>
#include <nsIServiceManager.h>

static NS_DEFINE_IID(kJVMManagerCID, NS_JVMMANAGER_CID);
static NS_DEFINE_IID(kILiveConnectManagerIID, NS_ILIVECONNECTMANAGER_IID);

extern nsresult GetLiveConnectManager(nsILiveConnectManager** lc);