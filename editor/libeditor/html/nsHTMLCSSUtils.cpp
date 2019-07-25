





































#include "nsHTMLEditor.h"
#include "nsCOMPtr.h"
#include "nsHTMLEditUtils.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsIServiceManager.h"
#include "nsEditProperty.h"
#include "ChangeCSSInlineStyleTxn.h"
#include "nsIDOMElement.h"
#include "nsIDOMElementCSSInlineStyle.h"
#include "nsIDOMDocument.h"
#include "nsIContent.h"
#include "nsIAtom.h"
#include "nsTextEditUtils.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsHTMLCSSUtils.h"
#include "nsColor.h"
#include "nsAttrName.h"
#include "nsAutoPtr.h"

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
    if (-1 == aOutputString.FindChar(PRUnichar('%'))) {
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
  { nsHTMLCSSUtils::eCSSEditableProperty_font_weight, ProcessBValue, nsnull, nsnull, nsnull, PR_TRUE, PR_FALSE },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable italicEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_font_style, ProcessDefaultValue, "italic", nsnull, nsnull, PR_TRUE, PR_FALSE },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable underlineEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_text_decoration, ProcessDefaultValue, "underline", nsnull, nsnull, PR_TRUE, PR_FALSE },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable strikeEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_text_decoration, ProcessDefaultValue, "line-through", nsnull, nsnull, PR_TRUE, PR_FALSE },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable ttEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_font_family, ProcessDefaultValue, "monospace", nsnull, nsnull, PR_TRUE, PR_FALSE },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable fontColorEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_color, ProcessSameValue, nsnull, nsnull, nsnull, PR_TRUE, PR_FALSE },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable fontFaceEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_font_family, ProcessSameValue, nsnull, nsnull, nsnull, PR_TRUE, PR_FALSE },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable bgcolorEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_background_color, ProcessSameValue, nsnull, nsnull, nsnull, PR_TRUE, PR_FALSE },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable backgroundImageEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_background_image, ProcessExtendedValue, nsnull, "url(", ")", PR_TRUE, PR_TRUE },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable textColorEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_color, ProcessSameValue, nsnull, nsnull, nsnull, PR_TRUE, PR_FALSE },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable borderEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_border, ProcessExtendedValue, nsnull, nsnull, "px solid", PR_TRUE, PR_FALSE },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable textAlignEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_text_align, ProcessSameValue, nsnull, nsnull, nsnull, PR_TRUE, PR_FALSE },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable captionAlignEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_caption_side, ProcessSameValue, nsnull, nsnull, nsnull, PR_TRUE, PR_FALSE },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable verticalAlignEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_vertical_align, ProcessSameValue, nsnull, nsnull, nsnull, PR_TRUE, PR_FALSE },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable nowrapEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_whitespace, ProcessDefaultValue, "nowrap", nsnull, nsnull, PR_TRUE, PR_FALSE },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable widthEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_width, ProcessLengthValue, nsnull, nsnull, nsnull, PR_TRUE, PR_FALSE },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable heightEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_height, ProcessLengthValue, nsnull, nsnull, nsnull, PR_TRUE, PR_FALSE },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable listStyleTypeEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_list_style_type, ProcessListStyleTypeValue, nsnull, nsnull, nsnull, PR_TRUE, PR_TRUE },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable tableAlignEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_text_align, ProcessDefaultValue, "left", nsnull, nsnull, PR_FALSE, PR_FALSE },
  { nsHTMLCSSUtils::eCSSEditableProperty_margin_left, ProcessMarginLeftValue, nsnull, nsnull, nsnull, PR_TRUE, PR_FALSE },
  { nsHTMLCSSUtils::eCSSEditableProperty_margin_right, ProcessMarginRightValue, nsnull, nsnull, nsnull, PR_TRUE, PR_FALSE },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

const nsHTMLCSSUtils::CSSEquivTable hrAlignEquivTable[] = {
  { nsHTMLCSSUtils::eCSSEditableProperty_margin_left, ProcessMarginLeftValue, nsnull, nsnull, nsnull, PR_TRUE, PR_FALSE },
  { nsHTMLCSSUtils::eCSSEditableProperty_margin_right, ProcessMarginRightValue, nsnull, nsnull, nsnull, PR_TRUE, PR_FALSE },
  { nsHTMLCSSUtils::eCSSEditableProperty_NONE, 0 }
};

nsHTMLCSSUtils::nsHTMLCSSUtils()
: mIsCSSPrefChecked(PR_FALSE)
{
}

nsHTMLCSSUtils::~nsHTMLCSSUtils()
{
}

nsresult
nsHTMLCSSUtils::Init(nsHTMLEditor *aEditor)
{
  nsresult result = NS_OK;
  mHTMLEditor = static_cast<nsHTMLEditor*>(aEditor);

  
  nsCOMPtr<nsIPrefBranch> prefBranch =
    do_GetService(NS_PREFSERVICE_CONTRACTID, &result);
  if (NS_SUCCEEDED(result) && prefBranch) {
    result = prefBranch->GetBoolPref("editor.use_css", &mIsCSSPrefChecked);
    NS_ENSURE_SUCCESS(result, result);
  }
  return result;
}



