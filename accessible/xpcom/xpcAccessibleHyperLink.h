





#ifndef mozilla_a11y_xpcAccessibleHyperLink_h_
#define mozilla_a11y_xpcAccessibleHyperLink_h_

#include "nsIAccessibleHyperLink.h"

class nsIAccessible;

namespace mozilla {
namespace a11y {

class xpcAccessibleHyperLink : public nsIAccessibleHyperLink
{
public:
  NS_IMETHOD GetAnchorCount(int32_t* aAnchorCount) MOZ_FINAL;
  NS_IMETHOD GetStartIndex(int32_t* aStartIndex) MOZ_FINAL;
  NS_IMETHOD GetEndIndex(int32_t* aEndIndex) MOZ_FINAL;
  NS_IMETHOD GetURI(int32_t aIndex, nsIURI** aURI) MOZ_FINAL;
  NS_IMETHOD GetAnchor(int32_t aIndex, nsIAccessible** aAccessible) MOZ_FINAL;
  NS_IMETHOD GetValid(bool* aValid) MOZ_FINAL;

private:
  xpcAccessibleHyperLink() { }
  friend class Accessible;

  xpcAccessibleHyperLink(const xpcAccessibleHyperLink&) MOZ_DELETE;
  xpcAccessibleHyperLink& operator =(const xpcAccessibleHyperLink&) MOZ_DELETE;
};

} 
} 

#endif
