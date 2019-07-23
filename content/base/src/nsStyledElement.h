












































#ifndef __NS_STYLEDELEMENT_H_
#define __NS_STYLEDELEMENT_H_

#include "nsString.h"
#include "nsGenericElement.h"

class nsICSSStyleRule;

typedef nsGenericElement nsStyledElementBase;

class nsStyledElement : public nsStyledElementBase
{

protected:

  inline nsStyledElement(nsINodeInfo *aNodeInfo)
    : nsStyledElementBase(aNodeInfo)
  {}

public:

  
  virtual nsIAtom* GetClassAttributeName() const;
  virtual nsIAtom* GetIDAttributeName() const;
  virtual const nsAttrValue* GetClasses() const;

  virtual nsICSSStyleRule* GetInlineStyleRule();
  NS_IMETHOD SetInlineStyleRule(nsICSSStyleRule* aStyleRule, PRBool aNotify);

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              PRBool aCompileEventHandlers);

  



  nsresult  ReparseStyleAttribute(void);
  







  static void ParseStyleAttribute(nsIContent* aContent,
                                  const nsAString& aValue,
                                  nsAttrValue& aResult);

  static void Shutdown();
  
protected:

  virtual PRBool ParseAttribute(PRInt32 aNamespaceID, nsIAtom* aAttribute,
                                const nsAString& aValue, nsAttrValue& aResult);

  nsresult GetStyle(nsIDOMCSSStyleDeclaration** aStyle);

};

#endif 
