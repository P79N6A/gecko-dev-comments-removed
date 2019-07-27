




#include "ChangeCSSInlineStyleTxn.h"
#include "EditTxn.h"
#include "mozilla/Assertions.h"
#include "mozilla/Preferences.h"
#include "mozilla/css/Declaration.h"
#include "mozilla/css/StyleRule.h"
#include "mozilla/dom/Element.h"
#include "mozilla/mozalloc.h"
#include "nsAString.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsColor.h"
#include "nsComputedDOMStyle.h"
#include "nsDebug.h"
#include "nsDependentSubstring.h"
#include "nsEditProperty.h"
#include "nsError.h"
#include "nsGkAtoms.h"
#include "nsHTMLCSSUtils.h"
#include "nsHTMLEditor.h"
#include "nsIAtom.h"
#include "nsIContent.h"
#include "nsIDOMCSSStyleDeclaration.h"
#include "nsIDOMElement.h"
#include "nsIDOMElementCSSInlineStyle.h"
#include "nsIDOMNode.h"
#include "nsIDOMWindow.h"
#include "nsIDocument.h"
#include "nsIEditor.h"
#include "nsINode.h"
#include "nsISupportsImpl.h"
#include "nsISupportsUtils.h"
#include "nsLiteralString.h"
#include "nsPIDOMWindow.h"
#include "nsReadableUtils.h"
#include "nsString.h"
#include "nsStringFwd.h"
#include "nsStringIterator.h"
#include "nsSubstringTuple.h"
#include "nsUnicharUtils.h"

using namespace mozilla;

static
void ProcessBValue(const nsAString * aInputString, nsAString & aOutputString,
                   const char * aDefaultValueString,
                   const char * aPrependString, const char* aAppendString)
{
  if (aInputString && aInputString->EqualsLiteral("-moz-editor-invert-value")) {
      aOutputString.AssignLiteral("normal");
  }
  else {
    aOutputString.AssignLiteral("bold");
  }
}

static
void ProcessDefaultValue(const nsAString * aInputString, nsAString & aOutputString,
                         const char * aDefaultValueString,
                         const char * aPrependString, const char* aAppendString)
{
  CopyASCIItoUTF16(aDefaultValueString, aOutputString);
}

static
void ProcessSameValue(const nsAString * aInputString, nsAString & aOutputString,
                      const char * aDefaultValueString,
                      const char * aPrependString, const char* aAppendString)
{
  if (aInputString) {
    aOutputString.Assign(*aInputString);
  }
  else
    aOutputString.Truncate();
}

static
void ProcessExtendedValue(const nsAString * aInputString, nsAString & aOutputString,
                          const char * aDefaultValueString,
                          const char * aPrependString, const char* aAppendString)
{
  aOutputString.Truncate();
  if (aInputString) {
    if (aPrependString) {
      AppendASCIItoUTF16(aPrependString, aOutputString);
    }
    aOutputString.Append(*aInputString);
    if (aAppendString) {
      AppendASCIItoUTF16(aAppendString, aOutputString);
    }
  }
}

static
void ProcessLengthValue(const nsAString * aInputString, nsAString & aOutputString,
                        const char * aDefaultValueString,
                        const char * aPrependString, const char* aAppendString)
{
  aOutputString.Truncate();
  if (aInputString) {
    aOutputString.Append(*aInputString);
    if (-1 == aOutputString.FindChar(char16_t('%'))) {
      aOutputString.AppendLiteral("px");
    }
  }
}

static
void ProcessListStyleTypeValue(const nsAString * aInputString, nsAString & aOutputString,
                               const char * aDefaultValueString,
                               const char * aPrependString, const char* aAppendString)
{
  aOutputString.Truncate();
  if (aInputString) {
    if (aInputString->EqualsLiteral("1")) {
      aOutputString.AppendLiteral("decimal");
    }
    else if (aInputString->EqualsLiteral("a")) {
      aOutputString.AppendLiteral("lower-alpha");
    }
    else if (aInputString->EqualsLiteral("A")) {
      aOutputString.AppendLiteral("upper-alpha");
    }
    else if (aInputString->EqualsLiteral("i")) {
      aOutputString.AppendLiteral("lower-roman");
    }
    else if (aInputString->EqualsLiteral("I")) {
      aOutputString.AppendLiteral("upper-roman");
    }
    else if (aInputString->EqualsLiteral("square")
             || aInputString->EqualsLiteral("circle")
             || aInputString->EqualsLiteral("disc")) {
      aOutputString.Append(*aInputString);
    }
  }
}

static
void ProcessMarginLeftValue(const nsAString * aInputString, nsAString & aOutputString,
                            const char * aDefaultValueString,
                            const char * aPrependString, const char* aAppendString)
{
  aOutputString.Truncate();
  if (aInputString) {
    if (aInputString->EqualsLiteral("center") ||
        aInputString->EqualsLiteral("-moz-center")) {
      aOutputString.AppendLiteral("auto"); 
    }
    else if (aInputString->EqualsLiteral("right") ||
             aInputString->EqualsLiteral("-moz-right")) {
      aOutputString.AppendLiteral("auto"); 
    }
    else {
      aOutputString.AppendLiteral("0px"); 
    }
  }
}

static
void ProcessMarginRightValue(const nsAString * aInputString, nsAString & aOutputString,
                             const char * aDefaultValueString,
                             const char * aPrependString, const char* aAppendString)
{
  aOutputString.Truncate();
  if (aInputString) {
    if (aInputString->EqualsLiteral("center") ||
        aInputString->EqualsLiteral("-moz-center")) {
      aOutputString.AppendLiteral("auto"); 
    }
    else if (aInputString->EqualsLiteral("left") ||
             aInputString->EqualsLiteral("-moz-left")) {
      aOutputString.AppendLiteral("auto"); 
    }
    else {
      aOutputString.AppendLiteral("0px"); 
    }
  }
}

