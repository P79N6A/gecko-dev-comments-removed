













































#include "nsXHTMLContentSerializer.h"

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

static const char kMozStr[] = "moz";

static const PRInt32 kLongLineLen = 128;

#define kXMLNS "xmlns"

nsresult NS_NewXHTMLContentSerializer(nsIContentSerializer** aSerializer)
{
  nsXHTMLContentSerializer* it = new nsXHTMLContentSerializer();
  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return CallQueryInterface(it, aSerializer);
}

nsXHTMLContentSerializer::nsXHTMLContentSerializer()
  : mInBody(0),
    mIsHTMLSerializer(PR_FALSE)
{
}

nsXHTMLContentSerializer::~nsXHTMLContentSerializer()
{
  NS_ASSERTION(mOLStateStack.IsEmpty(), "Expected OL State stack to be empty");
}

NS_IMETHODIMP
nsXHTMLContentSerializer::Init(PRUint32 aFlags, PRUint32 aWrapColumn,
                              const char* aCharSet, PRBool aIsCopying,
                              PRBool aIsWholeDocument)
{
  
  
  
  
  if (aFlags & nsIDocumentEncoder::OutputFormatted ) {
      aFlags = aFlags | nsIDocumentEncoder::OutputWrap;
  }
  else if (!(aFlags & nsIDocumentEncoder::OutputRaw)) {
      aFlags = aFlags | nsIDocumentEncoder::OutputWrap;
  }

  nsresult rv;
  rv = nsXMLContentSerializer::Init(aFlags, aWrapColumn, aCharSet, aIsCopying, aIsWholeDocument);
  NS_ENSURE_SUCCESS(rv, rv);

  mIsWholeDocument = aIsWholeDocument;
  mIsCopying = aIsCopying;
  mIsFirstChildOfOL = PR_FALSE;
  mInBody = 0;
  mDisableEntityEncoding = 0;
  mBodyOnly = (mFlags & nsIDocumentEncoder::OutputBodyOnly) ? PR_TRUE
                                                            : PR_FALSE;

  
  if (mFlags & nsIDocumentEncoder::OutputEncodeW3CEntities) {
    mEntityConverter = do_CreateInstance(NS_ENTITYCONVERTER_CONTRACTID);
  }
  return NS_OK;
}





PRBool
nsXHTMLContentSerializer::HasLongLines(const nsString& text, PRInt32& aLastNewlineOffset)
{
  PRUint32 start=0;
  PRUint32 theLen = text.Length();
  PRBool rv = PR_FALSE;
  aLastNewlineOffset = kNotFound;
  for (start = 0; start < theLen; ) {
    PRInt32 eol = text.FindChar('\n', start);
    if (eol < 0) {
      eol = text.Length();
    }
    else {
      aLastNewlineOffset = eol;
    }
    if (PRInt32(eol - start) > kLongLineLen)
      rv = PR_TRUE;
    start = eol + 1;
  }
  return rv;
}

NS_IMETHODIMP
nsXHTMLContentSerializer::AppendText(nsIDOMText* aText,
                                     PRInt32 aStartOffset,
                                     PRInt32 aEndOffset,
                                     nsAString& aStr)
{
  NS_ENSURE_ARG(aText);

  nsAutoString data;
  nsresult rv;

  rv = AppendTextData(aText, aStartOffset, aEndOffset, data, PR_TRUE);
  if (NS_FAILED(rv))
    return NS_ERROR_FAILURE;

  if (mPreLevel > 0 || mDoRaw) {
    AppendToStringConvertLF(data, aStr);
  }
  else if (mDoFormat) {
    AppendToStringFormatedWrapped(data, aStr);
  }
  else if (mDoWrap) {
    AppendToStringWrapped(data, aStr);
  }
  else {
    PRInt32 lastNewlineOffset = kNotFound;
    if (HasLongLines(data, lastNewlineOffset)) {
      
      AppendToStringWrapped(data, aStr);
    }
    else {
      AppendToStringConvertLF(data, aStr);
    }
  }

  return NS_OK;
}

