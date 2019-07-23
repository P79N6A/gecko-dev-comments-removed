





































#include "nsDirIndex.h"
#include "nsReadableUtils.h"
#include "nsCRT.h"
#include "nsISupportsObsolete.h"

NS_IMPL_THREADSAFE_ISUPPORTS1(nsDirIndex,
                              nsIDirIndex)

nsDirIndex::nsDirIndex() : mType(TYPE_UNKNOWN),
                           mSize(LL_MAXUINT),
                           mLastModified(-1) {
}

nsDirIndex::~nsDirIndex() {}

NS_IMPL_GETSET(nsDirIndex, Type, PRUint32, mType)



NS_IMETHODIMP
nsDirIndex::GetContentType(char* *aContentType) {
  *aContentType = ToNewCString(mContentType);
  if (!*aContentType)
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}

NS_IMETHODIMP
nsDirIndex::SetContentType(const char* aContentType) {
  mContentType = aContentType;
  return NS_OK;
}

NS_IMETHODIMP
nsDirIndex::GetLocation(char* *aLocation) {
  *aLocation = ToNewCString(mLocation);
  if (!*aLocation)
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}

NS_IMETHODIMP
nsDirIndex::SetLocation(const char* aLocation) {
  mLocation = aLocation;
  return NS_OK;
}

NS_IMETHODIMP
nsDirIndex::GetDescription(PRUnichar* *aDescription) {
  *aDescription = ToNewUnicode(mDescription);
  if (!*aDescription)
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}

NS_IMETHODIMP
nsDirIndex::SetDescription(const PRUnichar* aDescription) {
  mDescription.Assign(aDescription);
  return NS_OK;
}

NS_IMPL_GETSET(nsDirIndex, Size, PRInt64, mSize)
NS_IMPL_GETSET(nsDirIndex, LastModified, PRInt64, mLastModified)

