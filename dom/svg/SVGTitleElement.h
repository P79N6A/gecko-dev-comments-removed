




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

class SVGTitleElement MOZ_FINAL : public SVGTitleElementBase,
                                  public nsStubMutationObserver
{
protected:
  friend nsresult (::NS_NewSVGTitleElement(nsIContent **aResult,
                                           already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo));
  explicit SVGTitleElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);
  ~SVGTitleElement();

  virtual JSObject* WrapNode(JSContext *aCx) MOZ_OVERRIDE;

public:
  

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIMUTATIONOBSERVER_CHARACTERDATACHANGED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED

  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

  virtual nsresult BindToTree(nsIDocument *aDocument, nsIContent *aParent,
                              nsIContent *aBindingParent,
                              bool aCompileEventHandlers) MOZ_OVERRIDE;

  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true) MOZ_OVERRIDE;

  virtual void DoneAddingChildren(bool aHaveNotified) MOZ_OVERRIDE;
private:
  void SendTitleChangeEvent(bool aBound);
};

} 
} 

#endif 

