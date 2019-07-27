




#ifndef nsGenericHTMLElement_h___
#define nsGenericHTMLElement_h___

#include "mozilla/Attributes.h"
#include "nsMappedAttributeElement.h"
#include "nsIDOMHTMLElement.h"
#include "nsNameSpaceManager.h"  
#include "nsIFormControl.h"
#include "nsGkAtoms.h"
#include "nsContentCreatorFunctions.h"
#include "mozilla/ErrorResult.h"
#include "nsIDOMHTMLMenuElement.h"
#include "mozilla/dom/DOMRect.h"
#include "mozilla/dom/ValidityState.h"
#include "mozilla/dom/ElementInlines.h"

class nsDOMSettableTokenList;
class nsIDOMHTMLMenuElement;
class nsIEditor;
class nsIFormControlFrame;
class nsIFrame;
class nsILayoutHistoryState;
class nsIURI;
class nsPresState;
struct nsSize;

namespace mozilla {
class EventChainPostVisitor;
class EventChainPreVisitor;
class EventChainVisitor;
class EventListenerManager;
class EventStates;
namespace dom {
class HTMLFormElement;
class HTMLPropertiesCollection;
class HTMLMenuElement;
}
}

typedef nsMappedAttributeElement nsGenericHTMLElementBase;




class nsGenericHTMLElement : public nsGenericHTMLElementBase,
                             public nsIDOMHTMLElement
{
public:
  explicit nsGenericHTMLElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
    : nsGenericHTMLElementBase(aNodeInfo),
      mScrollgrab(false)
  {
    NS_ASSERTION(mNodeInfo->NamespaceID() == kNameSpaceID_XHTML,
                 "Unexpected namespace");
    AddStatesSilently(NS_EVENT_STATE_LTR);
    SetFlags(NODE_HAS_DIRECTION_LTR);
  }

  NS_DECL_ISUPPORTS_INHERITED

  NS_IMPL_FROMCONTENT(nsGenericHTMLElement, kNameSpaceID_XHTML)

  
  nsresult CopyInnerTo(mozilla::dom::Element* aDest);

  void GetTitle(mozilla::dom::DOMString& aTitle)
  {
    GetHTMLAttr(nsGkAtoms::title, aTitle);
  }
  NS_IMETHODIMP SetTitle(const nsAString& aTitle) override
  {
    SetHTMLAttr(nsGkAtoms::title, aTitle);
    return NS_OK;
  }
  void GetLang(mozilla::dom::DOMString& aLang)
  {
    GetHTMLAttr(nsGkAtoms::lang, aLang);
  }
  NS_IMETHODIMP SetLang(const nsAString& aLang) override
  {
    SetHTMLAttr(nsGkAtoms::lang, aLang);
    return NS_OK;
  }
  void GetDir(mozilla::dom::DOMString& aDir)
  {
    GetHTMLEnumAttr(nsGkAtoms::dir, aDir);
  }
  void SetDir(const nsAString& aDir, mozilla::ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::dir, aDir, aError);
  }
  already_AddRefed<nsDOMStringMap> Dataset();
  bool ItemScope() const
  {
    return GetBoolAttr(nsGkAtoms::itemscope);
  }
  void SetItemScope(bool aItemScope, mozilla::ErrorResult& aError)
  {
    SetHTMLBoolAttr(nsGkAtoms::itemscope, aItemScope, aError);
  }
  nsDOMSettableTokenList* ItemType()
  {
    return GetTokenList(nsGkAtoms::itemtype);
  }
  void GetItemId(nsString& aItemId)
  {
    GetHTMLURIAttr(nsGkAtoms::itemid, aItemId);
  }
  void SetItemId(const nsAString& aItemID, mozilla::ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::itemid, aItemID, aError);
  }
  nsDOMSettableTokenList* ItemRef()
  {
    return GetTokenList(nsGkAtoms::itemref);
  }
  nsDOMSettableTokenList* ItemProp()
  {
    return GetTokenList(nsGkAtoms::itemprop);
  }
  mozilla::dom::HTMLPropertiesCollection* Properties();
  void GetItemValue(JSContext* aCx, JSObject* aScope,
                    JS::MutableHandle<JS::Value> aRetval,
                    mozilla::ErrorResult& aError);
  void GetItemValue(JSContext* aCx, JS::MutableHandle<JS::Value> aRetval,
                    mozilla::ErrorResult& aError)
  {
    GetItemValue(aCx, GetWrapperPreserveColor(), aRetval, aError);
  }
  void SetItemValue(JSContext* aCx, JS::Value aValue,
                    mozilla::ErrorResult& aError);
  bool Hidden() const
  {
    return GetBoolAttr(nsGkAtoms::hidden);
  }
  void SetHidden(bool aHidden, mozilla::ErrorResult& aError)
  {
    SetHTMLBoolAttr(nsGkAtoms::hidden, aHidden, aError);
  }
  virtual void Click();
  virtual int32_t TabIndexDefault()
  {
    return -1;
  }
  int32_t TabIndex()
  {
    return GetIntAttr(nsGkAtoms::tabindex, TabIndexDefault());
  }
  void SetTabIndex(int32_t aTabIndex, mozilla::ErrorResult& aError)
  {
    SetHTMLIntAttr(nsGkAtoms::tabindex, aTabIndex, aError);
  }
  virtual void Focus(mozilla::ErrorResult& aError);
  virtual void Blur(mozilla::ErrorResult& aError);
  void GetAccessKey(nsString& aAccessKey)
  {
    GetHTMLAttr(nsGkAtoms::accesskey, aAccessKey);
  }
  void SetAccessKey(const nsAString& aAccessKey, mozilla::ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::accesskey, aAccessKey, aError);
  }
  void GetAccessKeyLabel(nsString& aAccessKeyLabel);
  virtual bool Draggable() const
  {
    return AttrValueIs(kNameSpaceID_None, nsGkAtoms::draggable,
                       nsGkAtoms::_true, eIgnoreCase);
  }
  void SetDraggable(bool aDraggable, mozilla::ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::draggable,
                aDraggable ? NS_LITERAL_STRING("true")
                           : NS_LITERAL_STRING("false"),
                aError);
  }
  void GetContentEditable(nsString& aContentEditable)
  {
    ContentEditableTristate value = GetContentEditableValue();
    if (value == eTrue) {
      aContentEditable.AssignLiteral("true");
    } else if (value == eFalse) {
      aContentEditable.AssignLiteral("false");
    } else {
      aContentEditable.AssignLiteral("inherit");
    }
  }
  void SetContentEditable(const nsAString& aContentEditable,
                          mozilla::ErrorResult& aError)
  {
    if (aContentEditable.LowerCaseEqualsLiteral("inherit")) {
      UnsetHTMLAttr(nsGkAtoms::contenteditable, aError);
    } else if (aContentEditable.LowerCaseEqualsLiteral("true")) {
      SetHTMLAttr(nsGkAtoms::contenteditable, NS_LITERAL_STRING("true"), aError);
    } else if (aContentEditable.LowerCaseEqualsLiteral("false")) {
      SetHTMLAttr(nsGkAtoms::contenteditable, NS_LITERAL_STRING("false"), aError);
    } else {
      aError.Throw(NS_ERROR_DOM_SYNTAX_ERR);
    }
  }
  bool IsContentEditable()
  {
    for (nsIContent* node = this; node; node = node->GetParent()) {
      nsGenericHTMLElement* element = FromContent(node);
      if (element) {
        ContentEditableTristate value = element->GetContentEditableValue();
        if (value != eInherit) {
          return value == eTrue;
        }
      }
    }
    return false;
  }
  mozilla::dom::HTMLMenuElement* GetContextMenu() const;
  bool Spellcheck();
  void SetSpellcheck(bool aSpellcheck, mozilla::ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::spellcheck,
                aSpellcheck ? NS_LITERAL_STRING("true")
                            : NS_LITERAL_STRING("false"),
                aError);
  }
  bool Scrollgrab() const
  {
    return mScrollgrab;
  }
  void SetScrollgrab(bool aValue)
  {
    mScrollgrab = aValue;
  }

  




  virtual bool IsEventAttributeName(nsIAtom* aName) override;

