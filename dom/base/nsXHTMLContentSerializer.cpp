











#include "nsXHTMLContentSerializer.h"

#include "nsIDOMElement.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsNameSpaceManager.h"
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
#include "nsParserConstants.h"
#include "nsComputedDOMStyle.h"
#include "mozilla/dom/Element.h"

static const int32_t kLongLineLen = 128;

#define kXMLNS "xmlns"

nsresult
NS_NewXHTMLContentSerializer(nsIContentSerializer** aSerializer)
{
  nsRefPtr<nsXHTMLContentSerializer> it = new nsXHTMLContentSerializer();
  it.forget(aSerializer);
  return NS_OK;
}

nsXHTMLContentSerializer::nsXHTMLContentSerializer()
  : mIsHTMLSerializer(false)
{
}

nsXHTMLContentSerializer::~nsXHTMLContentSerializer()
{
  NS_ASSERTION(mOLStateStack.IsEmpty(), "Expected OL State stack to be empty");
}

NS_IMETHODIMP
nsXHTMLContentSerializer::Init(uint32_t aFlags, uint32_t aWrapColumn,
                              const char* aCharSet, bool aIsCopying,
                              bool aRewriteEncodingDeclaration)
{
  
  
  
  
  if (aFlags & nsIDocumentEncoder::OutputFormatted ) {
      aFlags = aFlags | nsIDocumentEncoder::OutputWrap;
  }

  nsresult rv;
  rv = nsXMLContentSerializer::Init(aFlags, aWrapColumn, aCharSet, aIsCopying, aRewriteEncodingDeclaration);
  NS_ENSURE_SUCCESS(rv, rv);

  mRewriteEncodingDeclaration = aRewriteEncodingDeclaration;
  mIsCopying = aIsCopying;
  mIsFirstChildOfOL = false;
  mInBody = 0;
  mDisableEntityEncoding = 0;
  mBodyOnly = (mFlags & nsIDocumentEncoder::OutputBodyOnly) ? true
                                                            : false;

  
  if (mFlags & nsIDocumentEncoder::OutputEncodeW3CEntities) {
    mEntityConverter = do_CreateInstance(NS_ENTITYCONVERTER_CONTRACTID);
  }
  return NS_OK;
}





bool
nsXHTMLContentSerializer::HasLongLines(const nsString& text, int32_t& aLastNewlineOffset)
{
  uint32_t start=0;
  uint32_t theLen = text.Length();
  bool rv = false;
  aLastNewlineOffset = kNotFound;
  for (start = 0; start < theLen; ) {
    int32_t eol = text.FindChar('\n', start);
    if (eol < 0) {
      eol = text.Length();
    }
    else {
      aLastNewlineOffset = eol;
    }
    if (int32_t(eol - start) > kLongLineLen)
      rv = true;
    start = eol + 1;
  }
  return rv;
}