PRBool
nsHTMLCSSUtils::IsCSSEditableProperty(nsIDOMNode * aNode,
                                      nsIAtom * aProperty,
                                      const nsAString * aAttribute)
{
  NS_ASSERTION(aNode, "Shouldn't you pass aNode? - Bug 214025");

  nsCOMPtr<nsIDOMNode> node = aNode;
  
  if (mHTMLEditor->IsTextNode(aNode)) {
    aNode->GetParentNode(getter_AddRefs(node));
  }
  nsCOMPtr<nsIContent> content = do_QueryInterface(node);
  NS_ENSURE_TRUE(content, PR_FALSE);

  nsIAtom *tagName = content->Tag();
  
  

  
  if (nsEditProperty::b == aProperty
      || nsEditProperty::i == aProperty
      || nsEditProperty::tt == aProperty
      || nsEditProperty::u == aProperty
      || nsEditProperty::strike == aProperty
      || ((nsEditProperty::font == aProperty) && aAttribute &&
           (aAttribute->EqualsLiteral("color") ||
            aAttribute->EqualsLiteral("face")))) {
    return PR_TRUE;
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
    return PR_TRUE;
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
    return PR_TRUE;
  }

  
  if (aAttribute && (nsEditProperty::body == tagName) &&
      (aAttribute->EqualsLiteral("text")
       || aAttribute->EqualsLiteral("background")
       || aAttribute->EqualsLiteral("bgcolor"))) {
    return PR_TRUE;
  }

  
  if (aAttribute && aAttribute->EqualsLiteral("bgcolor")) {
    return PR_TRUE;
  }

  
  if (aAttribute && ((nsEditProperty::td == tagName)
                      || (nsEditProperty::th == tagName)) &&
      (aAttribute->EqualsLiteral("height")
       || aAttribute->EqualsLiteral("width")
       || aAttribute->EqualsLiteral("nowrap"))) {
    return PR_TRUE;
  }

  
  if (aAttribute && (nsEditProperty::table == tagName) &&
      (aAttribute->EqualsLiteral("height")
       || aAttribute->EqualsLiteral("width"))) {
    return PR_TRUE;
  }

  
  if (aAttribute && (nsEditProperty::hr == tagName) &&
      (aAttribute->EqualsLiteral("size")
       || aAttribute->EqualsLiteral("width"))) {
    return PR_TRUE;
  }

  
  if (aAttribute && (nsEditProperty::ol == tagName
                     || nsEditProperty::ul == tagName
                     || nsEditProperty::li == tagName) &&
      aAttribute->EqualsLiteral("type")) {
    return PR_TRUE;
  }

  if (aAttribute && nsEditProperty::img == tagName &&
      (aAttribute->EqualsLiteral("border")
       || aAttribute->EqualsLiteral("width")
       || aAttribute->EqualsLiteral("height"))) {
    return PR_TRUE;
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
    return PR_TRUE;
  }

  return PR_FALSE;
}



nsresult
nsHTMLCSSUtils::SetCSSProperty(nsIDOMElement *aElement, nsIAtom * aProperty, const nsAString & aValue,
                               PRBool aSuppressTransaction)
{
  nsRefPtr<ChangeCSSInlineStyleTxn> txn;
  nsresult result = CreateCSSPropertyTxn(aElement, aProperty, aValue,
                                         getter_AddRefs(txn), PR_FALSE);
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
                                     PRInt32 aIntValue,
                                     PRBool aSuppressTransaction)
{
  nsAutoString s;
  s.AppendInt(aIntValue);
  return SetCSSProperty(aElement, aProperty, s + NS_LITERAL_STRING("px"),
                        aSuppressTransaction);
}




nsresult
nsHTMLCSSUtils::RemoveCSSProperty(nsIDOMElement *aElement, nsIAtom * aProperty, const nsAString & aValue,
                                  PRBool aSuppressTransaction)
{
  nsRefPtr<ChangeCSSInlineStyleTxn> txn;
  nsresult result = CreateCSSPropertyTxn(aElement, aProperty, aValue,
                                         getter_AddRefs(txn), PR_TRUE);
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
                                     PRBool aRemoveProperty)
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
  return GetCSSInlinePropertyBase(aNode, aProperty, aValue, nsnull, SPECIFIED_STYLE_TYPE);
}

nsresult
nsHTMLCSSUtils::GetComputedProperty(nsIDOMNode *aNode, nsIAtom *aProperty,
                                    nsAString & aValue)
{
  nsCOMPtr<nsIDOMWindow> window;
  nsresult res = GetDefaultViewCSS(aNode, getter_AddRefs(window));
  NS_ENSURE_SUCCESS(res, res);

  return GetCSSInlinePropertyBase(aNode, aProperty, aValue, window, COMPUTED_STYLE_TYPE);
}