#define EVENT(name_, id_, type_, struct_)


#define FORWARDED_EVENT(name_, id_, type_, struct_)                           \
  using nsINode::GetOn##name_;                                                \
  using nsINode::SetOn##name_;                                                \
  mozilla::dom::EventHandlerNonNull* GetOn##name_();                          \
  void SetOn##name_(mozilla::dom::EventHandlerNonNull* handler);
#define ERROR_EVENT(name_, id_, type_, struct_)                               \
  using nsINode::GetOn##name_;                                                \
  using nsINode::SetOn##name_;                                                \
  already_AddRefed<mozilla::dom::EventHandlerNonNull> GetOn##name_();         \
  void SetOn##name_(mozilla::dom::EventHandlerNonNull* handler);
#include "mozilla/EventNameList.h" 
#undef ERROR_EVENT
#undef FORWARDED_EVENT
#undef EVENT
  mozilla::dom::Element* GetOffsetParent()
  {
    mozilla::CSSIntRect rcFrame;
    return GetOffsetRect(rcFrame);
  }
  int32_t OffsetTop()
  {
    mozilla::CSSIntRect rcFrame;
    GetOffsetRect(rcFrame);

    return rcFrame.y;
  }
  int32_t OffsetLeft()
  {
    mozilla::CSSIntRect rcFrame;
    GetOffsetRect(rcFrame);

    return rcFrame.x;
  }
  int32_t OffsetWidth()
  {
    mozilla::CSSIntRect rcFrame;
    GetOffsetRect(rcFrame);

    return rcFrame.width;
  }
  int32_t OffsetHeight()
  {
    mozilla::CSSIntRect rcFrame;
    GetOffsetRect(rcFrame);

    return rcFrame.height;
  }

  
  
  
  inline bool IsHTMLElement() const
  {
    return true;
  }

  inline bool IsHTMLElement(nsIAtom* aTag) const
  {
    return mNodeInfo->Equals(aTag);
  }

  template<typename First, typename... Args>
  inline bool IsAnyOfHTMLElements(First aFirst, Args... aArgs) const
  {
    return IsNodeInternal(aFirst, aArgs...);
  }

protected:
  virtual ~nsGenericHTMLElement() {}

  
  
  virtual void GetItemValueText(mozilla::dom::DOMString& text);
  virtual void SetItemValueText(const nsAString& text);
