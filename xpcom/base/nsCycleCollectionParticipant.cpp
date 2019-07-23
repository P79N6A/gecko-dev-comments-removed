




































#include "nsCycleCollectionParticipant.h"
#include "nsCOMPtr.h"

NS_INTERFACE_MAP_BEGIN(nsCycleCollectionParticipant)
  NS_INTERFACE_MAP_ENTRY(nsCycleCollectionParticipant)
NS_INTERFACE_MAP_END

NS_IMETHODIMP_(nsrefcnt) nsCycleCollectionParticipant::AddRef(void)
{
  
  return 1;
}

NS_IMETHODIMP_(nsrefcnt) nsCycleCollectionParticipant::Release(void)
{
  
  return 1;
}

NS_IMETHODIMP nsCycleCollectionParticipant::Unlink(nsISupports *n)
{
  return NS_OK;
}

NS_IMETHODIMP 
nsCycleCollectionParticipant::Traverse(nsISupports *n, 
                                       nsCycleCollectionTraversalCallback &cb)
{
  return NS_OK;
}

NS_IMETHODIMP_(void) nsCycleCollectionParticipant::UnmarkPurple(nsISupports *n)
{
}

#ifdef DEBUG
PRBool
nsCycleCollectionParticipant::CheckForRightISupports(nsISupports *s)
{
    nsCOMPtr<nsISupports> foo;
    s->QueryInterface(NS_GET_IID(nsCycleCollectionISupports),
                      getter_AddRefs(foo));
    return s == foo;
}
#endif
