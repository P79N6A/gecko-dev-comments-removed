











#ifndef __NS_STYLEDELEMENT_H_
#define __NS_STYLEDELEMENT_H_

#include "mozilla/Attributes.h"
#include "nsString.h"
#include "mozilla/dom/Element.h"

namespace mozilla {
namespace css {
class StyleRule;
}
}

typedef mozilla::dom::Element nsStyledElementBase;

class nsStyledElementNotElementCSSInlineStyle : public nsStyledElementBase
{

protected:

  inline explicit nsStyledElementNotElementCSSInlineStyle(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
    : nsStyledElementBase(aNodeInfo)
  {}

public:
  
  virtual mozilla::css::StyleRule* GetInlineStyleRule();
  virtual nsresult SetInlineStyleRule(mozilla::css::StyleRule* aStyleRule,
                                      const nsAString* aSerialized,
                                      bool aNotify) MOZ_OVERRIDE;

  nsICSSDeclaration* Style();

protected:

  






  void ParseStyleAttribute(const nsAString& aValue,
                           nsAttrValue& aResult,
                           bool aForceInDataDoc);

  virtual bool ParseAttribute(int32_t aNamespaceID, nsIAtom* aAttribute,
                                const nsAString& aValue, nsAttrValue& aResult) MOZ_OVERRIDE;

  friend class mozilla::dom::Element;

  





  nsresult  ReparseStyleAttribute(bool aForceInDataDoc);
};

class nsStyledElement : public nsStyledElementNotElementCSSInlineStyle {
protected:
  inline explicit nsStyledElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
    : nsStyledElementNotElementCSSInlineStyle(aNodeInfo)
  {}
};

#endif 