public:
  virtual already_AddRefed<mozilla::dom::UndoManager> GetUndoManager() override;
  virtual bool UndoScope() override;
  virtual void SetUndoScope(bool aUndoScope, mozilla::ErrorResult& aError) override;
  
  nsresult ClearDataset();

  




  nsSize GetWidthHeightForImage(nsRefPtr<imgRequestProxy>& aImageRequest);

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  NS_IMETHOD GetTitle(nsAString& aTitle) final override {
    mozilla::dom::DOMString title;
    GetTitle(title);
    title.ToString(aTitle);
    return NS_OK;
  }
  NS_IMETHOD GetLang(nsAString& aLang) final override {
    mozilla::dom::DOMString lang;
    GetLang(lang);
    lang.ToString(aLang);
    return NS_OK;
  }
  NS_IMETHOD GetDir(nsAString& aDir) final override {
    mozilla::dom::DOMString dir;
    GetDir(dir);
    dir.ToString(aDir);
    return NS_OK;
  }
  NS_IMETHOD SetDir(const nsAString& aDir) final override {
    mozilla::ErrorResult rv;
    SetDir(aDir, rv);
    return rv.StealNSResult();
  }
  NS_IMETHOD GetDOMClassName(nsAString& aClassName) final {
    GetHTMLAttr(nsGkAtoms::_class, aClassName);
    return NS_OK;
  }
  NS_IMETHOD SetDOMClassName(const nsAString& aClassName) final {
    SetClassName(aClassName);
    return NS_OK;
  }
  NS_IMETHOD GetDataset(nsISupports** aDataset) final override;
  NS_IMETHOD GetHidden(bool* aHidden) final override {
    *aHidden = Hidden();
    return NS_OK;
  }
  NS_IMETHOD SetHidden(bool aHidden) final override {
    mozilla::ErrorResult rv;
    SetHidden(aHidden, rv);
    return rv.StealNSResult();
  }
  NS_IMETHOD DOMBlur() final override {
    mozilla::ErrorResult rv;
    Blur(rv);
    return rv.StealNSResult();
  }
  NS_IMETHOD GetItemScope(bool* aItemScope) final override {
    *aItemScope = ItemScope();
    return NS_OK;
  }
  NS_IMETHOD SetItemScope(bool aItemScope) final override {
    mozilla::ErrorResult rv;
    SetItemScope(aItemScope, rv);
    return rv.StealNSResult();
  }
  NS_IMETHOD GetItemType(nsIVariant** aType) final override {
    GetTokenList(nsGkAtoms::itemtype, aType);
    return NS_OK;
  }
  NS_IMETHOD SetItemType(nsIVariant* aType) final override {
    return SetTokenList(nsGkAtoms::itemtype, aType);
  }
  NS_IMETHOD GetItemId(nsAString& aId) final override {
    nsString id;
    GetItemId(id);
    aId.Assign(aId);
    return NS_OK;
  }
  NS_IMETHOD SetItemId(const nsAString& aId) final override {
    mozilla::ErrorResult rv;
    SetItemId(aId, rv);
    return rv.StealNSResult();
  }
  NS_IMETHOD GetProperties(nsISupports** aReturn) final override;
  NS_IMETHOD GetItemValue(nsIVariant** aValue) final override;
  NS_IMETHOD SetItemValue(nsIVariant* aValue) final override;
  NS_IMETHOD GetItemRef(nsIVariant** aRef) final override {
    GetTokenList(nsGkAtoms::itemref, aRef);
    return NS_OK;
  }
  NS_IMETHOD SetItemRef(nsIVariant* aRef) final override {
    return SetTokenList(nsGkAtoms::itemref, aRef);
  }
  NS_IMETHOD GetItemProp(nsIVariant** aProp) final override {
    GetTokenList(nsGkAtoms::itemprop, aProp);
    return NS_OK;
  }
  NS_IMETHOD SetItemProp(nsIVariant* aProp) final override {
    return SetTokenList(nsGkAtoms::itemprop, aProp);
  }
  NS_IMETHOD GetAccessKey(nsAString& aAccessKey) final override {
    nsString accessKey;
    GetAccessKey(accessKey);
    aAccessKey.Assign(accessKey);
    return NS_OK;
  }
  NS_IMETHOD SetAccessKey(const nsAString& aAccessKey) final override {
    mozilla::ErrorResult rv;
    SetAccessKey(aAccessKey, rv);
    return rv.StealNSResult();
  }
  NS_IMETHOD GetAccessKeyLabel(nsAString& aAccessKeyLabel)
    final override {
    nsString accessKeyLabel;
    GetAccessKeyLabel(accessKeyLabel);
    aAccessKeyLabel.Assign(accessKeyLabel);
    return NS_OK;
  }
  NS_IMETHOD SetDraggable(bool aDraggable) final override {
    mozilla::ErrorResult rv;
    SetDraggable(aDraggable, rv);
    return rv.StealNSResult();
  }
  NS_IMETHOD GetContentEditable(nsAString& aContentEditable)
    final override {
    nsString contentEditable;
    GetContentEditable(contentEditable);
    aContentEditable.Assign(contentEditable);
    return NS_OK;
  }
  NS_IMETHOD SetContentEditable(const nsAString& aContentEditable)
    final override {
    mozilla::ErrorResult rv;
    SetContentEditable(aContentEditable, rv);
    return rv.StealNSResult();
  }
  NS_IMETHOD GetIsContentEditable(bool* aIsContentEditable)
    final override {
    *aIsContentEditable = IsContentEditable();
    return NS_OK;
  }
  NS_IMETHOD GetContextMenu(nsIDOMHTMLMenuElement** aContextMenu)
    final override;
  NS_IMETHOD GetSpellcheck(bool* aSpellcheck) final override {
    *aSpellcheck = Spellcheck();
    return NS_OK;
  }
  NS_IMETHOD SetSpellcheck(bool aSpellcheck) final override {
    mozilla::ErrorResult rv;
    SetSpellcheck(aSpellcheck, rv);
    return rv.StealNSResult();
  }
  NS_IMETHOD GetOuterHTML(nsAString& aOuterHTML) final override {
    mozilla::dom::Element::GetOuterHTML(aOuterHTML);
    return NS_OK;
  }
  NS_IMETHOD SetOuterHTML(const nsAString& aOuterHTML) final override {
    mozilla::ErrorResult rv;
    mozilla::dom::Element::SetOuterHTML(aOuterHTML, rv);
    return rv.StealNSResult();
  }
  NS_IMETHOD InsertAdjacentHTML(const nsAString& position,
                                const nsAString& text) final override;
  NS_IMETHOD ScrollIntoView(bool top, uint8_t _argc) final override {
    if (!_argc) {
      top = true;
    }
    mozilla::dom::Element::ScrollIntoView(top);
    return NS_OK;
  }
  NS_IMETHOD GetOffsetParent(nsIDOMElement** aOffsetParent)
    final override {
    mozilla::dom::Element* offsetParent = GetOffsetParent();
    if (!offsetParent) {
      *aOffsetParent = nullptr;
      return NS_OK;
    }
    return CallQueryInterface(offsetParent, aOffsetParent);
  }
  NS_IMETHOD GetOffsetTop(int32_t* aOffsetTop) final override {
    *aOffsetTop = OffsetTop();
    return NS_OK;
  }
  NS_IMETHOD GetOffsetLeft(int32_t* aOffsetLeft) final override {
    *aOffsetLeft = OffsetLeft();
    return NS_OK;
  }
  NS_IMETHOD GetOffsetWidth(int32_t* aOffsetWidth) final override {
    *aOffsetWidth = OffsetWidth();
    return NS_OK;
  }
  NS_IMETHOD GetOffsetHeight(int32_t* aOffsetHeight) final override {
    *aOffsetHeight = OffsetHeight();
    return NS_OK;
  }
  NS_IMETHOD DOMClick() final override {
    Click();
    return NS_OK;
  }
  NS_IMETHOD GetTabIndex(int32_t* aTabIndex) final override {
    *aTabIndex = TabIndex();
    return NS_OK;
  }
  NS_IMETHOD SetTabIndex(int32_t aTabIndex) final override {
    mozilla::ErrorResult rv;
    SetTabIndex(aTabIndex, rv);
    return rv.StealNSResult();
  }
  NS_IMETHOD Focus() final override {
    mozilla::ErrorResult rv;
    Focus(rv);
    return rv.StealNSResult();
  }
  NS_IMETHOD GetDraggable(bool* aDraggable) final override {
    *aDraggable = Draggable();
    return NS_OK;
  }
  NS_IMETHOD GetInnerHTML(nsAString& aInnerHTML) override {
    return mozilla::dom::Element::GetInnerHTML(aInnerHTML);
  }
  using mozilla::dom::Element::SetInnerHTML;
  NS_IMETHOD SetInnerHTML(const nsAString& aInnerHTML) final override {
    mozilla::ErrorResult rv;
    SetInnerHTML(aInnerHTML, rv);
    return rv.StealNSResult();
  }

  using nsGenericHTMLElementBase::GetOwnerDocument;

  virtual nsIDOMNode* AsDOMNode() override { return this; }

