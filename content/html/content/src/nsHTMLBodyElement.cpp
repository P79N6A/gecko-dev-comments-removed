




#include "mozilla/Util.h"

#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsIDOMHTMLBodyElement.h"
#include "nsGenericHTMLElement.h"
#include "nsAttrValueInlines.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsIDocument.h"
#include "nsHTMLStyleSheet.h"
#include "nsIMarkupDocumentViewer.h"
#include "nsMappedAttributes.h"
#include "nsRuleData.h"
#include "nsIDocShell.h"
#include "nsIEditorDocShell.h"
#include "nsRuleWalker.h"
#include "jspubtd.h"



using namespace mozilla;
using namespace mozilla::dom;

class nsHTMLBodyElement;

class BodyRule: public nsIStyleRule {
public:
  BodyRule(nsHTMLBodyElement* aPart);
  virtual ~BodyRule();

  NS_DECL_ISUPPORTS

  
  virtual void MapRuleInfoInto(nsRuleData* aRuleData);
#ifdef DEBUG
  virtual void List(FILE* out = stdout, int32_t aIndent = 0) const;
#endif

  nsHTMLBodyElement*  mPart;  
};



class nsHTMLBodyElement : public nsGenericHTMLElement,
                          public nsIDOMHTMLBodyElement
{
public:
  using Element::GetText;
  using Element::SetText;

  nsHTMLBodyElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~nsHTMLBodyElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  
  NS_DECL_NSIDOMHTMLBODYELEMENT

  
  
#define EVENT(name_, id_, type_, struct_)
#define FORWARDED_EVENT(name_, id_, type_, struct_)               \
    NS_IMETHOD GetOn##name_(JSContext *cx, jsval *vp);            \
    NS_IMETHOD SetOn##name_(JSContext *cx, const jsval &v);
#include "nsEventNameList.h"
#undef FORWARDED_EVENT
#undef EVENT

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
  virtual nsXPCClassInfo* GetClassInfo();
  virtual nsIDOMNode* AsDOMNode() { return this; }
private:
  nsresult GetColorHelper(nsIAtom* aAtom, nsAString& aColor);

protected:
  BodyRule* mContentStyleRule;
};



BodyRule::BodyRule(nsHTMLBodyElement* aPart)
{
  mPart = aPart;
}

BodyRule::~BodyRule()
{
}

NS_IMPL_ISUPPORTS1(BodyRule, nsIStyleRule)

 void
