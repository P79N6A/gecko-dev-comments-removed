




































#include "nsIVersionComparator.h"

class nsVersionComparatorImpl : public nsIVersionComparator
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIVERSIONCOMPARATOR
};

#define NS_VERSIONCOMPARATOR_CONTRACTID "@mozilla.org/xpcom/version-comparator;1"


#define NS_VERSIONCOMPARATOR_CID \
{ 0xc6e47036, 0xca94, 0x4be3, \
  { 0x96, 0x3a, 0x9a, 0xbd, 0x87, 0x05, 0xf7, 0xa8 } }

#define NS_VERSIONCOMPARATOR_CLASSNAME "nsVersionComparatorImpl"