public:
  
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers) override;
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true) override;

  MOZ_ALWAYS_INLINE 
  nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, bool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nullptr, aValue, aNotify);
  }
  virtual nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           bool aNotify) override;
  virtual nsresult UnsetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                             bool aNotify) override;
  virtual bool IsFocusableInternal(int32_t *aTabIndex, bool aWithMouse) override
  {
    bool isFocusable = false;
    IsHTMLFocusable(aWithMouse, &isFocusable, aTabIndex);
    return isFocusable;
  }
  



  virtual bool IsHTMLFocusable(bool aWithMouse,
                               bool *aIsFocusable,
                               int32_t *aTabIndex);
  virtual void PerformAccesskey(bool aKeyCausesActivation,
                                bool aIsTrustedEvent) override;

  



  bool CheckHandleEventForAnchorsPreconditions(
         mozilla::EventChainVisitor& aVisitor);
  nsresult PreHandleEventForAnchors(mozilla::EventChainPreVisitor& aVisitor);
  nsresult PostHandleEventForAnchors(mozilla::EventChainPostVisitor& aVisitor);
  bool IsHTMLLink(nsIURI** aURI) const;

  
  void Compact() { mAttrsAndChildren.Compact(); }

  virtual void UpdateEditableState(bool aNotify) override;

  virtual mozilla::EventStates IntrinsicState() const override;

  
  void DoSetEditableFlag(bool aEditable, bool aNotify) {
    SetEditableFlag(aEditable);
    UpdateState(aNotify);
  }

  virtual bool ParseAttribute(int32_t aNamespaceID,
                              nsIAtom* aAttribute,
                              const nsAString& aValue,
                              nsAttrValue& aResult) override;

  bool ParseBackgroundAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);

  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const override;
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const override;

  







  void GetBaseTarget(nsAString& aBaseTarget) const;

  






  nsIFormControlFrame* GetFormControlFrame(bool aFlushFrames);

  

  






  static bool ParseAlignValue(const nsAString& aString,
                                nsAttrValue& aResult);

  






  static bool ParseDivAlignValue(const nsAString& aString,
                                   nsAttrValue& aResult);

  






  static bool ParseTableHAlignValue(const nsAString& aString,
                                      nsAttrValue& aResult);

  






  static bool ParseTableCellHAlignValue(const nsAString& aString,
                                          nsAttrValue& aResult);

  







  static bool ParseTableVAlignValue(const nsAString& aString,
                                      nsAttrValue& aResult);

  







  static bool ParseImageAttribute(nsIAtom* aAttribute,
                                    const nsAString& aString,
                                    nsAttrValue& aResult);
  






  static bool ParseFrameborderValue(const nsAString& aString,
                                      nsAttrValue& aResult);

  






  static bool ParseScrollingValue(const nsAString& aString,
                                    nsAttrValue& aResult);

  



  








  static void MapCommonAttributesInto(const nsMappedAttributes* aAttributes, 
                                      nsRuleData* aRuleData);
  






  static void MapCommonAttributesIntoExceptHidden(const nsMappedAttributes* aAttributes,
                                                  nsRuleData* aRuleData);

  static const MappedAttributeEntry sCommonAttributeMap[];
  static const MappedAttributeEntry sImageMarginSizeAttributeMap[];
  static const MappedAttributeEntry sImageBorderAttributeMap[];
  static const MappedAttributeEntry sImageAlignAttributeMap[];
  static const MappedAttributeEntry sDivAlignAttributeMap[];
  static const MappedAttributeEntry sBackgroundAttributeMap[];
  static const MappedAttributeEntry sBackgroundColorAttributeMap[];
  
  






  static void MapImageAlignAttributeInto(const nsMappedAttributes* aAttributes,
                                         nsRuleData* aData);

  







  static void MapDivAlignAttributeInto(const nsMappedAttributes* aAttributes,
                                       nsRuleData* aData);

  






  static void MapImageBorderAttributeInto(const nsMappedAttributes* aAttributes,
                                          nsRuleData* aData);
  






  static void MapImageMarginAttributeInto(const nsMappedAttributes* aAttributes,
                                          nsRuleData* aData);
  






  static void MapImageSizeAttributesInto(const nsMappedAttributes* aAttributes,
                                         nsRuleData* aData);
  







  static void MapBackgroundInto(const nsMappedAttributes* aAttributes,
                                nsRuleData* aData);
  







  static void MapBGColorInto(const nsMappedAttributes* aAttributes,
                             nsRuleData* aData);
  







  static void MapBackgroundAttributesInto(const nsMappedAttributes* aAttributes,
                                          nsRuleData* aData);
  







  static void MapScrollingAttributeInto(const nsMappedAttributes* aAttributes,
                                        nsRuleData* aData);
  



  enum PresContextFor
  {
    eForComposedDoc,
    eForUncomposedDoc
  };
  nsPresContext* GetPresContext(PresContextFor aFor);

  
  








  mozilla::dom::HTMLFormElement*
  FindAncestorForm(mozilla::dom::HTMLFormElement* aCurrentForm = nullptr);

  virtual void RecompileScriptEventListeners() override;

  



  static bool InNavQuirksMode(nsIDocument* aDoc);

  


  nsresult GetEditor(nsIEditor** aEditor);

  










  void GetURIAttr(nsIAtom* aAttr, nsIAtom* aBaseAttr, nsAString& aResult) const;

  





  bool GetURIAttr(nsIAtom* aAttr, nsIAtom* aBaseAttr, nsIURI** aURI) const;

  


  virtual bool IsDisabled() const {
    return false;
  }

  bool IsHidden() const
  {
    return HasAttr(kNameSpaceID_None, nsGkAtoms::hidden);
  }

  virtual bool IsLabelable() const override;
  virtual bool IsInteractiveHTMLContent(bool aIgnoreTabindex) const override;

  static bool TouchEventsEnabled(JSContext* , JSObject* );

  static inline bool
  CanHaveName(nsIAtom* aTag)
  {
    return aTag == nsGkAtoms::img ||
           aTag == nsGkAtoms::form ||
           aTag == nsGkAtoms::applet ||
           aTag == nsGkAtoms::embed ||
           aTag == nsGkAtoms::object;
  }
  static inline bool
  ShouldExposeNameAsHTMLDocumentProperty(Element* aElement)
  {
    return aElement->IsHTMLElement() &&
           CanHaveName(aElement->NodeInfo()->NameAtom());
  }
  static inline bool
  ShouldExposeIdAsHTMLDocumentProperty(Element* aElement)
  {
    return aElement->IsAnyOfHTMLElements(nsGkAtoms::img,
                                         nsGkAtoms::applet,
                                         nsGkAtoms::embed,
                                         nsGkAtoms::object);
  }

  static bool
  IsScrollGrabAllowed(JSContext*, JSObject*);

