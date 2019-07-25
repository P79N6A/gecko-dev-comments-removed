












































#ifndef __NS_STYLEDELEMENT_H_
#define __NS_STYLEDELEMENT_H_

#include "nsString.h"
#include "nsGenericElement.h"

class nsICSSStyleRule;

typedef nsGenericElement nsStyledElementBase;

class nsStyledElement : public nsStyledElementBase
{

protected:

  inline nsStyledElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsStyledElementBase(aNodeInfo)
  {}

public:

  
  virtual nsIAtom* GetClassAttributeName() const;
  virtual nsIAtom* GetIDAttributeName() const;
  virtual nsIAtom* DoGetID() const;
  virtual const nsAttrValue* DoGetClasses() const;

  virtual nsICSSStyleRule* GetInlineStyleRule();
  NS_IMETHOD SetInlineStyleRule(nsICSSStyleRule* aStyleRule, PRBool aNotify);

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              PRBool aCompileEventHandlers);
  virtual void UnbindFromTree(PRBool aDeep, PRBool aNullParent);

  virtual nsresult UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                             PRBool aNotify);

  nsIDOMCSSStyleDeclaration* GetStyle(nsresult* retval);

protected:

  






  void ParseStyleAttribute(const nsAString& aValue,
                           nsAttrValue& aResult,
                           PRBool aForceInDataDoc);

  virtual PRBool ParseAttribute(PRInt32 aNamespaceID, nsIAtom* aAttribute,
                                const nsAString& aValue, nsAttrValue& aResult);

  





  nsresult  ReparseStyleAttribute(PRBool aForceInDataDoc);
};

#endif 
