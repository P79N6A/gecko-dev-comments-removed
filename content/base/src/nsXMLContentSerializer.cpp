











































#include "nsXMLContentSerializer.h"

#include "nsGkAtoms.h"
#include "nsIDOMProcessingInstruction.h"
#include "nsIDOMComment.h"
#include "nsIDOMDocumentType.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIDocumentEncoder.h"
#include "nsINameSpaceManager.h"
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

static const char kMozStr[] = "moz";

#define kXMLNS "xmlns"




#define MIN_INDENTED_LINE_LENGTH 15


#define INDENT_STRING "  "
#define INDENT_STRING_LENGTH 2

nsresult NS_NewXMLContentSerializer(nsIContentSerializer** aSerializer)
{
  nsXMLContentSerializer* it = new nsXMLContentSerializer();
  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return CallQueryInterface(it, aSerializer);
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

NS_IMPL_ISUPPORTS1(nsXMLContentSerializer, nsIContentSerializer)

NS_IMETHODIMP 
nsXMLContentSerializer::Init(PRUint32 aFlags, PRUint32 aWrapColumn,
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
    mLineBreak.AssignLiteral("\r");
  }
  else if (mFlags & nsIDocumentEncoder::OutputLFLineBreak) { 
    mLineBreak.AssignLiteral("\n");
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
                                       PRInt32 aStartOffset,
                                       PRInt32 aEndOffset,
                                       nsAString& aStr,
                                       bool aTranslateEntities)
{
  nsIContent* content = aNode;
  const nsTextFragment* frag;
  if (!content || !(frag = content->GetText())) {
    return NS_ERROR_FAILURE;
  }

  PRInt32 endoffset = (aEndOffset == -1) ? frag->GetLength() : aEndOffset;
  PRInt32 length = endoffset - aStartOffset;

  NS_ASSERTION(aStartOffset >= 0, "Negative start offset for text fragment!");
  NS_ASSERTION(aStartOffset <= endoffset, "A start offset is beyond the end of the text fragment!");

  if (length <= 0) {
    
    
    return NS_OK;
  }
    
  if (frag->Is2b()) {
    const PRUnichar *strStart = frag->Get2b() + aStartOffset;
    if (aTranslateEntities) {
      AppendAndTranslateEntities(Substring(strStart, strStart + length), aStr);
    }
    else {
      aStr.Append(Substring(strStart, strStart + length));
    }
  }
  else {
    if (aTranslateEntities) {
      AppendAndTranslateEntities(NS_ConvertASCIItoUTF16(frag->Get1b()+aStartOffset, length), aStr);
    }
    else {
      aStr.Append(NS_ConvertASCIItoUTF16(frag->Get1b()+aStartOffset, length));
    }
  }

  return NS_OK;
}

NS_IMETHODIMP 
nsXMLContentSerializer::AppendText(nsIContent* aText,
                                   PRInt32 aStartOffset,
                                   PRInt32 aEndOffset,
                                   nsAString& aStr)
{
  NS_ENSURE_ARG(aText);

  nsAutoString data;
  nsresult rv;

  rv = AppendTextData(aText, aStartOffset, aEndOffset, data, true);
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
    AppendToStringConvertLF(data, aStr);
  }

  return NS_OK;
}

NS_IMETHODIMP 
nsXMLContentSerializer::AppendCDATASection(nsIContent* aCDATASection,
                                           PRInt32 aStartOffset,
                                           PRInt32 aEndOffset,
                                           nsAString& aStr)
{
  NS_ENSURE_ARG(aCDATASection);
  nsresult rv;

  NS_NAMED_LITERAL_STRING(cdata , "<![CDATA[");

  if (mPreLevel > 0 || mDoRaw) {
    AppendToString(cdata, aStr);
  }
  else if (mDoFormat) {
    AppendToStringFormatedWrapped(cdata, aStr);
  }
  else if (mDoWrap) {
    AppendToStringWrapped(cdata, aStr);
  }
  else {
    AppendToString(cdata, aStr);
  }

  nsAutoString data;
  rv = AppendTextData(aCDATASection, aStartOffset, aEndOffset, data, false);
  if (NS_FAILED(rv)) return NS_ERROR_FAILURE;

  AppendToStringConvertLF(data, aStr);

  AppendToString(NS_LITERAL_STRING("]]>"), aStr);

  return NS_OK;
}

