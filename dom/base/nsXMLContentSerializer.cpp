










#include "nsXMLContentSerializer.h"

#include "nsGkAtoms.h"
#include "nsIDOMProcessingInstruction.h"
#include "nsIDOMComment.h"
#include "nsIDOMDocumentType.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIDocumentEncoder.h"
#include "nsNameSpaceManager.h"
#include "nsTextFragment.h"
#include "nsString.h"
#include "prprf.h"
#include "nsUnicharUtils.h"
#include "nsCRT.h"
#include "nsContentUtils.h"
#include "nsAttrName.h"
#include "nsILineBreaker.h"
#include "mozilla/dom/Element.h"
#include "nsParserConstants.h"

using namespace mozilla::dom;

#define kXMLNS "xmlns"




#define MIN_INDENTED_LINE_LENGTH 15


#define INDENT_STRING "  "
#define INDENT_STRING_LENGTH 2

nsresult
NS_NewXMLContentSerializer(nsIContentSerializer** aSerializer)
{
  nsRefPtr<nsXMLContentSerializer> it = new nsXMLContentSerializer();
  it.forget(aSerializer);
  return NS_OK;
}

nsXMLContentSerializer::nsXMLContentSerializer()
  : mPrefixIndex(0),
    mColPos(0),
    mIndentOverflow(0),
    mIsIndentationAddedOnCurrentLine(false),
    mInAttribute(false),
    mAddNewlineForRootNode(false),
    mAddSpace(false),
    mMayIgnoreLineBreakSequence(false),
    mBodyOnly(false),
    mInBody(0)
{
}

nsXMLContentSerializer::~nsXMLContentSerializer()
{
}

NS_IMPL_ISUPPORTS(nsXMLContentSerializer, nsIContentSerializer)

NS_IMETHODIMP 
nsXMLContentSerializer::Init(uint32_t aFlags, uint32_t aWrapColumn,
                             const char* aCharSet, bool aIsCopying,
                             bool aRewriteEncodingDeclaration)
{
  mPrefixIndex = 0;
  mColPos = 0;
  mIndentOverflow = 0;
  mIsIndentationAddedOnCurrentLine = false;
  mInAttribute = false;
  mAddNewlineForRootNode = false;
  mAddSpace = false;
  mMayIgnoreLineBreakSequence = false;
  mBodyOnly = false;
  mInBody = 0;

  mCharset = aCharSet;
  mFlags = aFlags;

  
  if ((mFlags & nsIDocumentEncoder::OutputCRLineBreak)
      && (mFlags & nsIDocumentEncoder::OutputLFLineBreak)) { 
    mLineBreak.AssignLiteral("\r\n");
  }
  else if (mFlags & nsIDocumentEncoder::OutputCRLineBreak) { 
    mLineBreak.Assign('\r');
  }
  else if (mFlags & nsIDocumentEncoder::OutputLFLineBreak) { 
    mLineBreak.Assign('\n');
  }
  else {
    mLineBreak.AssignLiteral(NS_LINEBREAK);         
  }

  mDoRaw = !!(mFlags & nsIDocumentEncoder::OutputRaw);

  mDoFormat = (mFlags & nsIDocumentEncoder::OutputFormatted && !mDoRaw);

  mDoWrap = (mFlags & nsIDocumentEncoder::OutputWrap && !mDoRaw);

  if (!aWrapColumn) {
    mMaxColumn = 72;
  }
  else {
    mMaxColumn = aWrapColumn;
  }

  mPreLevel = 0;
  mIsIndentationAddedOnCurrentLine = false;
  return NS_OK;
}

