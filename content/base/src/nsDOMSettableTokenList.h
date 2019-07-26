







#ifndef nsDOMSettableTokenList_h___
#define nsDOMSettableTokenList_h___

#include "nsIDOMDOMSettableTokenList.h"
#include "nsDOMTokenList.h"

namespace mozilla {
namespace dom {
class Element;
} 
} 

class nsIAtom;



class nsDOMSettableTokenList : public nsDOMTokenList,
                               public nsIDOMDOMSettableTokenList
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMDOMSETTABLETOKENLIST

  NS_FORWARD_NSIDOMDOMTOKENLIST(nsDOMTokenList::);

  nsDOMSettableTokenList(mozilla::dom::Element* aElement, nsIAtom* aAttrAtom);

  virtual JSObject* WrapObject(JSContext *cx, JSObject *scope,
                               bool *triedToWrap);

  virtual ~nsDOMSettableTokenList();
};

#endif 

