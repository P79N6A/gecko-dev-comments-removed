





#include "nsCOMPtr.h"

nsresult
nsQueryInterface::operator()(const nsIID& aIID, void** aAnswer) const
{
  nsresult status;
  if (mRawPtr) {
    status = mRawPtr->QueryInterface(aIID, aAnswer);
#ifdef NSCAP_FEATURE_TEST_NONNULL_QUERY_SUCCEEDS
    NS_ASSERTION(NS_SUCCEEDED(status),
                 "interface not found---were you expecting that?");
#endif
  } else {
    status = NS_ERROR_NULL_POINTER;
  }

  return status;
}

nsresult
nsQueryInterfaceWithError::operator()(const nsIID& aIID, void** aAnswer) const
{
  nsresult status;
  if (mRawPtr) {
    status = mRawPtr->QueryInterface(aIID, aAnswer);
#ifdef NSCAP_FEATURE_TEST_NONNULL_QUERY_SUCCEEDS
    NS_ASSERTION(NS_SUCCEEDED(status),
                 "interface not found---were you expecting that?");
#endif
  } else {
    status = NS_ERROR_NULL_POINTER;
  }

  if (mErrorPtr) {
    *mErrorPtr = status;
  }
  return status;
}

void
nsCOMPtr_base::assign_with_AddRef(nsISupports* aRawPtr)
{
  if (aRawPtr) {
    NSCAP_ADDREF(this, aRawPtr);
  }
  assign_assuming_AddRef(aRawPtr);
}

void
nsCOMPtr_base::assign_from_qi(const nsQueryInterface aQI, const nsIID& aIID)
{
  void* newRawPtr;
  if (NS_FAILED(aQI(aIID, &newRawPtr))) {
    newRawPtr = 0;
  }
  assign_assuming_AddRef(static_cast<nsISupports*>(newRawPtr));
}

void
nsCOMPtr_base::assign_from_qi_with_error(const nsQueryInterfaceWithError& aQI,
                                         const nsIID& aIID)
{
  void* newRawPtr;
  if (NS_FAILED(aQI(aIID, &newRawPtr))) {
    newRawPtr = 0;
  }
  assign_assuming_AddRef(static_cast<nsISupports*>(newRawPtr));
}

void
nsCOMPtr_base::assign_from_gs_cid(const nsGetServiceByCID aGS,
                                  const nsIID& aIID)
{
  void* newRawPtr;
  if (NS_FAILED(aGS(aIID, &newRawPtr))) {
    newRawPtr = 0;
  }
  assign_assuming_AddRef(static_cast<nsISupports*>(newRawPtr));
}

void
nsCOMPtr_base::assign_from_gs_cid_with_error(
    const nsGetServiceByCIDWithError& aGS, const nsIID& aIID)
{
  void* newRawPtr;
  if (NS_FAILED(aGS(aIID, &newRawPtr))) {
    newRawPtr = 0;
  }
  assign_assuming_AddRef(static_cast<nsISupports*>(newRawPtr));
}

void
nsCOMPtr_base::assign_from_gs_contractid(const nsGetServiceByContractID aGS,
                                         const nsIID& aIID)
{
  void* newRawPtr;
  if (NS_FAILED(aGS(aIID, &newRawPtr))) {
    newRawPtr = 0;
  }
  assign_assuming_AddRef(static_cast<nsISupports*>(newRawPtr));
}

void
nsCOMPtr_base::assign_from_gs_contractid_with_error(
    const nsGetServiceByContractIDWithError& aGS, const nsIID& aIID)
{
  void* newRawPtr;
  if (NS_FAILED(aGS(aIID, &newRawPtr))) {
    newRawPtr = 0;
  }
  assign_assuming_AddRef(static_cast<nsISupports*>(newRawPtr));
}

void
nsCOMPtr_base::assign_from_helper(const nsCOMPtr_helper& aHelper,
                                  const nsIID& aIID)
{
  void* newRawPtr;
  if (NS_FAILED(aHelper(aIID, &newRawPtr))) {
    newRawPtr = 0;
  }
  assign_assuming_AddRef(static_cast<nsISupports*>(newRawPtr));
}

void**
nsCOMPtr_base::begin_assignment()
{
  assign_assuming_AddRef(0);
  return reinterpret_cast<void**>(&mRawPtr);
}
