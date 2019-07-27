




#ifndef mozilla_dom_HTMLShadowElement_h__
#define mozilla_dom_HTMLShadowElement_h__

#include "nsGenericHTMLElement.h"

namespace mozilla {
namespace dom {

class HTMLShadowElement MOZ_FINAL : public nsGenericHTMLElement,
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

  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers);

  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true);

  bool IsInsertionPoint() { return mIsInsertionPoint; }

  



  void SetProjectedShadow(ShadowRoot* aProjectedShadow);

  



  void DistributeSingleNode(nsIContent* aContent);

  



  void RemoveDistributedNode(nsIContent* aContent);

  




  void DistributeAllNodes();

  
  ShadowRoot* GetOlderShadowRoot() { return mProjectedShadow; }

protected:
  virtual ~HTMLShadowElement();

  virtual JSObject* WrapNode(JSContext *aCx) MOZ_OVERRIDE;

  
  nsRefPtr<ShadowRoot> mProjectedShadow;

  bool mIsInsertionPoint;
};

} 
} 

#endif 