protected:
  


  void AddToNameTable(nsIAtom* aName) {
    NS_ASSERTION(HasName(), "Node doesn't have name?");
    nsIDocument* doc = GetCurrentDoc();
    if (doc && !IsInAnonymousSubtree()) {
      doc->AddToNameTable(this, aName);
    }
  }
  void RemoveFromNameTable() {
    if (HasName()) {
      nsIDocument* doc = GetCurrentDoc();
      if (doc) {
        doc->RemoveFromNameTable(this, GetParsedAttr(nsGkAtoms::name)->
                                         GetAtomValue());
      }
    }
  }

  



  void RegAccessKey()
  {
    if (HasFlag(NODE_HAS_ACCESSKEY)) {
      RegUnRegAccessKey(true);
    }
  }

  void UnregAccessKey()
  {
    if (HasFlag(NODE_HAS_ACCESSKEY)) {
      RegUnRegAccessKey(false);
    }
  }

private:
  void RegUnRegAccessKey(bool aDoReg);

protected:
  virtual nsresult AfterSetAttr(int32_t aNamespaceID, nsIAtom* aName,
                                const nsAttrValue* aValue, bool aNotify) override;

  virtual mozilla::EventListenerManager*
    GetEventListenerManagerForAttr(nsIAtom* aAttrName,
                                   bool* aDefer) override;

  virtual const nsAttrName* InternalGetExistingAttrNameFromQName(const nsAString& aStr) const override;

  




  nsresult NewURIFromString(const nsAutoString& aURISpec, nsIURI** aURI);

  void GetHTMLAttr(nsIAtom* aName, nsAString& aResult) const
  {
    GetAttr(kNameSpaceID_None, aName, aResult);
  }
  void GetHTMLAttr(nsIAtom* aName, mozilla::dom::DOMString& aResult) const
  {
    GetAttr(kNameSpaceID_None, aName, aResult);
  }
  void GetHTMLEnumAttr(nsIAtom* aName, nsAString& aResult) const
  {
    GetEnumAttr(aName, nullptr, aResult);
  }
  void GetHTMLURIAttr(nsIAtom* aName, nsAString& aResult) const
  {
    GetURIAttr(aName, nullptr, aResult);
  }

  void SetHTMLAttr(nsIAtom* aName, const nsAString& aValue)
  {
    SetAttr(kNameSpaceID_None, aName, aValue, true);
  }
  void SetHTMLAttr(nsIAtom* aName, const nsAString& aValue, mozilla::ErrorResult& aError)
  {
    mozilla::dom::Element::SetAttr(aName, aValue, aError);
  }
  void UnsetHTMLAttr(nsIAtom* aName, mozilla::ErrorResult& aError)
  {
    mozilla::dom::Element::UnsetAttr(aName, aError);
  }
  void SetHTMLBoolAttr(nsIAtom* aName, bool aValue, mozilla::ErrorResult& aError)
  {
    if (aValue) {
      SetHTMLAttr(aName, EmptyString(), aError);
    } else {
      UnsetHTMLAttr(aName, aError);
    }
  }
  void SetHTMLIntAttr(nsIAtom* aName, int32_t aValue, mozilla::ErrorResult& aError)
  {
    nsAutoString value;
    value.AppendInt(aValue);

    SetHTMLAttr(aName, value, aError);
  }

  








  nsresult SetAttrHelper(nsIAtom* aAttr, const nsAString& aValue);

  








  int32_t GetIntAttr(nsIAtom* aAttr, int32_t aDefault) const;

  







  nsresult SetIntAttr(nsIAtom* aAttr, int32_t aValue);

  








  uint32_t GetUnsignedIntAttr(nsIAtom* aAttr, uint32_t aDefault) const;

  







  void SetUnsignedIntAttr(nsIAtom* aName, uint32_t aValue,
                          mozilla::ErrorResult& aError)
  {
    nsAutoString value;
    value.AppendInt(aValue);

    SetHTMLAttr(aName, value, aError);
  }

  






  void SetDoubleAttr(nsIAtom* aAttr, double aValue, mozilla::ErrorResult& aRv)
  {
    nsAutoString value;
    value.AppendFloat(aValue);

    SetHTMLAttr(aAttr, value, aRv);
  }

  











  nsresult GetURIListAttr(nsIAtom* aAttr, nsAString& aResult);

  







  virtual already_AddRefed<nsIEditor> GetAssociatedEditor();

  





  mozilla::dom::Element* GetOffsetRect(mozilla::CSSIntRect& aRect);

  


  bool IsCurrentBodyElement();

  



  static void SyncEditorsOnSubtree(nsIContent* content);

  enum ContentEditableTristate {
    eInherit = -1,
    eFalse = 0,
    eTrue = 1
  };

  





  ContentEditableTristate GetContentEditableValue() const
  {
    static const nsIContent::AttrValuesArray values[] =
      { &nsGkAtoms::_false, &nsGkAtoms::_true, &nsGkAtoms::_empty, nullptr };

    if (!MayHaveContentEditableAttr())
      return eInherit;

    int32_t value = FindAttrValueIn(kNameSpaceID_None,
                                    nsGkAtoms::contenteditable, values,
                                    eIgnoreCase);

    return value > 0 ? eTrue : (value == 0 ? eFalse : eInherit);
  }

  
  already_AddRefed<nsIURI> GetHrefURIForAnchors() const;

  









  bool IsEditableRoot() const;

  nsresult SetUndoScopeInternal(bool aUndoScope);

