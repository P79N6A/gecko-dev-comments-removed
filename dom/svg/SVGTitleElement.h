




#ifndef mozilla_dom_SVGTitleElement_h
#define mozilla_dom_SVGTitleElement_h

#include "mozilla/Attributes.h"
#include "nsSVGElement.h"
#include "nsStubMutationObserver.h"

typedef nsSVGElement SVGTitleElementBase;

nsresult NS_NewSVGTitleElement(nsIContent **aResult,
                               already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo);
namespace mozilla {
namespace dom {

class SVGTitleElement final : public SVGTitleElementBase,
                              public nsStubMutationObserver
{
protected:
  friend nsresult (::NS_NewSVGTitleElement(nsIContent **aResult,
                                           already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo));
  explicit SVGTitleElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);
  ~SVGTitleElement();

  virtual JSObject* WrapNode(JSContext *aCx, JS::Handle<JSObject*> aGivenProto) override;

public:
  

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIMUTATIONOBSERVER_CHARACTERDATACHANGED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED

  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const override;

  virtual nsresult BindToTree(nsIDocument *aDocument, nsIContent *aParent,
                              nsIContent *aBindingParent,
                              bool aCompileEventHandlers) override;

  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true) override;

  virtual void DoneAddingChildren(bool aHaveNotified) override;
private:
  void SendTitleChangeEvent(bool aBound);
};

} 
} 

#endif 

