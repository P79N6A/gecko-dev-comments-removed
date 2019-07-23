



































#include "nsJavaInterfaces.h"


void XXXNeverCalled_javaxpcom()
{
  MOZILLA_NATIVE(initialize) (nsnull, nsnull);

  GRE_NATIVE(initEmbedding) (nsnull, nsnull, nsnull, nsnull, nsnull);

  GRE_NATIVE(termEmbedding) (nsnull, nsnull);

  GRE_NATIVE(lockProfileDirectory) (nsnull, nsnull, nsnull);

  GRE_NATIVE(notifyProfile) (nsnull, nsnull);

  GRE_NATIVE(lockProfileDirectory) (nsnull, nsnull, nsnull);

  GRE_NATIVE(notifyProfile) (nsnull, nsnull);

  XPCOM_NATIVE(initXPCOM) (nsnull, nsnull, nsnull, nsnull);

  XPCOM_NATIVE(shutdownXPCOM) (nsnull, nsnull, nsnull);

  XPCOM_NATIVE(newLocalFile) (nsnull, nsnull, nsnull, nsnull);

  XPCOM_NATIVE(getComponentManager) (nsnull, nsnull);

  XPCOM_NATIVE(getComponentRegistrar) (nsnull, nsnull);

  XPCOM_NATIVE(getServiceManager) (nsnull, nsnull);

  JAVAPROXY_NATIVE(callXPCOMMethod) (nsnull, nsnull, nsnull, nsnull, nsnull);

  JAVAPROXY_NATIVE(finalizeProxy) (nsnull, nsnull, nsnull);

  JAVAPROXY_NATIVE(isSameXPCOMObject) (nsnull, nsnull, nsnull, nsnull);

  LOCKPROXY_NATIVE(release) (nsnull, nsnull, nsnull);

  MOZILLA_NATIVE(getNativeHandleFromAWT) (nsnull, nsnull, nsnull);

  JXUTILS_NATIVE(wrapJavaObject) (nsnull, nsnull, nsnull, nsnull);

  JXUTILS_NATIVE(wrapXPCOMObject) (nsnull, nsnull, nsnull, nsnull);
}

