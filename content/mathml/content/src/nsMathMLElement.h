






































#ifndef nsMathMLElement_h
#define nsMathMLElement_h

#include "nsMappedAttributeElement.h"
#include "nsIDOMElement.h"

class nsCSSValue;

typedef nsMappedAttributeElement nsMathMLElementBase;




class nsMathMLElement : public nsMathMLElementBase
                      , public nsIDOMElement
{
public:
  nsMathMLElement(nsINodeInfo* aNodeInfo)
    : nsMathMLElementBase(aNodeInfo), mIncrementScriptLevel(PR_FALSE)
  {}

  
  NS_DECL_ISUPPORTS_INHERITED

  
  
  NS_FORWARD_NSIDOMNODE(nsMathMLElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsMathMLElementBase::)

  nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                      nsIContent* aBindingParent,
                      PRBool aCompileEventHandlers);

  virtual void UnbindFromTree(PRBool aDeep = PR_TRUE,
                              PRBool aNullParent = PR_TRUE);

  virtual nsresult SetAttr(PRInt32 aNamespaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           PRBool aNotify);

  virtual nsresult UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                             PRBool aNotify);

  virtual PRBool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);

  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const;
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;

  enum {
    PARSE_ALLOW_UNITLESS = 0x01, 
    PARSE_ALLOW_NEGATIVE = 0x02
  };
  static PRBool ParseNumericValue(const nsString& aString,
                                  nsCSSValue&     aCSSValue,
                                  PRUint32        aFlags);

  static void MapMathMLAttributesInto(const nsMappedAttributes* aAttributes, 
                                      nsRuleData* aRuleData);
  
  nsresult Clone(nsINodeInfo*, nsINode**) const;
  virtual PRInt32 IntrinsicState() const;
  virtual PRBool IsNodeOfType(PRUint32 aFlags) const;

  
  
  void SetIncrementScriptLevel(PRBool aIncrementScriptLevel, PRBool aNotify);
  PRBool GetIncrementScriptLevel() const {
    return mIncrementScriptLevel;
  }

private:
  PRPackedBool mIncrementScriptLevel;
};

#endif 