NS_IMETHODIMP 
nsXMLContentSerializer::AppendProcessingInstruction(nsIContent* aPI,
                                                    PRInt32 aStartOffset,
                                                    PRInt32 aEndOffset,
                                                    nsAString& aStr)
{
  nsCOMPtr<nsIDOMProcessingInstruction> pi = do_QueryInterface(aPI);
  NS_ENSURE_ARG(pi);
  nsresult rv;
  nsAutoString target, data, start;

  MaybeAddNewlineForRootNode(aStr);

  rv = pi->GetTarget(target);
  if (NS_FAILED(rv)) return NS_ERROR_FAILURE;

  rv = pi->GetData(data);
  if (NS_FAILED(rv)) return NS_ERROR_FAILURE;

  start.AppendLiteral("<?");
  start.Append(target);

  if (mPreLevel > 0 || mDoRaw) {
    AppendToString(start, aStr);
  }
  else if (mDoFormat) {
    if (mAddSpace) {
      AppendNewLineToString(aStr);
    }
    AppendToStringFormatedWrapped(start, aStr);
  }
  else if (mDoWrap) {
    AppendToStringWrapped(start, aStr);
  }
  else {
    AppendToString(start, aStr);
  }

  if (!data.IsEmpty()) {
    AppendToString(PRUnichar(' '), aStr);
    AppendToStringConvertLF(data, aStr);
  }
  AppendToString(NS_LITERAL_STRING("?>"), aStr);

  MaybeFlagNewlineForRootNode(aPI);

  return NS_OK;
}

NS_IMETHODIMP 
nsXMLContentSerializer::AppendComment(nsIContent* aComment,
                                      PRInt32 aStartOffset,
                                      PRInt32 aEndOffset,
                                      nsAString& aStr)
{
  nsCOMPtr<nsIDOMComment> comment = do_QueryInterface(aComment);
  NS_ENSURE_ARG(comment);
  nsresult rv;
  nsAutoString data;

  rv = comment->GetData(data);
  if (NS_FAILED(rv)) return NS_ERROR_FAILURE;

  if (aStartOffset || (aEndOffset != -1)) {
    PRInt32 length = (aEndOffset == -1) ? data.Length() : aEndOffset;
    length -= aStartOffset;

    nsAutoString frag;
    data.Mid(frag, aStartOffset, length);
    data.Assign(frag);
  }

  MaybeAddNewlineForRootNode(aStr);

  NS_NAMED_LITERAL_STRING(startComment, "<!--");

  if (mPreLevel > 0 || mDoRaw) {
    AppendToString(startComment, aStr);
  }
  else if (mDoFormat) {
    if (mAddSpace) {
      AppendNewLineToString(aStr);
    }
    AppendToStringFormatedWrapped(startComment, aStr);
  }
  else if (mDoWrap) {
    AppendToStringWrapped(startComment, aStr);
  }
  else {
    AppendToString(startComment, aStr);
  }

  
  
  AppendToStringConvertLF(data, aStr);
  AppendToString(NS_LITERAL_STRING("-->"), aStr);

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

  MaybeAddNewlineForRootNode(aStr);

  AppendToString(NS_LITERAL_STRING("<!DOCTYPE "), aStr);
  AppendToString(name, aStr);

  PRUnichar quote;
  if (!publicId.IsEmpty()) {
    AppendToString(NS_LITERAL_STRING(" PUBLIC "), aStr);
    if (publicId.FindChar(PRUnichar('"')) == -1) {
      quote = PRUnichar('"');
    }
    else {
      quote = PRUnichar('\'');
    }
    AppendToString(quote, aStr);
    AppendToString(publicId, aStr);
    AppendToString(quote, aStr);

    if (!systemId.IsEmpty()) {
      AppendToString(PRUnichar(' '), aStr);
      if (systemId.FindChar(PRUnichar('"')) == -1) {
        quote = PRUnichar('"');
      }
      else {
        quote = PRUnichar('\'');
      }
      AppendToString(quote, aStr);
      AppendToString(systemId, aStr);
      AppendToString(quote, aStr);
    }
  }
  else if (!systemId.IsEmpty()) {
    if (systemId.FindChar(PRUnichar('"')) == -1) {
      quote = PRUnichar('"');
    }
    else {
      quote = PRUnichar('\'');
    }
    AppendToString(NS_LITERAL_STRING(" SYSTEM "), aStr);
    AppendToString(quote, aStr);
    AppendToString(systemId, aStr);
    AppendToString(quote, aStr);
  }
  
  if (!internalSubset.IsEmpty()) {
    AppendToString(NS_LITERAL_STRING(" ["), aStr);
    AppendToString(internalSubset, aStr);
    AppendToString(PRUnichar(']'), aStr);
  }
    
  AppendToString(kGreaterThan, aStr);
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
  PRInt32 index, count;

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

  PRInt32 count = mNameSpaceStack.Length();
  PRInt32 index = count - 1;
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
      PRInt32 index2;
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
  aPrefix.AssignLiteral("a");
  char buf[128];
  PR_snprintf(buf, sizeof(buf), "%d", mPrefixIndex++);
  AppendASCIItoUTF16(buf, aPrefix);
}

