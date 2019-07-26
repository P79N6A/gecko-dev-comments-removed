



#ifndef HTMLDataListElement_h___
#define HTMLDataListElement_h___

#include "nsGenericHTMLElement.h"
#include "nsIDOMHTMLDataListElement.h"
#include "nsContentList.h"

namespace mozilla {
namespace dom {

class HTMLDataListElement : public nsGenericHTMLElement,
                            public nsIDOMHTMLDataListElement
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

  
  NS_DECL_NSIDOMHTMLDATALISTELEMENT

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  
  static bool MatchOptions(nsIContent* aContent, int32_t aNamespaceID,
                             nsIAtom* aAtom, void* aData);

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(HTMLDataListElement,
                                           nsGenericHTMLElement)

  virtual nsXPCClassInfo* GetClassInfo();
  virtual nsIDOMNode* AsDOMNode() { return this; }
protected:

  
  nsRefPtr<nsContentList> mOptions;
};

} 
} 

#endif 
