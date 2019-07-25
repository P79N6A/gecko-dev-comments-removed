











#ifndef __NS_STYLEDELEMENT_H_
#define __NS_STYLEDELEMENT_H_

#include "nsString.h"
#include "nsGenericElement.h"

namespace mozilla {
namespace css {
class StyleRule;
}
}

typedef nsGenericElement nsStyledElementBase;

class nsStyledElementNotElementCSSInlineStyle : public nsStyledElementBase
{

protected:

  inline nsStyledElementNotElementCSSInlineStyle(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsStyledElementBase(aNodeInfo)
  {}

public:
  
  virtual nsIAtom* GetClassAttributeName() const;
  virtual nsIAtom* GetIDAttributeName() const;
  virtual nsIAtom* DoGetID() const;
  virtual const nsAttrValue* DoGetClasses() const;

  virtual mozilla::css::StyleRule* GetInlineStyleRule();
  virtual nsresult SetInlineStyleRule(mozilla::css::StyleRule* aStyleRule,
                                      const nsAString* aSerialized,
                                      bool aNotify);

  virtual nsresult UnsetAttr(int32_t aNameSpaceID, nsIAtom* aAttribute,
                             bool aNotify);
  virtual nsresult AfterSetAttr(int32_t aNameSpaceID, nsIAtom* aAttribute,
                                const nsAttrValue* aValue, bool aNotify);

  nsIDOMCSSStyleDeclaration* GetStyle(nsresult* retval);

protected:

  






  void ParseStyleAttribute(const nsAString& aValue,
                           nsAttrValue& aResult,
                           bool aForceInDataDoc);

  virtual bool ParseAttribute(int32_t aNamespaceID, nsIAtom* aAttribute,
                                const nsAString& aValue, nsAttrValue& aResult);

  friend class nsGenericElement;

  





  nsresult  ReparseStyleAttribute(bool aForceInDataDoc);
};

class nsStyledElement : public nsStyledElementNotElementCSSInlineStyle {
protected:
  inline nsStyledElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsStyledElementNotElementCSSInlineStyle(aNodeInfo)
  {}
};

#endif 
