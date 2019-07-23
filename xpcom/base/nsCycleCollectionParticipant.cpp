




































#include "nsCycleCollectionParticipant.h"
#include "nsCOMPtr.h"

NS_IMETHODIMP
nsXPCOMCycleCollectionParticipant::Root(void *p)
{
    nsISupports *s = static_cast<nsISupports*>(p);
    NS_ADDREF(s);
    return NS_OK;
}

NS_IMETHODIMP
nsXPCOMCycleCollectionParticipant::Unlink(void *p)
{
  return NS_OK;
}

NS_IMETHODIMP
nsXPCOMCycleCollectionParticipant::Unroot(void *p)
{
    nsISupports *s = static_cast<nsISupports*>(p);
    NS_RELEASE(s);
    return NS_OK;
}

NS_IMETHODIMP 
nsXPCOMCycleCollectionParticipant::Traverse
    (void *p, nsCycleCollectionTraversalCallback &cb)
{
  return NS_OK;
}

NS_IMETHODIMP_(void)
nsXPCOMCycleCollectionParticipant::UnmarkPurple(nsISupports *n)
{
}

#ifdef DEBUG
PRBool
nsXPCOMCycleCollectionParticipant::CheckForRightISupports(nsISupports *s)
{
    nsCOMPtr<nsISupports> foo;
    s->QueryInterface(NS_GET_IID(nsCycleCollectionISupports),
                      getter_AddRefs(foo));
    return s == foo;
}
#endif