nsresult
nsHTMLCSSUtils::GetCSSInlinePropertyBase(nsIDOMNode *aNode, nsIAtom *aProperty,
                                         nsAString& aValue,
                                         nsIDOMWindow* aWindow,
                                         PRUint8 aStyleType)
{
  aValue.Truncate();
  NS_ENSURE_TRUE(aProperty, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsIDOMElement> element;
  nsresult res = GetElementContainerOrSelf(aNode, getter_AddRefs(element));
  NS_ENSURE_SUCCESS(res, res);

  switch (aStyleType) {
    case COMPUTED_STYLE_TYPE:
      if (element && aWindow) {
        nsAutoString value, propString;
        nsCOMPtr<nsIDOMCSSStyleDeclaration> cssDecl;
        aProperty->ToString(propString);
        
        res = aWindow->GetComputedStyle(element, EmptyString(), getter_AddRefs(cssDecl));
        if (NS_FAILED(res) || !cssDecl)
          return res;
        
        res = cssDecl->GetPropertyValue(propString, value);
        NS_ENSURE_SUCCESS(res, res);
        aValue.Assign(value);
      }
      break;
    case SPECIFIED_STYLE_TYPE:
      if (element) {
        nsCOMPtr<nsIDOMCSSStyleDeclaration> cssDecl;
        PRUint32 length;
        res = GetInlineStyles(element, getter_AddRefs(cssDecl), &length);
        if (NS_FAILED(res) || !cssDecl) return res;
        nsAutoString value, propString;
        aProperty->ToString(propString);
        res = cssDecl->GetPropertyValue(propString, value);
        NS_ENSURE_SUCCESS(res, res);
        aValue.Assign(value);
      }
      break;
  }
  return NS_OK;
}

nsresult
nsHTMLCSSUtils::GetDefaultViewCSS(nsIDOMNode *aNode, nsIDOMWindow **aViewCSS)
{
  nsCOMPtr<nsIDOMElement> element;
  nsresult res = GetElementContainerOrSelf(aNode, getter_AddRefs(element));
  NS_ENSURE_SUCCESS(res, res);

  
  *aViewCSS = nsnull;
  if (!element) {
    return NS_OK;
  }
  
  nsCOMPtr<nsIDOMDocument> doc;
  nsCOMPtr<nsIDOMNode> node = do_QueryInterface(element);
  res = node->GetOwnerDocument(getter_AddRefs(doc));
  NS_ENSURE_SUCCESS(res, res);
  if (!doc) {
    return NS_OK;
  }
  nsCOMPtr<nsIDOMWindow> window;
  res = doc->GetDefaultView(getter_AddRefs(window));
  NS_ENSURE_SUCCESS(res, res);
  window.forget(aViewCSS);
  return NS_OK;
}

nsresult
NS_NewHTMLCSSUtils(nsHTMLCSSUtils** aInstancePtrResult)
{
  nsHTMLCSSUtils * rules = new nsHTMLCSSUtils();
  if (rules) {
    *aInstancePtrResult = rules;
    return NS_OK;
  }

  *aInstancePtrResult = nsnull;
  return NS_ERROR_OUT_OF_MEMORY;
}



nsresult
nsHTMLCSSUtils::RemoveCSSInlineStyle(nsIDOMNode *aNode, nsIAtom *aProperty, const nsAString & aPropertyValue)
{
  nsCOMPtr<nsIDOMElement> elem = do_QueryInterface(aNode);

  
  nsresult res = RemoveCSSProperty(elem, aProperty, aPropertyValue, PR_FALSE);
  NS_ENSURE_SUCCESS(res, res);

  if (nsEditor::NodeIsType(aNode, nsEditProperty::span)) {
    nsCOMPtr<nsIContent> content = do_QueryInterface(aNode);
    PRUint32 attrCount = content->GetAttrCount();

    if (0 == attrCount) {
      
      res = mHTMLEditor->RemoveContainer(aNode);
      NS_ENSURE_SUCCESS(res, res);
    }
    else if (1 == attrCount) {
      
      if (content->GetAttrNameAt(0)->Equals(nsEditProperty::mozdirty)) {
        res = mHTMLEditor->RemoveContainer(aNode);
        NS_ENSURE_SUCCESS(res, res);
      }
    }
  }
  return NS_OK;
}



PRBool
nsHTMLCSSUtils::IsCSSInvertable(nsIAtom *aProperty, const nsAString *aAttribute)
{
  return PRBool(nsEditProperty::b == aProperty);
}


nsresult
nsHTMLCSSUtils::GetDefaultBackgroundColor(nsAString & aColor)
{
  nsresult result;
  nsCOMPtr<nsIPrefBranch> prefBranch =
    do_GetService(NS_PREFSERVICE_CONTRACTID, &result);
  NS_ENSURE_SUCCESS(result, result);
  aColor.AssignLiteral("#ffffff");
  nsXPIDLCString returnColor;
  if (prefBranch) {
    PRBool useCustomColors;
    result = prefBranch->GetBoolPref("editor.use_custom_colors", &useCustomColors);
    NS_ENSURE_SUCCESS(result, result);
    if (useCustomColors) {
      result = prefBranch->GetCharPref("editor.background_color",
                                       getter_Copies(returnColor));
      NS_ENSURE_SUCCESS(result, result);
    }
    else {
      PRBool useSystemColors;
      result = prefBranch->GetBoolPref("browser.display.use_system_colors", &useSystemColors);
      NS_ENSURE_SUCCESS(result, result);
      if (!useSystemColors) {
        result = prefBranch->GetCharPref("browser.display.background_color",
                                         getter_Copies(returnColor));
        NS_ENSURE_SUCCESS(result, result);
      }
    }
  }
  if (returnColor) {
    CopyASCIItoUTF16(returnColor, aColor);
  }
  return NS_OK;
}


nsresult
nsHTMLCSSUtils::GetDefaultLengthUnit(nsAString & aLengthUnit)
{
  nsresult result;
  nsCOMPtr<nsIPrefBranch> prefBranch =
    do_GetService(NS_PREFSERVICE_CONTRACTID, &result);
  NS_ENSURE_SUCCESS(result, result);
  aLengthUnit.AssignLiteral("px");
  if (NS_SUCCEEDED(result) && prefBranch) {
    nsXPIDLCString returnLengthUnit;
    result = prefBranch->GetCharPref("editor.css.default_length_unit",
                                     getter_Copies(returnLengthUnit));
    NS_ENSURE_SUCCESS(result, result);
    if (returnLengthUnit) {
      CopyASCIItoUTF16(returnLengthUnit, aLengthUnit);
    }
  }
  return NS_OK;
}




void
nsHTMLCSSUtils::ParseLength(const nsAString & aString, float * aValue, nsIAtom ** aUnit)
{
  nsAString::const_iterator iter;
  aString.BeginReading(iter);

  float a = 10.0f , b = 1.0f, value = 0;
  PRInt8 sign = 1;
  PRInt32 i = 0, j = aString.Length();
  PRUnichar c;
  PRBool floatingPointFound = PR_FALSE;
  c = *iter;
  if (PRUnichar('-') == c) { sign = -1; iter++; i++; }
  else if (PRUnichar('+') == c) { iter++; i++; }
  while (i < j) {
    c = *iter;
    if ((PRUnichar('0') == c) ||
        (PRUnichar('1') == c) ||
        (PRUnichar('2') == c) ||
        (PRUnichar('3') == c) ||
        (PRUnichar('4') == c) ||
        (PRUnichar('5') == c) ||
        (PRUnichar('6') == c) ||
        (PRUnichar('7') == c) ||
        (PRUnichar('8') == c) ||
        (PRUnichar('9') == c)) {
      value = (value * a) + (b * (c - PRUnichar('0')));
      b = b / 10 * a;
    }
    else if (!floatingPointFound && (PRUnichar('.') == c)) {
      floatingPointFound = PR_TRUE;
      a = 1.0f; b = 0.1f;
    }
    else break;
    iter++;
    i++;
  }
  *aValue = value * sign;
  *aUnit = NS_NewAtom(StringTail(aString, j-i)); 
}

void
nsHTMLCSSUtils::GetCSSPropertyAtom(nsCSSEditableProperty aProperty, nsIAtom ** aAtom)
{
  *aAtom = nsnull;
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
                                     PRBool aGetOrRemoveRequest)
{
  
  aPropertyArray.Clear();
  aValueArray.Clear();

  
  nsAutoString value, lowerCasedValue;
  if (aValue) {
    value.Assign(*aValue);
    lowerCasedValue.Assign(*aValue);
    ToLowerCase(lowerCasedValue);
  }

  PRInt8 index = 0;
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
nsHTMLCSSUtils::GenerateCSSDeclarationsFromHTMLStyle(nsIDOMNode * aNode,
                                                     nsIAtom *aHTMLProperty,
                                                     const nsAString * aAttribute,
                                                     const nsAString * aValue,
                                                     nsTArray<nsIAtom*> & cssPropertyArray,
                                                     nsTArray<nsString> & cssValueArray,
                                                     PRBool aGetOrRemoveRequest)
{
  nsCOMPtr<nsIDOMNode> node = aNode;
  if (mHTMLEditor->IsTextNode(aNode)) {
    aNode->GetParentNode(getter_AddRefs(node));
  }
  if (!node) return;

  nsIAtom *tagName = nsEditor::GetTag(node);

  if (nsEditProperty::b == aHTMLProperty) {
    BuildCSSDeclarations(cssPropertyArray, cssValueArray, boldEquivTable, aValue, aGetOrRemoveRequest);
  }
  else if (nsEditProperty::i == aHTMLProperty) {
    BuildCSSDeclarations(cssPropertyArray, cssValueArray, italicEquivTable, aValue, aGetOrRemoveRequest);
  }
  else if (nsEditProperty::u == aHTMLProperty) {
    BuildCSSDeclarations(cssPropertyArray, cssValueArray, underlineEquivTable, aValue, aGetOrRemoveRequest);
  }
  else if (nsEditProperty::strike == aHTMLProperty) {
    BuildCSSDeclarations(cssPropertyArray, cssValueArray, strikeEquivTable, aValue, aGetOrRemoveRequest);
  }
  else if (nsEditProperty::tt == aHTMLProperty) {
    BuildCSSDeclarations(cssPropertyArray, cssValueArray, ttEquivTable, aValue, aGetOrRemoveRequest);
  }
  else if (aAttribute) {
    if (nsEditProperty::font == aHTMLProperty &&
        aAttribute->EqualsLiteral("color")) {
      BuildCSSDeclarations(cssPropertyArray, cssValueArray, fontColorEquivTable, aValue, aGetOrRemoveRequest);
    }
    else if (nsEditProperty::font == aHTMLProperty &&
             aAttribute->EqualsLiteral("face")) {
      BuildCSSDeclarations(cssPropertyArray, cssValueArray, fontFaceEquivTable, aValue, aGetOrRemoveRequest);
    }
    else if (aAttribute->EqualsLiteral("bgcolor")) {
      BuildCSSDeclarations(cssPropertyArray, cssValueArray, bgcolorEquivTable, aValue, aGetOrRemoveRequest);
    }
    else if (aAttribute->EqualsLiteral("background")) {
      BuildCSSDeclarations(cssPropertyArray, cssValueArray, backgroundImageEquivTable, aValue, aGetOrRemoveRequest);
    }
    else if (aAttribute->EqualsLiteral("text")) {
      BuildCSSDeclarations(cssPropertyArray, cssValueArray, textColorEquivTable, aValue, aGetOrRemoveRequest);
    }
    else if (aAttribute->EqualsLiteral("border")) {
      BuildCSSDeclarations(cssPropertyArray, cssValueArray, borderEquivTable, aValue, aGetOrRemoveRequest);
    }
    else if (aAttribute->EqualsLiteral("align")) {
      if (nsEditProperty::table  == tagName) {
        BuildCSSDeclarations(cssPropertyArray, cssValueArray, tableAlignEquivTable, aValue, aGetOrRemoveRequest);
      }
      else if (nsEditProperty::hr  == tagName) {
        BuildCSSDeclarations(cssPropertyArray, cssValueArray, hrAlignEquivTable, aValue, aGetOrRemoveRequest);
      }
      else if (nsEditProperty::legend  == tagName ||
               nsEditProperty::caption == tagName) {
        BuildCSSDeclarations(cssPropertyArray, cssValueArray, captionAlignEquivTable, aValue, aGetOrRemoveRequest);
      }
      else {
        BuildCSSDeclarations(cssPropertyArray, cssValueArray, textAlignEquivTable, aValue, aGetOrRemoveRequest);
      }
    }
    else if (aAttribute->EqualsLiteral("valign")) {
      BuildCSSDeclarations(cssPropertyArray, cssValueArray, verticalAlignEquivTable, aValue, aGetOrRemoveRequest);
    }
    else if (aAttribute->EqualsLiteral("nowrap")) {
      BuildCSSDeclarations(cssPropertyArray, cssValueArray, nowrapEquivTable, aValue, aGetOrRemoveRequest);
    }
    else if (aAttribute->EqualsLiteral("width")) {
      BuildCSSDeclarations(cssPropertyArray, cssValueArray, widthEquivTable, aValue, aGetOrRemoveRequest);
    }
    else if (aAttribute->EqualsLiteral("height") ||
             (nsEditProperty::hr == tagName && aAttribute->EqualsLiteral("size"))) {
      BuildCSSDeclarations(cssPropertyArray, cssValueArray, heightEquivTable, aValue, aGetOrRemoveRequest);
    }
    else if (aAttribute->EqualsLiteral("type") &&
             (nsEditProperty::ol == tagName
              || nsEditProperty::ul == tagName
              || nsEditProperty::li == tagName)) {
      BuildCSSDeclarations(cssPropertyArray, cssValueArray, listStyleTypeEquivTable, aValue, aGetOrRemoveRequest);
    }
  }
}



nsresult
nsHTMLCSSUtils::SetCSSEquivalentToHTMLStyle(nsIDOMNode * aNode,
                                            nsIAtom *aHTMLProperty,
                                            const nsAString *aAttribute,
                                            const nsAString *aValue,
                                            PRInt32 * aCount,
                                            PRBool aSuppressTransaction)
{
  nsCOMPtr<nsIDOMElement> theElement = do_QueryInterface(aNode);
  nsresult res = NS_OK;
  *aCount = 0;
  if (theElement && IsCSSEditableProperty(aNode, aHTMLProperty, aAttribute)) {
    
    

    
    nsTArray<nsIAtom*> cssPropertyArray;
    nsTArray<nsString> cssValueArray;
    GenerateCSSDeclarationsFromHTMLStyle(aNode, aHTMLProperty, aAttribute, aValue,
                                         cssPropertyArray, cssValueArray, PR_FALSE);

    
    *aCount = cssPropertyArray.Length();
    PRInt32 index;
    for (index = 0; index < *aCount; index++) {
      nsCOMPtr<nsIDOMElement> theElement = do_QueryInterface(aNode);
      res = SetCSSProperty(theElement, cssPropertyArray[index],
                           cssValueArray[index], aSuppressTransaction);
      NS_ENSURE_SUCCESS(res, res);
    }
  }
  return NS_OK;
}


nsresult
nsHTMLCSSUtils::RemoveCSSEquivalentToHTMLStyle(nsIDOMNode * aNode,
                                               nsIAtom *aHTMLProperty,
                                               const nsAString *aAttribute,
                                               const nsAString *aValue,
                                               PRBool aSuppressTransaction)
{
  nsCOMPtr<nsIDOMElement> theElement = do_QueryInterface(aNode);
  nsresult res = NS_OK;
  PRInt32 count = 0;
  if (theElement && IsCSSEditableProperty(aNode, aHTMLProperty, aAttribute)) {
    
    

    
    nsTArray<nsIAtom*> cssPropertyArray;
    nsTArray<nsString> cssValueArray;
    GenerateCSSDeclarationsFromHTMLStyle(aNode, aHTMLProperty, aAttribute, aValue,
                                         cssPropertyArray, cssValueArray, PR_TRUE);

    
    count = cssPropertyArray.Length();
    PRInt32 index;
    for (index = 0; index < count; index++) {
      res = RemoveCSSProperty(theElement,
                              cssPropertyArray[index],
                              cssValueArray[index],
                              aSuppressTransaction);
      NS_ENSURE_SUCCESS(res, res);
    }
  }
  return NS_OK;
}


nsresult
nsHTMLCSSUtils::HasClassOrID(nsIDOMElement * aElement, PRBool & aReturn)
{
  nsAutoString classVal, idVal;
  PRBool isClassSet, isIdSet;
  aReturn = PR_FALSE;

  nsresult res = mHTMLEditor->GetAttributeValue(aElement,  NS_LITERAL_STRING("class"), classVal, &isClassSet);
  NS_ENSURE_SUCCESS(res, res);
  res = mHTMLEditor->GetAttributeValue(aElement,  NS_LITERAL_STRING("id"), idVal, &isIdSet);
  NS_ENSURE_SUCCESS(res, res);

  
  
  aReturn = ((isClassSet && !classVal.IsEmpty()) ||
             (isIdSet    && !idVal.IsEmpty()));
  return NS_OK;
}





nsresult
nsHTMLCSSUtils::GetCSSEquivalentToHTMLInlineStyleSet(nsIDOMNode * aNode,
                                                     nsIAtom *aHTMLProperty,
                                                     const nsAString *aAttribute,
                                                     nsAString & aValueString,
                                                     PRUint8 aStyleType)
{
  aValueString.Truncate();
  nsCOMPtr<nsIDOMElement> theElement;
  nsresult res = GetElementContainerOrSelf(aNode, getter_AddRefs(theElement));
  NS_ENSURE_SUCCESS(res, res);

  if (theElement && IsCSSEditableProperty(theElement, aHTMLProperty, aAttribute)) {
    
    
    nsCOMPtr<nsIDOMWindow> window;
    if (COMPUTED_STYLE_TYPE == aStyleType) {
      res = GetDefaultViewCSS(theElement, getter_AddRefs(window));
      NS_ENSURE_SUCCESS(res, res);
    }
    nsTArray<nsIAtom*> cssPropertyArray;
    nsTArray<nsString> cssValueArray;
    
    
    GenerateCSSDeclarationsFromHTMLStyle(theElement, aHTMLProperty, aAttribute, nsnull,
                                         cssPropertyArray, cssValueArray, PR_TRUE);
    PRInt32 count = cssPropertyArray.Length();
    PRInt32 index;
    for (index = 0; index < count; index++) {
      nsAutoString valueString;
      
      res = GetCSSInlinePropertyBase(theElement, cssPropertyArray[index],
                                     valueString, window, aStyleType);
      NS_ENSURE_SUCCESS(res, res);
      
      if (index) aValueString.Append(PRUnichar(' '));
      aValueString.Append(valueString);
    }
  }
  return NS_OK;
}






nsresult
nsHTMLCSSUtils::IsCSSEquivalentToHTMLInlineStyleSet(nsIDOMNode * aNode,
                                                    nsIAtom *aHTMLProperty,
                                                    const nsAString * aHTMLAttribute,
                                                    PRBool & aIsSet,
                                                    nsAString & valueString,
                                                    PRUint8 aStyleType)
{
  NS_ENSURE_TRUE(aNode, NS_ERROR_NULL_POINTER);

  nsAutoString htmlValueString(valueString);
  aIsSet = PR_FALSE;
  nsCOMPtr<nsIDOMNode> node = aNode;
  NS_NAMED_LITERAL_STRING(boldStr, "bold");
  do {
    valueString.Assign(htmlValueString);
    
    nsresult res = GetCSSEquivalentToHTMLInlineStyleSet(node, aHTMLProperty, aHTMLAttribute,
                                                        valueString, aStyleType);
    NS_ENSURE_SUCCESS(res, res);

    
    if (valueString.IsEmpty()) return NS_OK;

    if (nsEditProperty::b == aHTMLProperty) {
      if (valueString.Equals(boldStr)) {
        aIsSet = PR_TRUE;
      }
      else if (valueString.EqualsLiteral("normal")) {
        aIsSet = PR_FALSE;
      }
      else if (valueString.EqualsLiteral("bolder")) {
        aIsSet = PR_TRUE;
        valueString.Assign(boldStr);
      }
      else {
        PRInt32 weight = 0;
        PRInt32 errorCode;
        nsAutoString value(valueString);
        weight = value.ToInteger(&errorCode, 10);
        if (400 < weight) {
          aIsSet = PR_TRUE;
          valueString.Assign(boldStr);
        }
        else {
          aIsSet = PR_FALSE;
          valueString.AssignLiteral("normal");
        }
      }
    }
    
    else if (nsEditProperty::i == aHTMLProperty) {
      if (valueString.EqualsLiteral("italic") ||
          valueString.EqualsLiteral("oblique")) {
        aIsSet= PR_TRUE;
      }
    }

    else if (nsEditProperty::u == aHTMLProperty) {
      nsAutoString val;
      val.AssignLiteral("underline");
      aIsSet = PRBool(ChangeCSSInlineStyleTxn::ValueIncludes(valueString, val, PR_FALSE));
    }

    else if (nsEditProperty::strike == aHTMLProperty) {
      nsAutoString val;
      val.AssignLiteral("line-through");
      aIsSet = PRBool(ChangeCSSInlineStyleTxn::ValueIncludes(valueString, val, PR_FALSE));
    }

    else if (aHTMLAttribute &&
             ( (nsEditProperty::font == aHTMLProperty && 
                aHTMLAttribute->EqualsLiteral("color")) ||
               aHTMLAttribute->EqualsLiteral("bgcolor"))) {
      if (htmlValueString.IsEmpty())
        aIsSet = PR_TRUE;
      else {
        nscolor rgba;
        nsAutoString subStr;
        htmlValueString.Right(subStr, htmlValueString.Length()-1);
        if (NS_ColorNameToRGB(htmlValueString, &rgba) ||
            NS_HexToRGB(subStr, &rgba)) {
          nsAutoString htmlColor, tmpStr;
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

          htmlColor.Append(PRUnichar(')'));
          aIsSet = htmlColor.Equals(valueString,
                                    nsCaseInsensitiveStringComparator());
        }
        else
          aIsSet = htmlValueString.Equals(valueString,
                                    nsCaseInsensitiveStringComparator());
      }
    }

    else if (nsEditProperty::tt == aHTMLProperty) {
      aIsSet = StringBeginsWith(valueString, NS_LITERAL_STRING("monospace"));
    }
    
    else if ((nsEditProperty::font == aHTMLProperty) && aHTMLAttribute
             && aHTMLAttribute->EqualsLiteral("face")) {
      if (!htmlValueString.IsEmpty()) {
        const PRUnichar commaSpace[] = { PRUnichar(','), PRUnichar(' '), 0 };
        const PRUnichar comma[] = { PRUnichar(','), 0 };
        htmlValueString.ReplaceSubstring(commaSpace, comma);
        nsAutoString valueStringNorm(valueString);
        valueStringNorm.ReplaceSubstring(commaSpace, comma);
        aIsSet = htmlValueString.Equals(valueStringNorm,
                                        nsCaseInsensitiveStringComparator());
      }
      else {
        
        nsAutoString valueStringLower;
        ToLowerCase(valueString, valueStringLower);
        aIsSet = !valueStringLower.EqualsLiteral("monospace") &&
                 !valueStringLower.EqualsLiteral("serif");
      }
      return NS_OK;
    }
    else if (aHTMLAttribute
             && aHTMLAttribute->EqualsLiteral("align")) {
      aIsSet = PR_TRUE;
    }
    else {
      aIsSet = PR_FALSE;
      return NS_OK;
    }

    if (!htmlValueString.IsEmpty()) {
      if (htmlValueString.Equals(valueString,
                                 nsCaseInsensitiveStringComparator())) {
        aIsSet = PR_TRUE;
      }
    }

    if (nsEditProperty::u == aHTMLProperty || nsEditProperty::strike == aHTMLProperty) {
      
      
      nsCOMPtr<nsIDOMNode> tmp;
      res = node->GetParentNode(getter_AddRefs(tmp));
      NS_ENSURE_SUCCESS(res, res);
      nsCOMPtr<nsIDOMElement> element = do_QueryInterface(tmp);
      node = element;  
    }
  } while ((nsEditProperty::u == aHTMLProperty || nsEditProperty::strike == aHTMLProperty) &&
           !aIsSet && node);
  return NS_OK;
}

nsresult
nsHTMLCSSUtils::SetCSSEnabled(PRBool aIsCSSPrefChecked)
{
  mIsCSSPrefChecked = aIsCSSPrefChecked;
  return NS_OK;
}

PRBool
nsHTMLCSSUtils::IsCSSPrefChecked()
{
  return mIsCSSPrefChecked ;
}




PRBool
nsHTMLCSSUtils::ElementsSameStyle(nsIDOMNode *aFirstNode, nsIDOMNode *aSecondNode)
{
  nsresult res;
  nsCOMPtr<nsIDOMElement> firstElement  = do_QueryInterface(aFirstNode);
  nsCOMPtr<nsIDOMElement> secondElement = do_QueryInterface(aSecondNode);

  NS_ASSERTION((firstElement && secondElement), "Non element nodes passed to ElementsSameStyle.");

  nsAutoString firstID, secondID;
  PRBool isFirstIDSet, isSecondIDSet;
  res = mHTMLEditor->GetAttributeValue(firstElement,  NS_LITERAL_STRING("id"), firstID,  &isFirstIDSet);
  res = mHTMLEditor->GetAttributeValue(secondElement, NS_LITERAL_STRING("id"), secondID, &isSecondIDSet);
  if (isFirstIDSet || isSecondIDSet) {
    
    
    return PR_FALSE;
  }

  nsAutoString firstClass, secondClass;
  PRBool isFirstClassSet, isSecondClassSet;
  res = mHTMLEditor->GetAttributeValue(firstElement,  NS_LITERAL_STRING("class"), firstClass,  &isFirstClassSet);
  res = mHTMLEditor->GetAttributeValue(secondElement, NS_LITERAL_STRING("class"), secondClass, &isSecondClassSet);
  if (isFirstClassSet && isSecondClassSet) {
    
    if (!firstClass.Equals(secondClass)) {
      
      
      
      
      
      
      return PR_FALSE;
    }
  }
  else if (isFirstClassSet || isSecondClassSet) {
    
    return PR_FALSE;
  }

  nsCOMPtr<nsIDOMCSSStyleDeclaration> firstCSSDecl, secondCSSDecl;
  PRUint32 firstLength, secondLength;
  res = GetInlineStyles(firstElement,  getter_AddRefs(firstCSSDecl),  &firstLength);
  if (NS_FAILED(res) || !firstCSSDecl) return PR_FALSE;
  res = GetInlineStyles(secondElement, getter_AddRefs(secondCSSDecl), &secondLength);
  if (NS_FAILED(res) || !secondCSSDecl) return PR_FALSE;

  if (firstLength != secondLength) {
    
    return PR_FALSE;
  }
  else if (0 == firstLength) {
    
    return PR_TRUE;
  }

  PRUint32 i;
  nsAutoString propertyNameString;
  nsAutoString firstValue, secondValue;
  for (i=0; i<firstLength; i++) {
    firstCSSDecl->Item(i, propertyNameString);
    firstCSSDecl->GetPropertyValue(propertyNameString, firstValue);
    secondCSSDecl->GetPropertyValue(propertyNameString, secondValue);
    if (!firstValue.Equals(secondValue)) {
      return PR_FALSE;
    }
  }
  for (i=0; i<secondLength; i++) {
    secondCSSDecl->Item(i, propertyNameString);
    secondCSSDecl->GetPropertyValue(propertyNameString, secondValue);
    firstCSSDecl->GetPropertyValue(propertyNameString, firstValue);
    if (!firstValue.Equals(secondValue)) {
      return PR_FALSE;
    }
  }

  return PR_TRUE;
}

nsresult
nsHTMLCSSUtils::GetInlineStyles(nsIDOMElement *aElement,
                                nsIDOMCSSStyleDeclaration **aCssDecl,
                                PRUint32 *aLength)
{
  NS_ENSURE_TRUE(aElement && aLength, NS_ERROR_NULL_POINTER);
  *aLength = 0;
  nsCOMPtr<nsIDOMElementCSSInlineStyle> inlineStyles = do_QueryInterface(aElement);
  NS_ENSURE_TRUE(inlineStyles, NS_ERROR_NULL_POINTER);
  nsresult res = inlineStyles->GetStyle(aCssDecl);
  if (NS_FAILED(res) || !aCssDecl) return NS_ERROR_NULL_POINTER;
  (*aCssDecl)->GetLength(aLength);
  return NS_OK;
}

nsresult
nsHTMLCSSUtils::GetElementContainerOrSelf(nsIDOMNode * aNode, nsIDOMElement ** aElement)
{
  NS_ENSURE_TRUE(aNode, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsIDOMNode> node=aNode, parentNode;
  PRUint16 type;
  nsresult res;
  res = node->GetNodeType(&type);
  NS_ENSURE_SUCCESS(res, res);

  if (nsIDOMNode::DOCUMENT_NODE == type) {
    return NS_ERROR_NULL_POINTER;
  }

  
  while (node && nsIDOMNode::ELEMENT_NODE != type) {
    parentNode = node;
    res = parentNode->GetParentNode(getter_AddRefs(node));
    NS_ENSURE_SUCCESS(res, res);
    if (node) {
      res = node->GetNodeType(&type);
      NS_ENSURE_SUCCESS(res, res);
    }
  }
  NS_ASSERTION(node, "we reached a null node ancestor !");
  NS_ENSURE_TRUE(node, NS_ERROR_NULL_POINTER);
  nsCOMPtr<nsIDOMElement> element = do_QueryInterface(node);
  (*aElement) = element;
  NS_IF_ADDREF(*aElement);
  return NS_OK;
}

nsresult
nsHTMLCSSUtils::SetCSSProperty(nsIDOMElement * aElement,
                               const nsAString & aProperty,
                               const nsAString & aValue)
{
  nsCOMPtr<nsIDOMCSSStyleDeclaration> cssDecl;
  PRUint32 length;
  nsresult res = GetInlineStyles(aElement, getter_AddRefs(cssDecl), &length);
  if (NS_FAILED(res) || !cssDecl) return res;

  return cssDecl->SetProperty(aProperty,
                              aValue,
                              EmptyString());
}

nsresult
nsHTMLCSSUtils::SetCSSPropertyPixels(nsIDOMElement * aElement,
                                     const nsAString & aProperty,
                                     PRInt32 aIntValue)
{
  nsAutoString s;
  s.AppendInt(aIntValue);
  return SetCSSProperty(aElement, aProperty, s + NS_LITERAL_STRING("px"));
}

nsresult
nsHTMLCSSUtils::RemoveCSSProperty(nsIDOMElement * aElement,
                                  const nsAString & aProperty)
{
  nsCOMPtr<nsIDOMCSSStyleDeclaration> cssDecl;
  PRUint32 length;
  nsresult res = GetInlineStyles(aElement, getter_AddRefs(cssDecl), &length);
  if (NS_FAILED(res) || !cssDecl) return res;

  nsAutoString returnString;
  return cssDecl->RemoveProperty(aProperty, returnString);
}