const nsHTMLCSSUtils::CSSEquivTable boldEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_font_weight, ProcessBValue, nullptr, nullptr, nullptr, true, false },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable italicEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_font_style, ProcessDefaultValue, "italic", nullptr, nullptr, true, false },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable underlineEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_text_decoration, ProcessDefaultValue, "underline", nullptr, nullptr, true, false },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable strikeEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_text_decoration, ProcessDefaultValue, "line-through", nullptr, nullptr, true, false },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable ttEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_font_family, ProcessDefaultValue, "monospace", nullptr, nullptr, true, false },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable fontColorEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_color, ProcessSameValue, nullptr, nullptr, nullptr, true, false },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable fontFaceEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_font_family, ProcessSameValue, nullptr, nullptr, nullptr, true, false },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable bgcolorEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_background_color, ProcessSameValue, nullptr, nullptr, nullptr, true, false },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable backgroundImageEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_background_image, ProcessExtendedValue, nullptr, "url(", ")", true, true },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable textColorEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_color, ProcessSameValue, nullptr, nullptr, nullptr, true, false },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable borderEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_border, ProcessExtendedValue, nullptr, nullptr, "px solid", true, false },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable textAlignEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_text_align, ProcessSameValue, nullptr, nullptr, nullptr, true, false },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable captionAlignEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_caption_side, ProcessSameValue, nullptr, nullptr, nullptr, true, false },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable verticalAlignEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_vertical_align, ProcessSameValue, nullptr, nullptr, nullptr, true, false },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable nowrapEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_whitespace, ProcessDefaultValue, "nowrap", nullptr, nullptr, true, false },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable widthEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_width, ProcessLengthValue, nullptr, nullptr, nullptr, true, false },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable heightEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_height, ProcessLengthValue, nullptr, nullptr, nullptr, true, false },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable listStyleTypeEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_list_style_type, ProcessListStyleTypeValue, nullptr, nullptr, nullptr, true, true },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable tableAlignEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_text_align, ProcessDefaultValue, "left", nullptr, nullptr, false, false },
  { nsHTMLCSSUtils::eCSSEditableProperty_margin_left, ProcessMarginLeftValue, nullptr, nullptr, nullptr, true, false },
  { nsHTMLCSSUtils::eCSSEditableProperty_margin_right, ProcessMarginRightValue, nullptr, nullptr, nullptr, true, false },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable hrAlignEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_margin_left, ProcessMarginLeftValue, nullptr, nullptr, nullptr, true, false },
  { nsHTMLCSSUtils::eCSSEditableProperty_margin_right, ProcessMarginRightValue, nullptr, nullptr, nullptr, true, false },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

nsHTMLCSSUtils::nsHTMLCSSUtils(nsHTMLEditor* aEditor)
  : mHTMLEditor(aEditor)
  , mIsCSSPrefChecked(true)
{
  
  mIsCSSPrefChecked = Preferences::GetBool("editor.use_css", mIsCSSPrefChecked);
}

nsHTMLCSSUtils::~nsHTMLCSSUtils()
{
}



bool
nsHTMLCSSUtils::IsCSSEditableProperty(nsIDOMNode* aNode,
                                      nsIAtom* aProperty,
                                      const nsAString* aAttribute)
{
  NS_ASSERTION(aNode, "Shouldn't you pass aNode? - Bug 214025");

  nsCOMPtr<nsIContent> content = do_QueryInterface(aNode);
  NS_ENSURE_TRUE(content, false);
  return IsCSSEditableProperty(content, aProperty, aAttribute);
}

bool
nsHTMLCSSUtils::IsCSSEditableProperty(nsIContent* aNode,
                                      nsIAtom* aProperty,
                                      const nsAString* aAttribute)
{
  MOZ_ASSERT(aNode);

  nsIContent* content = aNode;
  
  if (content->NodeType() == nsIDOMNode::TEXT_NODE) {
    content = content->GetParent();
    NS_ENSURE_TRUE(content, false);
  }

  nsIAtom *tagName = content->Tag();
  

  
  if (nsEditProperty::b == aProperty
      || nsEditProperty::i == aProperty
      || nsEditProperty::tt == aProperty
      || nsEditProperty::u == aProperty
      || nsEditProperty::strike == aProperty
      || ((nsEditProperty::font == aProperty) && aAttribute &&
           (aAttribute->EqualsLiteral("color") ||
            aAttribute->EqualsLiteral("face")))) {
    return true;
  }

  
  if (aAttribute && (aAttribute->EqualsLiteral("align")) &&
      (nsEditProperty::div == tagName
       || nsEditProperty::p   == tagName
       || nsEditProperty::h1  == tagName
       || nsEditProperty::h2  == tagName
       || nsEditProperty::h3  == tagName
       || nsEditProperty::h4  == tagName
       || nsEditProperty::h5  == tagName
       || nsEditProperty::h6  == tagName
       || nsEditProperty::td  == tagName
       || nsEditProperty::th  == tagName
       || nsEditProperty::table  == tagName
       || nsEditProperty::hr  == tagName
       
       
       
       
       || nsEditProperty::legend  == tagName
       || nsEditProperty::caption == tagName)) {
    return true;
  }

  if (aAttribute && (aAttribute->EqualsLiteral("valign")) &&
      (nsEditProperty::col == tagName
       || nsEditProperty::colgroup   == tagName
       || nsEditProperty::tbody  == tagName
       || nsEditProperty::td  == tagName
       || nsEditProperty::th  == tagName
       || nsEditProperty::tfoot  == tagName
       || nsEditProperty::thead  == tagName
       || nsEditProperty::tr  == tagName)) {
    return true;
  }

  
  if (aAttribute && (nsEditProperty::body == tagName) &&
      (aAttribute->EqualsLiteral("text")
       || aAttribute->EqualsLiteral("background")
       || aAttribute->EqualsLiteral("bgcolor"))) {
    return true;
  }

  
  if (aAttribute && aAttribute->EqualsLiteral("bgcolor")) {
    return true;
  }

  
  if (aAttribute && ((nsEditProperty::td == tagName)
                      || (nsEditProperty::th == tagName)) &&
      (aAttribute->EqualsLiteral("height")
       || aAttribute->EqualsLiteral("width")
       || aAttribute->EqualsLiteral("nowrap"))) {
    return true;
  }

  
  if (aAttribute && (nsEditProperty::table == tagName) &&
      (aAttribute->EqualsLiteral("height")
       || aAttribute->EqualsLiteral("width"))) {
    return true;
  }

  
  if (aAttribute && (nsEditProperty::hr == tagName) &&
      (aAttribute->EqualsLiteral("size")
       || aAttribute->EqualsLiteral("width"))) {
    return true;
  }

  
  if (aAttribute && (nsEditProperty::ol == tagName
                     || nsEditProperty::ul == tagName
                     || nsEditProperty::li == tagName) &&
      aAttribute->EqualsLiteral("type")) {
    return true;
  }

  if (aAttribute && nsEditProperty::img == tagName &&
      (aAttribute->EqualsLiteral("border")
       || aAttribute->EqualsLiteral("width")
       || aAttribute->EqualsLiteral("height"))) {
    return true;
  }

  
  
  if (aAttribute && aAttribute->EqualsLiteral("align") &&
      (nsEditProperty::ul == tagName
       || nsEditProperty::ol == tagName
       || nsEditProperty::dl == tagName
       || nsEditProperty::li == tagName
       || nsEditProperty::dd == tagName
       || nsEditProperty::dt == tagName
       || nsEditProperty::address == tagName
       || nsEditProperty::pre == tagName
       || nsEditProperty::ul == tagName)) {
    return true;
  }

  return false;
}



