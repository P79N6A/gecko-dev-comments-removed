





#ifndef nsArrayUtils_h__
#define nsArrayUtils_h__

#include "nsCOMPtr.h"
#include "nsIArray.h"


class MOZ_STACK_CLASS nsQueryArrayElementAt final : public nsCOMPtr_helper
{
public:
  nsQueryArrayElementAt(nsIArray* aArray, uint32_t aIndex,
                        nsresult* aErrorPtr)
    : mArray(aArray)
    , mIndex(aIndex)
    , mErrorPtr(aErrorPtr)
  {
  }

  virtual nsresult NS_FASTCALL operator()(const nsIID& aIID, void**) const
    override;

private:
  nsIArray* MOZ_NON_OWNING_REF mArray;
  uint32_t   mIndex;
  nsresult*  mErrorPtr;
};

inline const nsQueryArrayElementAt
do_QueryElementAt(nsIArray* aArray, uint32_t aIndex, nsresult* aErrorPtr = 0)
{
  return nsQueryArrayElementAt(aArray, aIndex, aErrorPtr);
}

#endif 
