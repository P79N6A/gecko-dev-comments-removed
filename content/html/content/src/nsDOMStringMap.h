





#ifndef nsDOMStringMap_h
#define nsDOMStringMap_h

#include "nsCycleCollectionParticipant.h"
#include "nsAutoPtr.h"
#include "nsTArray.h"
#include "nsString.h"
#include "nsWrapperCache.h"
#include "nsGenericHTMLElement.h"

namespace mozilla {
class ErrorResult;
}

class nsDOMStringMap : public nsISupports,
                       public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(nsDOMStringMap)

  nsINode* GetParentObject()
  {
    return mElement;
  }

  nsDOMStringMap(nsGenericHTMLElement* aElement);

  
  virtual JSObject* WrapObject(JSContext *cx, JSObject *scope) MOZ_OVERRIDE;
  void NamedGetter(const nsAString& aProp, bool& found, nsString& aResult) const;
  void NamedSetter(const nsAString& aProp, const nsAString& aValue,
                   mozilla::ErrorResult& rv);
  void NamedDeleter(const nsAString& aProp, bool &found);
  void GetSupportedNames(nsTArray<nsString>& aNames);

private:
  virtual ~nsDOMStringMap();

protected:
  nsRefPtr<nsGenericHTMLElement> mElement;
  
  bool mRemovingProp;
  static bool DataPropToAttr(const nsAString& aProp, nsAString& aResult);
  static bool AttrToDataProp(const nsAString& aAttr, nsAString& aResult);
};

#endif