nsresult
nsHTMLCSSUtils::SetCSSProperty(nsIDOMElement *aElement, nsIAtom * aProperty, const nsAString & aValue,
                               bool aSuppressTransaction)
{
  nsRefPtr<ChangeCSSInlineStyleTxn> txn;
  nsresult result = CreateCSSPropertyTxn(aElement, aProperty, aValue,
                                         getter_AddRefs(txn), false);
  if (NS_SUCCEEDED(result))  {
    if (aSuppressTransaction) {
      result = txn->DoTransaction();
    }
    else {
      result = mHTMLEditor->DoTransaction(txn);
    }
  }
  return result;
}

nsresult
nsHTMLCSSUtils::SetCSSPropertyPixels(nsIDOMElement *aElement,
                                     nsIAtom *aProperty,
                                     int32_t aIntValue,
                                     bool aSuppressTransaction)
{
  nsAutoString s;
  s.AppendInt(aIntValue);
  return SetCSSProperty(aElement, aProperty, s + NS_LITERAL_STRING("px"),
                        aSuppressTransaction);
}




nsresult
nsHTMLCSSUtils::RemoveCSSProperty(nsIDOMElement *aElement, nsIAtom * aProperty, const nsAString & aValue,
                                  bool aSuppressTransaction)
{
  nsRefPtr<ChangeCSSInlineStyleTxn> txn;
  nsresult result = CreateCSSPropertyTxn(aElement, aProperty, aValue,
                                         getter_AddRefs(txn), true);
  if (NS_SUCCEEDED(result))  {
    if (aSuppressTransaction) {
      result = txn->DoTransaction();
    }
    else {
      result = mHTMLEditor->DoTransaction(txn);
    }
  }
  return result;
}

nsresult 
nsHTMLCSSUtils::CreateCSSPropertyTxn(nsIDOMElement *aElement, 
                                     nsIAtom * aAttribute,
                                     const nsAString& aValue,
                                     ChangeCSSInlineStyleTxn ** aTxn,
                                     bool aRemoveProperty)
{
  NS_ENSURE_TRUE(aElement, NS_ERROR_NULL_POINTER);

  *aTxn = new ChangeCSSInlineStyleTxn();
  NS_ENSURE_TRUE(*aTxn, NS_ERROR_OUT_OF_MEMORY);
  NS_ADDREF(*aTxn);
  return (*aTxn)->Init(mHTMLEditor, aElement, aAttribute, aValue, aRemoveProperty);
}

nsresult
nsHTMLCSSUtils::GetSpecifiedProperty(nsIDOMNode *aNode, nsIAtom *aProperty,
                                     nsAString & aValue)
{
  return GetCSSInlinePropertyBase(aNode, aProperty, aValue, eSpecified);
}

nsresult
nsHTMLCSSUtils::GetComputedProperty(nsIDOMNode *aNode, nsIAtom *aProperty,
                                    nsAString & aValue)
{
  return GetCSSInlinePropertyBase(aNode, aProperty, aValue, eComputed);
}

nsresult
nsHTMLCSSUtils::GetCSSInlinePropertyBase(nsIDOMNode* aNode, nsIAtom* aProperty,
                                         nsAString& aValue,
                                         StyleType aStyleType)
{
  nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
  return GetCSSInlinePropertyBase(node, aProperty, aValue, aStyleType);
}

nsresult
nsHTMLCSSUtils::GetCSSInlinePropertyBase(nsINode* aNode, nsIAtom* aProperty,
                                         nsAString& aValue,
                                         StyleType aStyleType)
{
  MOZ_ASSERT(aNode && aProperty);
  aValue.Truncate();

  nsCOMPtr<dom::Element> element = GetElementContainerOrSelf(aNode);
  NS_ENSURE_TRUE(element, NS_ERROR_NULL_POINTER);

  if (aStyleType == eComputed) {
    
    nsRefPtr<nsComputedDOMStyle> cssDecl = GetComputedStyle(element);
    NS_ENSURE_STATE(cssDecl);

    
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
      cssDecl->GetPropertyValue(nsDependentAtomString(aProperty), aValue)));

    return NS_OK;
  }

  MOZ_ASSERT(aStyleType == eSpecified);
  nsRefPtr<css::StyleRule> rule = element->GetInlineStyleRule();
  if (!rule) {
    return NS_OK;
  }
  nsCSSProperty prop =
    nsCSSProps::LookupProperty(nsDependentAtomString(aProperty),
                               nsCSSProps::eEnabledForAllContent);
  MOZ_ASSERT(prop != eCSSProperty_UNKNOWN);
  rule->GetDeclaration()->GetValue(prop, aValue);

  return NS_OK;
}

