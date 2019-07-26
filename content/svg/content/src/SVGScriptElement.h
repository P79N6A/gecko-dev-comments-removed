





#ifndef mozilla_dom_SVGScriptElement_h
#define mozilla_dom_SVGScriptElement_h

#include "nsSVGElement.h"
#include "nsCOMPtr.h"
#include "nsSVGString.h"
#include "nsIDocument.h"
#include "nsScriptElement.h"

nsresult NS_NewSVGScriptElement(nsIContent **aResult,
                                already_AddRefed<nsINodeInfo> aNodeInfo,
                                mozilla::dom::FromParser aFromParser);

namespace mozilla {
namespace dom {

typedef nsSVGElement SVGScriptElementBase;

class SVGScriptElement MOZ_FINAL : public SVGScriptElementBase,
                                   public nsIDOMSVGElement,
                                   public nsScriptElement
{
protected:
  friend nsresult (::NS_NewSVGScriptElement(nsIContent **aResult,
                                            already_AddRefed<nsINodeInfo> aNodeInfo,
                                            mozilla::dom::FromParser aFromParser));
  SVGScriptElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                   FromParser aFromParser);

  virtual JSObject* WrapNode(JSContext *aCx, JSObject *aScope, bool *aTriedToWrap) MOZ_OVERRIDE;

public:
  

  NS_DECL_ISUPPORTS_INHERITED

  
  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC
  NS_FORWARD_NSIDOMSVGELEMENT(SVGScriptElementBase::)

  
  virtual void GetScriptType(nsAString& type);
  virtual void GetScriptText(nsAString& text);
  virtual void GetScriptCharset(nsAString& charset);
  virtual void FreezeUriAsyncDefer();
  virtual CORSMode GetCORSMode() const;

  
  virtual bool HasScriptContent();

  
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers);
  virtual nsresult AfterSetAttr(int32_t aNamespaceID, nsIAtom* aName,
                                const nsAttrValue* aValue, bool aNotify);
  virtual bool ParseAttribute(int32_t aNamespaceID,
                              nsIAtom* aAttribute,
                              const nsAString& aValue,
                              nsAttrValue& aResult);

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsIDOMNode* AsDOMNode() { return this; }

  
  void GetType(nsAString & aType);
  void SetType(const nsAString & aType, ErrorResult& rv);
  void GetCrossOrigin(nsAString & aOrigin);
  void SetCrossOrigin(const nsAString & aOrigin, ErrorResult& rv);
  already_AddRefed<nsIDOMSVGAnimatedString> Href();

protected:
  virtual StringAttributesInfo GetStringInfo();

  enum { HREF };
  nsSVGString mStringAttributes[1];
  static StringInfo sStringInfo[1];
};

} 
} 

#endif 
