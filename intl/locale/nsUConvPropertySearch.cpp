



#include "nsUConvPropertySearch.h"
#include "nsCRT.h"
#include "nsString.h"
#include "mozilla/BinarySearch.h"

namespace {

struct PropertyComparator
{
  const nsCString& mKey;
  PropertyComparator(const nsCString& aKey) : mKey(aKey) {}
  int operator()(const char* (&aProperty)[3]) const {
    return mKey.Compare(aProperty[0]);
  }
};

}


nsresult
nsUConvPropertySearch::SearchPropertyValue(const char* aProperties[][3],
                                           int32_t aNumberOfProperties,
                                           const nsACString& aKey,
                                           nsACString& aValue)
{
  using mozilla::BinarySearchIf;

  const nsCString& flat = PromiseFlatCString(aKey);
  size_t index;
  if (BinarySearchIf(aProperties, 0, aNumberOfProperties,
                     PropertyComparator(flat), &index)) {
    nsDependentCString val(aProperties[index][1],
                           NS_PTR_TO_UINT32(aProperties[index][2]));
    aValue.Assign(val);
    return NS_OK;
  }

  aValue.Truncate();
  return NS_ERROR_FAILURE;
}