nsresult
nsXHTMLContentSerializer::EscapeURI(nsIContent* aContent, const nsAString& aURI, nsAString& aEscapedURI)
{
  
  
  if (IsJavaScript(aContent, nsGkAtoms::href, kNameSpaceID_None, aURI)) {
    aEscapedURI = aURI;
    return NS_OK;
  }

  
  
  
  
  nsCOMPtr<nsITextToSubURI> textToSubURI;
  nsAutoString uri(aURI); 
  nsresult rv = NS_OK;

  if (!mCharset.IsEmpty() && !IsASCII(uri)) {
    textToSubURI = do_GetService(NS_ITEXTTOSUBURI_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  PRInt32 start = 0;
  PRInt32 end;
  nsAutoString part;
  nsXPIDLCString escapedURI;
  aEscapedURI.Truncate(0);

  
  
  while ((end = uri.FindCharInSet("%#;/?:@&=+$,[]", start)) != -1) {
    part = Substring(aURI, start, (end-start));
    if (textToSubURI && !IsASCII(part)) {
      rv = textToSubURI->ConvertAndEscape(mCharset.get(), part.get(), getter_Copies(escapedURI));
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      escapedURI.Adopt(nsEscape(NS_ConvertUTF16toUTF8(part).get(), url_Path));
    }
    AppendASCIItoUTF16(escapedURI, aEscapedURI);

    
    part = Substring(aURI, end, 1);
    aEscapedURI.Append(part);
    start = end + 1;
  }

  if (start < (PRInt32) aURI.Length()) {
    
    part = Substring(aURI, start, aURI.Length()-start);
    if (textToSubURI) {
      rv = textToSubURI->ConvertAndEscape(mCharset.get(), part.get(), getter_Copies(escapedURI));
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      escapedURI.Adopt(nsEscape(NS_ConvertUTF16toUTF8(part).get(), url_Path));
    }
    AppendASCIItoUTF16(escapedURI, aEscapedURI);
  }

  return rv;
}

void
nsXHTMLContentSerializer::SerializeAttributes(nsIContent* aContent,
                                              nsIDOMElement *aOriginalElement,
                                              nsAString& aTagPrefix,
                                              const nsAString& aTagNamespaceURI,
                                              nsIAtom* aTagName,
                                              nsAString& aStr,
                                              PRUint32 aSkipAttr,
                                              PRBool aAddNSAttr)
{
  nsresult rv;
  PRUint32 index, count;
  nsAutoString nameStr, prefixStr, uriStr, valueStr;
  nsAutoString xmlnsStr;
  xmlnsStr.AssignLiteral(kXMLNS);

  PRInt32 contentNamespaceID = aContent->GetNameSpaceID();

  
  

  if (mIsCopying && kNameSpaceID_XHTML == contentNamespaceID) {

    
    
    if (aTagName == nsGkAtoms::ol) {
      
      
      nsAutoString start;
      PRInt32 startAttrVal = 0;
      aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::start, start);
      if (!start.IsEmpty()) {
        PRInt32 rv = 0;
        startAttrVal = start.ToInteger(&rv);
        
        
        
        if (NS_SUCCEEDED(rv))
          --startAttrVal;
        else
          startAttrVal = 0;
      }
      olState state (startAttrVal, PR_TRUE);
      mOLStateStack.AppendElement(state);
    }
    else if (aTagName == nsGkAtoms::li) {
      mIsFirstChildOfOL = IsFirstChildOfOL(aOriginalElement);
      if (mIsFirstChildOfOL) {
        nsCOMPtr<nsIDOMElement> element (do_QueryInterface(aContent));
        
        SerializeLIValueAttribute(element, aStr);
      }
    }
  }

  
  
  if (aAddNSAttr) {
    if (aTagPrefix.IsEmpty()) {
      
      SerializeAttr(EmptyString(), xmlnsStr, aTagNamespaceURI, aStr, PR_TRUE);
    } else {
      
      SerializeAttr(xmlnsStr, aTagPrefix, aTagNamespaceURI, aStr, PR_TRUE);
    }
    PushNameSpaceDecl(aTagPrefix, aTagNamespaceURI, aOriginalElement);
  }

  NS_NAMED_LITERAL_STRING(_mozStr, "_moz");

  count = aContent->GetAttrCount();

  
  
  
  for (index = 0; index < count; index++) {

    if (aSkipAttr == index) {
        continue;
    }

    const nsAttrName* name = aContent->GetAttrNameAt(index);
    PRInt32 namespaceID = name->NamespaceID();
    nsIAtom* attrName = name->LocalName();
    nsIAtom* attrPrefix = name->GetPrefix();

    
    const char* sharedName;
    attrName->GetUTF8String(&sharedName);
    if ((('_' == *sharedName) || ('-' == *sharedName)) &&
        !nsCRT::strncmp(sharedName+1, kMozStr, PRUint32(sizeof(kMozStr)-1))) {
      continue;
    }

    if (attrPrefix) {
      attrPrefix->ToString(prefixStr);
    }
    else {
      prefixStr.Truncate();
    }

    PRBool addNSAttr = PR_FALSE;
    if (kNameSpaceID_XMLNS != namespaceID) {
      nsContentUtils::NameSpaceManager()->GetNameSpaceURI(namespaceID, uriStr);
      addNSAttr = ConfirmPrefix(prefixStr, uriStr, aOriginalElement, PR_TRUE);
    }

    aContent->GetAttr(namespaceID, attrName, valueStr);

    attrName->ToString(nameStr);

    
    
    
    if (!nameStr.IsEmpty() && nameStr.First() == '-')
      continue;

    PRBool isJS = PR_FALSE;

    if (kNameSpaceID_XHTML == contentNamespaceID) {
      
      
      
      
      if (namespaceID == kNameSpaceID_None && aTagName == nsGkAtoms::br && attrName == nsGkAtoms::type
          && StringBeginsWith(valueStr, _mozStr)) {
        continue;
      }

      if (mIsCopying && mIsFirstChildOfOL && (aTagName == nsGkAtoms::li)
          && (attrName == nsGkAtoms::value)) {
        
        continue;
      }

      isJS = IsJavaScript(aContent, attrName, namespaceID, valueStr);

      if (namespaceID == kNameSpaceID_None && 
          ((attrName == nsGkAtoms::href) ||
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

      if (mIsWholeDocument && aTagName == nsGkAtoms::meta &&
          attrName == nsGkAtoms::content) {
        
        
        nsAutoString header;
        aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::httpEquiv, header);
        if (header.LowerCaseEqualsLiteral("content-type")) {
          valueStr = NS_LITERAL_STRING("text/html; charset=") +
            NS_ConvertASCIItoUTF16(mCharset);
        }
      }

      
      if (namespaceID == kNameSpaceID_None && IsShorthandAttr(attrName, aTagName) && valueStr.IsEmpty()) {
        valueStr = nameStr;
      }
    }
    else {
      isJS = IsJavaScript(aContent, attrName, namespaceID, valueStr);
    }

    SerializeAttr(prefixStr, nameStr, valueStr, aStr, !isJS);

    if (addNSAttr) {
      NS_ASSERTION(!prefixStr.IsEmpty(),
                   "Namespaced attributes must have a prefix");
      SerializeAttr(xmlnsStr, prefixStr, uriStr, aStr, PR_TRUE);
      PushNameSpaceDecl(prefixStr, uriStr, aOriginalElement);
    }
  }
}


void 
nsXHTMLContentSerializer::AppendEndOfElementStart(nsIDOMElement *aOriginalElement,
                                                  nsIAtom * aName,
                                                  PRInt32 aNamespaceID,
                                                  nsAString& aStr)
{
  
  
  NS_ASSERTION(!mIsHTMLSerializer, "nsHTMLContentSerializer shouldn't call this method !");

  if (kNameSpaceID_XHTML != aNamespaceID) {
    nsXMLContentSerializer::AppendEndOfElementStart(aOriginalElement, aName,
                                                    aNamespaceID, aStr);
    return;
  }

  nsIParserService* parserService = nsContentUtils::GetParserService();

  if (parserService) {
    PRBool isContainer;
    parserService->IsContainer(parserService->HTMLAtomTagToId(aName),
                             isContainer);
    if (!isContainer) {
      
      
      
      AppendToString(NS_LITERAL_STRING(" />"), aStr);
      return;
    }
  }
  AppendToString(kGreaterThan, aStr);
}

void
nsXHTMLContentSerializer::AfterElementStart(nsIContent * aContent,
                                            nsIDOMElement *aOriginalElement,
                                            nsAString& aStr)
{
  nsIAtom *name = aContent->Tag();
  if (aContent->GetNameSpaceID() == kNameSpaceID_XHTML &&
      mIsWholeDocument &&
      name == nsGkAtoms::head) {

    
    
    
    PRBool hasMeta = PR_FALSE;
    PRUint32 i, childCount = aContent->GetChildCount();
    for (i = 0; i < childCount; ++i) {
      nsIContent* child = aContent->GetChildAt(i);
      if (child->IsNodeOfType(nsINode::eHTML) &&
          child->Tag() == nsGkAtoms::meta &&
          child->HasAttr(kNameSpaceID_None, nsGkAtoms::content)) {
        nsAutoString header;
        child->GetAttr(kNameSpaceID_None, nsGkAtoms::httpEquiv, header);

        if (header.LowerCaseEqualsLiteral("content-type")) {
          hasMeta = PR_TRUE;
          break;
        }
      }
    }

    if (!hasMeta) {
      AppendNewLineToString(aStr);
      if (mDoFormat) {
        AppendIndentation(aStr);
      }
      AppendToString(NS_LITERAL_STRING("<meta http-equiv=\"content-type\""),
                    aStr);
      AppendToString(NS_LITERAL_STRING(" content=\"text/html; charset="), aStr);
      AppendToString(NS_ConvertASCIItoUTF16(mCharset), aStr);
      if (mIsHTMLSerializer)
        AppendToString(NS_LITERAL_STRING("\">"), aStr);
      else
        AppendToString(NS_LITERAL_STRING("\" />"), aStr);
    }
  }
}

void
nsXHTMLContentSerializer::AfterElementEnd(nsIContent * aContent,
                                          nsAString& aStr)
{
  NS_ASSERTION(!mIsHTMLSerializer, "nsHTMLContentSerializer shouldn't call this method !");

  PRInt32 namespaceID = aContent->GetNameSpaceID();
  nsIAtom *name = aContent->Tag();

  
  
  if (kNameSpaceID_XHTML == namespaceID && name == nsGkAtoms::body) {
    --mInBody;
  }
}


NS_IMETHODIMP
nsXHTMLContentSerializer::AppendDocumentStart(nsIDOMDocument *aDocument,
                                              nsAString& aStr)
{
  if (!mBodyOnly)
    return nsXMLContentSerializer::AppendDocumentStart(aDocument, aStr);

  return NS_OK;
}

PRBool
nsXHTMLContentSerializer::CheckElementStart(nsIContent * aContent,
                                            PRBool & aForceFormat,
                                            nsAString& aStr)
{
  
  
  
  aForceFormat = aContent->HasAttr(kNameSpaceID_None,
                                   nsGkAtoms::mozdirty);

  nsIAtom *name = aContent->Tag();
  PRInt32 namespaceID = aContent->GetNameSpaceID();

  if (namespaceID == kNameSpaceID_XHTML) {
    if (name == nsGkAtoms::br && mPreLevel > 0 && 
        (mFlags & nsIDocumentEncoder::OutputNoFormattingInPre)) {
      AppendNewLineToString(aStr);
      return PR_FALSE;
    }

    if (name == nsGkAtoms::body) {
      ++mInBody;
    }
  }
  return PR_TRUE;
}

PRBool
nsXHTMLContentSerializer::CheckElementEnd(nsIContent * aContent,
                                          PRBool & aForceFormat,
                                          nsAString& aStr)
{
  NS_ASSERTION(!mIsHTMLSerializer, "nsHTMLContentSerializer shouldn't call this method !");

  aForceFormat = aContent->HasAttr(kNameSpaceID_None,
                                   nsGkAtoms::mozdirty);

  nsIAtom *name = aContent->Tag();
  PRInt32 namespaceID = aContent->GetNameSpaceID();

  
  
  if (namespaceID == kNameSpaceID_XHTML) {
    if (mIsCopying && name == nsGkAtoms::ol) {
      NS_ASSERTION((!mOLStateStack.IsEmpty()), "Cannot have an empty OL Stack");
      

      if (!mOLStateStack.IsEmpty()) {
        mOLStateStack.RemoveElementAt(mOLStateStack.Length() -1);
      }
    }

    nsIParserService* parserService = nsContentUtils::GetParserService();

    if (parserService) {
      PRBool isContainer;

      parserService->IsContainer(parserService->HTMLAtomTagToId(name),
                                 isContainer);
      if (!isContainer) {
        
        
        return PR_FALSE;
      }
    }
    
    
    
    return PR_TRUE;
  }

  PRBool dummyFormat;
  return nsXMLContentSerializer::CheckElementEnd(aContent, dummyFormat, aStr);
}

void
nsXHTMLContentSerializer::AppendToString(const PRUnichar* aStr,
                                         PRInt32 aLength,
                                         nsAString& aOutputStr)
{
  if (mBodyOnly && !mInBody) {
    return;
  }
  nsXMLContentSerializer::AppendToString(aStr, aLength, aOutputStr);
}

void 
nsXHTMLContentSerializer::AppendToString(const PRUnichar aChar,
                                         nsAString& aOutputStr)
{
  if (mBodyOnly && !mInBody) {
    return;
  }
  nsXMLContentSerializer::AppendToString(aChar, aOutputStr);
}

void
nsXHTMLContentSerializer::AppendToString(const nsAString& aStr,
                                         nsAString& aOutputStr)
{
  if (mBodyOnly && !mInBody) {
    return;
  }
  nsXMLContentSerializer::AppendToString(aStr, aOutputStr);
}


static const PRUint16 kValNBSP = 160;
static const char kEntityNBSP[] = "nbsp";

static const PRUint16 kGTVal = 62;
static const char* kEntities[] = {
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "amp", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "lt", "", "gt"
};

static const char* kAttrEntities[] = {
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "quot", "", "", "", "amp", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "lt", "", "gt"
};

void
nsXHTMLContentSerializer::AppendAndTranslateEntities(const nsAString& aStr,
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

    for (aStr.BeginReading(iter);
          iter != done_reading;
          iter.advance(PRInt32(advanceLength))) {
      PRUint32 fragmentLength = iter.size_forward();
      PRUint32 lengthReplaced = 0; 
                                    
      const PRUnichar* c = iter.get();
      const PRUnichar* fragmentStart = c;
      const PRUnichar* fragmentEnd = c + fragmentLength;
      const char* entityText = nsnull;
      nsCAutoString entityReplacement;
      char* fullEntityText = nsnull;

      advanceLength = 0;
      
      
      for (; c < fragmentEnd; c++, advanceLength++) {
        PRUnichar val = *c;
        if (val == kValNBSP) {
          entityText = kEntityNBSP;
          break;
        }
        else if ((val <= kGTVal) && (entityTable[val][0] != 0)) {
          entityText = entityTable[val];
          break;
        } else if (val > 127 &&
                  ((val < 256 &&
                    mFlags & nsIDocumentEncoder::OutputEncodeLatin1Entities) ||
                    mFlags & nsIDocumentEncoder::OutputEncodeHTMLEntities)) {
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

PRBool
nsXHTMLContentSerializer::IsShorthandAttr(const nsIAtom* aAttrName,
                                          const nsIAtom* aElementName)
{
  
  if ((aAttrName == nsGkAtoms::checked) &&
      (aElementName == nsGkAtoms::input)) {
    return PR_TRUE;
  }

  
  if ((aAttrName == nsGkAtoms::compact) &&
      (aElementName == nsGkAtoms::dir || 
       aElementName == nsGkAtoms::dl ||
       aElementName == nsGkAtoms::menu ||
       aElementName == nsGkAtoms::ol ||
       aElementName == nsGkAtoms::ul)) {
    return PR_TRUE;
  }

  
  if ((aAttrName == nsGkAtoms::declare) &&
      (aElementName == nsGkAtoms::object)) {
    return PR_TRUE;
  }

  
  if ((aAttrName == nsGkAtoms::defer) &&
      (aElementName == nsGkAtoms::script)) {
    return PR_TRUE;
  }

  
  if ((aAttrName == nsGkAtoms::disabled) &&
      (aElementName == nsGkAtoms::button ||
       aElementName == nsGkAtoms::input ||
       aElementName == nsGkAtoms::optgroup ||
       aElementName == nsGkAtoms::option ||
       aElementName == nsGkAtoms::select ||
       aElementName == nsGkAtoms::textarea)) {
    return PR_TRUE;
  }

  
  if ((aAttrName == nsGkAtoms::ismap) &&
      (aElementName == nsGkAtoms::img ||
       aElementName == nsGkAtoms::input)) {
    return PR_TRUE;
  }

  
  if ((aAttrName == nsGkAtoms::multiple) &&
      (aElementName == nsGkAtoms::select)) {
    return PR_TRUE;
  }

  
  if ((aAttrName == nsGkAtoms::noresize) &&
      (aElementName == nsGkAtoms::frame)) {
    return PR_TRUE;
  }

  
  if ((aAttrName == nsGkAtoms::noshade) &&
      (aElementName == nsGkAtoms::hr)) {
    return PR_TRUE;
  }

  
  if ((aAttrName == nsGkAtoms::nowrap) &&
      (aElementName == nsGkAtoms::td ||
       aElementName == nsGkAtoms::th)) {
    return PR_TRUE;
  }

  
  if ((aAttrName == nsGkAtoms::readonly) &&
      (aElementName == nsGkAtoms::input ||
       aElementName == nsGkAtoms::textarea)) {
    return PR_TRUE;
  }

  
  if ((aAttrName == nsGkAtoms::selected) &&
      (aElementName == nsGkAtoms::option)) {
    return PR_TRUE;
  }

#ifdef MOZ_MEDIA
  
  if ((aElementName == nsGkAtoms::video || aElementName == nsGkAtoms::audio) &&
    (aAttrName == nsGkAtoms::autoplay ||
     aAttrName == nsGkAtoms::controls)) {
    return PR_TRUE;
  }
#endif

  return PR_FALSE;
}

void
nsXHTMLContentSerializer::AppendToStringConvertLF(const nsAString& aStr,
                                                  nsAString& aOutputStr)
{
  if (mBodyOnly && !mInBody) {
    return;
  }
  nsXMLContentSerializer::AppendToStringConvertLF(aStr, aOutputStr);
}

void
nsXHTMLContentSerializer::AppendToStringFormatedWrapped(const nsASingleFragmentString& aStr,
                                                        nsAString& aOutputStr)
{
  if (mBodyOnly && !mInBody) {
    return;
  }
  nsXMLContentSerializer::AppendToStringFormatedWrapped(aStr, aOutputStr);
}

void
nsXHTMLContentSerializer::AppendToStringWrapped(const nsASingleFragmentString& aStr,
                                                nsAString& aOutputStr)
{
  if (mBodyOnly && !mInBody) {
    return;
  }
  nsXMLContentSerializer::AppendToStringWrapped(aStr, aOutputStr);
}


PRBool
nsXHTMLContentSerializer::LineBreakBeforeOpen(PRInt32 aNamespaceID, nsIAtom* aName)
{

  if (aNamespaceID != kNameSpaceID_XHTML) {
    return mAddSpace;
  }

  if (aName == nsGkAtoms::title ||
      aName == nsGkAtoms::meta  ||
      aName == nsGkAtoms::link  ||
      aName == nsGkAtoms::style ||
      aName == nsGkAtoms::select ||
      aName == nsGkAtoms::option ||
      aName == nsGkAtoms::script ||
      aName == nsGkAtoms::html) {
    return PR_TRUE;
  }
  else {
    nsIParserService* parserService = nsContentUtils::GetParserService();

    if (parserService) {
      PRBool res;
      parserService->IsBlock(parserService->HTMLAtomTagToId(aName), res);
      return res;
    }
  }

  return mAddSpace;
}

PRBool 
nsXHTMLContentSerializer::LineBreakAfterOpen(PRInt32 aNamespaceID, nsIAtom* aName)
{

  if (aNamespaceID != kNameSpaceID_XHTML) {
    return PR_FALSE;
  }

  if ((aName == nsGkAtoms::html) ||
      (aName == nsGkAtoms::head) ||
      (aName == nsGkAtoms::body) ||
      (aName == nsGkAtoms::ul) ||
      (aName == nsGkAtoms::ol) ||
      (aName == nsGkAtoms::dl) ||
      (aName == nsGkAtoms::table) ||
      (aName == nsGkAtoms::tbody) ||
      (aName == nsGkAtoms::tr) ||
      (aName == nsGkAtoms::br) ||
      (aName == nsGkAtoms::meta) ||
      (aName == nsGkAtoms::link) ||
      (aName == nsGkAtoms::script) ||
      (aName == nsGkAtoms::select) ||
      (aName == nsGkAtoms::map) ||
      (aName == nsGkAtoms::area) ||
      (aName == nsGkAtoms::style)) {
    return PR_TRUE;
  }

  return PR_FALSE;
}

PRBool 
nsXHTMLContentSerializer::LineBreakBeforeClose(PRInt32 aNamespaceID, nsIAtom* aName)
{

  if (aNamespaceID != kNameSpaceID_XHTML) {
    return PR_FALSE;
  }

  if ((aName == nsGkAtoms::html) ||
      (aName == nsGkAtoms::head) ||
      (aName == nsGkAtoms::body) ||
      (aName == nsGkAtoms::ul) ||
      (aName == nsGkAtoms::ol) ||
      (aName == nsGkAtoms::dl) ||
      (aName == nsGkAtoms::select) ||
      (aName == nsGkAtoms::table) ||
      (aName == nsGkAtoms::tbody)) {
    return PR_TRUE;
  }
  return PR_FALSE;
}

PRBool 
nsXHTMLContentSerializer::LineBreakAfterClose(PRInt32 aNamespaceID, nsIAtom* aName)
{

  if (aNamespaceID != kNameSpaceID_XHTML) {
    return PR_FALSE;
  }

  if ((aName == nsGkAtoms::html) ||
      (aName == nsGkAtoms::head) ||
      (aName == nsGkAtoms::body) ||
      (aName == nsGkAtoms::tr) ||
      (aName == nsGkAtoms::th) ||
      (aName == nsGkAtoms::td) ||
      (aName == nsGkAtoms::pre) ||
      (aName == nsGkAtoms::title) ||
      (aName == nsGkAtoms::li) ||
      (aName == nsGkAtoms::dt) ||
      (aName == nsGkAtoms::dd) ||
      (aName == nsGkAtoms::blockquote) ||
      (aName == nsGkAtoms::select) ||
      (aName == nsGkAtoms::option) ||
      (aName == nsGkAtoms::p) ||
      (aName == nsGkAtoms::map) ||
      (aName == nsGkAtoms::div)) {
    return PR_TRUE;
  }
  else {
    nsIParserService* parserService = nsContentUtils::GetParserService();

    if (parserService) {
      PRBool res;
      parserService->IsBlock(parserService->HTMLAtomTagToId(aName), res);
      return res;
    }
  }

  return PR_FALSE;
}


void
nsXHTMLContentSerializer::MaybeEnterInPreContent(nsIContent* aNode)
{

  if (aNode->GetNameSpaceID() != kNameSpaceID_XHTML) {
    return;
  }

  nsIAtom *name = aNode->Tag();

  if (name == nsGkAtoms::pre ||
      name == nsGkAtoms::script ||
      name == nsGkAtoms::style ||
      name == nsGkAtoms::noscript ||
      name == nsGkAtoms::noframes
      ) {
    mPreLevel++;
  }
}

void
nsXHTMLContentSerializer::MaybeLeaveFromPreContent(nsIContent* aNode)
{
  if (aNode->GetNameSpaceID() != kNameSpaceID_XHTML) {
    return;
  }

  nsIAtom *name = aNode->Tag();
  if (name == nsGkAtoms::pre ||
      name == nsGkAtoms::script ||
      name == nsGkAtoms::style ||
      name == nsGkAtoms::noscript ||
      name == nsGkAtoms::noframes
    ) {
    --mPreLevel;
  }
}

void 
nsXHTMLContentSerializer::SerializeLIValueAttribute(nsIDOMElement* aElement,
                                                    nsAString& aStr)
{
  
  
  
  PRBool found = PR_FALSE;
  nsCOMPtr<nsIDOMNode> currNode = do_QueryInterface(aElement);
  nsAutoString valueStr;

  olState state (0, PR_FALSE);

  if (!mOLStateStack.IsEmpty()) {
    state = mOLStateStack[mOLStateStack.Length()-1];
    
    
    state.isFirstListItem = PR_FALSE;
    mOLStateStack[mOLStateStack.Length()-1] = state;
  }

  PRInt32 startVal = state.startVal;
  PRInt32 offset = 0;

  
  
  while (currNode && !found) {
    nsCOMPtr<nsIDOMElement> currElement = do_QueryInterface(currNode);
    
    if (currElement) {
      nsAutoString tagName;
      currElement->GetTagName(tagName);
      if (tagName.LowerCaseEqualsLiteral("li")) {
        currElement->GetAttribute(NS_LITERAL_STRING("value"), valueStr);
        if (valueStr.IsEmpty())
          offset++;
        else {
          found = PR_TRUE;
          PRInt32 rv = 0;
          startVal = valueStr.ToInteger(&rv);
        }
      }
    }
    nsCOMPtr<nsIDOMNode> tmp;
    currNode->GetPreviousSibling(getter_AddRefs(tmp));
    currNode.swap(tmp);
  }
  
  
  if (offset == 0 && found) {
    
    
    SerializeAttr(EmptyString(), NS_LITERAL_STRING("value"), valueStr, aStr, PR_FALSE);
  }
  else if (offset == 1 && !found) {
    



    
  }
  else if (offset > 0) {
    
    nsAutoString valueStr;

    
    valueStr.AppendInt(startVal + offset);
    SerializeAttr(EmptyString(), NS_LITERAL_STRING("value"), valueStr, aStr, PR_FALSE);
  }
}

PRBool
nsXHTMLContentSerializer::IsFirstChildOfOL(nsIDOMElement* aElement)
{
  nsCOMPtr<nsIDOMNode> node = do_QueryInterface(aElement);
  nsAutoString parentName;

  nsCOMPtr<nsIDOMNode> parentNode;
  node->GetParentNode(getter_AddRefs(parentNode));
  if (parentNode)
    parentNode->GetNodeName(parentName);
  else
    return PR_FALSE;

  if (parentName.LowerCaseEqualsLiteral("ol")) {

    if (!mOLStateStack.IsEmpty()) {
      olState state = mOLStateStack[mOLStateStack.Length()-1];
      if (state.isFirstListItem)
        return PR_TRUE;
    }

    return PR_FALSE;
  }
  else
    return PR_FALSE;
}
