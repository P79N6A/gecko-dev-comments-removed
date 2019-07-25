







































#ifndef nsDOMTokenList_h___
#define nsDOMTokenList_h___

#include "nsGenericElement.h"
#include "nsIDOMDOMTokenList.h"

class nsAttrValue;

class nsDOMTokenList : public nsIDOMDOMTokenList
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMDOMTOKENLIST

  nsDOMTokenList(nsGenericElement* aElement, nsIAtom* aAttrAtom);

  void DropReference();

protected:
  ~nsDOMTokenList();

  const nsAttrValue* GetParsedAttr() {
    if (!mElement) {
      return nsnull;
    }
    return mElement->GetAttrInfo(kNameSpaceID_None, mAttrAtom).mValue;
  }

  nsresult CheckToken(const nsAString& aStr);
  void AddInternal(const nsAttrValue* aAttr, const nsAString& aToken);
  void RemoveInternal(const nsAttrValue* aAttr, const nsAString& aToken);

  nsGenericElement* mElement;
  nsCOMPtr<nsIAtom> mAttrAtom;
};

#endif 