private:
  void ChangeEditableState(int32_t aChange);

  bool mScrollgrab;
};

namespace mozilla {
namespace dom {
class HTMLFieldSetElement;
}
}

#define FORM_ELEMENT_FLAG_BIT(n_) NODE_FLAG_BIT(ELEMENT_TYPE_SPECIFIC_BITS_OFFSET + (n_))


enum {
  
  
  
  
  ADDED_TO_FORM =                         FORM_ELEMENT_FLAG_BIT(0),

  
  
  
  
  MAYBE_ORPHAN_FORM_ELEMENT =             FORM_ELEMENT_FLAG_BIT(1)
};





ASSERT_NODE_FLAGS_SPACE(ELEMENT_TYPE_SPECIFIC_BITS_OFFSET + 2);

#undef FORM_ELEMENT_FLAG_BIT




class nsGenericHTMLFormElement : public nsGenericHTMLElement,
                                 public nsIFormControl
{
public:
  explicit nsGenericHTMLFormElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);

  NS_DECL_ISUPPORTS_INHERITED

  nsINode* GetScopeChainParent() const override;

  virtual bool IsNodeOfType(uint32_t aFlags) const override;
  virtual void SaveSubtreeState() override;

  
  virtual mozilla::dom::HTMLFieldSetElement* GetFieldSet() override;
  virtual mozilla::dom::Element* GetFormElement() override;
  mozilla::dom::HTMLFormElement* GetForm() const
  {
    return mForm;
  }
  virtual void SetForm(nsIDOMHTMLFormElement* aForm) override;
  virtual void ClearForm(bool aRemoveFromForm) override;

  nsresult GetForm(nsIDOMHTMLFormElement** aForm);

  NS_IMETHOD SaveState() override
  {
    return NS_OK;
  }

  virtual bool RestoreState(nsPresState* aState) override
  {
    return false;
  }
  virtual bool AllowDrop() override
  {
    return true;
  }

  
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers) override;
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true) override;
  virtual IMEState GetDesiredIMEState() override;
  virtual mozilla::EventStates IntrinsicState() const override;

  virtual nsresult PreHandleEvent(
                     mozilla::EventChainPreVisitor& aVisitor) override;

  virtual bool IsDisabled() const override;

  








  virtual void FieldSetDisabledChanged(bool aNotify);

  void FieldSetFirstLegendChanged(bool aNotify) {
    UpdateFieldSet(aNotify);
  }

  






  void ForgetFieldSet(nsIContent* aFieldset);

  


  bool CanBeDisabled() const;

  virtual bool IsHTMLFocusable(bool aWithMouse, bool* aIsFocusable,
                                 int32_t* aTabIndex) override;

  virtual bool IsLabelable() const override;

protected:
  virtual ~nsGenericHTMLFormElement();

  virtual nsresult BeforeSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                                 const nsAttrValueOrString* aValue,
                                 bool aNotify) override;

  virtual nsresult AfterSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                                const nsAttrValue* aValue, bool aNotify) override;

  










  void UpdateFormOwner(bool aBindToTree, Element* aFormIdElement);

  


  void UpdateFieldSet(bool aNotify);

  





  Element* AddFormIdObserver();

  


  void RemoveFormIdObserver();

  




  static bool FormIdUpdated(Element* aOldElement, Element* aNewElement,
                              void* aData);

  
  bool IsElementDisabledForEvents(uint32_t aMessage, nsIFrame* aFrame);

  
  
  
  enum FocusTristate {
    eUnfocusable,
    eInactiveWindow,
    eActiveWindow
  };

  
  
  FocusTristate FocusState();

  
  mozilla::dom::HTMLFormElement* mForm;

  
  mozilla::dom::HTMLFieldSetElement* mFieldSet;
};

class nsGenericHTMLFormElementWithState : public nsGenericHTMLFormElement
{
public:
  explicit nsGenericHTMLFormElementWithState(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);

  



  nsPresState* GetPrimaryPresState();

  






  already_AddRefed<nsILayoutHistoryState>
    GetLayoutHistory(bool aRead);

  






  bool RestoreFormControlState();

  



  virtual void NodeInfoChanged(mozilla::dom::NodeInfo* aOldNodeInfo) override;

protected:
  

  nsresult GenerateStateKey();

  

  nsCString mStateKey;
};







