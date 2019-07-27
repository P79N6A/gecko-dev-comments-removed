





#ifndef mozilla_dom_SVGScriptElement_h
#define mozilla_dom_SVGScriptElement_h

#include "nsSVGElement.h"
#include "nsCOMPtr.h"
#include "nsSVGString.h"
#include "nsScriptElement.h"

class nsIDocument;

nsresult NS_NewSVGScriptElement(nsIContent **aResult,
                                already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo,
                                mozilla::dom::FromParser aFromParser);

namespace mozilla {
namespace dom {

typedef nsSVGElement SVGScriptElementBase;

class SVGScriptElement final : public SVGScriptElementBase,
                               public nsScriptElement
{
protected:
  friend nsresult (::NS_NewSVGScriptElement(nsIContent **aResult,
                                            already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo,
                                            mozilla::dom::FromParser aFromParser));
  SVGScriptElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo,
                   FromParser aFromParser);

  virtual JSObject* WrapNode(JSContext *aCx, JS::Handle<JSObject*> aGivenProto) override;

public:
  

  NS_DECL_ISUPPORTS_INHERITED

  
  virtual void GetScriptType(nsAString& type) override;
  virtual void GetScriptText(nsAString& text) override;
  virtual void GetScriptCharset(nsAString& charset) override;
  virtual void FreezeUriAsyncDefer() override;
  virtual CORSMode GetCORSMode() const override;

  
  virtual bool HasScriptContent() override;

  
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers) override;
  virtual nsresult AfterSetAttr(int32_t aNamespaceID, nsIAtom* aName,
                                const nsAttrValue* aValue, bool aNotify) override;
  virtual bool ParseAttribute(int32_t aNamespaceID,
                              nsIAtom* aAttribute,
                              const nsAString& aValue,
                              nsAttrValue& aResult) override;

  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const override;

  
  void GetType(nsAString & aType);
  void SetType(const nsAString & aType, ErrorResult& rv);
  void GetCrossOrigin(nsAString & aCrossOrigin);
  void SetCrossOrigin(const nsAString & aCrossOrigin, ErrorResult& aError);
  already_AddRefed<SVGAnimatedString> Href();

protected:
  ~SVGScriptElement();

  virtual StringAttributesInfo GetStringInfo() override;

  enum { HREF };
  nsSVGString mStringAttributes[1];
  static StringInfo sStringInfo[1];
};

} 
} 

#endif 
