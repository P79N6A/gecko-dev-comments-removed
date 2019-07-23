





































#include "nsSVGValue.h"
#include "nsIWeakReference.h"

nsSVGValue::nsSVGValue()
    : mModifyNestCount(0)
{
}

nsSVGValue::~nsSVGValue()
{
  ReleaseObservers();
}

void
nsSVGValue::ReleaseObservers()
{
  PRUint32 count = mObservers.Length();
  PRUint32 i;
  for (i = 0; i < count; ++i) {
    nsIWeakReference* wr = mObservers.ElementAt(i);
    NS_RELEASE(wr);
  }
  mObservers.Clear();
}

void
nsSVGValue::NotifyObservers(SVGObserverNotifyFunction f,
                            modificationType aModType)
{
  PRInt32 count = mObservers.Length();

  
  
  
  for (PRInt32 i = count - 1; i >= 0; i--) {
    nsIWeakReference* wr = mObservers.ElementAt(i);
    nsCOMPtr<nsISVGValueObserver> observer = do_QueryReferent(wr);
    if (observer)
       (static_cast<nsISVGValueObserver*>(observer)->*f)(this, aModType);
  }
}

void
nsSVGValue::WillModify(modificationType aModType)
{
  if (++mModifyNestCount == 1)
    NotifyObservers(&nsISVGValueObserver::WillModifySVGObservable, aModType);
}

void
nsSVGValue::DidModify(modificationType aModType)
{
  NS_ASSERTION(mModifyNestCount>0, "unbalanced Will/DidModify calls");
  if (--mModifyNestCount == 0) {
    OnDidModify();
    NotifyObservers(&nsISVGValueObserver::DidModifySVGObservable, aModType);
  }
}


NS_IMETHODIMP
nsSVGValue::AddObserver(nsISVGValueObserver* observer)
{
  nsIWeakReference* wr = NS_GetWeakReference(observer);
  if (!wr) return NS_ERROR_FAILURE;

  
  
  
  
  
  if (mObservers.Contains(wr)) {
    NS_RELEASE(wr);
    return NS_OK;
  }

  mObservers.AppendElement(wr);
  return NS_OK;
}

NS_IMETHODIMP
nsSVGValue::RemoveObserver(nsISVGValueObserver* observer)
{
  nsCOMPtr<nsIWeakReference> wr = do_GetWeakReference(observer);
  if (!wr) return NS_ERROR_FAILURE;
  PRInt32 i = mObservers.IndexOf(wr);
  if (i<0) return NS_ERROR_FAILURE;
  nsIWeakReference* wr2 = mObservers.ElementAt(i);
  NS_RELEASE(wr2);
  mObservers.RemoveElementAt(i);
  return NS_OK;
}

NS_IMETHODIMP
nsSVGValue::BeginBatchUpdate()
{
  WillModify();
  return NS_OK;
}

NS_IMETHODIMP
nsSVGValue::EndBatchUpdate()
{
  DidModify();
  return NS_OK;
}

  
