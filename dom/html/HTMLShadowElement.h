





#ifndef mozilla_dom_HTMLShadowElement_h__
#define mozilla_dom_HTMLShadowElement_h__

#include "nsGenericHTMLElement.h"

namespace mozilla {
namespace dom {

class HTMLShadowElement final : public nsGenericHTMLElement,
                                public nsStubMutationObserver
{
public:
  explicit HTMLShadowElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);

  NS_IMPL_FROMCONTENT_HTML_WITH_TAG(HTMLShadowElement, shadow)

  
  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(HTMLShadowElement,
                                           nsGenericHTMLElement)

  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const override;

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers) override;

  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true) override;

  bool IsInsertionPoint() { return mIsInsertionPoint; }

  



  void SetProjectedShadow(ShadowRoot* aProjectedShadow);

  



  void DistributeSingleNode(nsIContent* aContent);

  



  void RemoveDistributedNode(nsIContent* aContent);

  




  void DistributeAllNodes();

  
  ShadowRoot* GetOlderShadowRoot() { return mProjectedShadow; }

protected:
  virtual ~HTMLShadowElement();

  virtual JSObject* WrapNode(JSContext *aCx, JS::Handle<JSObject*> aGivenProto) override;

  
  nsRefPtr<ShadowRoot> mProjectedShadow;

  bool mIsInsertionPoint;
};

} 
} 

#endif 