already_AddRefed<nsComputedDOMStyle>
nsHTMLCSSUtils::GetComputedStyle(nsIDOMElement* aElement)
{
  nsCOMPtr<dom::Element> element = do_QueryInterface(aElement);
  return GetComputedStyle(element);
}

already_AddRefed<nsComputedDOMStyle>
nsHTMLCSSUtils::GetComputedStyle(dom::Element* aElement)
{
  MOZ_ASSERT(aElement);

  nsIDocument* doc = aElement->GetCurrentDoc();
  NS_ENSURE_TRUE(doc, nullptr);

  nsIPresShell* presShell = doc->GetShell();
  NS_ENSURE_TRUE(presShell, nullptr);

  nsRefPtr<nsComputedDOMStyle> style =
    NS_NewComputedDOMStyle(aElement, EmptyString(), presShell);

  return style.forget();
}



nsresult
nsHTMLCSSUtils::RemoveCSSInlineStyle(nsIDOMNode *aNode, nsIAtom *aProperty, const nsAString & aPropertyValue)
{
  nsCOMPtr<nsIDOMElement> elem = do_QueryInterface(aNode);

  
  nsresult res = RemoveCSSProperty(elem, aProperty, aPropertyValue, false);
  NS_ENSURE_SUCCESS(res, res);

  nsCOMPtr<dom::Element> element = do_QueryInterface(aNode);
  if (!element || !element->IsHTML(nsGkAtoms::span) ||
      nsHTMLEditor::HasAttributes(element)) {
    return NS_OK;
  }

  return mHTMLEditor->RemoveContainer(aNode);
}



bool
nsHTMLCSSUtils::IsCSSInvertable(nsIAtom *aProperty, const nsAString *aAttribute)
{
  return bool(nsEditProperty::b == aProperty);
}


void
nsHTMLCSSUtils::GetDefaultBackgroundColor(nsAString & aColor)
{
  if (Preferences::GetBool("editor.use_custom_colors", false)) {
    nsresult rv = Preferences::GetString("editor.background_color", &aColor);
    
    if (NS_FAILED(rv)) {
      NS_WARNING("failed to get editor.background_color");
      aColor.AssignLiteral("#ffffff");  
    }
    return;
  }

  if (Preferences::GetBool("browser.display.use_system_colors", false)) {
    return;
  }

  nsresult rv =
    Preferences::GetString("browser.display.background_color", &aColor);
  
  if (NS_FAILED(rv)) {
    NS_WARNING("failed to get browser.display.background_color");
    aColor.AssignLiteral("#ffffff");  
  }
}


void
nsHTMLCSSUtils::GetDefaultLengthUnit(nsAString & aLengthUnit)
{
  nsresult rv =
    Preferences::GetString("editor.css.default_length_unit", &aLengthUnit);
  
  if (NS_FAILED(rv)) {
    aLengthUnit.AssignLiteral("px");
  }
}




void
nsHTMLCSSUtils::ParseLength(const nsAString & aString, float * aValue, nsIAtom ** aUnit)
{
  nsAString::const_iterator iter;
  aString.BeginReading(iter);

  float a = 10.0f , b = 1.0f, value = 0;
  int8_t sign = 1;
  int32_t i = 0, j = aString.Length();
  char16_t c;
  bool floatingPointFound = false;
  c = *iter;
  if (char16_t('-') == c) { sign = -1; iter++; i++; }
  else if (char16_t('+') == c) { iter++; i++; }
  while (i < j) {
    c = *iter;
    if ((char16_t('0') == c) ||
        (char16_t('1') == c) ||
        (char16_t('2') == c) ||
        (char16_t('3') == c) ||
        (char16_t('4') == c) ||
        (char16_t('5') == c) ||
        (char16_t('6') == c) ||
        (char16_t('7') == c) ||
        (char16_t('8') == c) ||
        (char16_t('9') == c)) {
      value = (value * a) + (b * (c - char16_t('0')));
      b = b / 10 * a;
    }
    else if (!floatingPointFound && (char16_t('.') == c)) {
      floatingPointFound = true;
      a = 1.0f; b = 0.1f;
    }
    else break;
    iter++;
    i++;
  }
  *aValue = value * sign;
  *aUnit = NS_NewAtom(StringTail(aString, j-i)).take();
}

void
nsHTMLCSSUtils::GetCSSPropertyAtom(nsCSSEditableProperty aProperty, nsIAtom ** aAtom)
{
  *aAtom = nullptr;
  switch (aProperty) {
    case eCSSEditableProperty_background_color:
      *aAtom = nsEditProperty::cssBackgroundColor;
      break;
    case eCSSEditableProperty_background_image:
      *aAtom = nsEditProperty::cssBackgroundImage;
      break;
    case eCSSEditableProperty_border:
      *aAtom = nsEditProperty::cssBorder;
      break;
    case eCSSEditableProperty_caption_side:
      *aAtom = nsEditProperty::cssCaptionSide;
      break;
    case eCSSEditableProperty_color:
      *aAtom = nsEditProperty::cssColor;
      break;
    case eCSSEditableProperty_float:
      *aAtom = nsEditProperty::cssFloat;
      break;
    case eCSSEditableProperty_font_family:
      *aAtom = nsEditProperty::cssFontFamily;
      break;
    case eCSSEditableProperty_font_size:
      *aAtom = nsEditProperty::cssFontSize;
      break;
    case eCSSEditableProperty_font_style:
      *aAtom = nsEditProperty::cssFontStyle;
      break;
    case eCSSEditableProperty_font_weight:
      *aAtom = nsEditProperty::cssFontWeight;
      break;
    case eCSSEditableProperty_height:
      *aAtom = nsEditProperty::cssHeight;
      break;
    case eCSSEditableProperty_list_style_type:
      *aAtom = nsEditProperty::cssListStyleType;
      break;
    case eCSSEditableProperty_margin_left:
      *aAtom = nsEditProperty::cssMarginLeft;
      break;
    case eCSSEditableProperty_margin_right:
      *aAtom = nsEditProperty::cssMarginRight;
      break;
    case eCSSEditableProperty_text_align:
      *aAtom = nsEditProperty::cssTextAlign;
      break;
    case eCSSEditableProperty_text_decoration:
      *aAtom = nsEditProperty::cssTextDecoration;
      break;
    case eCSSEditableProperty_vertical_align:
      *aAtom = nsEditProperty::cssVerticalAlign;
      break;
    case eCSSEditableProperty_whitespace:
      *aAtom = nsEditProperty::cssWhitespace;
      break;
    case eCSSEditableProperty_width:
      *aAtom = nsEditProperty::cssWidth;
      break;
    case eCSSEditableProperty_NONE:
      
      break;
  }
}



