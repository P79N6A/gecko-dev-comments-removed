




#include "nsHTMLFrameSetElement.h"
#include "jsapi.h"

NS_IMPL_NS_NEW_HTML_ELEMENT(FrameSet)


nsHTMLFrameSetElement::nsHTMLFrameSetElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo), mNumRows(0), mNumCols(0),
    mCurrentRowColHint(NS_STYLE_HINT_REFLOW)
{
}

nsHTMLFrameSetElement::~nsHTMLFrameSetElement()
{
}


NS_IMPL_ADDREF_INHERITED(nsHTMLFrameSetElement, nsGenericElement) 
NS_IMPL_RELEASE_INHERITED(nsHTMLFrameSetElement, nsGenericElement) 


DOMCI_NODE_DATA(HTMLFrameSetElement, nsHTMLFrameSetElement)


NS_INTERFACE_TABLE_HEAD(nsHTMLFrameSetElement)
  NS_HTML_CONTENT_INTERFACE_TABLE1(nsHTMLFrameSetElement,
                                   nsIDOMHTMLFrameSetElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLFrameSetElement,
                                               nsGenericHTMLElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLFrameSetElement)


NS_IMPL_ELEMENT_CLONE(nsHTMLFrameSetElement)


NS_IMPL_STRING_ATTR(nsHTMLFrameSetElement, Cols, cols)
NS_IMPL_STRING_ATTR(nsHTMLFrameSetElement, Rows, rows)

nsresult
nsHTMLFrameSetElement::SetAttr(PRInt32 aNameSpaceID,
                               nsIAtom* aAttribute,
                               nsIAtom* aPrefix,
                               const nsAString& aValue,
                               bool aNotify)
{
  nsresult rv;
  







  if (aAttribute == nsGkAtoms::rows && aNameSpaceID == kNameSpaceID_None) {
    PRInt32 oldRows = mNumRows;
    ParseRowCol(aValue, mNumRows, getter_Transfers(mRowSpecs));
    
    if (mNumRows != oldRows) {
      mCurrentRowColHint = NS_STYLE_HINT_FRAMECHANGE;
    }
  } else if (aAttribute == nsGkAtoms::cols &&
             aNameSpaceID == kNameSpaceID_None) {
    PRInt32 oldCols = mNumCols;
    ParseRowCol(aValue, mNumCols, getter_Transfers(mColSpecs));

    if (mNumCols != oldCols) {
      mCurrentRowColHint = NS_STYLE_HINT_FRAMECHANGE;
    }
  }
  
  rv = nsGenericHTMLElement::SetAttr(aNameSpaceID, aAttribute, aPrefix,
                                     aValue, aNotify);
  mCurrentRowColHint = NS_STYLE_HINT_REFLOW;
  
  return rv;
}

nsresult
nsHTMLFrameSetElement::GetRowSpec(PRInt32 *aNumValues,
                                  const nsFramesetSpec** aSpecs)
{
  NS_PRECONDITION(aNumValues, "Must have a pointer to an integer here!");
  NS_PRECONDITION(aSpecs, "Must have a pointer to an array of nsFramesetSpecs");
  *aNumValues = 0;
  *aSpecs = nullptr;
  
  if (!mRowSpecs) {
    const nsAttrValue* value = GetParsedAttr(nsGkAtoms::rows);
    if (value && value->Type() == nsAttrValue::eString) {
      nsresult rv = ParseRowCol(value->GetStringValue(), mNumRows,
                                getter_Transfers(mRowSpecs));
      NS_ENSURE_SUCCESS(rv, rv);
    }

    if (!mRowSpecs) {  
      mRowSpecs = new nsFramesetSpec[1];
      if (!mRowSpecs) {
        mNumRows = 0;
        return NS_ERROR_OUT_OF_MEMORY;
      }
      mNumRows = 1;
      mRowSpecs[0].mUnit  = eFramesetUnit_Relative;
      mRowSpecs[0].mValue = 1;
    }
  }

  *aSpecs = mRowSpecs;
  *aNumValues = mNumRows;
  return NS_OK;
}

