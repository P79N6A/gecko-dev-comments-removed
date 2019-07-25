






































#ifndef nsDOMStringMap_h
#define nsDOMStringMap_h

#include "nsIDOMDOMStringMap.h"

#include "nsCycleCollectionParticipant.h"
#include "nsAutoPtr.h"
#include "nsTArray.h"
#include "nsString.h"

class nsGenericHTMLElement;

class nsDOMStringMap : public nsIDOMDOMStringMap
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIDOMDOMSTRINGMAP
  NS_DECL_CYCLE_COLLECTION_CLASS(nsDOMStringMap)

  nsDOMStringMap(nsGenericHTMLElement* aElement);

  
  
  
  nsresult GetDataPropList(nsTArray<nsString>& aResult);

  nsresult RemovePropInternal(nsIAtom* aAttr);
  nsGenericHTMLElement* GetElement();

private:
  virtual ~nsDOMStringMap();

protected:
  nsRefPtr<nsGenericHTMLElement> mElement;
  
  PRBool mRemovingProp;
  PRBool DataPropToAttr(const nsAString& aProp, nsAString& aResult);
  PRBool AttrToDataProp(const nsAString& aAttr, nsAString& aResult);
};

#endif
