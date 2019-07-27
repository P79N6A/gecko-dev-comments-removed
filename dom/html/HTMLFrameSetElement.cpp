




#include "HTMLFrameSetElement.h"
#include "mozilla/dom/HTMLFrameSetElementBinding.h"
#include "mozilla/dom/EventHandlerBinding.h"
#include "nsGlobalWindow.h"

NS_IMPL_NS_NEW_HTML_ELEMENT(FrameSet)

namespace mozilla {
namespace dom {

HTMLFrameSetElement::~HTMLFrameSetElement()
{
}

JSObject*
HTMLFrameSetElement::WrapNode(JSContext *aCx, JS::Handle<JSObject*> aGivenProto)
{
  return HTMLFrameSetElementBinding::Wrap(aCx, this, aGivenProto);
}

NS_IMPL_ISUPPORTS_INHERITED(HTMLFrameSetElement, nsGenericHTMLElement,
                            nsIDOMHTMLFrameSetElement)

NS_IMPL_ELEMENT_CLONE(HTMLFrameSetElement)

NS_IMETHODIMP 
HTMLFrameSetElement::SetCols(const nsAString& aCols)
{
  ErrorResult rv;
  SetCols(aCols, rv);
  return rv.StealNSResult();
}

NS_IMETHODIMP
HTMLFrameSetElement::GetCols(nsAString& aCols)
{
  DOMString cols;
  GetCols(cols);
  cols.ToString(aCols);
  return NS_OK;
}

NS_IMETHODIMP 
HTMLFrameSetElement::SetRows(const nsAString& aRows)
{
  ErrorResult rv;
  SetRows(aRows, rv);
  return rv.StealNSResult();
}

NS_IMETHODIMP
HTMLFrameSetElement::GetRows(nsAString& aRows)
{
  DOMString rows;
  GetRows(rows);
  rows.ToString(aRows);
  return NS_OK;
}

nsresult
HTMLFrameSetElement::SetAttr(int32_t aNameSpaceID,
                             nsIAtom* aAttribute,
                             nsIAtom* aPrefix,
                             const nsAString& aValue,
                             bool aNotify)
{
  nsresult rv;
  







  if (aAttribute == nsGkAtoms::rows && aNameSpaceID == kNameSpaceID_None) {
    int32_t oldRows = mNumRows;
    ParseRowCol(aValue, mNumRows, getter_Transfers(mRowSpecs));
    
    if (mNumRows != oldRows) {
      mCurrentRowColHint = NS_STYLE_HINT_FRAMECHANGE;
    }
  } else if (aAttribute == nsGkAtoms::cols &&
             aNameSpaceID == kNameSpaceID_None) {
    int32_t oldCols = mNumCols;
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
HTMLFrameSetElement::GetRowSpec(int32_t *aNumValues,
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
HTMLFrameSetElement::GetColSpec(int32_t *aNumValues,
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
HTMLFrameSetElement::ParseAttribute(int32_t aNamespaceID,
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
HTMLFrameSetElement::GetAttributeChangeHint(const nsIAtom* aAttribute,
                                            int32_t aModType) const
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
HTMLFrameSetElement::ParseRowCol(const nsAString & aValue,
                                 int32_t& aNumSpecs,
                                 nsFramesetSpec** aSpecs) 
{
  if (aValue.IsEmpty()) {
    aNumSpecs = 0;
    *aSpecs = nullptr;
    return NS_OK;
  }

  static const char16_t sAster('*');
  static const char16_t sPercent('%');
  static const char16_t sComma(',');

  nsAutoString spec(aValue);
  
  
  spec.StripChars(" \n\r\t\"\'");
  spec.Trim(",");
  
  
  static_assert(NS_MAX_FRAMESET_SPEC_COUNT * sizeof(nsFramesetSpec) < (1 << 30),
                "Too many frameset specs allowed to allocate");
  int32_t commaX = spec.FindChar(sComma);
  int32_t count = 1;
  while (commaX != kNotFound && count < NS_MAX_FRAMESET_SPEC_COUNT) {
    count++;
    commaX = spec.FindChar(sComma, commaX + 1);
  }

  nsFramesetSpec* specs = new (fallible) nsFramesetSpec[count];
  if (!specs) {
    *aSpecs = nullptr;
    aNumSpecs = 0;
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  bool isInQuirks = InNavQuirksMode(OwnerDoc());
      
  

  int32_t start = 0;
  int32_t specLen = spec.Length();

  for (int32_t i = 0; i < count; i++) {
    
    commaX = spec.FindChar(sComma, start);
    NS_ASSERTION(i == count - 1 || commaX != kNotFound,
                 "Failed to find comma, somehow");
    int32_t end = (commaX == kNotFound) ? specLen : commaX;

    
    
    
    specs[i].mUnit = eFramesetUnit_Fixed;
    specs[i].mValue = 0;
    if (end > start) {
      int32_t numberEnd = end;
      char16_t ch = spec.CharAt(numberEnd - 1);
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
        if (NS_FAILED(err)) {
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

bool
HTMLFrameSetElement::IsEventAttributeName(nsIAtom *aName)
{
  return nsContentUtils::IsEventAttributeName(aName,
                                              EventNameType_HTML |
                                              EventNameType_HTMLBodyOrFramesetOnly);
}


#define EVENT(name_, id_, type_, struct_)



#define WINDOW_EVENT_HELPER(name_, type_)                                      \
  type_*                                                                       \
  HTMLFrameSetElement::GetOn##name_()                                          \
  {                                                                            \
    nsPIDOMWindow* win = OwnerDoc()->GetInnerWindow();                         \
    if (win) {                                                                 \
      nsCOMPtr<nsISupports> supports = do_QueryInterface(win);                 \
      nsGlobalWindow* globalWin = nsGlobalWindow::FromSupports(supports);      \
      return globalWin->GetOn##name_();                                        \
    }                                                                          \
    return nullptr;                                                            \
  }                                                                            \
  void                                                                         \
  HTMLFrameSetElement::SetOn##name_(type_* handler)                            \
  {                                                                            \
    nsPIDOMWindow* win = OwnerDoc()->GetInnerWindow();                         \
    if (!win) {                                                                \
      return;                                                                  \
    }                                                                          \
                                                                               \
    nsCOMPtr<nsISupports> supports = do_QueryInterface(win);                   \
    nsGlobalWindow* globalWin = nsGlobalWindow::FromSupports(supports);        \
    return globalWin->SetOn##name_(handler);                                   \
  }
#define WINDOW_EVENT(name_, id_, type_, struct_)                               \
  WINDOW_EVENT_HELPER(name_, EventHandlerNonNull)
#define BEFOREUNLOAD_EVENT(name_, id_, type_, struct_)                         \
  WINDOW_EVENT_HELPER(name_, OnBeforeUnloadEventHandlerNonNull)
#include "mozilla/EventNameList.h" 
#undef BEFOREUNLOAD_EVENT
#undef WINDOW_EVENT
#undef WINDOW_EVENT_HELPER
#undef EVENT

} 
} 
