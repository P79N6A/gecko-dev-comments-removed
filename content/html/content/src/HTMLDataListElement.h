



#ifndef HTMLDataListElement_h___
#define HTMLDataListElement_h___

#include "mozilla/Attributes.h"
#include "nsGenericHTMLElement.h"
#include "nsContentList.h"

namespace mozilla {
namespace dom {

class HTMLDataListElement MOZ_FINAL : public nsGenericHTMLElement,
                                      public nsIDOMHTMLElement
{
public:
  HTMLDataListElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsGenericHTMLElement(aNodeInfo)
  {
  }
  virtual ~HTMLDataListElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  nsContentList* Options()
  {
    if (!mOptions) {
      mOptions = new nsContentList(this, MatchOptions, nullptr, nullptr, true);
    }

    return mOptions;
  }


  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

  
  static bool MatchOptions(nsIContent* aContent, int32_t aNamespaceID,
                             nsIAtom* aAtom, void* aData);

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(HTMLDataListElement,
                                           nsGenericHTMLElement)

  virtual nsIDOMNode* AsDOMNode() MOZ_OVERRIDE { return this; }
protected:
  virtual JSObject* WrapNode(JSContext *aCx,
                             JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  
  nsRefPtr<nsContentList> mOptions;
};

} 
} 

#endif 
