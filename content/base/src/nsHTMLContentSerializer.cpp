













































#include "nsHTMLContentSerializer.h"

#include "nsIDOMElement.h"
#include "nsIDOMText.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsINameSpaceManager.h"
#include "nsString.h"
#include "nsUnicharUtils.h"
#include "nsXPIDLString.h"
#include "nsIServiceManager.h"
#include "nsIDocumentEncoder.h"
#include "nsGkAtoms.h"
#include "nsIURI.h"
#include "nsNetUtil.h"
#include "nsEscape.h"
#include "nsITextToSubURI.h"
#include "nsCRT.h"
#include "nsIParserService.h"
#include "nsContentUtils.h"
#include "nsLWBrkCIID.h"
#include "nsIScriptElement.h"
#include "nsAttrName.h"

static const PRInt32 kLongLineLen = 128;

nsresult NS_NewHTMLContentSerializer(nsIContentSerializer** aSerializer)
{
  nsHTMLContentSerializer* it = new nsHTMLContentSerializer();
  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return CallQueryInterface(it, aSerializer);
}

nsHTMLContentSerializer::nsHTMLContentSerializer()
{
    mIsHTMLSerializer = PR_TRUE;
}

nsHTMLContentSerializer::~nsHTMLContentSerializer()
{
}


NS_IMETHODIMP
nsHTMLContentSerializer::AppendDocumentStart(nsIDocument *aDocument,
                                             nsAString& aStr)
{
  return NS_OK;
}

void 
nsHTMLContentSerializer::SerializeHTMLAttributes(nsIContent* aContent,
                                                 nsIContent *aOriginalElement,
                                                 nsAString& aTagPrefix,
                                                 const nsAString& aTagNamespaceURI,
                                                 nsIAtom* aTagName,
                                                 nsAString& aStr)
{
  PRInt32 count = aContent->GetAttrCount();
  if (!count)
    return;

  nsresult rv;
  nsAutoString valueStr;
  NS_NAMED_LITERAL_STRING(_mozStr, "_moz");

  for (PRInt32 index = count; index > 0;) {
    --index;
    const nsAttrName* name = aContent->GetAttrNameAt(index);
    PRInt32 namespaceID = name->NamespaceID();
    nsIAtom* attrName = name->LocalName();

    
    nsDependentAtomString attrNameStr(attrName);
    if (StringBeginsWith(attrNameStr, NS_LITERAL_STRING("_moz")) ||
        StringBeginsWith(attrNameStr, NS_LITERAL_STRING("-moz"))) {
      continue;
    }
    aContent->GetAttr(namespaceID, attrName, valueStr);

    
    
    
    
    if (aTagName == nsGkAtoms::br && attrName == nsGkAtoms::type &&
        StringBeginsWith(valueStr, _mozStr)) {
      continue;
    }

    if (mIsCopying && mIsFirstChildOfOL && (aTagName == nsGkAtoms::li) && 
        (attrName == nsGkAtoms::value)){
      
      continue;
    }
    PRBool isJS = IsJavaScript(aContent, attrName, namespaceID, valueStr);
    
    if (((attrName == nsGkAtoms::href) || 
         (attrName == nsGkAtoms::src))) {
      
      if (mFlags & nsIDocumentEncoder::OutputAbsoluteLinks) {
        
        
        
        
        nsCOMPtr<nsIURI> uri = aContent->GetBaseURI();
        if (uri) {
          nsAutoString absURI;
          rv = NS_MakeAbsoluteURI(absURI, valueStr, uri);
          if (NS_SUCCEEDED(rv)) {
            valueStr = absURI;
          }
        }
      }
      
      nsAutoString tempURI(valueStr);
      if (!isJS && NS_FAILED(EscapeURI(aContent, tempURI, valueStr)))
        valueStr = tempURI;
    }

    if (mRewriteEncodingDeclaration && aTagName == nsGkAtoms::meta &&
        attrName == nsGkAtoms::content) {
      
      
      nsAutoString header;
      aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::httpEquiv, header);
      if (header.LowerCaseEqualsLiteral("content-type")) {
        valueStr = NS_LITERAL_STRING("text/html; charset=") +
          NS_ConvertASCIItoUTF16(mCharset);
      }
    }

    nsDependentAtomString nameStr(attrName);

    
    if (IsShorthandAttr(attrName, aTagName) && valueStr.IsEmpty()) {
      valueStr = nameStr;
    }
    SerializeAttr(EmptyString(), nameStr, valueStr, aStr, !isJS);
  }
}

