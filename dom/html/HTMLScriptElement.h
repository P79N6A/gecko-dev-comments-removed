





#ifndef mozilla_dom_HTMLScriptElement_h
#define mozilla_dom_HTMLScriptElement_h

#include "nsIDOMHTMLScriptElement.h"
#include "nsScriptElement.h"
#include "nsGenericHTMLElement.h"
#include "mozilla/Attributes.h"

namespace mozilla {
namespace dom {

class HTMLScriptElement final : public nsGenericHTMLElement,
                                    public nsIDOMHTMLScriptElement,
                                    public nsScriptElement
{
public:
  using Element::GetText;
  using Element::SetText;

  HTMLScriptElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo,
                    FromParser aFromParser);

  
  NS_DECL_ISUPPORTS_INHERITED

  NS_IMETHOD GetInnerHTML(nsAString& aInnerHTML) override;
  using nsGenericHTMLElement::SetInnerHTML;
  virtual void SetInnerHTML(const nsAString& aInnerHTML,
                            mozilla::ErrorResult& aError) override;

  
  NS_DECL_NSIDOMHTMLSCRIPTELEMENT

  
  virtual void GetScriptType(nsAString& type) override;
  virtual void GetScriptText(nsAString& text) override;
  virtual void GetScriptCharset(nsAString& charset) override;
  virtual void FreezeUriAsyncDefer() override;
  virtual CORSMode GetCORSMode() const override;

  
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers) override;
  virtual bool ParseAttribute(int32_t aNamespaceID,
                              nsIAtom* aAttribute,
                              const nsAString& aValue,
                              nsAttrValue& aResult) override;

  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const override;

  
  virtual nsresult AfterSetAttr(int32_t aNamespaceID, nsIAtom* aName,
                                const nsAttrValue* aValue, bool aNotify) override;

  
  void SetText(const nsAString& aValue, ErrorResult& rv);
  void SetCharset(const nsAString& aCharset, ErrorResult& rv);
  void SetDefer(bool aDefer, ErrorResult& rv);
  bool Defer();
  void SetSrc(const nsAString& aSrc, ErrorResult& rv);
  void SetType(const nsAString& aType, ErrorResult& rv);
  void SetHtmlFor(const nsAString& aHtmlFor, ErrorResult& rv);
  void SetEvent(const nsAString& aEvent, ErrorResult& rv);
  void GetCrossOrigin(nsAString& aResult)
  {
    
    
    
    GetEnumAttr(nsGkAtoms::crossorigin, nullptr, aResult);
  }
  void SetCrossOrigin(const nsAString& aCrossOrigin, ErrorResult& aError)
  {
    SetOrRemoveNullableStringAttr(nsGkAtoms::crossorigin, aCrossOrigin, aError);
  }
  bool Async();
  void SetAsync(bool aValue, ErrorResult& rv);

protected:
  virtual ~HTMLScriptElement();

  virtual JSObject* WrapNode(JSContext *aCx, JS::Handle<JSObject*> aGivenProto) override;
  
  virtual bool HasScriptContent() override;
};

} 
} 

#endif 