#define NS_IMPL_STRING_ATTR_WITH_FALLBACK(_class, _method, _atom, _fallback) \
  NS_IMETHODIMP                                                              \
  _class::Get##_method(nsAString& aValue)                                    \
  {                                                                          \
    if (!GetAttr(kNameSpaceID_None, nsGkAtoms::_atom, aValue)) {             \
      _fallback(aValue);                                                     \
    }                                                                        \
    return NS_OK;                                                            \
  }                                                                          \
  NS_IMETHODIMP                                                              \
  _class::Set##_method(const nsAString& aValue)                              \
  {                                                                          \
    return SetAttrHelper(nsGkAtoms::_atom, aValue);                          \
  }






#define NS_IMPL_INT_ATTR(_class, _method, _atom)                    \
  NS_IMPL_INT_ATTR_DEFAULT_VALUE(_class, _method, _atom, 0)

#define NS_IMPL_INT_ATTR_DEFAULT_VALUE(_class, _method, _atom, _default)  \
  NS_IMETHODIMP                                                           \
  _class::Get##_method(int32_t* aValue)                                   \
  {                                                                       \
    *aValue = GetIntAttr(nsGkAtoms::_atom, _default);                     \
    return NS_OK;                                                         \
  }                                                                       \
  NS_IMETHODIMP                                                           \
  _class::Set##_method(int32_t aValue)                                    \
  {                                                                       \
    return SetIntAttr(nsGkAtoms::_atom, aValue);                          \
  }






#define NS_IMPL_UINT_ATTR(_class, _method, _atom)                         \
  NS_IMPL_UINT_ATTR_DEFAULT_VALUE(_class, _method, _atom, 0)

#define NS_IMPL_UINT_ATTR_DEFAULT_VALUE(_class, _method, _atom, _default) \
  NS_IMETHODIMP                                                           \
  _class::Get##_method(uint32_t* aValue)                                  \
  {                                                                       \
    *aValue = GetUnsignedIntAttr(nsGkAtoms::_atom, _default);             \
    return NS_OK;                                                         \
  }                                                                       \
  NS_IMETHODIMP                                                           \
  _class::Set##_method(uint32_t aValue)                                   \
  {                                                                       \
    mozilla::ErrorResult rv;                                              \
    SetUnsignedIntAttr(nsGkAtoms::_atom, aValue, rv);                     \
    return rv.StealNSResult();                                            \
  }







#define NS_IMPL_UINT_ATTR_NON_ZERO(_class, _method, _atom)                \
  NS_IMPL_UINT_ATTR_NON_ZERO_DEFAULT_VALUE(_class, _method, _atom, 1)

#define NS_IMPL_UINT_ATTR_NON_ZERO_DEFAULT_VALUE(_class, _method, _atom, _default) \
  NS_IMETHODIMP                                                           \
  _class::Get##_method(uint32_t* aValue)                                  \
  {                                                                       \
    *aValue = GetUnsignedIntAttr(nsGkAtoms::_atom, _default);             \
    return NS_OK;                                                         \
  }                                                                       \
  NS_IMETHODIMP                                                           \
  _class::Set##_method(uint32_t aValue)                                   \
  {                                                                       \
    if (aValue == 0) {                                                    \
      return NS_ERROR_DOM_INDEX_SIZE_ERR;                                 \
    }                                                                     \
    mozilla::ErrorResult rv;                                              \
    SetUnsignedIntAttr(nsGkAtoms::_atom, aValue, rv);                     \
    return rv.StealNSResult();                                            \
  }








#define NS_IMPL_URI_ATTR(_class, _method, _atom)                    \
  NS_IMETHODIMP                                                     \
  _class::Get##_method(nsAString& aValue)                           \
  {                                                                 \
    GetURIAttr(nsGkAtoms::_atom, nullptr, aValue);                  \
    return NS_OK;                                                   \
  }                                                                 \
  NS_IMETHODIMP                                                     \
  _class::Set##_method(const nsAString& aValue)                     \
  {                                                                 \
    return SetAttrHelper(nsGkAtoms::_atom, aValue);               \
  }

#define NS_IMPL_URI_ATTR_WITH_BASE(_class, _method, _atom, _base_atom)       \
  NS_IMETHODIMP                                                              \
  _class::Get##_method(nsAString& aValue)                                    \
  {                                                                          \
    GetURIAttr(nsGkAtoms::_atom, nsGkAtoms::_base_atom, aValue);             \
    return NS_OK;                                                            \
  }                                                                          \
  NS_IMETHODIMP                                                              \
  _class::Set##_method(const nsAString& aValue)                              \
  {                                                                          \
    return SetAttrHelper(nsGkAtoms::_atom, aValue);                        \
  }






#define NS_IMPL_ACTION_ATTR(_class, _method, _atom)                 \
  NS_IMETHODIMP                                                     \
  _class::Get##_method(nsAString& aValue)                           \
  {                                                                 \
    GetAttr(kNameSpaceID_None, nsGkAtoms::_atom, aValue);           \
    if (!aValue.IsEmpty()) {                                        \
      GetURIAttr(nsGkAtoms::_atom, nullptr, aValue);                 \
    }                                                               \
    return NS_OK;                                                   \
  }                                                                 \
  NS_IMETHODIMP                                                     \
  _class::Set##_method(const nsAString& aValue)                     \
  {                                                                 \
    return SetAttrHelper(nsGkAtoms::_atom, aValue);                 \
  }








#define NS_IMPL_NON_NEGATIVE_INT_ATTR(_class, _method, _atom)             \
  NS_IMPL_NON_NEGATIVE_INT_ATTR_DEFAULT_VALUE(_class, _method, _atom, -1)

#define NS_IMPL_NON_NEGATIVE_INT_ATTR_DEFAULT_VALUE(_class, _method, _atom, _default)  \
  NS_IMETHODIMP                                                           \
  _class::Get##_method(int32_t* aValue)                                   \
  {                                                                       \
    *aValue = GetIntAttr(nsGkAtoms::_atom, _default);                     \
    return NS_OK;                                                         \
  }                                                                       \
  NS_IMETHODIMP                                                           \
  _class::Set##_method(int32_t aValue)                                    \
  {                                                                       \
    if (aValue < 0) {                                                     \
      return NS_ERROR_DOM_INDEX_SIZE_ERR;                                 \
    }                                                                     \
    return SetIntAttr(nsGkAtoms::_atom, aValue);                          \
  }