void
nsHTMLCSSUtils::BuildCSSDeclarations(nsTArray<nsIAtom*> & aPropertyArray,
                                     nsTArray<nsString> & aValueArray,
                                     const CSSEquivTable * aEquivTable,
                                     const nsAString * aValue,
                                     bool aGetOrRemoveRequest)
{
  
  aPropertyArray.Clear();
  aValueArray.Clear();

  
  nsAutoString value, lowerCasedValue;
  if (aValue) {
    value.Assign(*aValue);
    lowerCasedValue.Assign(*aValue);
    ToLowerCase(lowerCasedValue);
  }

  int8_t index = 0;
  nsCSSEditableProperty cssProperty = aEquivTable[index].cssProperty;
  while (cssProperty) {
    if (!aGetOrRemoveRequest|| aEquivTable[index].gettable) {
      nsAutoString cssValue, cssPropertyString;
      nsIAtom * cssPropertyAtom;
      
      
      (*aEquivTable[index].processValueFunctor) ((!aGetOrRemoveRequest || aEquivTable[index].caseSensitiveValue) ? &value : &lowerCasedValue,
                                                 cssValue,
                                                 aEquivTable[index].defaultValue,
                                                 aEquivTable[index].prependValue,
                                                 aEquivTable[index].appendValue);
      GetCSSPropertyAtom(cssProperty, &cssPropertyAtom);
      aPropertyArray.AppendElement(cssPropertyAtom);
      aValueArray.AppendElement(cssValue);
    }
    index++;
    cssProperty = aEquivTable[index].cssProperty;
  }
}



void
nsHTMLCSSUtils::GenerateCSSDeclarationsFromHTMLStyle(dom::Element* aElement,
                                                     nsIAtom* aHTMLProperty,
                                                     const nsAString* aAttribute,
                                                     const nsAString* aValue,
                                                     nsTArray<nsIAtom*>& cssPropertyArray,
                                                     nsTArray<nsString>& cssValueArray,
                                                     bool aGetOrRemoveRequest)
{
  MOZ_ASSERT(aElement);
  nsIAtom* tagName = aElement->Tag();
  const nsHTMLCSSUtils::CSSEquivTable* equivTable = nullptr;

  if (nsEditProperty::b == aHTMLProperty) {
    equivTable = boldEquivTable;
  } else if (nsEditProperty::i == aHTMLProperty) {
    equivTable = italicEquivTable;
  } else if (nsEditProperty::u == aHTMLProperty) {
    equivTable = underlineEquivTable;
  } else if (nsEditProperty::strike == aHTMLProperty) {
    equivTable = strikeEquivTable;
  } else if (nsEditProperty::tt == aHTMLProperty) {
    equivTable = ttEquivTable;
  } else if (aAttribute) {
    if (nsEditProperty::font == aHTMLProperty &&
        aAttribute->EqualsLiteral("color")) {
      equivTable = fontColorEquivTable;
    } else if (nsEditProperty::font == aHTMLProperty &&
               aAttribute->EqualsLiteral("face")) {
      equivTable = fontFaceEquivTable;
    } else if (aAttribute->EqualsLiteral("bgcolor")) {
      equivTable = bgcolorEquivTable;
    } else if (aAttribute->EqualsLiteral("background")) {
      equivTable = backgroundImageEquivTable;
    } else if (aAttribute->EqualsLiteral("text")) {
      equivTable = textColorEquivTable;
    } else if (aAttribute->EqualsLiteral("border")) {
      equivTable = borderEquivTable;
    } else if (aAttribute->EqualsLiteral("align")) {
      if (nsEditProperty::table  == tagName) {
        equivTable = tableAlignEquivTable;
      } else if (nsEditProperty::hr  == tagName) {
        equivTable = hrAlignEquivTable;
      } else if (nsEditProperty::legend  == tagName ||
               nsEditProperty::caption == tagName) {
        equivTable = captionAlignEquivTable;
      } else {
        equivTable = textAlignEquivTable;
      }
    } else if (aAttribute->EqualsLiteral("valign")) {
      equivTable = verticalAlignEquivTable;
    } else if (aAttribute->EqualsLiteral("nowrap")) {
      equivTable = nowrapEquivTable;
    } else if (aAttribute->EqualsLiteral("width")) {
      equivTable = widthEquivTable;
    } else if (aAttribute->EqualsLiteral("height") ||
               (nsEditProperty::hr == tagName &&
                aAttribute->EqualsLiteral("size"))) {
      equivTable = heightEquivTable;
    } else if (aAttribute->EqualsLiteral("type") &&
               (nsEditProperty::ol == tagName
                || nsEditProperty::ul == tagName
                || nsEditProperty::li == tagName)) {
      equivTable = listStyleTypeEquivTable;
    }
  }
  if (equivTable) {
    BuildCSSDeclarations(cssPropertyArray, cssValueArray, equivTable,
                         aValue, aGetOrRemoveRequest);
  }
}




int32_t
nsHTMLCSSUtils::SetCSSEquivalentToHTMLStyle(dom::Element* aElement,
                                            nsIAtom* aProperty,
                                            const nsAString* aAttribute,
                                            const nsAString* aValue,
                                            bool aSuppressTransaction)
{
  MOZ_ASSERT(aElement && aProperty);
  MOZ_ASSERT_IF(aAttribute, aValue);
  int32_t count;
  
  
  
  
  nsresult res = SetCSSEquivalentToHTMLStyle(aElement->AsDOMNode(),
                                             aProperty, aAttribute,
                                             aValue, &count,
                                             aSuppressTransaction);
  NS_ASSERTION(NS_SUCCEEDED(res), "SetCSSEquivalentToHTMLStyle failed");
  NS_ENSURE_SUCCESS(res, count);
  return count;
}

