






































#ifndef nsMathMLElement_h
#define nsMathMLElement_h

#include "nsMappedAttributeElement.h"
#include "nsIDOMElement.h"
#include "nsILink.h"
#include "Link.h"

class nsCSSValue;

typedef nsMappedAttributeElement nsMathMLElementBase;




class nsMathMLElement : public nsMathMLElementBase,
                        public nsIDOMElement,
                        public nsILink,
                        public mozilla::dom::Link
{
public:
  nsMathMLElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsMathMLElementBase(aNodeInfo), Link(this),
      mIncrementScriptLevel(PR_FALSE)
  {}

  
  NS_DECL_ISUPPORTS_INHERITED

  
  
  NS_FORWARD_NSIDOMNODE(nsMathMLElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsMathMLElementBase::)

  nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                      nsIContent* aBindingParent,
                      PRBool aCompileEventHandlers);
  virtual void UnbindFromTree(PRBool aDeep = PR_TRUE,
                              PRBool aNullParent = PR_TRUE);

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
  
  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);
  virtual nsresult PostHandleEvent(nsEventChainPostVisitor& aVisitor);
  nsresult Clone(nsINodeInfo*, nsINode**) const;
  virtual nsEventStates IntrinsicState() const;
  virtual PRBool IsNodeOfType(PRUint32 aFlags) const;

  
  
  void SetIncrementScriptLevel(PRBool aIncrementScriptLevel, PRBool aNotify);
  PRBool GetIncrementScriptLevel() const {
    return mIncrementScriptLevel;
  }

  NS_IMETHOD LinkAdded() { return NS_OK; }
  NS_IMETHOD LinkRemoved() { return NS_OK; }
  virtual PRBool IsFocusable(PRInt32 *aTabIndex = nsnull,
                             PRBool aWithMouse = PR_FALSE);
  virtual PRBool IsLink(nsIURI** aURI) const;
  virtual void GetLinkTarget(nsAString& aTarget);
  virtual nsLinkState GetLinkState() const;
  virtual void RequestLinkStateUpdate();
  virtual already_AddRefed<nsIURI> GetHrefURI() const;
  nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, PRBool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nsnull, aValue, aNotify);
  }
  virtual nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           PRBool aNotify);
  virtual nsresult UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                             PRBool aNotify);

  virtual nsXPCClassInfo* GetClassInfo();
private:
  PRPackedBool mIncrementScriptLevel;
};

#endif 