void
nsXMLContentSerializer::SerializeAttr(const nsAString& aPrefix,
                                      const nsAString& aName,
                                      const nsAString& aValue,
                                      nsAString& aStr,
                                      bool aDoEscapeEntities)
{
  nsAutoString attrString_;
  
  
  bool rawAppend = mDoRaw && aDoEscapeEntities;
  nsAString& attrString = (rawAppend) ? aStr : attrString_;

  attrString.Append(PRUnichar(' '));
  if (!aPrefix.IsEmpty()) {
    attrString.Append(aPrefix);
    attrString.Append(PRUnichar(':'));
  }
  attrString.Append(aName);

  if (aDoEscapeEntities) {
    
    
    attrString.AppendLiteral("=\"");

    mInAttribute = true;
    AppendAndTranslateEntities(aValue, attrString);
    mInAttribute = false;

    attrString.Append(PRUnichar('"'));
    if (rawAppend) {
      return;
    }
  }
  else {
    
    
    
    
    
    
    
    bool bIncludesSingle = false;
    bool bIncludesDouble = false;
    nsAString::const_iterator iCurr, iEnd;
    PRUint32 uiSize, i;
    aValue.BeginReading(iCurr);
    aValue.EndReading(iEnd);
    for ( ; iCurr != iEnd; iCurr.advance(uiSize) ) {
      const PRUnichar * buf = iCurr.get();
      uiSize = iCurr.size_forward();
      for ( i = 0; i < uiSize; i++, buf++ ) {
        if ( *buf == PRUnichar('\'') )
        {
          bIncludesSingle = true;
          if ( bIncludesDouble ) break;
        }
        else if ( *buf == PRUnichar('"') )
        {
          bIncludesDouble = true;
          if ( bIncludesSingle ) break;
        }
      }
      
      if ( bIncludesDouble && bIncludesSingle ) break;
    }

    
    
    
    
    
    
    PRUnichar cDelimiter = 
        (bIncludesDouble && !bIncludesSingle) ? PRUnichar('\'') : PRUnichar('"');
    attrString.Append(PRUnichar('='));
    attrString.Append(cDelimiter);
    nsAutoString sValue(aValue);
    sValue.ReplaceSubstring(NS_LITERAL_STRING("&"),
                            NS_LITERAL_STRING("&amp;"));
    if (bIncludesDouble && bIncludesSingle) {
      sValue.ReplaceSubstring(NS_LITERAL_STRING("\""),
                              NS_LITERAL_STRING("&quot;"));
    }
    attrString.Append(sValue);
    attrString.Append(cDelimiter);
  }
  if (mPreLevel > 0 || mDoRaw) {
    AppendToStringConvertLF(attrString, aStr);
  }
  else if (mDoFormat) {
    AppendToStringFormatedWrapped(attrString, aStr);
  }
  else if (mDoWrap) {
    AppendToStringWrapped(attrString, aStr);
  }
  else {
    AppendToStringConvertLF(attrString, aStr);
  }
}

