




#ifndef mozilla_dom_HTMLTITLEElement_h_
#define mozilla_dom_HTMLTITLEElement_h_

#include "mozilla/Attributes.h"
#include "nsGenericHTMLElement.h"
#include "nsStubMutationObserver.h"

namespace mozilla {
class ErrorResult;

namespace dom {

class HTMLTitleElement final : public nsGenericHTMLElement,
                               public nsStubMutationObserver
{
public:
  using Element::GetText;
  using Element::SetText;

  explicit HTMLTitleElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  void GetText(DOMString& aText, ErrorResult& aError);
  void SetText(const nsAString& aText, ErrorResult& aError);

  
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

protected:
  virtual ~HTMLTitleElement();

  virtual JSObject* WrapNode(JSContext* cx, JS::Handle<JSObject*> aGivenProto)
    override final;

private:
  void SendTitleChangeEvent(bool aBound);
};

} 
} 

#endif 