nsresult
nsHTMLCSSUtils::SetCSSEquivalentToHTMLStyle(nsIDOMNode * aNode,
                                            nsIAtom *aHTMLProperty,
                                            const nsAString *aAttribute,
                                            const nsAString *aValue,
                                            int32_t * aCount,
                                            bool aSuppressTransaction)
{
  nsCOMPtr<dom::Element> element = do_QueryInterface(aNode);
  *aCount = 0;
  if (!element || !IsCSSEditableProperty(element, aHTMLProperty, aAttribute)) {
    return NS_OK;
  }

  
  

  
  nsTArray<nsIAtom*> cssPropertyArray;
  nsTArray<nsString> cssValueArray;
  GenerateCSSDeclarationsFromHTMLStyle(element, aHTMLProperty, aAttribute,
                                       aValue, cssPropertyArray, cssValueArray,
                                       false);

  nsCOMPtr<nsIDOMElement> domElement = do_QueryInterface(element);
  
  *aCount = cssPropertyArray.Length();
  for (int32_t index = 0; index < *aCount; index++) {
    nsresult res = SetCSSProperty(domElement, cssPropertyArray[index],
                                  cssValueArray[index], aSuppressTransaction);
    NS_ENSURE_SUCCESS(res, res);
  }
  return NS_OK;
}


nsresult
nsHTMLCSSUtils::RemoveCSSEquivalentToHTMLStyle(nsIDOMNode * aNode,
                                               nsIAtom *aHTMLProperty,
                                               const nsAString *aAttribute,
                                               const nsAString *aValue,
                                               bool aSuppressTransaction)
{
  nsCOMPtr<dom::Element> element = do_QueryInterface(aNode);
  NS_ENSURE_TRUE(element, NS_OK);

  return RemoveCSSEquivalentToHTMLStyle(element, aHTMLProperty, aAttribute,
                                        aValue, aSuppressTransaction);
}

nsresult
nsHTMLCSSUtils::RemoveCSSEquivalentToHTMLStyle(dom::Element* aElement,
                                               nsIAtom* aHTMLProperty,
                                               const nsAString* aAttribute,
                                               const nsAString* aValue,
                                               bool aSuppressTransaction)
{
  MOZ_ASSERT(aElement);

  if (!IsCSSEditableProperty(aElement, aHTMLProperty, aAttribute)) {
    return NS_OK;
  }

  
  

  
  nsTArray<nsIAtom*> cssPropertyArray;
  nsTArray<nsString> cssValueArray;
  GenerateCSSDeclarationsFromHTMLStyle(aElement, aHTMLProperty, aAttribute,
                                       aValue, cssPropertyArray, cssValueArray,
                                       true);

  nsCOMPtr<nsIDOMElement> domElement = do_QueryInterface(aElement);
  
  int32_t count = cssPropertyArray.Length();
  for (int32_t index = 0; index < count; index++) {
    nsresult res = RemoveCSSProperty(domElement,
                                     cssPropertyArray[index],
                                     cssValueArray[index],
                                     aSuppressTransaction);
    NS_ENSURE_SUCCESS(res, res);
  }
  return NS_OK;
}





nsresult
nsHTMLCSSUtils::GetCSSEquivalentToHTMLInlineStyleSet(nsINode* aNode,
                                                     nsIAtom *aHTMLProperty,
                                                     const nsAString *aAttribute,
                                                     nsAString & aValueString,
                                                     StyleType aStyleType)
{
  aValueString.Truncate();
  nsCOMPtr<dom::Element> theElement = GetElementContainerOrSelf(aNode);
  NS_ENSURE_TRUE(theElement, NS_ERROR_NULL_POINTER);

  if (!theElement || !IsCSSEditableProperty(theElement, aHTMLProperty, aAttribute)) {
    return NS_OK;
  }

  
  nsTArray<nsIAtom*> cssPropertyArray;
  nsTArray<nsString> cssValueArray;
  
  
  GenerateCSSDeclarationsFromHTMLStyle(theElement, aHTMLProperty, aAttribute, nullptr,
                                       cssPropertyArray, cssValueArray, true);
  int32_t count = cssPropertyArray.Length();
  for (int32_t index = 0; index < count; index++) {
    nsAutoString valueString;
    
    nsresult res = GetCSSInlinePropertyBase(theElement, cssPropertyArray[index],
                                            valueString, aStyleType);
    NS_ENSURE_SUCCESS(res, res);
    
    if (index) {
      aValueString.Append(char16_t(' '));
    }
    aValueString.Append(valueString);
  }
  return NS_OK;
}








bool
nsHTMLCSSUtils::IsCSSEquivalentToHTMLInlineStyleSet(nsIContent* aContent,
                                                    nsIAtom* aProperty,
                                                    const nsAString* aAttribute,
                                                    const nsAString& aValue,
                                                    StyleType aStyleType)
{
  MOZ_ASSERT(aContent && aProperty);
  bool isSet;
  nsAutoString value(aValue);
  nsresult res = IsCSSEquivalentToHTMLInlineStyleSet(aContent->AsDOMNode(),
                                                     aProperty, aAttribute,
                                                     isSet, value, aStyleType);
  NS_ASSERTION(NS_SUCCEEDED(res), "IsCSSEquivalentToHTMLInlineStyleSet failed");
  NS_ENSURE_SUCCESS(res, false);
  return isSet;
}

