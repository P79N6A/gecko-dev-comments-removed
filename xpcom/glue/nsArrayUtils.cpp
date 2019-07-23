





































#include "nsArrayUtils.h"




nsresult
nsQueryArrayElementAt::operator()(const nsIID& aIID, void** aResult) const
  {
    nsresult status = mArray
        ? mArray->QueryElementAt(mIndex, aIID, aResult)
        : NS_ERROR_NULL_POINTER;

    if (mErrorPtr)
      *mErrorPtr = status;

    return status;
  }