NS_IMETHODIMP
nsHTMLContentSerializer::AppendElementStart(nsIContent *aElement,
                                            nsIContent *aOriginalElement,
                                            nsAString& aStr)
{
  NS_ENSURE_ARG(aElement);

  nsIContent* content = aElement;

  PRBool forceFormat = PR_FALSE;
  if (!CheckElementStart(content, forceFormat, aStr)) {
    return NS_OK;
  }

  nsIAtom *name = content->Tag();
  PRBool lineBreakBeforeOpen = LineBreakBeforeOpen(content->GetNameSpaceID(), name);

  if ((mDoFormat || forceFormat) && !mPreLevel && !mDoRaw) {
    if (mColPos && lineBreakBeforeOpen) {
      AppendNewLineToString(aStr);
    }
    else {
      MaybeAddNewlineForRootNode(aStr);
    }
    if (!mColPos) {
      AppendIndentation(aStr);
    }
    else if (mAddSpace) {
      AppendToString(PRUnichar(' '), aStr);
      mAddSpace = PR_FALSE;
    }
  }
  else if (mAddSpace) {
    AppendToString(PRUnichar(' '), aStr);
    mAddSpace = PR_FALSE;
  }
  else {
    MaybeAddNewlineForRootNode(aStr);
  }
  
  
  mAddNewlineForRootNode = PR_FALSE;
  
  AppendToString(kLessThan, aStr);

  AppendToString(nsDependentAtomString(name), aStr);

  MaybeEnterInPreContent(content);

  
  if ((mDoFormat || forceFormat) && !mPreLevel && !mDoRaw)
    IncrIndentation(name);

  
  
  if (mIsCopying && name == nsGkAtoms::ol){
    
    
    nsAutoString start;
    PRInt32 startAttrVal = 0;

    aElement->GetAttr(kNameSpaceID_None, nsGkAtoms::start, start);
    if (!start.IsEmpty()){
      PRInt32 rv = 0;
      startAttrVal = start.ToInteger(&rv);
      
      
      
      if (NS_SUCCEEDED(rv))
        startAttrVal--; 
      else
        startAttrVal = 0;
    }
    mOLStateStack.AppendElement(olState(startAttrVal, PR_TRUE));
  }

  if (mIsCopying && name == nsGkAtoms::li) {
    mIsFirstChildOfOL = IsFirstChildOfOL(aOriginalElement);
    if (mIsFirstChildOfOL){
      
      SerializeLIValueAttribute(aElement, aStr);
    }
  }

  
  
  nsAutoString dummyPrefix;
  SerializeHTMLAttributes(content, aOriginalElement, dummyPrefix, EmptyString(), name, aStr);

  AppendToString(kGreaterThan, aStr);

  if (name == nsGkAtoms::script ||
      name == nsGkAtoms::style ||
      name == nsGkAtoms::noscript ||
      name == nsGkAtoms::noframes) {
    ++mDisableEntityEncoding;
  }

  if ((mDoFormat || forceFormat) && !mPreLevel &&
    !mDoRaw && LineBreakAfterOpen(content->GetNameSpaceID(), name)) {
    AppendNewLineToString(aStr);
  }

  AfterElementStart(content, aOriginalElement, aStr);

  return NS_OK;
}
  
NS_IMETHODIMP 
nsHTMLContentSerializer::AppendElementEnd(nsIContent *aElement,
                                          nsAString& aStr)
{
  NS_ENSURE_ARG(aElement);

  nsIContent* content = aElement;

  nsIAtom *name = content->Tag();

  if (name == nsGkAtoms::script ||
      name == nsGkAtoms::style ||
      name == nsGkAtoms::noscript ||
      name == nsGkAtoms::noframes) {
    --mDisableEntityEncoding;
  }

  PRBool forceFormat = content->HasAttr(kNameSpaceID_None,
                                        nsGkAtoms::mozdirty);

  if ((mDoFormat || forceFormat) && !mPreLevel && !mDoRaw) {
    DecrIndentation(name);
  }

  if (name == nsGkAtoms::script) {
    nsCOMPtr<nsIScriptElement> script = do_QueryInterface(aElement);

    if (script && script->IsMalformed()) {
      
      
      
      --mPreLevel;
      return NS_OK;
    }
  }
  else if (mIsCopying && name == nsGkAtoms::ol) {
    NS_ASSERTION((!mOLStateStack.IsEmpty()), "Cannot have an empty OL Stack");
    

    if (!mOLStateStack.IsEmpty()) {
      mOLStateStack.RemoveElementAt(mOLStateStack.Length() -1);
    }
  }
  
  nsIParserService* parserService = nsContentUtils::GetParserService();

  if (parserService) {
    PRBool isContainer;

    parserService->
      IsContainer(parserService->HTMLCaseSensitiveAtomTagToId(name),
                  isContainer);
    if (!isContainer)
      return NS_OK;
  }

  if ((mDoFormat || forceFormat) && !mPreLevel && !mDoRaw) {

    PRBool lineBreakBeforeClose = LineBreakBeforeClose(content->GetNameSpaceID(), name);

    if (mColPos && lineBreakBeforeClose) {
      AppendNewLineToString(aStr);
    }
    if (!mColPos) {
      AppendIndentation(aStr);
    }
    else if (mAddSpace) {
      AppendToString(PRUnichar(' '), aStr);
      mAddSpace = PR_FALSE;
    }
  }
  else if (mAddSpace) {
    AppendToString(PRUnichar(' '), aStr);
    mAddSpace = PR_FALSE;
  }

  AppendToString(kEndTag, aStr);
  AppendToString(nsDependentAtomString(name), aStr);
  AppendToString(kGreaterThan, aStr);

  MaybeLeaveFromPreContent(content);

  if ((mDoFormat || forceFormat) && !mPreLevel
      && !mDoRaw && LineBreakAfterClose(content->GetNameSpaceID(), name)) {
    AppendNewLineToString(aStr);
  }
  else {
    MaybeFlagNewlineForRootNode(aElement);
  }

  if (name == nsGkAtoms::body) {
    --mInBody;
  }

  return NS_OK;
}

