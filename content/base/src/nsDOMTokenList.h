







#ifndef nsDOMTokenList_h___
#define nsDOMTokenList_h___

#include "nsIDOMDOMTokenList.h"
#include "nsCOMPtr.h"
#include "nsDOMString.h"
#include "nsWrapperCache.h"

namespace mozilla {
class ErrorResult;

namespace dom {
class Element;
} 
} 

class nsAttrValue;
class nsIAtom;



class nsDOMTokenList : public nsIDOMDOMTokenList,
                       public nsWrapperCache
{
  typedef mozilla::dom::Element Element;

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(nsDOMTokenList)
  NS_DECL_NSIDOMDOMTOKENLIST

  nsDOMTokenList(Element* aElement, nsIAtom* aAttrAtom);

  void DropReference();

  virtual JSObject* WrapObject(JSContext *cx, JSObject *scope,
                               bool *triedToWrap);

  Element* GetParentObject()
  {
    return mElement;
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
  inline const nsAttrValue* GetParsedAttr();

  Element* mElement;
  nsCOMPtr<nsIAtom> mAttrAtom;
};

#endif 
