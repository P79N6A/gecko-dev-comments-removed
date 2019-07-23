





































#ifndef nsProxyRelease_h_
#define nsProxyRelease_h__

#include "nsIEventTarget.h"
#include "nsCOMPtr.h"

#ifdef XPCOM_GLUE_AVOID_NSPR
#error NS_ProxyRelease implementation depends on NSPR.
#endif






template <class T>
inline NS_HIDDEN_(nsresult)
NS_ProxyRelease
    (nsIEventTarget *target, nsCOMPtr<T> &doomed, PRBool alwaysProxy=PR_FALSE)
{
   T* raw = nsnull;
   doomed.swap(raw);
   return NS_ProxyRelease(target, doomed, alwaysProxy);
}














NS_COM_GLUE nsresult
NS_ProxyRelease
    (nsIEventTarget *target, nsISupports *doomed, PRBool alwaysProxy=PR_FALSE);

#endif