PRUint32 
nsXMLContentSerializer::ScanNamespaceDeclarations(nsIContent* aContent,
                                                  nsIContent *aOriginalElement,
                                                  const nsAString& aTagNamespaceURI)
{
  PRUint32 index, count;
  nsAutoString uriStr, valueStr;

  count = aContent->GetAttrCount();

  
  PRUint32 skipAttr = count;
  for (index = 0; index < count; index++) {
    
    const nsAttrName* name = aContent->GetAttrNameAt(index);
    PRInt32 namespaceID = name->NamespaceID();
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
                                     PRInt32 aAttrNamespaceID, const nsAString& aValueString)
{
  PRInt32 namespaceID = aContent->GetNameSpaceID();
  bool isHtml = aContent->IsHTML();

  if (aAttrNamespaceID == kNameSpaceID_None &&
      (isHtml ||
       namespaceID == kNameSpaceID_XUL ||
       namespaceID == kNameSpaceID_SVG) &&
      (aAttrNameAtom == nsGkAtoms::href ||
       aAttrNameAtom == nsGkAtoms::src)) {

    static const char kJavaScript[] = "javascript";
    PRInt32 pos = aValueString.FindChar(':');
    if (pos < (PRInt32)(sizeof kJavaScript - 1))
        return false;
    nsAutoString scheme(Substring(aValueString, 0, pos));
    scheme.StripWhitespace();
    if ((scheme.Length() == (sizeof kJavaScript - 1)) &&
        scheme.EqualsIgnoreCase(kJavaScript))
      return true;
    else
      return false;
  }

  if (isHtml) {
    return nsContentUtils::IsEventAttributeName(aAttrNameAtom, EventNameType_HTML);
  }
  else if (namespaceID == kNameSpaceID_XUL) {
    return nsContentUtils::IsEventAttributeName(aAttrNameAtom, EventNameType_XUL);
  }
  else if (namespaceID == kNameSpaceID_SVG) {
    return nsContentUtils::IsEventAttributeName(aAttrNameAtom,
                                                EventNameType_SVGGraphic | EventNameType_SVGSVG);
  }
  return false;
}


void 
nsXMLContentSerializer::SerializeAttributes(nsIContent* aContent,
                                            nsIContent *aOriginalElement,
                                            nsAString& aTagPrefix,
                                            const nsAString& aTagNamespaceURI,
                                            nsIAtom* aTagName,
                                            nsAString& aStr,
                                            PRUint32 aSkipAttr,
                                            bool aAddNSAttr)
{

  nsAutoString prefixStr, uriStr, valueStr;
  nsAutoString xmlnsStr;
  xmlnsStr.AssignLiteral(kXMLNS);
  PRUint32 index, count;

  
  
  if (aAddNSAttr) {
    if (aTagPrefix.IsEmpty()) {
      
      SerializeAttr(EmptyString(), xmlnsStr, aTagNamespaceURI, aStr, true);
    }
    else {
      
      SerializeAttr(xmlnsStr, aTagPrefix, aTagNamespaceURI, aStr, true);
    }
    PushNameSpaceDecl(aTagPrefix, aTagNamespaceURI, aOriginalElement);
  }

  count = aContent->GetAttrCount();

  
  
  
  for (index = 0; index < count; index++) {
    if (aSkipAttr == index) {
        continue;
    }

    const nsAttrName* name = aContent->GetAttrNameAt(index);
    PRInt32 namespaceID = name->NamespaceID();
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

    SerializeAttr(prefixStr, nameStr, valueStr, aStr, !isJS);
    
    if (addNSAttr) {
      NS_ASSERTION(!prefixStr.IsEmpty(),
                   "Namespaced attributes must have a prefix");
      SerializeAttr(xmlnsStr, prefixStr, uriStr, aStr, true);
      PushNameSpaceDecl(prefixStr, uriStr, aOriginalElement);
    }
  }
}

NS_IMETHODIMP 
nsXMLContentSerializer::AppendElementStart(Element* aElement,
                                           Element* aOriginalElement,
                                           nsAString& aStr)
{
  NS_ENSURE_ARG(aElement);

  nsIContent* content = aElement;

  bool forceFormat = false;
  if (!CheckElementStart(content, forceFormat, aStr)) {
    return NS_OK;
  }

  nsAutoString tagPrefix, tagLocalName, tagNamespaceURI;
  aElement->NodeInfo()->GetPrefix(tagPrefix);
  aElement->NodeInfo()->GetName(tagLocalName);
  aElement->NodeInfo()->GetNamespaceURI(tagNamespaceURI);

  PRUint32 skipAttr = ScanNamespaceDeclarations(content,
                          aOriginalElement, tagNamespaceURI);

  nsIAtom *name = content->Tag();
  bool lineBreakBeforeOpen = LineBreakBeforeOpen(content->GetNameSpaceID(), name);

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
      mAddSpace = false;
    }
  }
  else if (mAddSpace) {
    AppendToString(PRUnichar(' '), aStr);
    mAddSpace = false;
  }
  else {
    MaybeAddNewlineForRootNode(aStr);
  }

  
  
  mAddNewlineForRootNode = false;

  bool addNSAttr;
  addNSAttr = ConfirmPrefix(tagPrefix, tagNamespaceURI, aOriginalElement,
                            false);

  
  AppendToString(kLessThan, aStr);
  if (!tagPrefix.IsEmpty()) {
    AppendToString(tagPrefix, aStr);
    AppendToString(NS_LITERAL_STRING(":"), aStr);
  }
  AppendToString(tagLocalName, aStr);

  MaybeEnterInPreContent(content);

  if ((mDoFormat || forceFormat) && !mPreLevel && !mDoRaw) {
    IncrIndentation(name);
  }

  SerializeAttributes(content, aOriginalElement, tagPrefix, tagNamespaceURI,
                      name, aStr, skipAttr, addNSAttr);

  AppendEndOfElementStart(aOriginalElement, name, content->GetNameSpaceID(),
                          aStr);

  if ((mDoFormat || forceFormat) && !mPreLevel 
    && !mDoRaw && LineBreakAfterOpen(content->GetNameSpaceID(), name)) {
    AppendNewLineToString(aStr);
  }

  AfterElementStart(content, aOriginalElement, aStr);

  return NS_OK;
}

