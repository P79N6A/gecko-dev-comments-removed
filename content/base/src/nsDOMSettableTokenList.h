







































#ifndef nsDOMSettableTokenList_h___
#define nsDOMSettableTokenList_h___

#include "nsIDOMDOMSettableTokenList.h"
#include "nsDOMTokenList.h"


class nsGenericElement;
class nsIAtom;

class nsDOMSettableTokenList : public nsDOMTokenList,
                               public nsIDOMDOMSettableTokenList
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMDOMSETTABLETOKENLIST

  NS_FORWARD_NSIDOMDOMTOKENLIST(nsDOMTokenList::);

  nsDOMSettableTokenList(nsGenericElement* aElement, nsIAtom* aAttrAtom);

protected:
  ~nsDOMSettableTokenList();
};

#endif 