NS_IMETHODIMP
nsXHTMLContentSerializer::AppendText(nsIContent* aText,
                                     int32_t aStartOffset,
                                     int32_t aEndOffset,
                                     nsAString& aStr)
{
  NS_ENSURE_ARG(aText);

  nsAutoString data;
  nsresult rv;

  rv = AppendTextData(aText, aStartOffset, aEndOffset, data, true);
  if (NS_FAILED(rv))
    return NS_ERROR_FAILURE;

  if (mDoRaw || PreLevel() > 0) {
    NS_ENSURE_TRUE(AppendToStringConvertLF(data, aStr), NS_ERROR_OUT_OF_MEMORY);
  }
  else if (mDoFormat) {
    NS_ENSURE_TRUE(AppendToStringFormatedWrapped(data, aStr), NS_ERROR_OUT_OF_MEMORY);
  }
  else if (mDoWrap) {
    NS_ENSURE_TRUE(AppendToStringWrapped(data, aStr), NS_ERROR_OUT_OF_MEMORY);
  }
  else {
    int32_t lastNewlineOffset = kNotFound;
    if (HasLongLines(data, lastNewlineOffset)) {
      
      mDoWrap = true;
      bool result = AppendToStringWrapped(data, aStr);
      mDoWrap = false;
      NS_ENSURE_TRUE(result, NS_ERROR_OUT_OF_MEMORY);
    }
    else {
      NS_ENSURE_TRUE(AppendToStringConvertLF(data, aStr), NS_ERROR_OUT_OF_MEMORY);
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

  int32_t start = 0;
  int32_t end;
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

  if (start < (int32_t) aURI.Length()) {
    
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

bool
nsXHTMLContentSerializer::SerializeAttributes(nsIContent* aContent,
                                              nsIContent *aOriginalElement,
                                              nsAString& aTagPrefix,
                                              const nsAString& aTagNamespaceURI,
                                              nsIAtom* aTagName,
                                              nsAString& aStr,
                                              uint32_t aSkipAttr,
                                              bool aAddNSAttr)
{
  nsresult rv;
  uint32_t index, count;
  nsAutoString prefixStr, uriStr, valueStr;
  nsAutoString xmlnsStr;
  xmlnsStr.AssignLiteral(kXMLNS);

  int32_t contentNamespaceID = aContent->GetNameSpaceID();

  
  

  if (mIsCopying && kNameSpaceID_XHTML == contentNamespaceID) {

    
    
    if (aTagName == nsGkAtoms::ol) {
      
      
      nsAutoString start;
      int32_t startAttrVal = 0;
      aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::start, start);
      if (!start.IsEmpty()) {
        nsresult rv = NS_OK;
        startAttrVal = start.ToInteger(&rv);
        
        
        
        if (NS_SUCCEEDED(rv))
          --startAttrVal;
        else
          startAttrVal = 0;
      }
      olState state (startAttrVal, true);
      mOLStateStack.AppendElement(state);
    }
    else if (aTagName == nsGkAtoms::li) {
      mIsFirstChildOfOL = IsFirstChildOfOL(aOriginalElement);
      if (mIsFirstChildOfOL) {
        
        NS_ENSURE_TRUE(SerializeLIValueAttribute(aContent, aStr), false);
      }
    }
  }

  
  
  if (aAddNSAttr) {
    if (aTagPrefix.IsEmpty()) {
      
      NS_ENSURE_TRUE(SerializeAttr(EmptyString(), xmlnsStr,
                                   aTagNamespaceURI,
                                   aStr, true), false);
    } else {
      
      NS_ENSURE_TRUE(SerializeAttr(xmlnsStr, aTagPrefix,
                                   aTagNamespaceURI,
                                   aStr, true), false);
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
    int32_t namespaceID = name->NamespaceID();
    nsIAtom* attrName = name->LocalName();
    nsIAtom* attrPrefix = name->GetPrefix();

    
    nsDependentAtomString attrNameStr(attrName);
    if (StringBeginsWith(attrNameStr, NS_LITERAL_STRING("_moz")) ||
        StringBeginsWith(attrNameStr, NS_LITERAL_STRING("-moz"))) {
      continue;
    }

    if (attrPrefix) {
      attrPrefix->ToString(prefixStr);
    }
    else {
      prefixStr.Truncate();
    }

    bool addNSAttr = false;
    if (kNameSpaceID_XMLNS != namespaceID) {
      nsContentUtils::NameSpaceManager()->GetNameSpaceURI(namespaceID, uriStr);
      addNSAttr = ConfirmPrefix(prefixStr, uriStr, aOriginalElement, true);
    }

    aContent->GetAttr(namespaceID, attrName, valueStr);

    nsDependentAtomString nameStr(attrName);
    bool isJS = false;

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

      if (mRewriteEncodingDeclaration && aTagName == nsGkAtoms::meta &&
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

    NS_ENSURE_TRUE(SerializeAttr(prefixStr, nameStr, valueStr, aStr, !isJS), false);

    if (addNSAttr) {
      NS_ASSERTION(!prefixStr.IsEmpty(),
                   "Namespaced attributes must have a prefix");
      NS_ENSURE_TRUE(SerializeAttr(xmlnsStr, prefixStr, uriStr, aStr, true), false);
      PushNameSpaceDecl(prefixStr, uriStr, aOriginalElement);
    }
  }

  return true;
}


bool
nsXHTMLContentSerializer::AppendEndOfElementStart(nsIContent *aOriginalElement,
                                                  nsIAtom * aName,
                                                  int32_t aNamespaceID,
                                                  nsAString& aStr)
{
  
  
  NS_ASSERTION(!mIsHTMLSerializer, "nsHTMLContentSerializer shouldn't call this method !");

  if (kNameSpaceID_XHTML != aNamespaceID) {
    return nsXMLContentSerializer::AppendEndOfElementStart(aOriginalElement, aName,
                                                           aNamespaceID, aStr);
  }

  nsIContent* content = aOriginalElement;

  
  
  
  if (HasNoChildren(content)) {

    nsIParserService* parserService = nsContentUtils::GetParserService();
  
    if (parserService) {
      bool isContainer;
      parserService->
        IsContainer(parserService->HTMLCaseSensitiveAtomTagToId(aName),
                    isContainer);
      if (!isContainer) {
        
        
        
        return AppendToString(NS_LITERAL_STRING(" />"), aStr);
      }
    }
  }
  return AppendToString(kGreaterThan, aStr);
}

bool
nsXHTMLContentSerializer::AfterElementStart(nsIContent* aContent,
                                            nsIContent* aOriginalElement,
                                            nsAString& aStr)
{
  if (mRewriteEncodingDeclaration &&
      aContent->IsHTMLElement(nsGkAtoms::head)) {

    
    
    
    bool hasMeta = false;
    for (nsIContent* child = aContent->GetFirstChild();
         child;
         child = child->GetNextSibling()) {
      if (child->IsHTMLElement(nsGkAtoms::meta) &&
          child->HasAttr(kNameSpaceID_None, nsGkAtoms::content)) {
        nsAutoString header;
        child->GetAttr(kNameSpaceID_None, nsGkAtoms::httpEquiv, header);

        if (header.LowerCaseEqualsLiteral("content-type")) {
          hasMeta = true;
          break;
        }
      }
    }

    if (!hasMeta) {
      NS_ENSURE_TRUE(AppendNewLineToString(aStr), false);
      if (mDoFormat) {
        NS_ENSURE_TRUE(AppendIndentation(aStr), false);
      }
      NS_ENSURE_TRUE(AppendToString(NS_LITERAL_STRING("<meta http-equiv=\"content-type\""), aStr), false);
      NS_ENSURE_TRUE(AppendToString(NS_LITERAL_STRING(" content=\"text/html; charset="), aStr), false);
      NS_ENSURE_TRUE(AppendToString(NS_ConvertASCIItoUTF16(mCharset), aStr), false);
      if (mIsHTMLSerializer) {
        NS_ENSURE_TRUE(AppendToString(NS_LITERAL_STRING("\">"), aStr), false);
      } else {
        NS_ENSURE_TRUE(AppendToString(NS_LITERAL_STRING("\" />"), aStr), false);
      }
    }
  }

  return true;
}

void
nsXHTMLContentSerializer::AfterElementEnd(nsIContent * aContent,
                                          nsAString& aStr)
{
  NS_ASSERTION(!mIsHTMLSerializer, "nsHTMLContentSerializer shouldn't call this method !");

  
  
  if (aContent->IsHTMLElement(nsGkAtoms::body)) {
    --mInBody;
  }
}


NS_IMETHODIMP
nsXHTMLContentSerializer::AppendDocumentStart(nsIDocument *aDocument,
                                              nsAString& aStr)
{
  if (!mBodyOnly)
    return nsXMLContentSerializer::AppendDocumentStart(aDocument, aStr);

  return NS_OK;
}

bool
nsXHTMLContentSerializer::CheckElementStart(nsIContent * aContent,
                                            bool & aForceFormat,
                                            nsAString& aStr,
                                            nsresult& aResult)
{
  aResult = NS_OK;

  
  
  
  aForceFormat = !(mFlags & nsIDocumentEncoder::OutputIgnoreMozDirty) &&
                 aContent->HasAttr(kNameSpaceID_None, nsGkAtoms::mozdirty);

  if (aContent->IsHTMLElement(nsGkAtoms::br) &&
      (mFlags & nsIDocumentEncoder::OutputNoFormattingInPre) &&
      PreLevel() > 0) {
    aResult = AppendNewLineToString(aStr) ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
    return false;
  }

  if (aContent->IsHTMLElement(nsGkAtoms::body)) {
    ++mInBody;
  }

  return true;
}

bool
nsXHTMLContentSerializer::CheckElementEnd(nsIContent * aContent,
                                          bool & aForceFormat,
                                          nsAString& aStr)
{
  NS_ASSERTION(!mIsHTMLSerializer, "nsHTMLContentSerializer shouldn't call this method !");

  aForceFormat = !(mFlags & nsIDocumentEncoder::OutputIgnoreMozDirty) &&
                 aContent->HasAttr(kNameSpaceID_None, nsGkAtoms::mozdirty);

  
  
  if (aContent->IsHTMLElement()) {
    if (mIsCopying && aContent->IsHTMLElement(nsGkAtoms::ol)) {
      NS_ASSERTION((!mOLStateStack.IsEmpty()), "Cannot have an empty OL Stack");
      

      if (!mOLStateStack.IsEmpty()) {
        mOLStateStack.RemoveElementAt(mOLStateStack.Length() -1);
      }
    }

    if (HasNoChildren(aContent)) {
      nsIParserService* parserService = nsContentUtils::GetParserService();

      if (parserService) {
        bool isContainer;

        parserService->
          IsContainer(parserService->HTMLCaseSensitiveAtomTagToId(
                        aContent->NodeInfo()->NameAtom()),
                      isContainer);
        if (!isContainer) {
          
          
          return false;
        }
      }
    }
    
    
    
    return true;
  }

  bool dummyFormat;
  return nsXMLContentSerializer::CheckElementEnd(aContent, dummyFormat, aStr);
}

bool
nsXHTMLContentSerializer::AppendAndTranslateEntities(const nsAString& aStr,
                                                     nsAString& aOutputStr)
{
  if (mBodyOnly && !mInBody) {
    return true;
  }

  if (mDisableEntityEncoding) {
    return aOutputStr.Append(aStr, mozilla::fallible);
  }

  return nsXMLContentSerializer::AppendAndTranslateEntities(aStr, aOutputStr);
}

bool
nsXHTMLContentSerializer::IsShorthandAttr(const nsIAtom* aAttrName,
                                          const nsIAtom* aElementName)
{
  
  if ((aAttrName == nsGkAtoms::checked) &&
      (aElementName == nsGkAtoms::input)) {
    return true;
  }

  
  if ((aAttrName == nsGkAtoms::compact) &&
      (aElementName == nsGkAtoms::dir || 
       aElementName == nsGkAtoms::dl ||
       aElementName == nsGkAtoms::menu ||
       aElementName == nsGkAtoms::ol ||
       aElementName == nsGkAtoms::ul)) {
    return true;
  }

  
  if ((aAttrName == nsGkAtoms::declare) &&
      (aElementName == nsGkAtoms::object)) {
    return true;
  }

  
  if ((aAttrName == nsGkAtoms::defer) &&
      (aElementName == nsGkAtoms::script)) {
    return true;
  }

  
  if ((aAttrName == nsGkAtoms::disabled) &&
      (aElementName == nsGkAtoms::button ||
       aElementName == nsGkAtoms::input ||
       aElementName == nsGkAtoms::optgroup ||
       aElementName == nsGkAtoms::option ||
       aElementName == nsGkAtoms::select ||
       aElementName == nsGkAtoms::textarea)) {
    return true;
  }

  
  if ((aAttrName == nsGkAtoms::ismap) &&
      (aElementName == nsGkAtoms::img ||
       aElementName == nsGkAtoms::input)) {
    return true;
  }

  
  if ((aAttrName == nsGkAtoms::multiple) &&
      (aElementName == nsGkAtoms::select)) {
    return true;
  }

  
  if ((aAttrName == nsGkAtoms::noresize) &&
      (aElementName == nsGkAtoms::frame)) {
    return true;
  }

  
  if ((aAttrName == nsGkAtoms::noshade) &&
      (aElementName == nsGkAtoms::hr)) {
    return true;
  }

  
  if ((aAttrName == nsGkAtoms::nowrap) &&
      (aElementName == nsGkAtoms::td ||
       aElementName == nsGkAtoms::th)) {
    return true;
  }

  
  if ((aAttrName == nsGkAtoms::readonly) &&
      (aElementName == nsGkAtoms::input ||
       aElementName == nsGkAtoms::textarea)) {
    return true;
  }

  
  if ((aAttrName == nsGkAtoms::selected) &&
      (aElementName == nsGkAtoms::option)) {
    return true;
  }

  
  if ((aElementName == nsGkAtoms::video || aElementName == nsGkAtoms::audio) &&
    (aAttrName == nsGkAtoms::autoplay || aAttrName == nsGkAtoms::muted ||
     aAttrName == nsGkAtoms::controls)) {
    return true;
  }

  return false;
}

bool
nsXHTMLContentSerializer::LineBreakBeforeOpen(int32_t aNamespaceID, nsIAtom* aName)
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
    return true;
  }
  else {
    nsIParserService* parserService = nsContentUtils::GetParserService();

    if (parserService) {
      bool res;
      parserService->
        IsBlock(parserService->HTMLCaseSensitiveAtomTagToId(aName), res);
      return res;
    }
  }

  return mAddSpace;
}

bool 
nsXHTMLContentSerializer::LineBreakAfterOpen(int32_t aNamespaceID, nsIAtom* aName)
{

  if (aNamespaceID != kNameSpaceID_XHTML) {
    return false;
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
    return true;
  }

  return false;
}

bool 
nsXHTMLContentSerializer::LineBreakBeforeClose(int32_t aNamespaceID, nsIAtom* aName)
{

  if (aNamespaceID != kNameSpaceID_XHTML) {
    return false;
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
    return true;
  }
  return false;
}

bool 
nsXHTMLContentSerializer::LineBreakAfterClose(int32_t aNamespaceID, nsIAtom* aName)
{

  if (aNamespaceID != kNameSpaceID_XHTML) {
    return false;
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
    return true;
  }
  else {
    nsIParserService* parserService = nsContentUtils::GetParserService();

    if (parserService) {
      bool res;
      parserService->
        IsBlock(parserService->HTMLCaseSensitiveAtomTagToId(aName), res);
      return res;
    }
  }

  return false;
}


void
nsXHTMLContentSerializer::MaybeEnterInPreContent(nsIContent* aNode)
{
  if (!ShouldMaintainPreLevel() ||
      !aNode->IsHTMLElement()) {
    return;
  }

  if (IsElementPreformatted(aNode) ||
      aNode->IsAnyOfHTMLElements(nsGkAtoms::script,
                                 nsGkAtoms::style,
                                 nsGkAtoms::noscript,
                                 nsGkAtoms::noframes)) {
    PreLevel()++;
  }
}

void
nsXHTMLContentSerializer::MaybeLeaveFromPreContent(nsIContent* aNode)
{
  if (!ShouldMaintainPreLevel() ||
      !aNode->IsHTMLElement()) {
    return;
  }

  if (IsElementPreformatted(aNode) ||
      aNode->IsAnyOfHTMLElements(nsGkAtoms::script,
                                 nsGkAtoms::style,
                                 nsGkAtoms::noscript,
                                 nsGkAtoms::noframes)) {
    --PreLevel();
  }
}

bool
nsXHTMLContentSerializer::IsElementPreformatted(nsIContent* aNode)
{
  MOZ_ASSERT(ShouldMaintainPreLevel(), "We should not be calling this needlessly");

  if (!aNode->IsElement()) {
    return false;
  }
  nsRefPtr<nsStyleContext> styleContext =
    nsComputedDOMStyle::GetStyleContextForElementNoFlush(aNode->AsElement(),
                                                         nullptr, nullptr);
  if (styleContext) {
    const nsStyleText* textStyle = styleContext->StyleText();
    return textStyle->WhiteSpaceOrNewlineIsSignificant();
  }
  return false;
}

bool
nsXHTMLContentSerializer::SerializeLIValueAttribute(nsIContent* aElement,
                                                    nsAString& aStr)
{
  
  
  
  bool found = false;
  nsCOMPtr<nsIDOMNode> currNode = do_QueryInterface(aElement);
  nsAutoString valueStr;

  olState state (0, false);

  if (!mOLStateStack.IsEmpty()) {
    state = mOLStateStack[mOLStateStack.Length()-1];
    
    
    state.isFirstListItem = false;
    mOLStateStack[mOLStateStack.Length()-1] = state;
  }

  int32_t startVal = state.startVal;
  int32_t offset = 0;

  
  
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
          found = true;
          nsresult rv = NS_OK;
          startVal = valueStr.ToInteger(&rv);
        }
      }
    }
    nsCOMPtr<nsIDOMNode> tmp;
    currNode->GetPreviousSibling(getter_AddRefs(tmp));
    currNode.swap(tmp);
  }
  
  
  if (offset == 0 && found) {
    
    
    NS_ENSURE_TRUE(SerializeAttr(EmptyString(), NS_LITERAL_STRING("value"),
                                 valueStr, aStr, false), false);
  }
  else if (offset == 1 && !found) {
    



    
  }
  else if (offset > 0) {
    
    nsAutoString valueStr;

    
    valueStr.AppendInt(startVal + offset);
    NS_ENSURE_TRUE(SerializeAttr(EmptyString(), NS_LITERAL_STRING("value"),
                                 valueStr, aStr, false), false);
  }

  return true;
}

bool
nsXHTMLContentSerializer::IsFirstChildOfOL(nsIContent* aElement)
{
  nsCOMPtr<nsIDOMNode> node = do_QueryInterface(aElement);
  nsAutoString parentName;

  nsCOMPtr<nsIDOMNode> parentNode;
  node->GetParentNode(getter_AddRefs(parentNode));
  if (parentNode)
    parentNode->GetNodeName(parentName);
  else
    return false;

  if (parentName.LowerCaseEqualsLiteral("ol")) {

    if (!mOLStateStack.IsEmpty()) {
      olState state = mOLStateStack[mOLStateStack.Length()-1];
      if (state.isFirstListItem)
        return true;
    }

    return false;
  }
  else
    return false;
}

bool
nsXHTMLContentSerializer::HasNoChildren(nsIContent * aContent) {

  for (nsIContent* child = aContent->GetFirstChild();
       child;
       child = child->GetNextSibling()) {
       
    if (!child->IsNodeOfType(nsINode::eTEXT))
      return false;

    if (child->TextLength())
      return false;
  }

  return true;
}