void 
nsXMLContentSerializer::AppendEndOfElementStart(nsIContent *aOriginalElement,
                                                nsIAtom * aName,
                                                PRInt32 aNamespaceID,
                                                nsAString& aStr)
{
  
  if (!aOriginalElement->GetChildCount()) {
    AppendToString(NS_LITERAL_STRING("/>"), aStr);
  }
  else {
    AppendToString(kGreaterThan, aStr);
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

  nsIAtom *name = content->Tag();

  if ((mDoFormat || forceFormat) && !mPreLevel && !mDoRaw) {
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

  if ((mDoFormat || forceFormat) && !mPreLevel && !mDoRaw) {

    bool lineBreakBeforeClose = LineBreakBeforeClose(content->GetNameSpaceID(), name);

    if (mColPos && lineBreakBeforeClose) {
      AppendNewLineToString(aStr);
    }
    if (!mColPos) {
      AppendIndentation(aStr);
    }
    else if (mAddSpace) {
      AppendToString(PRUnichar(' '), aStr);
      mAddSpace = false;
    }
  }
  else if (mAddSpace) {
    AppendToString(PRUnichar(' '), aStr);
    mAddSpace = false;
  }

  AppendToString(kEndTag, aStr);
  if (!tagPrefix.IsEmpty()) {
    AppendToString(tagPrefix, aStr);
    AppendToString(NS_LITERAL_STRING(":"), aStr);
  }
  AppendToString(tagLocalName, aStr);
  AppendToString(kGreaterThan, aStr);

  PopNameSpaceDeclsFor(aElement);

  MaybeLeaveFromPreContent(content);

  if ((mDoFormat || forceFormat) && !mPreLevel
      && !mDoRaw && LineBreakAfterClose(content->GetNameSpaceID(), name)) {
    AppendNewLineToString(aStr);
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

  aStr.AppendLiteral("?>");
  mAddNewlineForRootNode = true;

  return NS_OK;
}

bool
nsXMLContentSerializer::CheckElementStart(nsIContent * aContent,
                                          bool & aForceFormat,
                                          nsAString& aStr)
{
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

void
nsXMLContentSerializer::AppendToString(const PRUnichar* aStr,
                                       PRInt32 aLength,
                                       nsAString& aOutputStr)
{
  if (mBodyOnly && !mInBody) {
    return;
  }
  PRInt32 length = (aLength == -1) ? nsCRT::strlen(aStr) : aLength;

  mColPos += length;

  aOutputStr.Append(aStr, length);
}

void 
nsXMLContentSerializer::AppendToString(const PRUnichar aChar,
                                       nsAString& aOutputStr)
{
  if (mBodyOnly && !mInBody) {
    return;
  }
  mColPos += 1;
  aOutputStr.Append(aChar);
}

void
nsXMLContentSerializer::AppendToString(const nsAString& aStr,
                                       nsAString& aOutputStr)
{
  if (mBodyOnly && !mInBody) {
    return;
  }
  mColPos += aStr.Length();
  aOutputStr.Append(aStr);
}


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
nsXMLContentSerializer::AppendAndTranslateEntities(const nsAString& aStr,
                                                   nsAString& aOutputStr)
{
  nsReadingIterator<PRUnichar> done_reading;
  aStr.EndReading(done_reading);

  
  PRUint32 advanceLength = 0;
  nsReadingIterator<PRUnichar> iter;

  const char **entityTable = mInAttribute ? kAttrEntities : kEntities;

  for (aStr.BeginReading(iter);
       iter != done_reading;
       iter.advance(PRInt32(advanceLength))) {
    PRUint32 fragmentLength = iter.size_forward();
    const PRUnichar* c = iter.get();
    const PRUnichar* fragmentStart = c;
    const PRUnichar* fragmentEnd = c + fragmentLength;
    const char* entityText = nsnull;

    advanceLength = 0;
    
    
    for (; c < fragmentEnd; c++, advanceLength++) {
      PRUnichar val = *c;
      if ((val <= kGTVal) && (entityTable[val][0] != 0)) {
        entityText = entityTable[val];
        break;
      }
    }

    aOutputStr.Append(fragmentStart, advanceLength);
    if (entityText) {
      AppendASCIItoUTF16(entityText, aOutputStr);
      advanceLength++;
    }
  }
}

void
nsXMLContentSerializer::MaybeAddNewlineForRootNode(nsAString& aStr)
{
  if (mAddNewlineForRootNode) {
    AppendNewLineToString(aStr);
  }
}

void
nsXMLContentSerializer::MaybeFlagNewlineForRootNode(nsINode* aNode)
{
  nsINode* parent = aNode->GetNodeParent();
  if (parent) {
    mAddNewlineForRootNode = parent->IsNodeOfType(nsINode::eDOCUMENT);
  }
}

void
nsXMLContentSerializer::MaybeEnterInPreContent(nsIContent* aNode)
{
  
  if (aNode->HasAttr(kNameSpaceID_XML, nsGkAtoms::space)) {
    nsAutoString space;
    aNode->GetAttr(kNameSpaceID_XML, nsGkAtoms::space, space);
    if (space.EqualsLiteral("preserve"))
      ++mPreLevel;
  }
}

void
nsXMLContentSerializer::MaybeLeaveFromPreContent(nsIContent* aNode)
{
  
  if (aNode->HasAttr(kNameSpaceID_XML, nsGkAtoms::space)) {
    nsAutoString space;
    aNode->GetAttr(kNameSpaceID_XML, nsGkAtoms::space, space);
    if (space.EqualsLiteral("preserve"))
      --mPreLevel;
  }
}

void
nsXMLContentSerializer::AppendNewLineToString(nsAString& aStr)
{
  AppendToString(mLineBreak, aStr);
  mMayIgnoreLineBreakSequence = true;
  mColPos = 0;
  mAddSpace = false;
  mIsIndentationAddedOnCurrentLine = false;
}

void
nsXMLContentSerializer::AppendIndentation(nsAString& aStr)
{
  mIsIndentationAddedOnCurrentLine = true;
  AppendToString(mIndent, aStr);
  mAddSpace = false;
  mMayIgnoreLineBreakSequence = false;
}

void
nsXMLContentSerializer::IncrIndentation(nsIAtom* aName)
{
  
  if (mDoWrap &&
      mIndent.Length() >= PRUint32(mMaxColumn) - MIN_INDENTED_LINE_LENGTH) {
    ++mIndentOverflow;
  }
  else {
    mIndent.AppendLiteral(INDENT_STRING);
  }
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
nsXMLContentSerializer::LineBreakBeforeOpen(PRInt32 aNamespaceID, nsIAtom* aName)
{
  return mAddSpace;
}

bool 
nsXMLContentSerializer::LineBreakAfterOpen(PRInt32 aNamespaceID, nsIAtom* aName)
{
  return false;
}

bool 
nsXMLContentSerializer::LineBreakBeforeClose(PRInt32 aNamespaceID, nsIAtom* aName)
{
  return mAddSpace;
}

bool 
nsXMLContentSerializer::LineBreakAfterClose(PRInt32 aNamespaceID, nsIAtom* aName)
{
  return false;
}

void
nsXMLContentSerializer::AppendToStringConvertLF(const nsAString& aStr,
                                                nsAString& aOutputStr)
{
  if (mBodyOnly && !mInBody) {
    return;
  }

  if (mDoRaw) {
    AppendToString(aStr, aOutputStr);
  }
  else {
    
    PRUint32 start = 0;
    PRUint32 theLen = aStr.Length();
    while (start < theLen) {
      PRInt32 eol = aStr.FindChar('\n', start);
      if (eol == kNotFound) {
        nsDependentSubstring dataSubstring(aStr, start, theLen - start);
        AppendToString(dataSubstring, aOutputStr);
        start = theLen;
        
        
        
        mMayIgnoreLineBreakSequence = false;
      }
      else {
        nsDependentSubstring dataSubstring(aStr, start, eol - start);
        AppendToString(dataSubstring, aOutputStr);
        AppendNewLineToString(aOutputStr);
        start = eol + 1;
      }
    }
  }
}

void
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
        
        
        aOutputStr.Append(mLineBreak);
        mColPos = 0;
        mIsIndentationAddedOnCurrentLine = false;
        mMayIgnoreLineBreakSequence = true;
      }
      else {
        
        
        mAddSpace = true;
        ++mColPos; 
      }
    }
    else {
      
      
      
      
      AppendNewLineToString(aOutputStr);
    }
  }
}

void
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
  PRUint32 length, colPos;

  do {

    if (mColPos) {
      colPos = mColPos;
    }
    else {
      if (mDoFormat && !mPreLevel && !onceAgainBecauseWeAddedBreakInFront) {
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
        AppendIndentation(aOutputStr);
      }
      else if (mAddSpace) {
        aOutputStr.Append(PRUnichar(' '));
        mAddSpace = false;
      }

      mColPos += length;
      aOutputStr.Append(aSequenceStart, aPos - aSequenceStart);

      
      
      
      
      onceAgainBecauseWeAddedBreakInFront = false;
    }
    else { 
      if (!thisSequenceStartsAtBeginningOfLine &&
          (mAddSpace || (!mDoFormat && aSequenceStartAfterAWhiteSpace))) { 
          
          

        
        

        AppendNewLineToString(aOutputStr);
        aPos = aSequenceStart;
        thisSequenceStartsAtBeginningOfLine = true;
        onceAgainBecauseWeAddedBreakInFront = true;
      }
      else {
        
        onceAgainBecauseWeAddedBreakInFront = false;
        bool foundWrapPosition = false;
        PRInt32 wrapPosition;

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
            AppendIndentation(aOutputStr);
          }
          else if (mAddSpace) {
            aOutputStr.Append(PRUnichar(' '));
            mAddSpace = false;
          }
          aOutputStr.Append(aSequenceStart, wrapPosition);

          AppendNewLineToString(aOutputStr);
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
            aOutputStr.Append(PRUnichar(' '));
            mAddSpace = false;
          }
          aOutputStr.Append(aSequenceStart, aPos - aSequenceStart);
        }
      }
      aSequenceStartAfterAWhiteSpace = false;
    }
  } while (onceAgainBecauseWeAddedBreakInFront);
}