nsresult
nsHTMLCSSUtils::IsCSSEquivalentToHTMLInlineStyleSet(nsIDOMNode *aNode,
                                                    nsIAtom *aHTMLProperty,
                                                    const nsAString *aHTMLAttribute,
                                                    bool& aIsSet,
                                                    nsAString& valueString,
                                                    StyleType aStyleType)
{
  NS_ENSURE_TRUE(aNode, NS_ERROR_NULL_POINTER);

  nsAutoString htmlValueString(valueString);
  aIsSet = false;
  nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
  do {
    valueString.Assign(htmlValueString);
    
    nsresult res = GetCSSEquivalentToHTMLInlineStyleSet(node, aHTMLProperty, aHTMLAttribute,
                                                        valueString, aStyleType);
    NS_ENSURE_SUCCESS(res, res);

    
    if (valueString.IsEmpty()) {
      return NS_OK;
    }

    if (nsEditProperty::b == aHTMLProperty) {
      if (valueString.EqualsLiteral("bold")) {
        aIsSet = true;
      } else if (valueString.EqualsLiteral("normal")) {
        aIsSet = false;
      } else if (valueString.EqualsLiteral("bolder")) {
        aIsSet = true;
        valueString.AssignLiteral("bold");
      } else {
        int32_t weight = 0;
        nsresult errorCode;
        nsAutoString value(valueString);
        weight = value.ToInteger(&errorCode);
        if (400 < weight) {
          aIsSet = true;
          valueString.AssignLiteral("bold");
        } else {
          aIsSet = false;
          valueString.AssignLiteral("normal");
        }
      }
    } else if (nsEditProperty::i == aHTMLProperty) {
      if (valueString.EqualsLiteral("italic") ||
          valueString.EqualsLiteral("oblique")) {
        aIsSet = true;
      }
    } else if (nsEditProperty::u == aHTMLProperty) {
      nsAutoString val;
      val.AssignLiteral("underline");
      aIsSet = bool(ChangeCSSInlineStyleTxn::ValueIncludes(valueString, val, false));
    } else if (nsEditProperty::strike == aHTMLProperty) {
      nsAutoString val;
      val.AssignLiteral("line-through");
      aIsSet = bool(ChangeCSSInlineStyleTxn::ValueIncludes(valueString, val, false));
    } else if (aHTMLAttribute &&
               ((nsEditProperty::font == aHTMLProperty &&
                 aHTMLAttribute->EqualsLiteral("color")) ||
                aHTMLAttribute->EqualsLiteral("bgcolor"))) {
      if (htmlValueString.IsEmpty()) {
        aIsSet = true;
      } else {
        nscolor rgba;
        nsAutoString subStr;
        htmlValueString.Right(subStr, htmlValueString.Length() - 1);
        if (NS_ColorNameToRGB(htmlValueString, &rgba) ||
            NS_HexToRGB(subStr, &rgba)) {
          nsAutoString htmlColor, tmpStr;

          if (NS_GET_A(rgba) != 255) {
            
            
            MOZ_ASSERT(NS_GET_R(rgba) == 0 && NS_GET_G(rgba) == 0 &&
                       NS_GET_B(rgba) == 0 && NS_GET_A(rgba) == 0);
            htmlColor.AppendLiteral("transparent");
          } else {
            htmlColor.AppendLiteral("rgb(");

            NS_NAMED_LITERAL_STRING(comma, ", ");

            tmpStr.AppendInt(NS_GET_R(rgba), 10);
            htmlColor.Append(tmpStr + comma);

            tmpStr.Truncate();
            tmpStr.AppendInt(NS_GET_G(rgba), 10);
            htmlColor.Append(tmpStr + comma);

            tmpStr.Truncate();
            tmpStr.AppendInt(NS_GET_B(rgba), 10);
            htmlColor.Append(tmpStr);

            htmlColor.Append(char16_t(')'));
          }

          aIsSet = htmlColor.Equals(valueString,
                                    nsCaseInsensitiveStringComparator());
        } else {
          aIsSet = htmlValueString.Equals(valueString,
                                    nsCaseInsensitiveStringComparator());
        }
      }
    } else if (nsEditProperty::tt == aHTMLProperty) {
      aIsSet = StringBeginsWith(valueString, NS_LITERAL_STRING("monospace"));
    } else if (nsEditProperty::font == aHTMLProperty && aHTMLAttribute &&
               aHTMLAttribute->EqualsLiteral("face")) {
      if (!htmlValueString.IsEmpty()) {
        const char16_t commaSpace[] = { char16_t(','), char16_t(' '), 0 };
        const char16_t comma[] = { char16_t(','), 0 };
        htmlValueString.ReplaceSubstring(commaSpace, comma);
        nsAutoString valueStringNorm(valueString);
        valueStringNorm.ReplaceSubstring(commaSpace, comma);
        aIsSet = htmlValueString.Equals(valueStringNorm,
                                        nsCaseInsensitiveStringComparator());
      } else {
        
        nsAutoString valueStringLower;
        ToLowerCase(valueString, valueStringLower);
        aIsSet = !valueStringLower.EqualsLiteral("monospace") &&
                 !valueStringLower.EqualsLiteral("serif");
      }
      return NS_OK;
    } else if (aHTMLAttribute && aHTMLAttribute->EqualsLiteral("align")) {
      aIsSet = true;
    } else {
      aIsSet = false;
      return NS_OK;
    }

    if (!htmlValueString.IsEmpty() &&
        htmlValueString.Equals(valueString,
                               nsCaseInsensitiveStringComparator())) {
      aIsSet = true;
    }

    if (htmlValueString.EqualsLiteral("-moz-editor-invert-value")) {
      aIsSet = !aIsSet;
    }

    if (nsEditProperty::u == aHTMLProperty || nsEditProperty::strike == aHTMLProperty) {
      
      
      node = node->GetParentElement();  
    }
  } while ((nsEditProperty::u == aHTMLProperty || nsEditProperty::strike == aHTMLProperty) &&
           !aIsSet && node);
  return NS_OK;
}

void
nsHTMLCSSUtils::SetCSSEnabled(bool aIsCSSPrefChecked)
{
  mIsCSSPrefChecked = aIsCSSPrefChecked;
}

bool
nsHTMLCSSUtils::IsCSSPrefChecked()
{
  return mIsCSSPrefChecked ;
}