nsresult
nsHTMLFrameSetElement::GetColSpec(PRInt32 *aNumValues,
                                  const nsFramesetSpec** aSpecs)
{
  NS_PRECONDITION(aNumValues, "Must have a pointer to an integer here!");
  NS_PRECONDITION(aSpecs, "Must have a pointer to an array of nsFramesetSpecs");
  *aNumValues = 0;
  *aSpecs = nullptr;

  if (!mColSpecs) {
    const nsAttrValue* value = GetParsedAttr(nsGkAtoms::cols);
    if (value && value->Type() == nsAttrValue::eString) {
      nsresult rv = ParseRowCol(value->GetStringValue(), mNumCols,
                                getter_Transfers(mColSpecs));
      NS_ENSURE_SUCCESS(rv, rv);
    }

    if (!mColSpecs) {  
      mColSpecs = new nsFramesetSpec[1];
      if (!mColSpecs) {
        mNumCols = 0;
        return NS_ERROR_OUT_OF_MEMORY;
      }
      mNumCols = 1;
      mColSpecs[0].mUnit  = eFramesetUnit_Relative;
      mColSpecs[0].mValue = 1;
    }
  }

  *aSpecs = mColSpecs;
  *aNumValues = mNumCols;
  return NS_OK;
}


bool
nsHTMLFrameSetElement::ParseAttribute(PRInt32 aNamespaceID,
                                      nsIAtom* aAttribute,
                                      const nsAString& aValue,
                                      nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::bordercolor) {
      return aResult.ParseColor(aValue);
    }
    if (aAttribute == nsGkAtoms::frameborder) {
      return nsGenericHTMLElement::ParseFrameborderValue(aValue, aResult);
    }
    if (aAttribute == nsGkAtoms::border) {
      return aResult.ParseIntWithBounds(aValue, 0, 100);
    }
  }
  
  return nsGenericHTMLElement::ParseAttribute(aNamespaceID, aAttribute, aValue,
                                              aResult);
}

nsChangeHint
nsHTMLFrameSetElement::GetAttributeChangeHint(const nsIAtom* aAttribute,
                                              PRInt32 aModType) const
{
  nsChangeHint retval =
    nsGenericHTMLElement::GetAttributeChangeHint(aAttribute, aModType);
  if (aAttribute == nsGkAtoms::rows ||
      aAttribute == nsGkAtoms::cols) {
    NS_UpdateHint(retval, mCurrentRowColHint);
  }
  return retval;
}




