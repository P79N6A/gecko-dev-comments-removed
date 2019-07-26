



#ifndef HTMLDataListElement_h___
#define HTMLDataListElement_h___

#include "mozilla/Attributes.h"
#include "nsGenericHTMLElement.h"
#include "nsContentList.h"

namespace mozilla {
namespace dom {

class HTMLDataListElement MOZ_FINAL : public nsGenericHTMLElement
{
public:
  HTMLDataListElement(already_AddRefed<nsINodeInfo>& aNodeInfo)
    : nsGenericHTMLElement(aNodeInfo)
  {
  }
  virtual ~HTMLDataListElement();

  
  NS_DECL_ISUPPORTS_INHERITED

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
protected:
  virtual JSObject* WrapNode(JSContext *aCx) MOZ_OVERRIDE;

  
  nsRefPtr<nsContentList> mOptions;
};

} 
} 

#endif 
