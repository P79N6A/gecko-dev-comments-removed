




































#include "nsCycleCollectionParticipant.h"
#include "nsCOMPtr.h"

nsresult
nsXPCOMCycleCollectionParticipant::Root(void *p)
{
    nsISupports *s = static_cast<nsISupports*>(p);
    NS_ADDREF(s);
    return NS_OK;
}

nsresult
nsXPCOMCycleCollectionParticipant::Unlink(void *p)
{
  return NS_OK;
}

nsresult
nsXPCOMCycleCollectionParticipant::Unroot(void *p)
{
    nsISupports *s = static_cast<nsISupports*>(p);
    NS_RELEASE(s);
    return NS_OK;
}

nsresult
nsXPCOMCycleCollectionParticipant::Traverse
    (void *p, nsCycleCollectionTraversalCallback &cb)
{
  return NS_OK;
}

void
nsXPCOMCycleCollectionParticipant::UnmarkPurple(nsISupports *n)
{
}

PRBool
nsXPCOMCycleCollectionParticipant::CheckForRightISupports(nsISupports *s)
{
    nsCOMPtr<nsISupports> foo;
    s->QueryInterface(NS_GET_IID(nsCycleCollectionISupports),
                      getter_AddRefs(foo));
    return s == foo;
}