nsresult
nsHTMLFrameSetElement::ParseRowCol(const nsAString & aValue,
                                   PRInt32& aNumSpecs,
                                   nsFramesetSpec** aSpecs) 
{
  if (aValue.IsEmpty()) {
    aNumSpecs = 0;
    *aSpecs = nullptr;
    return NS_OK;
  }

  static const PRUnichar sAster('*');
  static const PRUnichar sPercent('%');
  static const PRUnichar sComma(',');

  nsAutoString spec(aValue);
  
  
  spec.StripChars(" \n\r\t\"\'");
  spec.Trim(",");
  
  
  PR_STATIC_ASSERT(NS_MAX_FRAMESET_SPEC_COUNT * sizeof(nsFramesetSpec) < (1 << 30));
  PRInt32 commaX = spec.FindChar(sComma);
  PRInt32 count = 1;
  while (commaX != kNotFound && count < NS_MAX_FRAMESET_SPEC_COUNT) {
    count++;
    commaX = spec.FindChar(sComma, commaX + 1);
  }

  nsFramesetSpec* specs = new nsFramesetSpec[count];
  if (!specs) {
    *aSpecs = nullptr;
    aNumSpecs = 0;
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  bool isInQuirks = InNavQuirksMode(OwnerDoc());
      
  

  PRInt32 start = 0;
  PRInt32 specLen = spec.Length();

  for (PRInt32 i = 0; i < count; i++) {
    
    commaX = spec.FindChar(sComma, start);
    NS_ASSERTION(i == count - 1 || commaX != kNotFound,
                 "Failed to find comma, somehow");
    PRInt32 end = (commaX == kNotFound) ? specLen : commaX;

    
    
    
    specs[i].mUnit = eFramesetUnit_Fixed;
    specs[i].mValue = 0;
    if (end > start) {
      PRInt32 numberEnd = end;
      PRUnichar ch = spec.CharAt(numberEnd - 1);
      if (sAster == ch) {
        specs[i].mUnit = eFramesetUnit_Relative;
        numberEnd--;
      } else if (sPercent == ch) {
        specs[i].mUnit = eFramesetUnit_Percent;
        numberEnd--;
        
        if (numberEnd > start) {
          ch = spec.CharAt(numberEnd - 1);
          if (sAster == ch) {
            specs[i].mUnit = eFramesetUnit_Relative;
            numberEnd--;
          }
        }
      }

      
      nsAutoString token;
      spec.Mid(token, start, numberEnd - start);

      
      if ((eFramesetUnit_Relative == specs[i].mUnit) &&
        (0 == token.Length())) {
        specs[i].mValue = 1;
      }
      else {
        
        nsresult err;
        specs[i].mValue = token.ToInteger(&err);
        if (err) {
          specs[i].mValue = 0;
        }
      }

      
      if (isInQuirks) {
        if ((eFramesetUnit_Relative == specs[i].mUnit) &&
          (0 == specs[i].mValue)) {
          specs[i].mValue = 1;
        }
      }
        
      
      
      
      
      
      
      
      
      
      
      

      
      if (specs[i].mValue < 0) {
        specs[i].mValue = 0;
      }
      start = end + 1;
    }
  }

  aNumSpecs = count;
  
  *aSpecs = specs;
  
  return NS_OK;
}







#define EVENT(name_, id_, type_, struct_)
#define FORWARDED_EVENT(name_, id_, type_, struct_)                   \
  NS_IMETHODIMP nsHTMLFrameSetElement::GetOn##name_(JSContext *cx,    \
                                               jsval *vp) {           \
    /* XXXbz note to self: add tests for this! */                     \
    nsPIDOMWindow* win = OwnerDoc()->GetInnerWindow();             \
    if (win && win->IsInnerWindow()) {                                \
      nsCOMPtr<nsIInlineEventHandlers> ev = do_QueryInterface(win);   \
      return ev->GetOn##name_(cx, vp);                                \
    }                                                                 \
    *vp = JSVAL_NULL;                                                 \
    return NS_OK;                                                     \
  }                                                                   \
  NS_IMETHODIMP nsHTMLFrameSetElement::SetOn##name_(JSContext *cx,    \
                                               const jsval &v) {      \
    nsPIDOMWindow* win = OwnerDoc()->GetInnerWindow();             \
    if (win && win->IsInnerWindow()) {                                \
      nsCOMPtr<nsIInlineEventHandlers> ev = do_QueryInterface(win);   \
      return ev->SetOn##name_(cx, v);                                 \
    }                                                                 \
    return NS_OK;                                                     \
  }
#define WINDOW_EVENT(name_, id_, type_, struct_)                      \
  NS_IMETHODIMP nsHTMLFrameSetElement::GetOn##name_(JSContext *cx,    \
                                                    jsval *vp) {      \
    /* XXXbz note to self: add tests for this! */                     \
    nsPIDOMWindow* win = OwnerDoc()->GetInnerWindow();             \
    if (win && win->IsInnerWindow()) {                                \
      return win->GetOn##name_(cx, vp);                               \
    }                                                                 \
    *vp = JSVAL_NULL;                                                 \
    return NS_OK;                                                     \
  }                                                                   \
  NS_IMETHODIMP nsHTMLFrameSetElement::SetOn##name_(JSContext *cx,    \
                                                    const jsval &v) { \
    nsPIDOMWindow* win = OwnerDoc()->GetInnerWindow();             \
    if (win && win->IsInnerWindow()) {                                \
      return win->SetOn##name_(cx, v);                                \
    }                                                                 \
    return NS_OK;                                                     \
  }
#include "nsEventNameList.h"
#undef WINDOW_EVENT
#undef FORWARDED_EVENT
#undef EVENT