void 
nsXMLContentSerializer::AppendToStringFormatedWrapped(const nsASingleFragmentString& aStr,
                                                      nsAString& aOutputStr)
{
  if (mBodyOnly && !mInBody) {
    return;
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
                  PRUint32(mColPos) == mIndent.Length()));

  while (pos < end) {
    sequenceStart = pos;

    
    if (*pos == ' ' || *pos == '\n' || *pos == '\t') {
      AppendFormatedWrapped_WhitespaceSequence(pos, end, sequenceStart,
        mayIgnoreStartOfLineWhitespaceSequence, aOutputStr);
    }
    else { 
      AppendWrapped_NonWhitespaceSequence(pos, end, sequenceStart,
        mayIgnoreStartOfLineWhitespaceSequence, sequenceStartAfterAWhitespace, aOutputStr);
    }
  }
}

void
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
            aOutputStr.Append(lastPos, aPos - lastPos);
          }
          AppendToString(mLineBreak, aOutputStr);
          mColPos = 0;
          lastPos = aPos;
        }

        ++mColPos;
        ++aPos;
        break;
      case '\n':
        if (lastPos != aPos) {
          aOutputStr.Append(lastPos, aPos - lastPos);
        }
        AppendToString(mLineBreak, aOutputStr);
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
    aOutputStr.Append(lastPos, aPos - lastPos);
  }
}

void 
nsXMLContentSerializer::AppendToStringWrapped(const nsASingleFragmentString& aStr,
                                              nsAString& aOutputStr)
{
  if (mBodyOnly && !mInBody) {
    return;
  }

  nsASingleFragmentString::const_char_iterator pos, end, sequenceStart;

  aStr.BeginReading(pos);
  aStr.EndReading(end);

  
  bool mayIgnoreStartOfLineWhitespaceSequence = false;
  mMayIgnoreLineBreakSequence = false;

  bool sequenceStartAfterAWhitespace = false;
  if (pos < end) {
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
      AppendWrapped_WhitespaceSequence(pos, end, sequenceStart, aOutputStr);
    }
    else { 
      AppendWrapped_NonWhitespaceSequence(pos, end, sequenceStart,
        mayIgnoreStartOfLineWhitespaceSequence, sequenceStartAfterAWhitespace, aOutputStr);
    }
  }
}