bool
nsHTMLCSSUtils::ElementsSameStyle(nsIDOMNode *aFirstNode, nsIDOMNode *aSecondNode)
{
  nsCOMPtr<dom::Element> firstElement  = do_QueryInterface(aFirstNode);
  nsCOMPtr<dom::Element> secondElement = do_QueryInterface(aSecondNode);

  NS_ASSERTION((firstElement && secondElement), "Non element nodes passed to ElementsSameStyle.");
  NS_ENSURE_TRUE(firstElement, false);
  NS_ENSURE_TRUE(secondElement, false);

  return ElementsSameStyle(firstElement, secondElement);
}

bool
nsHTMLCSSUtils::ElementsSameStyle(dom::Element* aFirstElement,
                                  dom::Element* aSecondElement)
{
  MOZ_ASSERT(aFirstElement);
  MOZ_ASSERT(aSecondElement);

  if (aFirstElement->HasAttr(kNameSpaceID_None, nsGkAtoms::id) ||
      aSecondElement->HasAttr(kNameSpaceID_None, nsGkAtoms::id)) {
    
    
    return false;
  }

  nsAutoString firstClass, secondClass;
  bool isFirstClassSet = aFirstElement->GetAttr(kNameSpaceID_None, nsGkAtoms::_class, firstClass);
  bool isSecondClassSet = aSecondElement->GetAttr(kNameSpaceID_None, nsGkAtoms::_class, secondClass);
  if (isFirstClassSet && isSecondClassSet) {
    
    if (!firstClass.Equals(secondClass)) {
      
      
      
      
      
      
      return false;
    }
  } else if (isFirstClassSet || isSecondClassSet) {
    
    return false;
  }

  nsCOMPtr<nsIDOMCSSStyleDeclaration> firstCSSDecl, secondCSSDecl;
  uint32_t firstLength, secondLength;
  nsresult rv = GetInlineStyles(aFirstElement,  getter_AddRefs(firstCSSDecl),  &firstLength);
  if (NS_FAILED(rv) || !firstCSSDecl) {
    return false;
  }
  rv = GetInlineStyles(aSecondElement, getter_AddRefs(secondCSSDecl), &secondLength);
  if (NS_FAILED(rv) || !secondCSSDecl) {
    return false;
  }

  if (firstLength != secondLength) {
    
    return false;
  }

  if (!firstLength) {
    
    return true;
  }

  nsAutoString propertyNameString;
  nsAutoString firstValue, secondValue;
  for (uint32_t i = 0; i < firstLength; i++) {
    firstCSSDecl->Item(i, propertyNameString);
    firstCSSDecl->GetPropertyValue(propertyNameString, firstValue);
    secondCSSDecl->GetPropertyValue(propertyNameString, secondValue);
    if (!firstValue.Equals(secondValue)) {
      return false;
    }
  }
  for (uint32_t i = 0; i < secondLength; i++) {
    secondCSSDecl->Item(i, propertyNameString);
    secondCSSDecl->GetPropertyValue(propertyNameString, secondValue);
    firstCSSDecl->GetPropertyValue(propertyNameString, firstValue);
    if (!firstValue.Equals(secondValue)) {
      return false;
    }
  }

  return true;
}

nsresult
nsHTMLCSSUtils::GetInlineStyles(dom::Element* aElement,
                                nsIDOMCSSStyleDeclaration** aCssDecl,
                                uint32_t* aLength)
{
  return GetInlineStyles(static_cast<nsISupports*>(aElement), aCssDecl, aLength);
}

nsresult
nsHTMLCSSUtils::GetInlineStyles(nsIDOMElement* aElement,
                                nsIDOMCSSStyleDeclaration** aCssDecl,
                                uint32_t* aLength)
{
  return GetInlineStyles(static_cast<nsISupports*>(aElement), aCssDecl, aLength);
}

nsresult
nsHTMLCSSUtils::GetInlineStyles(nsISupports *aElement,
                                nsIDOMCSSStyleDeclaration **aCssDecl,
                                uint32_t *aLength)
{
  NS_ENSURE_TRUE(aElement && aLength, NS_ERROR_NULL_POINTER);
  *aLength = 0;
  nsCOMPtr<nsIDOMElementCSSInlineStyle> inlineStyles = do_QueryInterface(aElement);
  NS_ENSURE_TRUE(inlineStyles, NS_ERROR_NULL_POINTER);

  nsresult res = inlineStyles->GetStyle(aCssDecl);
  NS_ENSURE_SUCCESS(res, NS_ERROR_NULL_POINTER);
  MOZ_ASSERT(*aCssDecl);

  (*aCssDecl)->GetLength(aLength);
  return NS_OK;
}

already_AddRefed<nsIDOMElement>
nsHTMLCSSUtils::GetElementContainerOrSelf(nsIDOMNode* aNode)
{
  nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
  NS_ENSURE_TRUE(node, nullptr);
  nsCOMPtr<nsIDOMElement> element =
    do_QueryInterface(GetElementContainerOrSelf(node));
  return element.forget();
}

dom::Element*
nsHTMLCSSUtils::GetElementContainerOrSelf(nsINode* aNode)
{
  MOZ_ASSERT(aNode);
  if (nsIDOMNode::DOCUMENT_NODE == aNode->NodeType()) {
    return nullptr;
  }

  nsINode* node = aNode;
  
  while (node && !node->IsElement()) {
    node = node->GetParentNode();
  }

  NS_ENSURE_TRUE(node, nullptr);
  return node->AsElement();
}

nsresult
nsHTMLCSSUtils::SetCSSProperty(nsIDOMElement * aElement,
                               const nsAString & aProperty,
                               const nsAString & aValue)
{
  nsCOMPtr<nsIDOMCSSStyleDeclaration> cssDecl;
  uint32_t length;
  nsresult res = GetInlineStyles(aElement, getter_AddRefs(cssDecl), &length);
  if (NS_FAILED(res) || !cssDecl) return res;

  return cssDecl->SetProperty(aProperty,
                              aValue,
                              EmptyString());
}

nsresult
nsHTMLCSSUtils::SetCSSPropertyPixels(nsIDOMElement * aElement,
                                     const nsAString & aProperty,
                                     int32_t aIntValue)
{
  nsAutoString s;
  s.AppendInt(aIntValue);
  return SetCSSProperty(aElement, aProperty, s + NS_LITERAL_STRING("px"));
}
