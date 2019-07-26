





#ifndef nsDOMStringMap_h
#define nsDOMStringMap_h

#include "nsIDOMDOMStringMap.h"

#include "nsCycleCollectionParticipant.h"
#include "nsAutoPtr.h"
#include "nsTArray.h"
#include "nsString.h"
#include "nsWrapperCache.h"
#include "nsGenericHTMLElement.h"

class nsDOMStringMap : public nsIDOMDOMStringMap,
                       public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIDOMDOMSTRINGMAP
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(nsDOMStringMap)

  nsINode* GetParentObject()
  {
    return mElement;
  }

  static nsDOMStringMap* FromSupports(nsISupports* aSupports)
  {
    nsIDOMDOMStringMap* map =
      static_cast<nsDOMStringMap*>(aSupports);
#ifdef DEBUG
    {
      nsCOMPtr<nsIDOMDOMStringMap> map_qi =
        do_QueryInterface(aSupports);

      
      
      
      NS_ASSERTION(map_qi == map, "Uh, fix QI!");
    }
#endif

    return static_cast<nsDOMStringMap*>(map);
  }

  
  nsDOMStringMap(nsGenericHTMLElement* aElement);

  
  
  
  nsresult GetDataPropList(nsTArray<nsString>& aResult);

  nsresult RemovePropInternal(nsIAtom* aAttr);
  nsGenericHTMLElement* GetElement();

private:
  virtual ~nsDOMStringMap();

protected:
  nsRefPtr<nsGenericHTMLElement> mElement;
  
  bool mRemovingProp;
  bool DataPropToAttr(const nsAString& aProp, nsAString& aResult);
  bool AttrToDataProp(const nsAString& aAttr, nsAString& aResult);
};

#endif
