




































#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsIDOMHTMLBodyElement.h"
#include "nsIDOMEventTarget.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsIDocument.h"
#include "nsIHTMLDocument.h"
#include "nsHTMLStyleSheet.h"
#include "nsIHTMLCSSStyleSheet.h"
#include "nsICSSStyleRule.h"
#include "nsIContentViewer.h"
#include "nsIMarkupDocumentViewer.h"
#include "nsMappedAttributes.h"
#include "nsRuleData.h"
#include "nsIFrame.h"
#include "nsIDocShell.h"
#include "nsIEditorDocShell.h"
#include "nsCOMPtr.h"
#include "nsRuleWalker.h"



class nsHTMLBodyElement;

class BodyRule: public nsIStyleRule {
public:
  BodyRule(nsHTMLBodyElement* aPart);
  virtual ~BodyRule();

  NS_DECL_ISUPPORTS

  
  NS_IMETHOD MapRuleInfoInto(nsRuleData* aRuleData);
#ifdef DEBUG
  NS_IMETHOD List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

  nsHTMLBodyElement*  mPart;  
};



class nsHTMLBodyElement : public nsGenericHTMLElement,
                          public nsIDOMHTMLBodyElement
{
public:
  nsHTMLBodyElement(nsINodeInfo *aNodeInfo);
  virtual ~nsHTMLBodyElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  
  NS_DECL_NSIDOMHTMLBODYELEMENT

  virtual PRBool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  virtual void UnbindFromTree(PRBool aDeep = PR_TRUE,
                              PRBool aNullParent = PR_TRUE);
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;
  NS_IMETHOD WalkContentStyleRules(nsRuleWalker* aRuleWalker);
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const;
  virtual already_AddRefed<nsIEditor> GetAssociatedEditor();
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
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

NS_IMETHODIMP
BodyRule::MapRuleInfoInto(nsRuleData* aData)
{
  if (!aData || !(aData->mSIDs & NS_STYLE_INHERIT_BIT(Margin)) || !aData->mMarginData || !mPart)
    return NS_OK; 

  PRInt32 bodyMarginWidth  = -1;
  PRInt32 bodyMarginHeight = -1;
  PRInt32 bodyTopMargin = -1;
  PRInt32 bodyBottomMargin = -1;
  PRInt32 bodyLeftMargin = -1;
  PRInt32 bodyRightMargin = -1;

  
  NS_ASSERTION(aData->mPresContext, "null presContext in ruleNode was unexpected");
  nsCompatibility mode = aData->mPresContext->CompatibilityMode();


  const nsAttrValue* value;
  if (mPart->GetAttrCount() > 0) {
    
    value = mPart->GetParsedAttr(nsGkAtoms::marginwidth);
    if (value && value->Type() == nsAttrValue::eInteger) {
      bodyMarginWidth = value->GetIntegerValue();
      if (bodyMarginWidth < 0) bodyMarginWidth = 0;
      nsCSSRect& margin = aData->mMarginData->mMargin;
      if (margin.mLeft.GetUnit() == eCSSUnit_Null)
        margin.mLeft.SetFloatValue((float)bodyMarginWidth, eCSSUnit_Pixel);
      if (margin.mRight.GetUnit() == eCSSUnit_Null)
        margin.mRight.SetFloatValue((float)bodyMarginWidth, eCSSUnit_Pixel);
    }

    value = mPart->GetParsedAttr(nsGkAtoms::marginheight);
    if (value && value->Type() == nsAttrValue::eInteger) {
      bodyMarginHeight = value->GetIntegerValue();
      if (bodyMarginHeight < 0) bodyMarginHeight = 0;
      nsCSSRect& margin = aData->mMarginData->mMargin;
      if (margin.mTop.GetUnit() == eCSSUnit_Null)
        margin.mTop.SetFloatValue((float)bodyMarginHeight, eCSSUnit_Pixel);
      if (margin.mBottom.GetUnit() == eCSSUnit_Null)
        margin.mBottom.SetFloatValue((float)bodyMarginHeight, eCSSUnit_Pixel);
    }

    if (eCompatibility_NavQuirks == mode){
      
      value = mPart->GetParsedAttr(nsGkAtoms::topmargin);
      if (value && value->Type() == nsAttrValue::eInteger) {
        bodyTopMargin = value->GetIntegerValue();
        if (bodyTopMargin < 0) bodyTopMargin = 0;
        nsCSSRect& margin = aData->mMarginData->mMargin;
        if (margin.mTop.GetUnit() == eCSSUnit_Null)
          margin.mTop.SetFloatValue((float)bodyTopMargin, eCSSUnit_Pixel);
      }

      
      value = mPart->GetParsedAttr(nsGkAtoms::bottommargin);
      if (value && value->Type() == nsAttrValue::eInteger) {
        bodyBottomMargin = value->GetIntegerValue();
        if (bodyBottomMargin < 0) bodyBottomMargin = 0;
        nsCSSRect& margin = aData->mMarginData->mMargin;
        if (margin.mBottom.GetUnit() == eCSSUnit_Null)
          margin.mBottom.SetFloatValue((float)bodyBottomMargin, eCSSUnit_Pixel);
      }

      
      value = mPart->GetParsedAttr(nsGkAtoms::leftmargin);
      if (value && value->Type() == nsAttrValue::eInteger) {
        bodyLeftMargin = value->GetIntegerValue();
        if (bodyLeftMargin < 0) bodyLeftMargin = 0;
        nsCSSRect& margin = aData->mMarginData->mMargin;
        if (margin.mLeft.GetUnit() == eCSSUnit_Null)
          margin.mLeft.SetFloatValue((float)bodyLeftMargin, eCSSUnit_Pixel);
      }

      
      value = mPart->GetParsedAttr(nsGkAtoms::rightmargin);
      if (value && value->Type() == nsAttrValue::eInteger) {
        bodyRightMargin = value->GetIntegerValue();
        if (bodyRightMargin < 0) bodyRightMargin = 0;
        nsCSSRect& margin = aData->mMarginData->mMargin;
        if (margin.mRight.GetUnit() == eCSSUnit_Null)
          margin.mRight.SetFloatValue((float)bodyRightMargin, eCSSUnit_Pixel);
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
          nsCSSRect& margin = aData->mMarginData->mMargin;
          if (margin.mLeft.GetUnit() == eCSSUnit_Null)
            margin.mLeft.SetFloatValue((float)frameMarginWidth, eCSSUnit_Pixel);
          if (margin.mRight.GetUnit() == eCSSUnit_Null)
            margin.mRight.SetFloatValue((float)frameMarginWidth, eCSSUnit_Pixel);
        }

        if ((bodyMarginHeight == -1) && (frameMarginHeight >= 0)) {
          nsCSSRect& margin = aData->mMarginData->mMargin;
          if (margin.mTop.GetUnit() == eCSSUnit_Null)
            margin.mTop.SetFloatValue((float)frameMarginHeight, eCSSUnit_Pixel);
          if (margin.mBottom.GetUnit() == eCSSUnit_Null)
            margin.mBottom.SetFloatValue((float)frameMarginHeight, eCSSUnit_Pixel);
        }
      }
    }
  }
  return NS_OK;
}

