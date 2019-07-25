












































#ifndef __NS_STYLEDELEMENT_H_
#define __NS_STYLEDELEMENT_H_

#include "nsString.h"
#include "nsGenericElement.h"
#include "nsDOMMemoryReporter.h"

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

  NS_DECL_AND_IMPL_DOM_MEMORY_REPORTER_SIZEOF(nsStyledElementNotElementCSSInlineStyle,
                                              nsStyledElementBase)

  
  virtual nsIAtom* GetClassAttributeName() const;
  virtual nsIAtom* GetIDAttributeName() const;
  virtual nsIAtom* DoGetID() const;
  virtual const nsAttrValue* DoGetClasses() const;

  virtual mozilla::css::StyleRule* GetInlineStyleRule();
  NS_IMETHOD SetInlineStyleRule(mozilla::css::StyleRule* aStyleRule, bool aNotify);

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers);
  virtual void UnbindFromTree(bool aDeep, bool aNullParent);

  virtual nsresult UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                             bool aNotify);
  virtual nsresult AfterSetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                                const nsAString* aValue, bool aNotify);

  nsIDOMCSSStyleDeclaration* GetStyle(nsresult* retval);

protected:

  






  void ParseStyleAttribute(const nsAString& aValue,
                           nsAttrValue& aResult,
                           bool aForceInDataDoc);

  virtual bool ParseAttribute(PRInt32 aNamespaceID, nsIAtom* aAttribute,
                                const nsAString& aValue, nsAttrValue& aResult);

  





  nsresult  ReparseStyleAttribute(bool aForceInDataDoc);
};

class nsStyledElement : public nsStyledElementNotElementCSSInlineStyle {
public:
  NS_DECL_AND_IMPL_DOM_MEMORY_REPORTER_SIZEOF(nsStyledElement,
                                              nsStyledElementNotElementCSSInlineStyle)

protected:
  inline nsStyledElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsStyledElementNotElementCSSInlineStyle(aNodeInfo)
  {}
};

#endif 