BodyRule::MapRuleInfoInto(nsRuleData* aData)
{
  if (!(aData->mSIDs & NS_STYLE_INHERIT_BIT(Margin)) || !mPart)
    return; 

  int32_t bodyMarginWidth  = -1;
  int32_t bodyMarginHeight = -1;
  int32_t bodyTopMargin = -1;
  int32_t bodyBottomMargin = -1;
  int32_t bodyLeftMargin = -1;
  int32_t bodyRightMargin = -1;

  
  NS_ASSERTION(aData->mPresContext, "null presContext in ruleNode was unexpected");
  nsCompatibility mode = aData->mPresContext->CompatibilityMode();


  const nsAttrValue* value;
  if (mPart->GetAttrCount() > 0) {
    
    value = mPart->GetParsedAttr(nsGkAtoms::marginwidth);
    if (value && value->Type() == nsAttrValue::eInteger) {
      bodyMarginWidth = value->GetIntegerValue();
      if (bodyMarginWidth < 0) bodyMarginWidth = 0;
      nsCSSValue* marginLeft = aData->ValueForMarginLeftValue();
      if (marginLeft->GetUnit() == eCSSUnit_Null)
        marginLeft->SetFloatValue((float)bodyMarginWidth, eCSSUnit_Pixel);
      nsCSSValue* marginRight = aData->ValueForMarginRightValue();
      if (marginRight->GetUnit() == eCSSUnit_Null)
        marginRight->SetFloatValue((float)bodyMarginWidth, eCSSUnit_Pixel);
    }

    value = mPart->GetParsedAttr(nsGkAtoms::marginheight);
    if (value && value->Type() == nsAttrValue::eInteger) {
      bodyMarginHeight = value->GetIntegerValue();
      if (bodyMarginHeight < 0) bodyMarginHeight = 0;
      nsCSSValue* marginTop = aData->ValueForMarginTop();
      if (marginTop->GetUnit() == eCSSUnit_Null)
        marginTop->SetFloatValue((float)bodyMarginHeight, eCSSUnit_Pixel);
      nsCSSValue* marginBottom = aData->ValueForMarginBottom();
      if (marginBottom->GetUnit() == eCSSUnit_Null)
        marginBottom->SetFloatValue((float)bodyMarginHeight, eCSSUnit_Pixel);
    }

    if (eCompatibility_NavQuirks == mode){
      
      value = mPart->GetParsedAttr(nsGkAtoms::topmargin);
      if (value && value->Type() == nsAttrValue::eInteger) {
        bodyTopMargin = value->GetIntegerValue();
        if (bodyTopMargin < 0) bodyTopMargin = 0;
        nsCSSValue* marginTop = aData->ValueForMarginTop();
        if (marginTop->GetUnit() == eCSSUnit_Null)
          marginTop->SetFloatValue((float)bodyTopMargin, eCSSUnit_Pixel);
      }

      
      value = mPart->GetParsedAttr(nsGkAtoms::bottommargin);
      if (value && value->Type() == nsAttrValue::eInteger) {
        bodyBottomMargin = value->GetIntegerValue();
        if (bodyBottomMargin < 0) bodyBottomMargin = 0;
        nsCSSValue* marginBottom = aData->ValueForMarginBottom();
        if (marginBottom->GetUnit() == eCSSUnit_Null)
          marginBottom->SetFloatValue((float)bodyBottomMargin, eCSSUnit_Pixel);
      }

      
      value = mPart->GetParsedAttr(nsGkAtoms::leftmargin);
      if (value && value->Type() == nsAttrValue::eInteger) {
        bodyLeftMargin = value->GetIntegerValue();
        if (bodyLeftMargin < 0) bodyLeftMargin = 0;
        nsCSSValue* marginLeft = aData->ValueForMarginLeftValue();
        if (marginLeft->GetUnit() == eCSSUnit_Null)
          marginLeft->SetFloatValue((float)bodyLeftMargin, eCSSUnit_Pixel);
      }

      
      value = mPart->GetParsedAttr(nsGkAtoms::rightmargin);
      if (value && value->Type() == nsAttrValue::eInteger) {
        bodyRightMargin = value->GetIntegerValue();
        if (bodyRightMargin < 0) bodyRightMargin = 0;
        nsCSSValue* marginRight = aData->ValueForMarginRightValue();
        if (marginRight->GetUnit() == eCSSUnit_Null)
          marginRight->SetFloatValue((float)bodyRightMargin, eCSSUnit_Pixel);
      }
    }

  }

  
  
  if (bodyMarginWidth == -1 || bodyMarginHeight == -1) {
    nsCOMPtr<nsISupports> container = aData->mPresContext->GetContainer();
    if (container) {
      nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(container));
      if (docShell) {
        nscoord frameMarginWidth=-1;  
        nscoord frameMarginHeight=-1; 
        docShell->GetMarginWidth(&frameMarginWidth); 
        docShell->GetMarginHeight(&frameMarginHeight); 
        if ((frameMarginWidth >= 0) && (bodyMarginWidth == -1)) { 
          if (eCompatibility_NavQuirks == mode) {
            if ((bodyMarginHeight == -1) && (0 > frameMarginHeight)) 
              frameMarginHeight = 0;
          }
        }
        if ((frameMarginHeight >= 0) && (bodyMarginHeight == -1)) { 
          if (eCompatibility_NavQuirks == mode) {
            if ((bodyMarginWidth == -1) && (0 > frameMarginWidth)) 
              frameMarginWidth = 0;
          }
        }

        if ((bodyMarginWidth == -1) && (frameMarginWidth >= 0)) {
          nsCSSValue* marginLeft = aData->ValueForMarginLeftValue();
          if (marginLeft->GetUnit() == eCSSUnit_Null)
            marginLeft->SetFloatValue((float)frameMarginWidth, eCSSUnit_Pixel);
          nsCSSValue* marginRight = aData->ValueForMarginRightValue();
          if (marginRight->GetUnit() == eCSSUnit_Null)
            marginRight->SetFloatValue((float)frameMarginWidth, eCSSUnit_Pixel);
        }

        if ((bodyMarginHeight == -1) && (frameMarginHeight >= 0)) {
          nsCSSValue* marginTop = aData->ValueForMarginTop();
          if (marginTop->GetUnit() == eCSSUnit_Null)
            marginTop->SetFloatValue((float)frameMarginHeight, eCSSUnit_Pixel);
          nsCSSValue* marginBottom = aData->ValueForMarginBottom();
          if (marginBottom->GetUnit() == eCSSUnit_Null)
            marginBottom->SetFloatValue((float)frameMarginHeight, eCSSUnit_Pixel);
        }
      }
    }
  }
}

#ifdef DEBUG
 void
BodyRule::List(FILE* out, int32_t aIndent) const
{
  for (int32_t index = aIndent; --index >= 0; ) fputs("  ", out);
  fputs("[body rule] {}\n", out);
}
#endif




