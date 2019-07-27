





#ifndef nsDOMStringMap_h
#define nsDOMStringMap_h

#include "nsCycleCollectionParticipant.h"
#include "nsAutoPtr.h"
#include "nsTArray.h"
#include "nsString.h"
#include "nsWrapperCache.h"
#include "nsGenericHTMLElement.h"
#include "jsfriendapi.h" 

namespace mozilla {
class ErrorResult;
}

class nsDOMStringMap : public nsStubMutationObserver,
                       public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(nsDOMStringMap)

  NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTECHANGED

  nsINode* GetParentObject()
  {
    return mElement;
  }

  explicit nsDOMStringMap(nsGenericHTMLElement* aElement);

  
  virtual JSObject* WrapObject(JSContext *cx) MOZ_OVERRIDE;
  void NamedGetter(const nsAString& aProp, bool& found,
                   mozilla::dom::DOMString& aResult) const;
  void NamedSetter(const nsAString& aProp, const nsAString& aValue,
                   mozilla::ErrorResult& rv);
  void NamedDeleter(const nsAString& aProp, bool &found);
  bool NameIsEnumerable(const nsAString& aName);
  void GetSupportedNames(unsigned, nsTArray<nsString>& aNames);

  js::ExpandoAndGeneration mExpandoAndGeneration;

private:
  virtual ~nsDOMStringMap();

protected:
  nsRefPtr<nsGenericHTMLElement> mElement;
  
  bool mRemovingProp;
  static bool DataPropToAttr(const nsAString& aProp, nsAutoString& aResult);
  static bool AttrToDataProp(const nsAString& aAttr, nsAutoString& aResult);
};

#endif
