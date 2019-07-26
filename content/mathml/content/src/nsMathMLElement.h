




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
  nsMathMLElement(already_AddRefed<nsINodeInfo> aNodeInfo);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                      nsIContent* aBindingParent,
                      bool aCompileEventHandlers);
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true);

  virtual bool ParseAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);

  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;

  enum {
    PARSE_ALLOW_UNITLESS = 0x01, 
    PARSE_ALLOW_NEGATIVE = 0x02,
    PARSE_SUPPRESS_WARNINGS = 0x04,
    CONVERT_UNITLESS_TO_PERCENT = 0x08
  };
  static bool ParseNamedSpaceValue(const nsString& aString,
                                   nsCSSValue&     aCSSValue,
                                   uint32_t        aFlags);

  static bool ParseNumericValue(const nsString& aString,
                                nsCSSValue&     aCSSValue,
                                uint32_t        aFlags,
                                nsIDocument*    aDocument);

  static void MapMathMLAttributesInto(const nsMappedAttributes* aAttributes, 
                                      nsRuleData* aRuleData);
  
  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);
  virtual nsresult PostHandleEvent(nsEventChainPostVisitor& aVisitor);
  nsresult Clone(nsINodeInfo*, nsINode**) const;
  virtual nsEventStates IntrinsicState() const;
  virtual bool IsNodeOfType(uint32_t aFlags) const;

  
  
  void SetIncrementScriptLevel(bool aIncrementScriptLevel, bool aNotify);
  bool GetIncrementScriptLevel() const {
    return mIncrementScriptLevel;
  }

  NS_IMETHOD LinkAdded() { return NS_OK; }
  NS_IMETHOD LinkRemoved() { return NS_OK; }
  virtual bool IsFocusable(int32_t *aTabIndex = nullptr,
                             bool aWithMouse = false);
  virtual bool IsLink(nsIURI** aURI) const;
  virtual void GetLinkTarget(nsAString& aTarget);
  virtual nsLinkState GetLinkState() const;
  virtual already_AddRefed<nsIURI> GetHrefURI() const;
  nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, bool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nullptr, aValue, aNotify);
  }
  virtual nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           bool aNotify);
  virtual nsresult UnsetAttr(int32_t aNameSpaceID, nsIAtom* aAttribute,
                             bool aNotify);

  virtual nsIDOMNode* AsDOMNode() { return this; }

protected:
  virtual JSObject* WrapNode(JSContext *aCx, JSObject *aScope) MOZ_OVERRIDE;

private:
  bool mIncrementScriptLevel;
};

#endif 