#ifdef DEBUG
NS_IMETHODIMP
BodyRule::List(FILE* out, PRInt32 aIndent) const
{
  return NS_OK;
}
#endif




NS_IMPL_NS_NEW_HTML_ELEMENT(Body)


nsHTMLBodyElement::nsHTMLBodyElement(nsINodeInfo *aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo),
    mContentStyleRule(nsnull)
{
}

nsHTMLBodyElement::~nsHTMLBodyElement()
{
  if (mContentStyleRule) {
    mContentStyleRule->mPart = nsnull;
    NS_RELEASE(mContentStyleRule);
  }
}


NS_IMPL_ADDREF_INHERITED(nsHTMLBodyElement, nsGenericElement) 
NS_IMPL_RELEASE_INHERITED(nsHTMLBodyElement, nsGenericElement) 


NS_INTERFACE_TABLE_HEAD(nsHTMLBodyElement)
  NS_HTML_CONTENT_INTERFACE_TABLE1(nsHTMLBodyElement, nsIDOMHTMLBodyElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLBodyElement,
                                               nsGenericHTMLElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLBodyElement)

NS_IMPL_ELEMENT_CLONE(nsHTMLBodyElement)


NS_IMPL_URI_ATTR(nsHTMLBodyElement, Background, background)

NS_IMPL_STRING_ATTR(nsHTMLBodyElement, VLink, vlink)
NS_IMPL_STRING_ATTR(nsHTMLBodyElement, ALink, alink)
NS_IMPL_STRING_ATTR(nsHTMLBodyElement, Link, link)
NS_IMPL_STRING_ATTR(nsHTMLBodyElement, Text, text)
NS_IMPL_STRING_ATTR(nsHTMLBodyElement, BgColor, bgcolor)

PRBool
nsHTMLBodyElement::ParseAttribute(PRInt32 aNamespaceID,
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
      return aResult.ParseColor(aValue, GetOwnerDoc());
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

  return nsGenericHTMLElement::ParseAttribute(aNamespaceID, aAttribute, aValue,
                                              aResult);
}

void
nsHTMLBodyElement::UnbindFromTree(PRBool aDeep, PRBool aNullParent)
{
  if (mContentStyleRule) {
    mContentStyleRule->mPart = nsnull;

    
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
    if (aData->mColorData->mColor.GetUnit() == eCSSUnit_Null &&
        aData->mPresContext->UseDocumentColors()) {
      
      nscolor color;
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::text);
      if (value && value->GetColorValue(color))
        aData->mColorData->mColor.SetColorValue(color);
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

NS_IMETHODIMP_(PRBool)
nsHTMLBodyElement::IsAttributeMapped(const nsIAtom* aAttribute) const
{
  static const MappedAttributeEntry attributes[] = {
    { &nsGkAtoms::link },
    { &nsGkAtoms::vlink },
    { &nsGkAtoms::alink },
    { &nsGkAtoms::text },
    
    
    
    
    { &nsGkAtoms::marginwidth },
    { &nsGkAtoms::marginheight },
    { nsnull },
  };

  static const MappedAttributeEntry* const map[] = {
    attributes,
    sCommonAttributeMap,
    sBackgroundAttributeMap,
  };

  return FindAttributeDependence(aAttribute, map, NS_ARRAY_LENGTH(map));
}

already_AddRefed<nsIEditor>
nsHTMLBodyElement::GetAssociatedEditor()
{
  nsIEditor* editor = nsnull;
  if (NS_SUCCEEDED(GetEditorInternal(&editor)) && editor) {
    return editor;
  }

  
  if (!IsCurrentBodyElement()) {
    return nsnull;
  }

  
  nsPresContext* presContext = GetPresContext();
  if (!presContext) {
    return nsnull;
  }

  nsCOMPtr<nsISupports> container = presContext->GetContainer();
  nsCOMPtr<nsIEditorDocShell> editorDocShell = do_QueryInterface(container);
  if (!editorDocShell) {
    return nsnull;
  }

  editorDocShell->GetEditor(&editor);
  return editor;
}
