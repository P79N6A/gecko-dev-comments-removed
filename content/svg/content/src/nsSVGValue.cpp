





































#include "nsSVGValue.h"

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
  nsWeakPtr wr = do_GetWeakReference(observer);
  if (!wr) return NS_ERROR_FAILURE;

  
  
  
  
  
  if (!mObservers.Contains(wr)) {
    mObservers.AppendElement(wr);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsSVGValue::RemoveObserver(nsISVGValueObserver* observer)
{
  nsWeakPtr wr = do_GetWeakReference(observer);
  if (!wr) return NS_ERROR_FAILURE;
  return mObservers.RemoveElement(wr) ? NS_OK : NS_ERROR_FAILURE;
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

  