NS_IMPL_NS_NEW_HTML_ELEMENT(Body)


nsHTMLBodyElement::nsHTMLBodyElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo),
    mContentStyleRule(nullptr)
{
}

nsHTMLBodyElement::~nsHTMLBodyElement()
{
  if (mContentStyleRule) {
    mContentStyleRule->mPart = nullptr;
    NS_RELEASE(mContentStyleRule);
  }
}


NS_IMPL_ADDREF_INHERITED(nsHTMLBodyElement, Element)
NS_IMPL_RELEASE_INHERITED(nsHTMLBodyElement, Element)

DOMCI_NODE_DATA(HTMLBodyElement, nsHTMLBodyElement)


NS_INTERFACE_TABLE_HEAD(nsHTMLBodyElement)
  NS_HTML_CONTENT_INTERFACE_TABLE1(nsHTMLBodyElement, nsIDOMHTMLBodyElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLBodyElement,
                                               nsGenericHTMLElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLBodyElement)

NS_IMPL_ELEMENT_CLONE(nsHTMLBodyElement)


NS_IMPL_STRING_ATTR(nsHTMLBodyElement, Background, background)
NS_IMPL_STRING_ATTR(nsHTMLBodyElement, VLink, vlink)
NS_IMPL_STRING_ATTR(nsHTMLBodyElement, ALink, alink)
NS_IMPL_STRING_ATTR(nsHTMLBodyElement, Link, link)
NS_IMPL_STRING_ATTR(nsHTMLBodyElement, Text, text)
NS_IMPL_STRING_ATTR(nsHTMLBodyElement, BgColor, bgcolor)

bool
nsHTMLBodyElement::ParseAttribute(int32_t aNamespaceID,
                                  nsIAtom* aAttribute,
                                  const nsAString& aValue,
                                  nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::bgcolor ||
        aAttribute == nsGkAtoms::text ||
        aAttribute == nsGkAtoms::link ||
        aAttribute == nsGkAtoms::alink ||
        aAttribute == nsGkAtoms::vlink) {
      return aResult.ParseColor(aValue);
    }
    if (aAttribute == nsGkAtoms::marginwidth ||
        aAttribute == nsGkAtoms::marginheight ||
        aAttribute == nsGkAtoms::topmargin ||
        aAttribute == nsGkAtoms::bottommargin ||
        aAttribute == nsGkAtoms::leftmargin ||
        aAttribute == nsGkAtoms::rightmargin) {
      return aResult.ParseIntWithBounds(aValue, 0);
    }
  }

  return nsGenericHTMLElement::ParseBackgroundAttribute(aNamespaceID,
                                                        aAttribute, aValue,
                                                        aResult) ||
         nsGenericHTMLElement::ParseAttribute(aNamespaceID, aAttribute, aValue,
                                              aResult);
}

void
nsHTMLBodyElement::UnbindFromTree(bool aDeep, bool aNullParent)
{
  if (mContentStyleRule) {
    mContentStyleRule->mPart = nullptr;

    
    NS_RELEASE(mContentStyleRule);
  }

  nsGenericHTMLElement::UnbindFromTree(aDeep, aNullParent);  
}

static 
void MapAttributesIntoRule(const nsMappedAttributes* aAttributes, nsRuleData* aData)
{
  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(Display)) {
    
    nsIPresShell *presShell = aData->mPresContext->GetPresShell();
    if (presShell) {
      nsIDocument *doc = presShell->GetDocument();
      if (doc) {
        nsHTMLStyleSheet* styleSheet = doc->GetAttributeStyleSheet();
        if (styleSheet) {
          const nsAttrValue* value;
          nscolor color;
          value = aAttributes->GetAttr(nsGkAtoms::link);
          if (value && value->GetColorValue(color)) {
            styleSheet->SetLinkColor(color);
          }

          value = aAttributes->GetAttr(nsGkAtoms::alink);
          if (value && value->GetColorValue(color)) {
            styleSheet->SetActiveLinkColor(color);
          }

          value = aAttributes->GetAttr(nsGkAtoms::vlink);
          if (value && value->GetColorValue(color)) {
            styleSheet->SetVisitedLinkColor(color);
          }
        }
      }
    }
  }

  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(Color)) {
    nsCSSValue *colorValue = aData->ValueForColor();
    if (colorValue->GetUnit() == eCSSUnit_Null &&
        aData->mPresContext->UseDocumentColors()) {
      
      nscolor color;
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::text);
      if (value && value->GetColorValue(color))
        colorValue->SetColorValue(color);
    }
  }

  nsGenericHTMLElement::MapBackgroundAttributesInto(aAttributes, aData);
  nsGenericHTMLElement::MapCommonAttributesInto(aAttributes, aData);
}

