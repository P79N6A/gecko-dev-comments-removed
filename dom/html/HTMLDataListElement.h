




#ifndef HTMLDataListElement_h___
#define HTMLDataListElement_h___

#include "mozilla/Attributes.h"
#include "nsGenericHTMLElement.h"
#include "nsContentList.h"

namespace mozilla {
namespace dom {

class HTMLDataListElement final : public nsGenericHTMLElement
{
public:
  explicit HTMLDataListElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
    : nsGenericHTMLElement(aNodeInfo)
  {
  }

  
  NS_DECL_ISUPPORTS_INHERITED

  nsContentList* Options()
  {
    if (!mOptions) {
      mOptions = new nsContentList(this, MatchOptions, nullptr, nullptr, true);
    }

    return mOptions;
  }


  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const override;

  
  static bool MatchOptions(nsIContent* aContent, int32_t aNamespaceID,
                             nsIAtom* aAtom, void* aData);

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(HTMLDataListElement,
                                           nsGenericHTMLElement)
protected:
  virtual ~HTMLDataListElement();

  virtual JSObject* WrapNode(JSContext *aCx, JS::Handle<JSObject*> aGivenProto) override;

  
  nsRefPtr<nsContentList> mOptions;
};

} 
} 

#endif 
