






#include "nsSHTransaction.h"
#include "nsISHEntry.h"





nsSHTransaction::nsSHTransaction() : mPersist(true), mPrev(nullptr) 
{
}


nsSHTransaction::~nsSHTransaction()
{
}





NS_IMPL_ADDREF(nsSHTransaction)
NS_IMPL_RELEASE(nsSHTransaction)

NS_INTERFACE_MAP_BEGIN(nsSHTransaction)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsISHTransaction)
   NS_INTERFACE_MAP_ENTRY(nsISHTransaction)
NS_INTERFACE_MAP_END





NS_IMETHODIMP
nsSHTransaction::Create(nsISHEntry* aSHEntry, nsISHTransaction* aPrev)
{
   SetSHEntry(aSHEntry);
	if(aPrev)
      aPrev->SetNext(this);

   SetPrev(aPrev);
	return NS_OK;
}

NS_IMETHODIMP
nsSHTransaction::GetSHEntry(nsISHEntry ** aResult)
{
   NS_ENSURE_ARG_POINTER(aResult);
	*aResult = mSHEntry;
	NS_IF_ADDREF(*aResult);
	return NS_OK;
}


NS_IMETHODIMP
nsSHTransaction::SetSHEntry(nsISHEntry * aSHEntry)
{
	mSHEntry = aSHEntry;
	return NS_OK;
}


NS_IMETHODIMP
nsSHTransaction::GetNext(nsISHTransaction * * aResult)
{
   NS_ENSURE_ARG_POINTER(aResult);
   *aResult = mNext;
   NS_IF_ADDREF(*aResult);
   return NS_OK;
}


NS_IMETHODIMP
nsSHTransaction::SetNext(nsISHTransaction * aNext)
{
  if (aNext) {
    NS_ENSURE_SUCCESS(aNext->SetPrev(this), NS_ERROR_FAILURE);
  }

  mNext = aNext;
  return NS_OK;
}

NS_IMETHODIMP
nsSHTransaction::SetPrev(nsISHTransaction * aPrev)
{
	
     mPrev = aPrev;
	 return NS_OK;
}

nsresult
nsSHTransaction::GetPrev(nsISHTransaction ** aResult)
{
   NS_ENSURE_ARG_POINTER(aResult);
   *aResult  = mPrev;
   NS_IF_ADDREF(*aResult);
   return NS_OK;
}

NS_IMETHODIMP
nsSHTransaction::SetPersist(bool aPersist)
{
   mPersist = aPersist;
   return NS_OK;
}

NS_IMETHODIMP
nsSHTransaction::GetPersist(bool* aPersist)
{
   NS_ENSURE_ARG_POINTER(aPersist);

   *aPersist = mPersist;
   return NS_OK;
}
