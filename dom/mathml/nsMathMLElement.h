




#ifndef nsMathMLElement_h
#define nsMathMLElement_h

#include "mozilla/Attributes.h"
#include "mozilla/dom/ElementInlines.h"
#include "nsMappedAttributeElement.h"
#include "nsIDOMElement.h"
#include "Link.h"
#include "mozilla/dom/DOMRect.h"

class nsCSSValue;

typedef nsMappedAttributeElement nsMathMLElementBase;

namespace mozilla {
class EventChainPostVisitor;
class EventChainPreVisitor;
} 




class nsMathMLElement final : public nsMathMLElementBase,
                              public nsIDOMElement,
                              public mozilla::dom::Link
{
public:
  explicit nsMathMLElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);
  explicit nsMathMLElement(already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                      nsIContent* aBindingParent,
                      bool aCompileEventHandlers) override;
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true) override;

  virtual bool ParseAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult) override;

  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const override;
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const override;

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
  
  virtual nsresult PreHandleEvent(
                     mozilla::EventChainPreVisitor& aVisitor) override;
  virtual nsresult PostHandleEvent(
                     mozilla::EventChainPostVisitor& aVisitor) override;
  nsresult Clone(mozilla::dom::NodeInfo*, nsINode**) const override;
  virtual mozilla::EventStates IntrinsicState() const override;
  virtual bool IsNodeOfType(uint32_t aFlags) const override;

  
  
  void SetIncrementScriptLevel(bool aIncrementScriptLevel, bool aNotify);
  bool GetIncrementScriptLevel() const {
    return mIncrementScriptLevel;
  }

  virtual bool IsFocusableInternal(int32_t* aTabIndex, bool aWithMouse) override;
  virtual bool IsLink(nsIURI** aURI) const override;
  virtual void GetLinkTarget(nsAString& aTarget) override;
  virtual already_AddRefed<nsIURI> GetHrefURI() const override;
  nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, bool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nullptr, aValue, aNotify);
  }
  virtual nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           bool aNotify) override;
  virtual nsresult UnsetAttr(int32_t aNameSpaceID, nsIAtom* aAttribute,
                             bool aNotify) override;

  virtual nsIDOMNode* AsDOMNode() override { return this; }

protected:
  virtual ~nsMathMLElement() {}

  virtual JSObject* WrapNode(JSContext *aCx, JS::Handle<JSObject*> aGivenProto) override;

private:
  bool mIncrementScriptLevel;
};

#endif 
