




#ifndef mozilla_dom_HTMLTITLEElement_h_
#define mozilla_dom_HTMLTITLEElement_h_

#include "mozilla/Attributes.h"
#include "nsIDOMHTMLTitleElement.h"
#include "nsGenericHTMLElement.h"
#include "nsStubMutationObserver.h"

namespace mozilla {
class ErrorResult;

namespace dom {

class HTMLTitleElement MOZ_FINAL : public nsGenericHTMLElement,
                                   public nsIDOMHTMLTitleElement,
                                   public nsStubMutationObserver
{
public:
  using Element::GetText;
  using Element::SetText;

  HTMLTitleElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~HTMLTitleElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  
  NS_DECL_NSIDOMHTMLTITLEELEMENT

  
  
  void SetText(const nsAString& aText, ErrorResult& aError)
  {
    aError = SetText(aText);
  }

  
  NS_DECL_NSIMUTATIONOBSERVER_CHARACTERDATACHANGED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

  virtual nsresult BindToTree(nsIDocument *aDocument, nsIContent *aParent,
                              nsIContent *aBindingParent,
                              bool aCompileEventHandlers) MOZ_OVERRIDE;

  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true) MOZ_OVERRIDE;

  virtual void DoneAddingChildren(bool aHaveNotified) MOZ_OVERRIDE;

  virtual nsIDOMNode* AsDOMNode() MOZ_OVERRIDE { return this; }

protected:

  virtual JSObject* WrapNode(JSContext* cx, JS::Handle<JSObject*> scope)
    MOZ_OVERRIDE MOZ_FINAL;

private:
  void SendTitleChangeEvent(bool aBound);
};

} 
} 

#endif 
