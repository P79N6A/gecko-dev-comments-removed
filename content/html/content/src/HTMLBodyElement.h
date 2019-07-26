



#ifndef HTMLBodyElement_h___
#define HTMLBodyElement_h___

#include "nsGenericHTMLElement.h"
#include "nsIDOMHTMLBodyElement.h"
#include "nsIStyleRule.h"

namespace mozilla {
namespace dom {

class BeforeUnloadEventHandlerNonNull;
class HTMLBodyElement;

class BodyRule: public nsIStyleRule
{
public:
  BodyRule(HTMLBodyElement* aPart);
  virtual ~BodyRule();

  NS_DECL_ISUPPORTS

  
  virtual void MapRuleInfoInto(nsRuleData* aRuleData);
#ifdef DEBUG
  virtual void List(FILE* out = stdout, int32_t aIndent = 0) const;
#endif

  HTMLBodyElement*  mPart;  
};

class HTMLBodyElement : public nsGenericHTMLElement,
                        public nsIDOMHTMLBodyElement
{
public:
  using Element::GetText;
  using Element::SetText;

  HTMLBodyElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsGenericHTMLElement(aNodeInfo)
  {
    SetIsDOMBinding();
  }
  virtual ~HTMLBodyElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  
  NS_DECL_NSIDOMHTMLBODYELEMENT

  
  
#define EVENT(name_, id_, type_, struct_)
#define FORWARDED_EVENT(name_, id_, type_, struct_)                     \
  NS_IMETHOD GetOn##name_(JSContext *cx, jsval *vp);                    \
  NS_IMETHOD SetOn##name_(JSContext *cx, const jsval &v);
#define WINDOW_EVENT_HELPER(name_, type_)                               \
  type_* GetOn##name_();                                                \
  void SetOn##name_(type_* handler, ErrorResult& error);
#define WINDOW_EVENT(name_, id_, type_, struct_)                        \
  WINDOW_EVENT_HELPER(name_, EventHandlerNonNull)
#define BEFOREUNLOAD_EVENT(name_, id_, type_, struct_)                  \
  WINDOW_EVENT_HELPER(name_, BeforeUnloadEventHandlerNonNull)
#include "nsEventNameList.h"
#undef BEFOREUNLOAD_EVENT
#undef WINDOW_EVENT
#undef WINDOW_EVENT_HELPER
#undef FORWARDED_EVENT
#undef EVENT

  void GetText(nsString& aText)
  {
    GetHTMLAttr(nsGkAtoms::text, aText);
  }
  void SetText(const nsAString& aText, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::text, aText, aError);
  }
  void GetLink(nsString& aLink)
  {
    GetHTMLAttr(nsGkAtoms::link, aLink);
  }
  void SetLink(const nsAString& aLink, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::link, aLink, aError);
  }
  void GetVLink(nsString& aVLink)
  {
    GetHTMLAttr(nsGkAtoms::vlink, aVLink);
  }
  void SetVLink(const nsAString& aVLink, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::vlink, aVLink, aError);
  }
  void GetALink(nsString& aALink)
  {
    GetHTMLAttr(nsGkAtoms::alink, aALink);
  }
  void SetALink(const nsAString& aALink, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::alink, aALink, aError);
  }
  void GetBgColor(nsString& aBgColor)
  {
    GetHTMLAttr(nsGkAtoms::bgcolor, aBgColor);
  }
  void SetBgColor(const nsAString& aBgColor, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::bgcolor, aBgColor, aError);
  }
  void GetBackground(nsString& aBackground)
  {
    GetHTMLAttr(nsGkAtoms::background, aBackground);
  }
  void SetBackground(const nsAString& aBackground, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::background, aBackground, aError);
  }

  virtual bool ParseAttribute(int32_t aNamespaceID,
                              nsIAtom* aAttribute,
                              const nsAString& aValue,
                              nsAttrValue& aResult);
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true);
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;
  NS_IMETHOD WalkContentStyleRules(nsRuleWalker* aRuleWalker);
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;
  virtual already_AddRefed<nsIEditor> GetAssociatedEditor();
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
  virtual nsIDOMNode* AsDOMNode() { return this; }

  virtual bool IsEventAttributeName(nsIAtom* aName) MOZ_OVERRIDE;

private:
  nsresult GetColorHelper(nsIAtom* aAtom, nsAString& aColor);

protected:
  virtual JSObject* WrapNode(JSContext *aCx, JSObject *aScope) MOZ_OVERRIDE;

  nsRefPtr<BodyRule> mContentStyleRule;
};

} 
} 

#endif 