#define NS_IMPL_ENUM_ATTR_DEFAULT_VALUE(_class, _method, _atom, _default) \
  NS_IMETHODIMP                                                           \
  _class::Get##_method(nsAString& aValue)                                 \
  {                                                                       \
    GetEnumAttr(nsGkAtoms::_atom, _default, aValue);                      \
    return NS_OK;                                                         \
  }                                                                       \
  NS_IMETHODIMP                                                           \
  _class::Set##_method(const nsAString& aValue)                           \
  {                                                                       \
    return SetAttrHelper(nsGkAtoms::_atom, aValue);                       \
  }







#define NS_IMPL_ENUM_ATTR_DEFAULT_MISSING_INVALID_VALUES(_class, _method, _atom, _defaultMissing, _defaultInvalid) \
  NS_IMETHODIMP                                                                                   \
  _class::Get##_method(nsAString& aValue)                                                         \
  {                                                                                               \
    GetEnumAttr(nsGkAtoms::_atom, _defaultMissing, _defaultInvalid, aValue);                      \
    return NS_OK;                                                                                 \
  }                                                                                               \
  NS_IMETHODIMP                                                                                   \
  _class::Set##_method(const nsAString& aValue)                                                   \
  {                                                                                               \
    return SetAttrHelper(nsGkAtoms::_atom, aValue);                                               \
  }

#define NS_INTERFACE_MAP_ENTRY_IF_TAG(_interface, _tag)                       \
  NS_INTERFACE_MAP_ENTRY_CONDITIONAL(_interface,                              \
                                     mNodeInfo->Equals(nsGkAtoms::_tag))





#define NS_DECLARE_NS_NEW_HTML_ELEMENT(_elementName)                       \
namespace mozilla {                                                        \
namespace dom {                                                            \
class HTML##_elementName##Element;                                         \
}                                                                          \
}                                                                          \
nsGenericHTMLElement*                                                      \
NS_NewHTML##_elementName##Element(already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo, \
                                  mozilla::dom::FromParser aFromParser = mozilla::dom::NOT_FROM_PARSER);

#define NS_DECLARE_NS_NEW_HTML_ELEMENT_AS_SHARED(_elementName)             \
inline nsGenericHTMLElement*                                               \
NS_NewHTML##_elementName##Element(already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo, \
                                  mozilla::dom::FromParser aFromParser = mozilla::dom::NOT_FROM_PARSER) \
{                                                                          \
  return NS_NewHTMLSharedElement(mozilla::Move(aNodeInfo), aFromParser);   \
}




#define NS_IMPL_NS_NEW_HTML_ELEMENT(_elementName)                            \
nsGenericHTMLElement*                                                        \
NS_NewHTML##_elementName##Element(already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo, \
                                  mozilla::dom::FromParser aFromParser)      \
{                                                                            \
  return new mozilla::dom::HTML##_elementName##Element(aNodeInfo);           \
}

#define NS_IMPL_NS_NEW_HTML_ELEMENT_CHECK_PARSER(_elementName)               \
nsGenericHTMLElement*                                                        \
NS_NewHTML##_elementName##Element(already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo, \
                                  mozilla::dom::FromParser aFromParser)      \
{                                                                            \
  return new mozilla::dom::HTML##_elementName##Element(aNodeInfo,            \
                                                       aFromParser);         \
}



nsGenericHTMLElement*
NS_NewHTMLElement(already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo,
                  mozilla::dom::FromParser aFromParser = mozilla::dom::NOT_FROM_PARSER);

NS_DECLARE_NS_NEW_HTML_ELEMENT(Shared)
NS_DECLARE_NS_NEW_HTML_ELEMENT(SharedList)
NS_DECLARE_NS_NEW_HTML_ELEMENT(SharedObject)

NS_DECLARE_NS_NEW_HTML_ELEMENT(Anchor)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Area)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Audio)
NS_DECLARE_NS_NEW_HTML_ELEMENT(BR)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Body)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Button)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Canvas)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Content)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Mod)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Data)
NS_DECLARE_NS_NEW_HTML_ELEMENT(DataList)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Div)
NS_DECLARE_NS_NEW_HTML_ELEMENT(FieldSet)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Font)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Form)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Frame)
NS_DECLARE_NS_NEW_HTML_ELEMENT(FrameSet)
NS_DECLARE_NS_NEW_HTML_ELEMENT(HR)
NS_DECLARE_NS_NEW_HTML_ELEMENT_AS_SHARED(Head)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Heading)
NS_DECLARE_NS_NEW_HTML_ELEMENT_AS_SHARED(Html)
NS_DECLARE_NS_NEW_HTML_ELEMENT(IFrame)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Image)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Input)
NS_DECLARE_NS_NEW_HTML_ELEMENT(LI)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Label)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Legend)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Link)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Map)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Menu)
NS_DECLARE_NS_NEW_HTML_ELEMENT(MenuItem)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Meta)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Meter)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Object)
NS_DECLARE_NS_NEW_HTML_ELEMENT(OptGroup)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Option)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Output)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Paragraph)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Picture)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Pre)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Progress)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Script)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Select)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Shadow)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Source)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Span)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Style)
NS_DECLARE_NS_NEW_HTML_ELEMENT(TableCaption)
NS_DECLARE_NS_NEW_HTML_ELEMENT(TableCell)
NS_DECLARE_NS_NEW_HTML_ELEMENT(TableCol)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Table)
NS_DECLARE_NS_NEW_HTML_ELEMENT(TableRow)
NS_DECLARE_NS_NEW_HTML_ELEMENT(TableSection)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Tbody)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Template)
NS_DECLARE_NS_NEW_HTML_ELEMENT(TextArea)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Tfoot)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Thead)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Time)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Title)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Track)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Unknown)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Video)

inline nsISupports*
ToSupports(nsGenericHTMLElement* aHTMLElement)
{
  return static_cast<nsIContent*>(aHTMLElement);
}

inline nsISupports*
ToCanonicalSupports(nsGenericHTMLElement* aHTMLElement)
{
  return static_cast<nsIContent*>(aHTMLElement);
}

#endif 
