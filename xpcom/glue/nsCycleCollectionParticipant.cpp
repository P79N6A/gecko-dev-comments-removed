




































#include "nsCycleCollectionParticipant.h"
#include "nsCOMPtr.h"

static void
NoteChild(PRUint32 aLangID, void *aScriptThing, const char *name,
          void *aClosure)
{
  nsCycleCollectionTraversalCallback *cb =
    static_cast<nsCycleCollectionTraversalCallback*>(aClosure);
  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*cb, name);
  cb->NoteScriptChild(aLangID, aScriptThing);
}

void
nsScriptObjectTracer::TraverseScriptObjects(void *p,
                                        nsCycleCollectionTraversalCallback &cb)
{
  Trace(p, NoteChild, &cb);
}

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

NS_IMETHODIMP_(void)
nsXPCOMCycleCollectionParticipant::Trace(void *p, TraceCallback cb,
                                         void *closure)
{
}

PRBool
nsXPCOMCycleCollectionParticipant::CheckForRightISupports(nsISupports *s)
{
    nsISupports* foo;
    s->QueryInterface(NS_GET_IID(nsCycleCollectionISupports),
                      reinterpret_cast<void**>(&foo));
    return s == foo;
}