nsMapRuleToAttributesFunc
nsHTMLBodyElement::GetAttributeMappingFunction() const
{
  return &MapAttributesIntoRule;
}

NS_IMETHODIMP
nsHTMLBodyElement::WalkContentStyleRules(nsRuleWalker* aRuleWalker)
{
  nsGenericHTMLElement::WalkContentStyleRules(aRuleWalker);

  if (!mContentStyleRule && IsInDoc()) {
    
    
    mContentStyleRule = new BodyRule(this);
    NS_IF_ADDREF(mContentStyleRule);
  }
  if (aRuleWalker && mContentStyleRule) {
    aRuleWalker->Forward(mContentStyleRule);
  }
  return NS_OK;
}

NS_IMETHODIMP_(bool)
nsHTMLBodyElement::IsAttributeMapped(const nsIAtom* aAttribute) const
{
  static const MappedAttributeEntry attributes[] = {
    { &nsGkAtoms::link },
    { &nsGkAtoms::vlink },
    { &nsGkAtoms::alink },
    { &nsGkAtoms::text },
    
    
    
    
    { &nsGkAtoms::marginwidth },
    { &nsGkAtoms::marginheight },
    { nullptr },
  };

  static const MappedAttributeEntry* const map[] = {
    attributes,
    sCommonAttributeMap,
    sBackgroundAttributeMap,
  };

  return FindAttributeDependence(aAttribute, map);
}

already_AddRefed<nsIEditor>
nsHTMLBodyElement::GetAssociatedEditor()
{
  nsIEditor* editor = nullptr;
  if (NS_SUCCEEDED(GetEditorInternal(&editor)) && editor) {
    return editor;
  }

  
  if (!IsCurrentBodyElement()) {
    return nullptr;
  }

  
  nsPresContext* presContext = GetPresContext();
  if (!presContext) {
    return nullptr;
  }

  nsCOMPtr<nsISupports> container = presContext->GetContainer();
  nsCOMPtr<nsIEditorDocShell> editorDocShell = do_QueryInterface(container);
  if (!editorDocShell) {
    return nullptr;
  }

  editorDocShell->GetEditor(&editor);
  return editor;
}







#define EVENT(name_, id_, type_, struct_)
#define FORWARDED_EVENT(name_, id_, type_, struct_)                 \
  NS_IMETHODIMP nsHTMLBodyElement::GetOn##name_(JSContext *cx,      \
                                           jsval *vp) {             \
    /* XXXbz note to self: add tests for this! */                   \
    nsPIDOMWindow* win = OwnerDoc()->GetInnerWindow();           \
    if (win && win->IsInnerWindow()) {                              \
      nsCOMPtr<nsIInlineEventHandlers> ev = do_QueryInterface(win); \
      return ev->GetOn##name_(cx, vp);                              \
    }                                                               \
    *vp = JSVAL_NULL;                                               \
    return NS_OK;                                                   \
  }                                                                 \
  NS_IMETHODIMP nsHTMLBodyElement::SetOn##name_(JSContext *cx,      \
                                           const jsval &v) {        \
    nsPIDOMWindow* win = OwnerDoc()->GetInnerWindow();           \
    if (win && win->IsInnerWindow()) {                              \
      nsCOMPtr<nsIInlineEventHandlers> ev = do_QueryInterface(win); \
      return ev->SetOn##name_(cx, v);                               \
    }                                                               \
    return NS_OK;                                                   \
  }
#define WINDOW_EVENT(name_, id_, type_, struct_)                  \
  NS_IMETHODIMP nsHTMLBodyElement::GetOn##name_(JSContext *cx,    \
                                                jsval *vp) {      \
    nsPIDOMWindow* win = OwnerDoc()->GetInnerWindow();         \
    if (win && win->IsInnerWindow()) {                            \
      return win->GetOn##name_(cx, vp);                           \
    }                                                             \
    *vp = JSVAL_NULL;                                             \
    return NS_OK;                                                 \
  }                                                               \
  NS_IMETHODIMP nsHTMLBodyElement::SetOn##name_(JSContext *cx,    \
                                                const jsval &v) { \
    nsPIDOMWindow* win = OwnerDoc()->GetInnerWindow();         \
    if (win && win->IsInnerWindow()) {                            \
      return win->SetOn##name_(cx, v);                            \
    }                                                             \
    return NS_OK;                                                 \
  }
#include "nsEventNameList.h"
#undef WINDOW_EVENT
#undef FORWARDED_EVENT
#undef EVENT
