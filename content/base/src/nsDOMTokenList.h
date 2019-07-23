







































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

private:
  ~nsDOMTokenList();

  const nsAttrValue* GetParsedAttr() {
    if (!mElement) {
      return nsnull;
    }
    return mElement->GetParsedAttr(mAttrAtom);
  }

  nsresult CheckToken(const nsAString& aStr);
  PRBool ContainsInternal(const nsAttrValue* aAttr, const nsAString& aToken);
  void AddInternal(const nsAttrValue* aAttr, const nsAString& aToken);
  void RemoveInternal(const nsAttrValue* aAttr, const nsAString& aToken);

  nsGenericElement* mElement;
  nsCOMPtr<nsIAtom> mAttrAtom;
};

#endif 
