





#ifndef mozilla_dom_HTMLScriptElement_h
#define mozilla_dom_HTMLScriptElement_h

#include "nsIDOMHTMLScriptElement.h"
#include "nsScriptElement.h"
#include "nsGenericHTMLElement.h"
#include "mozilla/Attributes.h"

namespace mozilla {
namespace dom {

class HTMLScriptElement MOZ_FINAL : public nsGenericHTMLElement,
                                    public nsIDOMHTMLScriptElement,
                                    public nsScriptElement
{
public:
  using Element::GetText;
  using Element::SetText;

  HTMLScriptElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                      FromParser aFromParser);
  virtual ~HTMLScriptElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  virtual void GetInnerHTML(nsAString& aInnerHTML,
                            mozilla::ErrorResult& aError) MOZ_OVERRIDE;
  virtual void SetInnerHTML(const nsAString& aInnerHTML,
                            mozilla::ErrorResult& aError) MOZ_OVERRIDE;

  
  NS_DECL_NSIDOMHTMLSCRIPTELEMENT

  
  virtual void GetScriptType(nsAString& type);
  virtual void GetScriptText(nsAString& text);
  virtual void GetScriptCharset(nsAString& charset);
  virtual void FreezeUriAsyncDefer();
  virtual CORSMode GetCORSMode() const;

  
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers);
  virtual bool ParseAttribute(int32_t aNamespaceID,
                              nsIAtom* aAttribute,
                              const nsAString& aValue,
                              nsAttrValue& aResult);

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  
  virtual nsresult AfterSetAttr(int32_t aNamespaceID, nsIAtom* aName,
                                const nsAttrValue* aValue, bool aNotify);

  virtual nsIDOMNode* AsDOMNode() { return this; }

  
  void SetText(const nsAString& aValue, ErrorResult& rv);
  void SetCharset(const nsAString& aCharset, ErrorResult& rv);
  void SetDefer(bool aDefer, ErrorResult& rv);
  bool Defer();
  void SetSrc(const nsAString& aSrc, ErrorResult& rv);
  void SetType(const nsAString& aType, ErrorResult& rv);
  void SetHtmlFor(const nsAString& aHtmlFor, ErrorResult& rv);
  void SetEvent(const nsAString& aEvent, ErrorResult& rv);
  void SetCrossOrigin(const nsAString& aCrossOrigin, ErrorResult& rv);
  bool Async();
  void SetAsync(bool aValue, ErrorResult& rv);

protected:
  virtual JSObject* WrapNode(JSContext *aCx, JSObject *aScope) MOZ_OVERRIDE;
  
  virtual bool HasScriptContent();
};

} 
} 

#endif 
