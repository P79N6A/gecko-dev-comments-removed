







#ifndef nsDOMTokenList_h___
#define nsDOMTokenList_h___

#include "nsGenericElement.h"
#include "nsIDOMDOMTokenList.h"

namespace mozilla {
class ErrorResult;
}
class nsAttrValue;



class nsDOMTokenList : public nsIDOMDOMTokenList,
                       public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(nsDOMTokenList)
  NS_DECL_NSIDOMDOMTOKENLIST

  nsDOMTokenList(nsGenericElement* aElement, nsIAtom* aAttrAtom);

  void DropReference();

  virtual JSObject* WrapObject(JSContext *cx, JSObject *scope,
                               bool *triedToWrap);

  nsINode *GetParentObject()
  {
    return mElement;
  }

  const nsAttrValue* GetParsedAttr() {
    if (!mElement) {
      return nullptr;
    }
    return mElement->GetAttrInfo(kNameSpaceID_None, mAttrAtom).mValue;
  }

  uint32_t Length();
  void Item(uint32_t aIndex, nsAString& aResult)
  {
    bool found;
    IndexedGetter(aIndex, found, aResult);
    if (!found) {
      SetDOMStringToNull(aResult);
    }
  }
  void IndexedGetter(uint32_t aIndex, bool& aFound, nsAString& aResult);
  bool Contains(const nsAString& aToken, mozilla::ErrorResult& aError);
  void Add(const nsAString& aToken, mozilla::ErrorResult& aError);
  void Remove(const nsAString& aToken, mozilla::ErrorResult& aError);
  bool Toggle(const nsAString& aToken, mozilla::ErrorResult& aError);
  void Stringify(nsAString& aResult);

protected:
  virtual ~nsDOMTokenList();

  nsresult CheckToken(const nsAString& aStr);
  void AddInternal(const nsAttrValue* aAttr, const nsAString& aToken);
  void RemoveInternal(const nsAttrValue* aAttr, const nsAString& aToken);

  nsGenericElement* mElement;
  nsCOMPtr<nsIAtom> mAttrAtom;
};

#endif 