nsresult
nsXMLContentSerializer::AppendTextData(nsIContent* aNode,
                                       int32_t aStartOffset,
                                       int32_t aEndOffset,
                                       nsAString& aStr,
                                       bool aTranslateEntities)
{
  nsIContent* content = aNode;
  const nsTextFragment* frag;
  if (!content || !(frag = content->GetText())) {
    return NS_ERROR_FAILURE;
  }

  int32_t fragLength = frag->GetLength();
  int32_t endoffset = (aEndOffset == -1) ? fragLength : std::min(aEndOffset, fragLength);
  int32_t length = endoffset - aStartOffset;

  NS_ASSERTION(aStartOffset >= 0, "Negative start offset for text fragment!");
  NS_ASSERTION(aStartOffset <= endoffset, "A start offset is beyond the end of the text fragment!");

  if (length <= 0) {
    
    
    return NS_OK;
  }
    
  if (frag->Is2b()) {
    const char16_t *strStart = frag->Get2b() + aStartOffset;
    if (aTranslateEntities) {
      NS_ENSURE_TRUE(AppendAndTranslateEntities(Substring(strStart, strStart + length), aStr),
                     NS_ERROR_OUT_OF_MEMORY);
    }
    else {
      NS_ENSURE_TRUE(aStr.Append(Substring(strStart, strStart + length), mozilla::fallible),
                     NS_ERROR_OUT_OF_MEMORY);
    }
  }
  else {
    if (aTranslateEntities) {
      NS_ENSURE_TRUE(AppendAndTranslateEntities(NS_ConvertASCIItoUTF16(frag->Get1b()+aStartOffset, length), aStr),
                     NS_ERROR_OUT_OF_MEMORY);
    }
    else {
      NS_ENSURE_TRUE(aStr.Append(NS_ConvertASCIItoUTF16(frag->Get1b()+aStartOffset, length), mozilla::fallible),
                     NS_ERROR_OUT_OF_MEMORY);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP 
nsXMLContentSerializer::AppendText(nsIContent* aText,
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
    NS_ENSURE_TRUE(AppendToStringConvertLF(data, aStr), NS_ERROR_OUT_OF_MEMORY);
  }

  return NS_OK;
}

NS_IMETHODIMP 
nsXMLContentSerializer::AppendCDATASection(nsIContent* aCDATASection,
                                           int32_t aStartOffset,
                                           int32_t aEndOffset,
                                           nsAString& aStr)
{
  NS_ENSURE_ARG(aCDATASection);
  nsresult rv;

  NS_NAMED_LITERAL_STRING(cdata , "<![CDATA[");

  if (mDoRaw || PreLevel() > 0) {
    NS_ENSURE_TRUE(AppendToString(cdata, aStr), NS_ERROR_OUT_OF_MEMORY);
  }
  else if (mDoFormat) {
    NS_ENSURE_TRUE(AppendToStringFormatedWrapped(cdata, aStr), NS_ERROR_OUT_OF_MEMORY);
  }
  else if (mDoWrap) {
    NS_ENSURE_TRUE(AppendToStringWrapped(cdata, aStr), NS_ERROR_OUT_OF_MEMORY);
  }
  else {
    NS_ENSURE_TRUE(AppendToString(cdata, aStr), NS_ERROR_OUT_OF_MEMORY);
  }

  nsAutoString data;
  rv = AppendTextData(aCDATASection, aStartOffset, aEndOffset, data, false);
  if (NS_FAILED(rv)) return NS_ERROR_FAILURE;

  NS_ENSURE_TRUE(AppendToStringConvertLF(data, aStr), NS_ERROR_OUT_OF_MEMORY);

  NS_ENSURE_TRUE(AppendToString(NS_LITERAL_STRING("]]>"), aStr), NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

NS_IMETHODIMP 
nsXMLContentSerializer::AppendProcessingInstruction(nsIContent* aPI,
                                                    int32_t aStartOffset,
                                                    int32_t aEndOffset,
                                                    nsAString& aStr)
{
  nsCOMPtr<nsIDOMProcessingInstruction> pi = do_QueryInterface(aPI);
  NS_ENSURE_ARG(pi);
  nsresult rv;
  nsAutoString target, data, start;

  NS_ENSURE_TRUE(MaybeAddNewlineForRootNode(aStr), NS_ERROR_OUT_OF_MEMORY);

  rv = pi->GetTarget(target);
  if (NS_FAILED(rv)) return NS_ERROR_FAILURE;

  rv = pi->GetData(data);
  if (NS_FAILED(rv)) return NS_ERROR_FAILURE;

  NS_ENSURE_TRUE(start.AppendLiteral("<?", mozilla::fallible), NS_ERROR_OUT_OF_MEMORY);
  NS_ENSURE_TRUE(start.Append(target, mozilla::fallible), NS_ERROR_OUT_OF_MEMORY);

  if (mDoRaw || PreLevel() > 0) {
    NS_ENSURE_TRUE(AppendToString(start, aStr), NS_ERROR_OUT_OF_MEMORY);
  }
  else if (mDoFormat) {
    if (mAddSpace) {
      NS_ENSURE_TRUE(AppendNewLineToString(aStr), NS_ERROR_OUT_OF_MEMORY);
    }
    NS_ENSURE_TRUE(AppendToStringFormatedWrapped(start, aStr), NS_ERROR_OUT_OF_MEMORY);
  }
  else if (mDoWrap) {
    NS_ENSURE_TRUE(AppendToStringWrapped(start, aStr), NS_ERROR_OUT_OF_MEMORY);
  }
  else {
    NS_ENSURE_TRUE(AppendToString(start, aStr), NS_ERROR_OUT_OF_MEMORY);
  }

  if (!data.IsEmpty()) {
    NS_ENSURE_TRUE(AppendToString(char16_t(' '), aStr), NS_ERROR_OUT_OF_MEMORY);
    NS_ENSURE_TRUE(AppendToStringConvertLF(data, aStr), NS_ERROR_OUT_OF_MEMORY);
  }
  NS_ENSURE_TRUE(AppendToString(NS_LITERAL_STRING("?>"), aStr), NS_ERROR_OUT_OF_MEMORY);

  MaybeFlagNewlineForRootNode(aPI);

  return NS_OK;
}

NS_IMETHODIMP 
nsXMLContentSerializer::AppendComment(nsIContent* aComment,
                                      int32_t aStartOffset,
                                      int32_t aEndOffset,
                                      nsAString& aStr)
{
  nsCOMPtr<nsIDOMComment> comment = do_QueryInterface(aComment);
  NS_ENSURE_ARG(comment);
  nsresult rv;
  nsAutoString data;

  rv = comment->GetData(data);
  if (NS_FAILED(rv)) return NS_ERROR_FAILURE;

  int32_t dataLength = data.Length();
  if (aStartOffset || (aEndOffset != -1 && aEndOffset < dataLength)) {
    int32_t length =
      (aEndOffset == -1) ? dataLength : std::min(aEndOffset, dataLength);
    length -= aStartOffset;

    nsAutoString frag;
    if (length > 0) {
      data.Mid(frag, aStartOffset, length);
    }
    data.Assign(frag);
  }

  NS_ENSURE_TRUE(MaybeAddNewlineForRootNode(aStr), NS_ERROR_OUT_OF_MEMORY);

  NS_NAMED_LITERAL_STRING(startComment, "<!--");

  if (mDoRaw || PreLevel() > 0) {
    NS_ENSURE_TRUE(AppendToString(startComment, aStr), NS_ERROR_OUT_OF_MEMORY);
  }
  else if (mDoFormat) {
    if (mAddSpace) {
      NS_ENSURE_TRUE(AppendNewLineToString(aStr), NS_ERROR_OUT_OF_MEMORY);
    }
    NS_ENSURE_TRUE(AppendToStringFormatedWrapped(startComment, aStr), NS_ERROR_OUT_OF_MEMORY);
  }
  else if (mDoWrap) {
    NS_ENSURE_TRUE(AppendToStringWrapped(startComment, aStr), NS_ERROR_OUT_OF_MEMORY);
  }
  else {
    NS_ENSURE_TRUE(AppendToString(startComment, aStr), NS_ERROR_OUT_OF_MEMORY);
  }

  
  
  NS_ENSURE_TRUE(AppendToStringConvertLF(data, aStr), NS_ERROR_OUT_OF_MEMORY);
  NS_ENSURE_TRUE(AppendToString(NS_LITERAL_STRING("-->"), aStr), NS_ERROR_OUT_OF_MEMORY);

  MaybeFlagNewlineForRootNode(aComment);

  return NS_OK;
}

NS_IMETHODIMP 
nsXMLContentSerializer::AppendDoctype(nsIContent* aDocType,
                                      nsAString& aStr)
{
  nsCOMPtr<nsIDOMDocumentType> docType = do_QueryInterface(aDocType);
  NS_ENSURE_ARG(docType);
  nsresult rv;
  nsAutoString name, publicId, systemId, internalSubset;

  rv = docType->GetName(name);
  if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
  rv = docType->GetPublicId(publicId);
  if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
  rv = docType->GetSystemId(systemId);
  if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
  rv = docType->GetInternalSubset(internalSubset);
  if (NS_FAILED(rv)) return NS_ERROR_FAILURE;

  NS_ENSURE_TRUE(MaybeAddNewlineForRootNode(aStr), NS_ERROR_OUT_OF_MEMORY);

  NS_ENSURE_TRUE(AppendToString(NS_LITERAL_STRING("<!DOCTYPE "), aStr), NS_ERROR_OUT_OF_MEMORY);
  NS_ENSURE_TRUE(AppendToString(name, aStr), NS_ERROR_OUT_OF_MEMORY);

  char16_t quote;
  if (!publicId.IsEmpty()) {
    NS_ENSURE_TRUE(AppendToString(NS_LITERAL_STRING(" PUBLIC "), aStr), NS_ERROR_OUT_OF_MEMORY);
    if (publicId.FindChar(char16_t('"')) == -1) {
      quote = char16_t('"');
    }
    else {
      quote = char16_t('\'');
    }
    NS_ENSURE_TRUE(AppendToString(quote, aStr), NS_ERROR_OUT_OF_MEMORY);
    NS_ENSURE_TRUE(AppendToString(publicId, aStr), NS_ERROR_OUT_OF_MEMORY);
    NS_ENSURE_TRUE(AppendToString(quote, aStr), NS_ERROR_OUT_OF_MEMORY);

    if (!systemId.IsEmpty()) {
      NS_ENSURE_TRUE(AppendToString(char16_t(' '), aStr), NS_ERROR_OUT_OF_MEMORY);
      if (systemId.FindChar(char16_t('"')) == -1) {
        quote = char16_t('"');
      }
      else {
        quote = char16_t('\'');
      }
      NS_ENSURE_TRUE(AppendToString(quote, aStr), NS_ERROR_OUT_OF_MEMORY);
      NS_ENSURE_TRUE(AppendToString(systemId, aStr), NS_ERROR_OUT_OF_MEMORY);
      NS_ENSURE_TRUE(AppendToString(quote, aStr), NS_ERROR_OUT_OF_MEMORY);
    }
  }
  else if (!systemId.IsEmpty()) {
    if (systemId.FindChar(char16_t('"')) == -1) {
      quote = char16_t('"');
    }
    else {
      quote = char16_t('\'');
    }
    NS_ENSURE_TRUE(AppendToString(NS_LITERAL_STRING(" SYSTEM "), aStr), NS_ERROR_OUT_OF_MEMORY);
    NS_ENSURE_TRUE(AppendToString(quote, aStr), NS_ERROR_OUT_OF_MEMORY);
    NS_ENSURE_TRUE(AppendToString(systemId, aStr), NS_ERROR_OUT_OF_MEMORY);
    NS_ENSURE_TRUE(AppendToString(quote, aStr), NS_ERROR_OUT_OF_MEMORY);
  }
  
  if (!internalSubset.IsEmpty()) {
    NS_ENSURE_TRUE(AppendToString(NS_LITERAL_STRING(" ["), aStr), NS_ERROR_OUT_OF_MEMORY);
    NS_ENSURE_TRUE(AppendToString(internalSubset, aStr), NS_ERROR_OUT_OF_MEMORY);
    NS_ENSURE_TRUE(AppendToString(char16_t(']'), aStr), NS_ERROR_OUT_OF_MEMORY);
  }
    
  NS_ENSURE_TRUE(AppendToString(kGreaterThan, aStr), NS_ERROR_OUT_OF_MEMORY);
  MaybeFlagNewlineForRootNode(aDocType);

  return NS_OK;
}

nsresult
nsXMLContentSerializer::PushNameSpaceDecl(const nsAString& aPrefix,
                                          const nsAString& aURI,
                                          nsIContent* aOwner)
{
  NameSpaceDecl* decl = mNameSpaceStack.AppendElement();
  if (!decl) return NS_ERROR_OUT_OF_MEMORY;

  decl->mPrefix.Assign(aPrefix);
  decl->mURI.Assign(aURI);
  
  
  decl->mOwner = aOwner;
  return NS_OK;
}

void
nsXMLContentSerializer::PopNameSpaceDeclsFor(nsIContent* aOwner)
{
  int32_t index, count;

  count = mNameSpaceStack.Length();
  for (index = count - 1; index >= 0; index--) {
    if (mNameSpaceStack[index].mOwner != aOwner) {
      break;
    }
    mNameSpaceStack.RemoveElementAt(index);
  }
}

bool
nsXMLContentSerializer::ConfirmPrefix(nsAString& aPrefix,
                                      const nsAString& aURI,
                                      nsIContent* aElement,
                                      bool aIsAttribute)
{
  if (aPrefix.EqualsLiteral(kXMLNS)) {
    return false;
  }

  if (aURI.EqualsLiteral("http://www.w3.org/XML/1998/namespace")) {
    
    
    aPrefix.AssignLiteral("xml");

    return false;
  }

  bool mustHavePrefix;
  if (aIsAttribute) {
    if (aURI.IsEmpty()) {
      
      
      aPrefix.Truncate();
      return false;
    }

    
    mustHavePrefix = true;
  } else {
    
    mustHavePrefix = false;
  }

  
  
  
  nsAutoString closestURIMatch;
  bool uriMatch = false;

  
  
  
  bool haveSeenOurPrefix = false;

  int32_t count = mNameSpaceStack.Length();
  int32_t index = count - 1;
  while (index >= 0) {
    NameSpaceDecl& decl = mNameSpaceStack.ElementAt(index);
    
    if (aPrefix.Equals(decl.mPrefix)) {

      
      
      if (!haveSeenOurPrefix && aURI.Equals(decl.mURI)) {
        
        
        uriMatch = true;
        closestURIMatch = aPrefix;
        break;
      }

      haveSeenOurPrefix = true;      

      
      
      
      
      
      
      
      
      
      
      if (!aPrefix.IsEmpty() || decl.mOwner == aElement) {
        NS_ASSERTION(!aURI.IsEmpty(),
                     "Not allowed to add a xmlns attribute with an empty "
                     "namespace name unless it declares the default "
                     "namespace.");

        GenerateNewPrefix(aPrefix);
        
        
        
        
        index = count - 1;
        haveSeenOurPrefix = false;
        continue;
      }
    }
    
    
    if (!uriMatch && aURI.Equals(decl.mURI)) {
      
      
      bool prefixOK = true;
      int32_t index2;
      for (index2 = count-1; index2 > index && prefixOK; --index2) {
        prefixOK = (mNameSpaceStack[index2].mPrefix != decl.mPrefix);
      }
      
      if (prefixOK) {
        uriMatch = true;
        closestURIMatch.Assign(decl.mPrefix);
      }
    }
    
    --index;
  }

  
  
  
  
  
  
  
  
  
  
  if (uriMatch && (!mustHavePrefix || !closestURIMatch.IsEmpty())) {
    aPrefix.Assign(closestURIMatch);
    return false;
  }
  
  if (aPrefix.IsEmpty()) {
    
    
    
    
    if (mustHavePrefix) {
      GenerateNewPrefix(aPrefix);
      return ConfirmPrefix(aPrefix, aURI, aElement, aIsAttribute);
    }

    
    
    
    
    if (!haveSeenOurPrefix && aURI.IsEmpty()) {
      return false;
    }
  }

  
  
  return true;
}

void
nsXMLContentSerializer::GenerateNewPrefix(nsAString& aPrefix)
{
  aPrefix.Assign('a');
  char buf[128];
  PR_snprintf(buf, sizeof(buf), "%d", mPrefixIndex++);
  AppendASCIItoUTF16(buf, aPrefix);
}

bool
nsXMLContentSerializer::SerializeAttr(const nsAString& aPrefix,
                                      const nsAString& aName,
                                      const nsAString& aValue,
                                      nsAString& aStr,
                                      bool aDoEscapeEntities)
{
  nsAutoString attrString_;
  
  
  bool rawAppend = mDoRaw && aDoEscapeEntities;
  nsAString& attrString = (rawAppend) ? aStr : attrString_;

  NS_ENSURE_TRUE(attrString.Append(char16_t(' '), mozilla::fallible), false);
  if (!aPrefix.IsEmpty()) {
    NS_ENSURE_TRUE(attrString.Append(aPrefix, mozilla::fallible), false);
    NS_ENSURE_TRUE(attrString.Append(char16_t(':'), mozilla::fallible), false);
  }
  NS_ENSURE_TRUE(attrString.Append(aName, mozilla::fallible), false);

  if (aDoEscapeEntities) {
    
    
    NS_ENSURE_TRUE(attrString.AppendLiteral("=\"", mozilla::fallible), false);

    mInAttribute = true;
    bool result = AppendAndTranslateEntities(aValue, attrString);
    mInAttribute = false;
    NS_ENSURE_TRUE(result, false);

    NS_ENSURE_TRUE(attrString.Append(char16_t('"'), mozilla::fallible), false);
    if (rawAppend) {
      return true;
    }
  }
  else {
    
    
    
    
    
    
    
    bool bIncludesSingle = false;
    bool bIncludesDouble = false;
    nsAString::const_iterator iCurr, iEnd;
    uint32_t uiSize, i;
    aValue.BeginReading(iCurr);
    aValue.EndReading(iEnd);
    for ( ; iCurr != iEnd; iCurr.advance(uiSize) ) {
      const char16_t * buf = iCurr.get();
      uiSize = iCurr.size_forward();
      for ( i = 0; i < uiSize; i++, buf++ ) {
        if ( *buf == char16_t('\'') )
        {
          bIncludesSingle = true;
          if ( bIncludesDouble ) break;
        }
        else if ( *buf == char16_t('"') )
        {
          bIncludesDouble = true;
          if ( bIncludesSingle ) break;
        }
      }
      
      if ( bIncludesDouble && bIncludesSingle ) break;
    }

    
    
    
    
    
    
    char16_t cDelimiter = 
        (bIncludesDouble && !bIncludesSingle) ? char16_t('\'') : char16_t('"');
    NS_ENSURE_TRUE(attrString.Append(char16_t('='), mozilla::fallible), false);
    NS_ENSURE_TRUE(attrString.Append(cDelimiter, mozilla::fallible), false);
    nsAutoString sValue(aValue);
    NS_ENSURE_TRUE(sValue.ReplaceSubstring(NS_LITERAL_STRING("&"),
                                           NS_LITERAL_STRING("&amp;"), mozilla::fallible), false);
    if (bIncludesDouble && bIncludesSingle) {
      NS_ENSURE_TRUE(sValue.ReplaceSubstring(NS_LITERAL_STRING("\""),
                                             NS_LITERAL_STRING("&quot;"), mozilla::fallible), false);
    }
    NS_ENSURE_TRUE(attrString.Append(sValue, mozilla::fallible), false);
    NS_ENSURE_TRUE(attrString.Append(cDelimiter, mozilla::fallible), false);
  }
  if (mDoRaw || PreLevel() > 0) {
    NS_ENSURE_TRUE(AppendToStringConvertLF(attrString, aStr), false);
  }
  else if (mDoFormat) {
    NS_ENSURE_TRUE(AppendToStringFormatedWrapped(attrString, aStr), false);
  }
  else if (mDoWrap) {
    NS_ENSURE_TRUE(AppendToStringWrapped(attrString, aStr), false);
  }
  else {
    NS_ENSURE_TRUE(AppendToStringConvertLF(attrString, aStr), false);
  }

  return true;
}

uint32_t 
nsXMLContentSerializer::ScanNamespaceDeclarations(nsIContent* aContent,
                                                  nsIContent *aOriginalElement,
                                                  const nsAString& aTagNamespaceURI)
{
  uint32_t index, count;
  nsAutoString uriStr, valueStr;

  count = aContent->GetAttrCount();

  
  uint32_t skipAttr = count;
  for (index = 0; index < count; index++) {
    
    const nsAttrName* name = aContent->GetAttrNameAt(index);
    int32_t namespaceID = name->NamespaceID();
    nsIAtom *attrName = name->LocalName();
    
    if (namespaceID == kNameSpaceID_XMLNS ||
        
        
        
        
        
        (namespaceID == kNameSpaceID_None &&
         attrName == nsGkAtoms::xmlns)) {
      aContent->GetAttr(namespaceID, attrName, uriStr);

      if (!name->GetPrefix()) {
        if (aTagNamespaceURI.IsEmpty() && !uriStr.IsEmpty()) {
          
          
          
          
          
          
          
          
          skipAttr = index;
        }
        else {
          
          PushNameSpaceDecl(EmptyString(), uriStr, aOriginalElement);
        }
      }
      else {
        PushNameSpaceDecl(nsDependentAtomString(attrName), uriStr,
                          aOriginalElement);
      }
    }
  }
  return skipAttr;
}


bool
nsXMLContentSerializer::IsJavaScript(nsIContent * aContent, nsIAtom* aAttrNameAtom,
                                     int32_t aAttrNamespaceID, const nsAString& aValueString)
{
  bool isHtml = aContent->IsHTMLElement();
  bool isXul = aContent->IsXULElement();
  bool isSvg = aContent->IsSVGElement();

  if (aAttrNamespaceID == kNameSpaceID_None &&
      (isHtml || isXul || isSvg) &&
      (aAttrNameAtom == nsGkAtoms::href ||
       aAttrNameAtom == nsGkAtoms::src)) {

    static const char kJavaScript[] = "javascript";
    int32_t pos = aValueString.FindChar(':');
    if (pos < (int32_t)(sizeof kJavaScript - 1))
        return false;
    nsAutoString scheme(Substring(aValueString, 0, pos));
    scheme.StripWhitespace();
    if ((scheme.Length() == (sizeof kJavaScript - 1)) &&
        scheme.EqualsIgnoreCase(kJavaScript))
      return true;
    else
      return false;
  }

  return aContent->IsEventAttributeName(aAttrNameAtom);
}


bool
nsXMLContentSerializer::SerializeAttributes(nsIContent* aContent,
                                            nsIContent *aOriginalElement,
                                            nsAString& aTagPrefix,
                                            const nsAString& aTagNamespaceURI,
                                            nsIAtom* aTagName,
                                            nsAString& aStr,
                                            uint32_t aSkipAttr,
                                            bool aAddNSAttr)
{

  nsAutoString prefixStr, uriStr, valueStr;
  nsAutoString xmlnsStr;
  xmlnsStr.AssignLiteral(kXMLNS);
  uint32_t index, count;

  
  
  if (aAddNSAttr) {
    if (aTagPrefix.IsEmpty()) {
      
      NS_ENSURE_TRUE(SerializeAttr(EmptyString(), xmlnsStr, aTagNamespaceURI, aStr, true), false);
    }
    else {
      
      NS_ENSURE_TRUE(SerializeAttr(xmlnsStr, aTagPrefix, aTagNamespaceURI, aStr, true), false);
    }
    PushNameSpaceDecl(aTagPrefix, aTagNamespaceURI, aOriginalElement);
  }

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
    bool isJS = IsJavaScript(aContent, attrName, namespaceID, valueStr);

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

NS_IMETHODIMP 
nsXMLContentSerializer::AppendElementStart(Element* aElement,
                                           Element* aOriginalElement,
                                           nsAString& aStr)
{
  NS_ENSURE_ARG(aElement);

  nsIContent* content = aElement;

  bool forceFormat = false;
  nsresult rv = NS_OK;
  if (!CheckElementStart(content, forceFormat, aStr, rv)) {
    return rv;
  }

  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString tagPrefix, tagLocalName, tagNamespaceURI;
  aElement->NodeInfo()->GetPrefix(tagPrefix);
  aElement->NodeInfo()->GetName(tagLocalName);
  aElement->NodeInfo()->GetNamespaceURI(tagNamespaceURI);

  uint32_t skipAttr = ScanNamespaceDeclarations(content,
                          aOriginalElement, tagNamespaceURI);

  nsIAtom *name = content->NodeInfo()->NameAtom();
  bool lineBreakBeforeOpen = LineBreakBeforeOpen(content->GetNameSpaceID(), name);

  if ((mDoFormat || forceFormat) && !mDoRaw && !PreLevel()) {
    if (mColPos && lineBreakBeforeOpen) {
      NS_ENSURE_TRUE(AppendNewLineToString(aStr), NS_ERROR_OUT_OF_MEMORY);
    }
    else {
      NS_ENSURE_TRUE(MaybeAddNewlineForRootNode(aStr), NS_ERROR_OUT_OF_MEMORY);
    }
    if (!mColPos) {
      NS_ENSURE_TRUE(AppendIndentation(aStr), NS_ERROR_OUT_OF_MEMORY);
    }
    else if (mAddSpace) {
      NS_ENSURE_TRUE(AppendToString(char16_t(' '), aStr), NS_ERROR_OUT_OF_MEMORY);
      mAddSpace = false;
    }
  }
  else if (mAddSpace) {
    NS_ENSURE_TRUE(AppendToString(char16_t(' '), aStr), NS_ERROR_OUT_OF_MEMORY);
    mAddSpace = false;
  }
  else {
    NS_ENSURE_TRUE(MaybeAddNewlineForRootNode(aStr), NS_ERROR_OUT_OF_MEMORY);
  }

  
  
  mAddNewlineForRootNode = false;

  bool addNSAttr;
  addNSAttr = ConfirmPrefix(tagPrefix, tagNamespaceURI, aOriginalElement,
                            false);

  
  NS_ENSURE_TRUE(AppendToString(kLessThan, aStr), NS_ERROR_OUT_OF_MEMORY);
  if (!tagPrefix.IsEmpty()) {
    NS_ENSURE_TRUE(AppendToString(tagPrefix, aStr), NS_ERROR_OUT_OF_MEMORY);
    NS_ENSURE_TRUE(AppendToString(NS_LITERAL_STRING(":"), aStr), NS_ERROR_OUT_OF_MEMORY);
  }
  NS_ENSURE_TRUE(AppendToString(tagLocalName, aStr), NS_ERROR_OUT_OF_MEMORY);

  MaybeEnterInPreContent(content);

  if ((mDoFormat || forceFormat) && !mDoRaw && !PreLevel()) {
    NS_ENSURE_TRUE(IncrIndentation(name), NS_ERROR_OUT_OF_MEMORY);
  }

  NS_ENSURE_TRUE(SerializeAttributes(content, aOriginalElement, tagPrefix, tagNamespaceURI,
                                     name, aStr, skipAttr, addNSAttr),
                 NS_ERROR_OUT_OF_MEMORY);

  NS_ENSURE_TRUE(AppendEndOfElementStart(aOriginalElement, name,
                                         content->GetNameSpaceID(), aStr),
                 NS_ERROR_OUT_OF_MEMORY);

  if ((mDoFormat || forceFormat) && !mDoRaw && !PreLevel()
    && LineBreakAfterOpen(content->GetNameSpaceID(), name)) {
    NS_ENSURE_TRUE(AppendNewLineToString(aStr), NS_ERROR_OUT_OF_MEMORY);
  }

  NS_ENSURE_TRUE(AfterElementStart(content, aOriginalElement, aStr), NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

bool
nsXMLContentSerializer::AppendEndOfElementStart(nsIContent *aOriginalElement,
                                                nsIAtom * aName,
                                                int32_t aNamespaceID,
                                                nsAString& aStr)
{
  
  if (!aOriginalElement->GetChildCount()) {
    return AppendToString(NS_LITERAL_STRING("/>"), aStr);
  }
  else {
    return AppendToString(kGreaterThan, aStr);
  }
}

NS_IMETHODIMP 
nsXMLContentSerializer::AppendElementEnd(Element* aElement,
                                         nsAString& aStr)
{
  NS_ENSURE_ARG(aElement);

  nsIContent* content = aElement;

  bool forceFormat = false, outputElementEnd;
  outputElementEnd = CheckElementEnd(content, forceFormat, aStr);

  nsIAtom *name = content->NodeInfo()->NameAtom();

  if ((mDoFormat || forceFormat) && !mDoRaw && !PreLevel()) {
    DecrIndentation(name);
  }

  if (!outputElementEnd) {
    PopNameSpaceDeclsFor(aElement);
    MaybeFlagNewlineForRootNode(aElement);
    return NS_OK;
  }

  nsAutoString tagPrefix, tagLocalName, tagNamespaceURI;
  
  aElement->NodeInfo()->GetPrefix(tagPrefix);
  aElement->NodeInfo()->GetName(tagLocalName);
  aElement->NodeInfo()->GetNamespaceURI(tagNamespaceURI);

#ifdef DEBUG
  bool debugNeedToPushNamespace =
#endif
  ConfirmPrefix(tagPrefix, tagNamespaceURI, aElement, false);
  NS_ASSERTION(!debugNeedToPushNamespace, "Can't push namespaces in closing tag!");

  if ((mDoFormat || forceFormat) && !mDoRaw && !PreLevel()) {

    bool lineBreakBeforeClose = LineBreakBeforeClose(content->GetNameSpaceID(), name);

    if (mColPos && lineBreakBeforeClose) {
      NS_ENSURE_TRUE(AppendNewLineToString(aStr), NS_ERROR_OUT_OF_MEMORY);
    }
    if (!mColPos) {
      NS_ENSURE_TRUE(AppendIndentation(aStr), NS_ERROR_OUT_OF_MEMORY);
    }
    else if (mAddSpace) {
      NS_ENSURE_TRUE(AppendToString(char16_t(' '), aStr), NS_ERROR_OUT_OF_MEMORY);
      mAddSpace = false;
    }
  }
  else if (mAddSpace) {
    NS_ENSURE_TRUE(AppendToString(char16_t(' '), aStr), NS_ERROR_OUT_OF_MEMORY);
    mAddSpace = false;
  }

  NS_ENSURE_TRUE(AppendToString(kEndTag, aStr), NS_ERROR_OUT_OF_MEMORY);
  if (!tagPrefix.IsEmpty()) {
    NS_ENSURE_TRUE(AppendToString(tagPrefix, aStr), NS_ERROR_OUT_OF_MEMORY);
    NS_ENSURE_TRUE(AppendToString(NS_LITERAL_STRING(":"), aStr), NS_ERROR_OUT_OF_MEMORY);
  }
  NS_ENSURE_TRUE(AppendToString(tagLocalName, aStr), NS_ERROR_OUT_OF_MEMORY);
  NS_ENSURE_TRUE(AppendToString(kGreaterThan, aStr), NS_ERROR_OUT_OF_MEMORY);

  PopNameSpaceDeclsFor(aElement);

  MaybeLeaveFromPreContent(content);

  if ((mDoFormat || forceFormat) && !mDoRaw && !PreLevel()
      && LineBreakAfterClose(content->GetNameSpaceID(), name)) {
    NS_ENSURE_TRUE(AppendNewLineToString(aStr), NS_ERROR_OUT_OF_MEMORY);
  }
  else {
    MaybeFlagNewlineForRootNode(aElement);
  }

  AfterElementEnd(content, aStr);

  return NS_OK;
}

NS_IMETHODIMP
nsXMLContentSerializer::AppendDocumentStart(nsIDocument *aDocument,
                                            nsAString& aStr)
{
  NS_ENSURE_ARG_POINTER(aDocument);

  nsAutoString version, encoding, standalone;
  aDocument->GetXMLDeclaration(version, encoding, standalone);

  if (version.IsEmpty())
    return NS_OK; 

  NS_NAMED_LITERAL_STRING(endQuote, "\"");

  aStr += NS_LITERAL_STRING("<?xml version=\"") + version + endQuote;
  
  if (!mCharset.IsEmpty()) {
    aStr += NS_LITERAL_STRING(" encoding=\"") +
      NS_ConvertASCIItoUTF16(mCharset) + endQuote;
  }
  
  
#ifdef DEBUG
  else {
    NS_WARNING("Empty mCharset?  How come?");
  }
#endif

  if (!standalone.IsEmpty()) {
    aStr += NS_LITERAL_STRING(" standalone=\"") + standalone + endQuote;
  }

  NS_ENSURE_TRUE(aStr.AppendLiteral("?>", mozilla::fallible), NS_ERROR_OUT_OF_MEMORY);
  mAddNewlineForRootNode = true;

  return NS_OK;
}

bool
nsXMLContentSerializer::CheckElementStart(nsIContent * aContent,
                                          bool & aForceFormat,
                                          nsAString& aStr,
                                          nsresult& aResult)
{
  aResult = NS_OK;
  aForceFormat = false;
  return true;
}

bool
nsXMLContentSerializer::CheckElementEnd(nsIContent * aContent,
                                        bool & aForceFormat,
                                        nsAString& aStr)
{
  
  aForceFormat = false;
  return aContent->GetChildCount() > 0;
}

bool
nsXMLContentSerializer::AppendToString(const char16_t aChar,
                                       nsAString& aOutputStr)
{
  if (mBodyOnly && !mInBody) {
    return true;
  }
  mColPos += 1;
  return aOutputStr.Append(aChar, mozilla::fallible);
}

bool
nsXMLContentSerializer::AppendToString(const nsAString& aStr,
                                       nsAString& aOutputStr)
{
  if (mBodyOnly && !mInBody) {
    return true;
  }
  mColPos += aStr.Length();
  return aOutputStr.Append(aStr, mozilla::fallible);
}


static const uint16_t kGTVal = 62;
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

bool
nsXMLContentSerializer::AppendAndTranslateEntities(const nsAString& aStr,
                                                   nsAString& aOutputStr)
{
  nsReadingIterator<char16_t> done_reading;
  aStr.EndReading(done_reading);

  
  uint32_t advanceLength = 0;
  nsReadingIterator<char16_t> iter;

  const char **entityTable = mInAttribute ? kAttrEntities : kEntities;

  for (aStr.BeginReading(iter);
       iter != done_reading;
       iter.advance(int32_t(advanceLength))) {
    uint32_t fragmentLength = iter.size_forward();
    const char16_t* c = iter.get();
    const char16_t* fragmentStart = c;
    const char16_t* fragmentEnd = c + fragmentLength;
    const char* entityText = nullptr;

    advanceLength = 0;
    
    
    for (; c < fragmentEnd; c++, advanceLength++) {
      char16_t val = *c;
      if ((val <= kGTVal) && (entityTable[val][0] != 0)) {
        entityText = entityTable[val];
        break;
      }
    }

    NS_ENSURE_TRUE(aOutputStr.Append(fragmentStart, advanceLength, mozilla::fallible), false);
    if (entityText) {
      NS_ENSURE_TRUE(AppendASCIItoUTF16(entityText, aOutputStr, mozilla::fallible), false);
      advanceLength++;
    }
  }

  return true;
}

bool
nsXMLContentSerializer::MaybeAddNewlineForRootNode(nsAString& aStr)
{
  if (mAddNewlineForRootNode) {
    return AppendNewLineToString(aStr);
  }

  return true;
}

void
nsXMLContentSerializer::MaybeFlagNewlineForRootNode(nsINode* aNode)
{
  nsINode* parent = aNode->GetParentNode();
  if (parent) {
    mAddNewlineForRootNode = parent->IsNodeOfType(nsINode::eDOCUMENT);
  }
}

void
nsXMLContentSerializer::MaybeEnterInPreContent(nsIContent* aNode)
{
  
  if (ShouldMaintainPreLevel() &&
      aNode->HasAttr(kNameSpaceID_XML, nsGkAtoms::space)) {
    nsAutoString space;
    aNode->GetAttr(kNameSpaceID_XML, nsGkAtoms::space, space);
    if (space.EqualsLiteral("preserve"))
      ++PreLevel();
  }
}

void
nsXMLContentSerializer::MaybeLeaveFromPreContent(nsIContent* aNode)
{
  
  if (ShouldMaintainPreLevel() &&
      aNode->HasAttr(kNameSpaceID_XML, nsGkAtoms::space)) {
    nsAutoString space;
    aNode->GetAttr(kNameSpaceID_XML, nsGkAtoms::space, space);
    if (space.EqualsLiteral("preserve"))
      --PreLevel();
  }
}

bool
nsXMLContentSerializer::AppendNewLineToString(nsAString& aStr)
{
  bool result = AppendToString(mLineBreak, aStr);
  mMayIgnoreLineBreakSequence = true;
  mColPos = 0;
  mAddSpace = false;
  mIsIndentationAddedOnCurrentLine = false;
  return result;
}

bool
nsXMLContentSerializer::AppendIndentation(nsAString& aStr)
{
  mIsIndentationAddedOnCurrentLine = true;
  bool result = AppendToString(mIndent, aStr);
  mAddSpace = false;
  mMayIgnoreLineBreakSequence = false;
  return result;
}

bool
nsXMLContentSerializer::IncrIndentation(nsIAtom* aName)
{
  
  if (mDoWrap &&
      mIndent.Length() >= uint32_t(mMaxColumn) - MIN_INDENTED_LINE_LENGTH) {
    ++mIndentOverflow;
  }
  else {
    return mIndent.AppendLiteral(INDENT_STRING, mozilla::fallible);
  }

  return true;
}

void
nsXMLContentSerializer::DecrIndentation(nsIAtom* aName)
{
  if(mIndentOverflow)
    --mIndentOverflow;
  else
    mIndent.Cut(0, INDENT_STRING_LENGTH);
}

bool
nsXMLContentSerializer::LineBreakBeforeOpen(int32_t aNamespaceID, nsIAtom* aName)
{
  return mAddSpace;
}

bool 
nsXMLContentSerializer::LineBreakAfterOpen(int32_t aNamespaceID, nsIAtom* aName)
{
  return false;
}

bool 
nsXMLContentSerializer::LineBreakBeforeClose(int32_t aNamespaceID, nsIAtom* aName)
{
  return mAddSpace;
}

bool 
nsXMLContentSerializer::LineBreakAfterClose(int32_t aNamespaceID, nsIAtom* aName)
{
  return false;
}

bool
nsXMLContentSerializer::AppendToStringConvertLF(const nsAString& aStr,
                                                nsAString& aOutputStr)
{
  if (mBodyOnly && !mInBody) {
    return true;
  }

  if (mDoRaw) {
    NS_ENSURE_TRUE(AppendToString(aStr, aOutputStr), false);
  }
  else {
    
    uint32_t start = 0;
    uint32_t theLen = aStr.Length();
    while (start < theLen) {
      int32_t eol = aStr.FindChar('\n', start);
      if (eol == kNotFound) {
        nsDependentSubstring dataSubstring(aStr, start, theLen - start);
        NS_ENSURE_TRUE(AppendToString(dataSubstring, aOutputStr), false);
        start = theLen;
        
        
        
        mMayIgnoreLineBreakSequence = false;
      }
      else {
        nsDependentSubstring dataSubstring(aStr, start, eol - start);
        NS_ENSURE_TRUE(AppendToString(dataSubstring, aOutputStr), false);
        NS_ENSURE_TRUE(AppendNewLineToString(aOutputStr), false);
        start = eol + 1;
      }
    }
  }

  return true;
}

bool
nsXMLContentSerializer::AppendFormatedWrapped_WhitespaceSequence(
                        nsASingleFragmentString::const_char_iterator &aPos,
                        const nsASingleFragmentString::const_char_iterator aEnd,
                        const nsASingleFragmentString::const_char_iterator aSequenceStart,
                        bool &aMayIgnoreStartOfLineWhitespaceSequence,
                        nsAString &aOutputStr)
{
  
  
  
  
  

  bool sawBlankOrTab = false;
  bool leaveLoop = false;

  do {
    switch (*aPos) {
      case ' ':
      case '\t':
        sawBlankOrTab = true;
        
      case '\n':
        ++aPos;
        
        
        break;
      default:
        leaveLoop = true;
        break;
    }
  } while (!leaveLoop && aPos < aEnd);

  if (mAddSpace) {
    
    
  }
  else if (!sawBlankOrTab && mMayIgnoreLineBreakSequence) {
    
    
    
    mMayIgnoreLineBreakSequence = false;
  }
  else if (aMayIgnoreStartOfLineWhitespaceSequence) {
    
    aMayIgnoreStartOfLineWhitespaceSequence = false;
  }
  else {
    if (sawBlankOrTab) {
      if (mDoWrap && mColPos + 1 >= mMaxColumn) {
        
        
        bool result = aOutputStr.Append(mLineBreak, mozilla::fallible);
        mColPos = 0;
        mIsIndentationAddedOnCurrentLine = false;
        mMayIgnoreLineBreakSequence = true;
        NS_ENSURE_TRUE(result, false);
      }
      else {
        
        
        mAddSpace = true;
        ++mColPos; 
      }
    }
    else {
      
      
      
      
      NS_ENSURE_TRUE(AppendNewLineToString(aOutputStr), false);
    }
  }

  return true;
}

bool
nsXMLContentSerializer::AppendWrapped_NonWhitespaceSequence(
                        nsASingleFragmentString::const_char_iterator &aPos,
                        const nsASingleFragmentString::const_char_iterator aEnd,
                        const nsASingleFragmentString::const_char_iterator aSequenceStart,
                        bool &aMayIgnoreStartOfLineWhitespaceSequence,
                        bool &aSequenceStartAfterAWhiteSpace,
                        nsAString& aOutputStr)
{
  mMayIgnoreLineBreakSequence = false;
  aMayIgnoreStartOfLineWhitespaceSequence = false;

  
  
  
  
  

  bool thisSequenceStartsAtBeginningOfLine = !mColPos;
  bool onceAgainBecauseWeAddedBreakInFront = false;
  bool foundWhitespaceInLoop;
  uint32_t length, colPos;

  do {

    if (mColPos) {
      colPos = mColPos;
    }
    else {
      if (mDoFormat && !mDoRaw && !PreLevel() && !onceAgainBecauseWeAddedBreakInFront) {
        colPos = mIndent.Length();
      }
      else
        colPos = 0;
    }
    foundWhitespaceInLoop = false;
    length = 0;
    
    
    
    do {
      if (*aPos == ' ' || *aPos == '\t' || *aPos == '\n') {
        foundWhitespaceInLoop = true;
        break;
      }

      ++aPos;
      ++length;
    } while ( (!mDoWrap || colPos + length < mMaxColumn) && aPos < aEnd);

    
    
    
    
    if (*aPos == ' ' || *aPos == '\t' || *aPos == '\n') {
      foundWhitespaceInLoop = true;
    }

    if (aPos == aEnd || foundWhitespaceInLoop) {
      
      if (mDoFormat && !mColPos) {
        NS_ENSURE_TRUE(AppendIndentation(aOutputStr), false);
      }
      else if (mAddSpace) {
        bool result = aOutputStr.Append(char16_t(' '), mozilla::fallible);
        mAddSpace = false;
        NS_ENSURE_TRUE(result, false);
      }

      mColPos += length;
      NS_ENSURE_TRUE(aOutputStr.Append(aSequenceStart, aPos - aSequenceStart, mozilla::fallible), false);

      
      
      
      
      onceAgainBecauseWeAddedBreakInFront = false;
    }
    else { 
      if (!thisSequenceStartsAtBeginningOfLine &&
          (mAddSpace || (!mDoFormat && aSequenceStartAfterAWhiteSpace))) { 
          
          

        
        

        NS_ENSURE_TRUE(AppendNewLineToString(aOutputStr), false);
        aPos = aSequenceStart;
        thisSequenceStartsAtBeginningOfLine = true;
        onceAgainBecauseWeAddedBreakInFront = true;
      }
      else {
        
        onceAgainBecauseWeAddedBreakInFront = false;
        bool foundWrapPosition = false;
        int32_t wrapPosition;

        nsILineBreaker *lineBreaker = nsContentUtils::LineBreaker();

        wrapPosition = lineBreaker->Prev(aSequenceStart,
                                         (aEnd - aSequenceStart),
                                         (aPos - aSequenceStart) + 1);
        if (wrapPosition != NS_LINEBREAKER_NEED_MORE_TEXT) {
          foundWrapPosition = true;
        }
        else {
          wrapPosition = lineBreaker->Next(aSequenceStart,
                                           (aEnd - aSequenceStart),
                                           (aPos - aSequenceStart));
          if (wrapPosition != NS_LINEBREAKER_NEED_MORE_TEXT) {
            foundWrapPosition = true;
          }
        }

        if (foundWrapPosition) {
          if (!mColPos && mDoFormat) {
            NS_ENSURE_TRUE(AppendIndentation(aOutputStr), false);
          }
          else if (mAddSpace) {
            bool result = aOutputStr.Append(char16_t(' '), mozilla::fallible);
            mAddSpace = false;
            NS_ENSURE_TRUE(result, false);
          }
          NS_ENSURE_TRUE(aOutputStr.Append(aSequenceStart, wrapPosition, mozilla::fallible), false);

          NS_ENSURE_TRUE(AppendNewLineToString(aOutputStr), false);
          aPos = aSequenceStart + wrapPosition;
          aMayIgnoreStartOfLineWhitespaceSequence = true;
        }
        else {
          
          
          

          
          
          mColPos += length;

          
          do {
            if (*aPos == ' ' || *aPos == '\t' || *aPos == '\n') {
              break;
            }

            ++aPos;
            ++mColPos;
          } while (aPos < aEnd);

          if (mAddSpace) {
            bool result = aOutputStr.Append(char16_t(' '), mozilla::fallible);
            mAddSpace = false;
            NS_ENSURE_TRUE(result, false);
          }
          NS_ENSURE_TRUE(aOutputStr.Append(aSequenceStart, aPos - aSequenceStart, mozilla::fallible), false);
        }
      }
      aSequenceStartAfterAWhiteSpace = false;
    }
  } while (onceAgainBecauseWeAddedBreakInFront);

  return true;
}

bool
nsXMLContentSerializer::AppendToStringFormatedWrapped(const nsASingleFragmentString& aStr,
                                                      nsAString& aOutputStr)
{
  if (mBodyOnly && !mInBody) {
    return true;
  }

  nsASingleFragmentString::const_char_iterator pos, end, sequenceStart;

  aStr.BeginReading(pos);
  aStr.EndReading(end);

  bool sequenceStartAfterAWhitespace = false;
  if (pos < end) {
    nsAString::const_char_iterator end2;
    aOutputStr.EndReading(end2);
    --end2;
    if (*end2 == ' ' || *end2 == '\n' || *end2 == '\t') {
      sequenceStartAfterAWhitespace = true;
    }
  }

  
  
  bool mayIgnoreStartOfLineWhitespaceSequence =
    (!mColPos || (mIsIndentationAddedOnCurrentLine &&
                  sequenceStartAfterAWhitespace &&
                  uint32_t(mColPos) == mIndent.Length()));

  while (pos < end) {
    sequenceStart = pos;

    
    if (*pos == ' ' || *pos == '\n' || *pos == '\t') {
      NS_ENSURE_TRUE(AppendFormatedWrapped_WhitespaceSequence(pos, end, sequenceStart,
        mayIgnoreStartOfLineWhitespaceSequence, aOutputStr), false);
    }
    else { 
      NS_ENSURE_TRUE(AppendWrapped_NonWhitespaceSequence(pos, end, sequenceStart,
        mayIgnoreStartOfLineWhitespaceSequence, sequenceStartAfterAWhitespace,
        aOutputStr), false);
    }
  }

  return true;
}

bool
nsXMLContentSerializer::AppendWrapped_WhitespaceSequence(
                        nsASingleFragmentString::const_char_iterator &aPos,
                        const nsASingleFragmentString::const_char_iterator aEnd,
                        const nsASingleFragmentString::const_char_iterator aSequenceStart,
                        nsAString &aOutputStr)
{
  
  
  
  mAddSpace = false;
  mIsIndentationAddedOnCurrentLine = false;

  bool leaveLoop = false;
  nsASingleFragmentString::const_char_iterator lastPos = aPos;

  do {
    switch (*aPos) {
      case ' ':
      case '\t':
        
        if (mColPos >= mMaxColumn) {
          if (lastPos != aPos) {
            NS_ENSURE_TRUE(aOutputStr.Append(lastPos, aPos - lastPos, mozilla::fallible), false);
          }
          NS_ENSURE_TRUE(AppendToString(mLineBreak, aOutputStr), false);
          mColPos = 0;
          lastPos = aPos;
        }

        ++mColPos;
        ++aPos;
        break;
      case '\n':
        if (lastPos != aPos) {
          NS_ENSURE_TRUE(aOutputStr.Append(lastPos, aPos - lastPos, mozilla::fallible), false);
        }
        NS_ENSURE_TRUE(AppendToString(mLineBreak, aOutputStr), false);
        mColPos = 0;
        ++aPos;
        lastPos = aPos;
        break;
      default:
        leaveLoop = true;
        break;
    }
  } while (!leaveLoop && aPos < aEnd);

  if (lastPos != aPos) {
    NS_ENSURE_TRUE(aOutputStr.Append(lastPos, aPos - lastPos, mozilla::fallible), false);
  }

  return true;
}

bool
nsXMLContentSerializer::AppendToStringWrapped(const nsASingleFragmentString& aStr,
                                              nsAString& aOutputStr)
{
  if (mBodyOnly && !mInBody) {
    return true;
  }

  nsASingleFragmentString::const_char_iterator pos, end, sequenceStart;

  aStr.BeginReading(pos);
  aStr.EndReading(end);

  
  bool mayIgnoreStartOfLineWhitespaceSequence = false;
  mMayIgnoreLineBreakSequence = false;

  bool sequenceStartAfterAWhitespace = false;
  if (pos < end && !aOutputStr.IsEmpty()) {
    nsAString::const_char_iterator end2;
    aOutputStr.EndReading(end2);
    --end2;
    if (*end2 == ' ' || *end2 == '\n' || *end2 == '\t') {
      sequenceStartAfterAWhitespace = true;
    }
  }

  while (pos < end) {
    sequenceStart = pos;

    
    if (*pos == ' ' || *pos == '\n' || *pos == '\t') {
      sequenceStartAfterAWhitespace = true;
      NS_ENSURE_TRUE(AppendWrapped_WhitespaceSequence(pos, end,
        sequenceStart, aOutputStr), false);
    }
    else { 
      NS_ENSURE_TRUE(AppendWrapped_NonWhitespaceSequence(pos, end, sequenceStart,
        mayIgnoreStartOfLineWhitespaceSequence,
        sequenceStartAfterAWhitespace, aOutputStr), false);
    }
  }

  return true;
}

bool
nsXMLContentSerializer::ShouldMaintainPreLevel() const
{
  
  return !mDoRaw || (mFlags & nsIDocumentEncoder::OutputNoFormattingInPre);
}