static const PRUint16 kValNBSP = 160;
static const char kEntityNBSP[] = "&nbsp;";

static const PRUint16 kGTVal = 62;
static const char* kEntities[] = {
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "&amp;", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "&lt;", "", "&gt;"
};

static const char* kAttrEntities[] = {
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "&quot;", "", "", "", "&amp;", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "&lt;", "", "&gt;"
};

void
nsHTMLContentSerializer::AppendAndTranslateEntities(const nsAString& aStr,
                                                     nsAString& aOutputStr)
{
  if (mBodyOnly && !mInBody) {
    return;
  }

  if (mDisableEntityEncoding) {
    aOutputStr.Append(aStr);
    return;
  }

  if (mFlags & (nsIDocumentEncoder::OutputEncodeBasicEntities  |
                nsIDocumentEncoder::OutputEncodeLatin1Entities |
                nsIDocumentEncoder::OutputEncodeHTMLEntities   |
                nsIDocumentEncoder::OutputEncodeW3CEntities)) {
    nsIParserService* parserService = nsContentUtils::GetParserService();

    if (!parserService) {
      NS_ERROR("Can't get parser service");
      return;
    }

    nsReadingIterator<PRUnichar> done_reading;
    aStr.EndReading(done_reading);

    
    PRUint32 advanceLength = 0;
    nsReadingIterator<PRUnichar> iter;

    const char **entityTable = mInAttribute ? kAttrEntities : kEntities;
    nsCAutoString entityReplacement;

    for (aStr.BeginReading(iter);
         iter != done_reading;
         iter.advance(PRInt32(advanceLength))) {
      PRUint32 fragmentLength = iter.size_forward();
      PRUint32 lengthReplaced = 0; 
                                    
      const PRUnichar* c = iter.get();
      const PRUnichar* fragmentStart = c;
      const PRUnichar* fragmentEnd = c + fragmentLength;
      const char* entityText = nsnull;
      const char* fullConstEntityText = nsnull;
      char* fullEntityText = nsnull;

      advanceLength = 0;
      
      
      for (; c < fragmentEnd; c++, advanceLength++) {
        PRUnichar val = *c;
        if (val == kValNBSP) {
          fullConstEntityText = kEntityNBSP;
          break;
        }
        else if ((val <= kGTVal) && (entityTable[val][0] != 0)) {
          fullConstEntityText = entityTable[val];
          break;
        } else if (val > 127 &&
                  ((val < 256 &&
                    mFlags & nsIDocumentEncoder::OutputEncodeLatin1Entities) ||
                    mFlags & nsIDocumentEncoder::OutputEncodeHTMLEntities)) {
          entityReplacement.Truncate();
          parserService->HTMLConvertUnicodeToEntity(val, entityReplacement);

          if (!entityReplacement.IsEmpty()) {
            entityText = entityReplacement.get();
            break;
          }
        }
        else if (val > 127 &&
                  mFlags & nsIDocumentEncoder::OutputEncodeW3CEntities &&
                  mEntityConverter) {
          if (NS_IS_HIGH_SURROGATE(val) &&
              c + 1 < fragmentEnd &&
              NS_IS_LOW_SURROGATE(*(c + 1))) {
            PRUint32 valUTF32 = SURROGATE_TO_UCS4(val, *(++c));
            if (NS_SUCCEEDED(mEntityConverter->ConvertUTF32ToEntity(valUTF32,
                              nsIEntityConverter::entityW3C, &fullEntityText))) {
              lengthReplaced = 2;
              break;
            }
            else {
              advanceLength++;
            }
          }
          else if (NS_SUCCEEDED(mEntityConverter->ConvertToEntity(val,
                                nsIEntityConverter::entityW3C, 
                                &fullEntityText))) {
            lengthReplaced = 1;
            break;
          }
        }
      }

      aOutputStr.Append(fragmentStart, advanceLength);
      if (entityText) {
        aOutputStr.Append(PRUnichar('&'));
        AppendASCIItoUTF16(entityText, aOutputStr);
        aOutputStr.Append(PRUnichar(';'));
        advanceLength++;
      }
      else if (fullConstEntityText) {
        aOutputStr.AppendASCII(fullConstEntityText);
        ++advanceLength;
      }
      
      else if (fullEntityText) {
        AppendASCIItoUTF16(fullEntityText, aOutputStr);
        nsMemory::Free(fullEntityText);
        advanceLength += lengthReplaced;
      }
    }
  } else {
    nsXMLContentSerializer::AppendAndTranslateEntities(aStr, aOutputStr);
  }
}
