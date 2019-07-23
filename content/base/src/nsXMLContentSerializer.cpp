










































#include "nsXMLContentSerializer.h"

#include "nsGkAtoms.h"
#include "nsIDOMText.h"
#include "nsIDOMCDATASection.h"
#include "nsIDOMProcessingInstruction.h"
#include "nsIDOMComment.h"
#include "nsIDOMDocument.h"
#include "nsIDOMDocumentType.h"
#include "nsIDOMElement.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsINameSpaceManager.h"
#include "nsTextFragment.h"
#include "nsString.h"
#include "prprf.h"
#include "nsUnicharUtils.h"
#include "nsCRT.h"
#include "nsContentUtils.h"
#include "nsAttrName.h"

typedef struct {
  nsString mPrefix;
  nsString mURI;
  nsIDOMElement* mOwner;
} NameSpaceDecl;

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
    mInAttribute(PR_FALSE),
    mAddNewline(PR_FALSE)
{
}
 
nsXMLContentSerializer::~nsXMLContentSerializer()
{
}

NS_IMPL_ISUPPORTS1(nsXMLContentSerializer, nsIContentSerializer)

NS_IMETHODIMP 
nsXMLContentSerializer::Init(PRUint32 flags, PRUint32 aWrapColumn,
                             const char* aCharSet, PRBool aIsCopying,
                             PRBool aIsWholeDocument)
{
  mCharset = aCharSet;
  return NS_OK;
}

nsresult
nsXMLContentSerializer::AppendTextData(nsIDOMNode* aNode, 
                                       PRInt32 aStartOffset,
                                       PRInt32 aEndOffset,
                                       nsAString& aStr,
                                       PRBool aTranslateEntities,
                                       PRBool aIncrColumn)
{
  nsCOMPtr<nsIContent> content = do_QueryInterface(aNode);
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
    AppendToString(Substring(strStart, strStart + length), aStr,
                   aTranslateEntities, aIncrColumn);
  }
  else {
    AppendToString(NS_ConvertASCIItoUTF16(frag->Get1b()+aStartOffset, length),
                   aStr, aTranslateEntities, aIncrColumn);
  }

  return NS_OK;
}

NS_IMETHODIMP 
nsXMLContentSerializer::AppendText(nsIDOMText* aText, 
                                   PRInt32 aStartOffset,
                                   PRInt32 aEndOffset,
                                   nsAString& aStr)
{
  NS_ENSURE_ARG(aText);

  return AppendTextData(aText, aStartOffset, aEndOffset, aStr, PR_TRUE, PR_TRUE);
}

NS_IMETHODIMP 
nsXMLContentSerializer::AppendCDATASection(nsIDOMCDATASection* aCDATASection,
                                           PRInt32 aStartOffset,
                                           PRInt32 aEndOffset,
                                           nsAString& aStr)
{
  NS_ENSURE_ARG(aCDATASection);
  nsresult rv;

  AppendToString(NS_LITERAL_STRING("<![CDATA["), aStr);
  rv = AppendTextData(aCDATASection, aStartOffset, aEndOffset, aStr, PR_FALSE, PR_TRUE);
  if (NS_FAILED(rv)) return NS_ERROR_FAILURE;  
  AppendToString(NS_LITERAL_STRING("]]>"), aStr);

  return NS_OK;
}

NS_IMETHODIMP 
nsXMLContentSerializer::AppendProcessingInstruction(nsIDOMProcessingInstruction* aPI,
                                                    PRInt32 aStartOffset,
                                                    PRInt32 aEndOffset,
                                                    nsAString& aStr)
{
  NS_ENSURE_ARG(aPI);
  nsresult rv;
  nsAutoString target, data;

  MaybeAddNewline(aStr);

  rv = aPI->GetTarget(target);
  if (NS_FAILED(rv)) return NS_ERROR_FAILURE;

  rv = aPI->GetData(data);
  if (NS_FAILED(rv)) return NS_ERROR_FAILURE;

  AppendToString(NS_LITERAL_STRING("<?"), aStr);
  AppendToString(target, aStr);
  if (!data.IsEmpty()) {
    AppendToString(NS_LITERAL_STRING(" "), aStr);
    AppendToString(data, aStr);
  }
  AppendToString(NS_LITERAL_STRING("?>"), aStr);
  MaybeFlagNewline(aPI);
  
  return NS_OK;
}

NS_IMETHODIMP 
nsXMLContentSerializer::AppendComment(nsIDOMComment* aComment,
                                      PRInt32 aStartOffset,
                                      PRInt32 aEndOffset,
                                      nsAString& aStr)
{
  NS_ENSURE_ARG(aComment);
  nsresult rv;
  nsAutoString data;

  rv = aComment->GetData(data);
  if (NS_FAILED(rv)) return NS_ERROR_FAILURE;

  MaybeAddNewline(aStr);

  AppendToString(NS_LITERAL_STRING("<!--"), aStr);
  if (aStartOffset || (aEndOffset != -1)) {
    PRInt32 length = (aEndOffset == -1) ? data.Length() : aEndOffset;
    length -= aStartOffset;

    nsAutoString frag;
    data.Mid(frag, aStartOffset, length);
    AppendToString(frag, aStr);
  }
  else {
    AppendToString(data, aStr);
  }
  AppendToString(NS_LITERAL_STRING("-->"), aStr);
  MaybeFlagNewline(aComment);
  
  return NS_OK;
}

NS_IMETHODIMP 
nsXMLContentSerializer::AppendDoctype(nsIDOMDocumentType *aDoctype,
                                      nsAString& aStr)
{
  NS_ENSURE_ARG(aDoctype);
  nsresult rv;
  nsAutoString name, publicId, systemId, internalSubset;

  rv = aDoctype->GetName(name);
  if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
  rv = aDoctype->GetPublicId(publicId);
  if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
  rv = aDoctype->GetSystemId(systemId);
  if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
  rv = aDoctype->GetInternalSubset(internalSubset);
  if (NS_FAILED(rv)) return NS_ERROR_FAILURE;

  MaybeAddNewline(aStr);

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
    
  AppendToString(PRUnichar('>'), aStr);
  MaybeFlagNewline(aDoctype);

  return NS_OK;
}

#define kXMLNS "xmlns"

nsresult
nsXMLContentSerializer::PushNameSpaceDecl(const nsAString& aPrefix,
                                          const nsAString& aURI,
                                          nsIDOMElement* aOwner)
{
  NameSpaceDecl* decl = new NameSpaceDecl();
  if (!decl) return NS_ERROR_OUT_OF_MEMORY;

  decl->mPrefix.Assign(aPrefix);
  decl->mURI.Assign(aURI);
  
  
  decl->mOwner = aOwner;

  mNameSpaceStack.AppendElement((void*)decl);
  return NS_OK;
}

void
nsXMLContentSerializer::PopNameSpaceDeclsFor(nsIDOMElement* aOwner)
{
  PRInt32 index, count;

  count = mNameSpaceStack.Count();
  for (index = count - 1; index >= 0; index--) {
    NameSpaceDecl* decl = (NameSpaceDecl*)mNameSpaceStack.ElementAt(index);
    if (decl->mOwner != aOwner) {
      break;
    }
    mNameSpaceStack.RemoveElementAt(index);
    delete decl;
  }
}

PRBool
nsXMLContentSerializer::ConfirmPrefix(nsAString& aPrefix,
                                      const nsAString& aURI,
                                      nsIDOMElement* aElement,
                                      PRBool aIsAttribute)
{
  if (aPrefix.EqualsLiteral(kXMLNS)) {
    return PR_FALSE;
  }

  if (aURI.EqualsLiteral("http://www.w3.org/XML/1998/namespace")) {
    
    
    aPrefix.AssignLiteral("xml");

    return PR_FALSE;
  }

  PRBool mustHavePrefix;
  if (aIsAttribute) {
    if (aURI.IsEmpty()) {
      
      
      aPrefix.Truncate();
      return PR_FALSE;
    }

    
    mustHavePrefix = PR_TRUE;
  } else {
    
    mustHavePrefix = PR_FALSE;
  }

  
  
  
  nsAutoString closestURIMatch;
  PRBool uriMatch = PR_FALSE;

  
  
  
  PRBool haveSeenOurPrefix = PR_FALSE;

  PRInt32 count = mNameSpaceStack.Count();
  PRInt32 index = count - 1;
  while (index >= 0) {
    NameSpaceDecl* decl = (NameSpaceDecl*)mNameSpaceStack.ElementAt(index);
    
    if (aPrefix.Equals(decl->mPrefix)) {

      
      
      if (!haveSeenOurPrefix && aURI.Equals(decl->mURI)) {
        
        
        uriMatch = PR_TRUE;
        closestURIMatch = aPrefix;
        break;
      }

      haveSeenOurPrefix = PR_TRUE;      

      
      
      
      
      
      
      
      
      
      
      if (!aPrefix.IsEmpty() || decl->mOwner == aElement) {
        NS_ASSERTION(!aURI.IsEmpty(),
                     "Not allowed to add a xmlns attribute with an empty "
                     "namespace name unless it declares the default "
                     "namespace.");

        GenerateNewPrefix(aPrefix);
        
        
        
        
        index = count - 1;
        haveSeenOurPrefix = PR_FALSE;
        continue;
      }
    }
    
    
    if (!uriMatch && aURI.Equals(decl->mURI)) {
      
      
      PRBool prefixOK = PR_TRUE;
      PRInt32 index2;
      for (index2 = count-1; index2 > index && prefixOK; --index2) {
        NameSpaceDecl* decl2 =
          (NameSpaceDecl*)mNameSpaceStack.ElementAt(index2);
        prefixOK = (decl2->mPrefix != decl->mPrefix);
      }
      
      if (prefixOK) {
        uriMatch = PR_TRUE;
        closestURIMatch.Assign(decl->mPrefix);
      }
    }
    
    --index;
  }

  
  
  
  
  
  
  
  
  
  
  if (uriMatch && (!mustHavePrefix || !closestURIMatch.IsEmpty())) {
    aPrefix.Assign(closestURIMatch);
    return PR_FALSE;
  }
  
  if (aPrefix.IsEmpty()) {
    
    
    
    
    if (mustHavePrefix) {
      GenerateNewPrefix(aPrefix);
      return ConfirmPrefix(aPrefix, aURI, aElement, aIsAttribute);
    }

    
    
    
    
    if (!haveSeenOurPrefix && aURI.IsEmpty()) {
      return PR_FALSE;
    }
  }

  
  
  return PR_TRUE;
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
                                      PRBool aDoEscapeEntities)
{
  AppendToString(PRUnichar(' '), aStr);
  if (!aPrefix.IsEmpty()) {
    AppendToString(aPrefix, aStr);
    AppendToString(PRUnichar(':'), aStr);
  }
  AppendToString(aName, aStr);
  
  if ( aDoEscapeEntities ) {
    
    
    AppendToString(NS_LITERAL_STRING("=\""), aStr);

    mInAttribute = PR_TRUE;
    AppendToString(aValue, aStr, PR_TRUE);
    mInAttribute = PR_FALSE;

    AppendToString(PRUnichar('"'), aStr);
  }
  else {
    
    
    
    
    
    PRBool bIncludesSingle = PR_FALSE;
    PRBool bIncludesDouble = PR_FALSE;
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
          bIncludesSingle = PR_TRUE;
          if ( bIncludesDouble ) break;
        }
        else if ( *buf == PRUnichar('"') )
        {
          bIncludesDouble = PR_TRUE;
          if ( bIncludesSingle ) break;
        }
      }
      
      if ( bIncludesDouble && bIncludesSingle ) break;
    }

    
    
    
    
    
    
    PRUnichar cDelimiter = 
        (bIncludesDouble && !bIncludesSingle) ? PRUnichar('\'') : PRUnichar('"');
    AppendToString(PRUnichar('='), aStr);
    AppendToString(cDelimiter, aStr);
    if (bIncludesDouble && bIncludesSingle) {
      nsAutoString sValue(aValue);
      sValue.ReplaceSubstring(NS_LITERAL_STRING("\"").get(), NS_LITERAL_STRING("&quot;").get());
      mInAttribute = PR_TRUE;
      AppendToString(sValue, aStr, PR_FALSE);
      mInAttribute = PR_FALSE;
    }
    else {
      mInAttribute = PR_TRUE;
      AppendToString(aValue, aStr, PR_FALSE);
      mInAttribute = PR_FALSE;
    }
    AppendToString(cDelimiter, aStr);
  }
}

NS_IMETHODIMP 
nsXMLContentSerializer::AppendElementStart(nsIDOMElement *aElement,
                                           nsIDOMElement *aOriginalElement,
                                           nsAString& aStr)
{
  NS_ENSURE_ARG(aElement);

  nsAutoString tagPrefix, tagLocalName, tagNamespaceURI;
  nsAutoString xmlnsStr;
  xmlnsStr.AssignLiteral(kXMLNS);

  nsCOMPtr<nsIContent> content(do_QueryInterface(aElement));
  if (!content) return NS_ERROR_FAILURE;

  aElement->GetPrefix(tagPrefix);
  aElement->GetLocalName(tagLocalName);
  aElement->GetNamespaceURI(tagNamespaceURI);

  PRUint32 index, count;
  nsAutoString nameStr, prefixStr, uriStr, valueStr;

  count = content->GetAttrCount();

  
  PRUint32 skipAttr = count;
  for (index = 0; index < count; index++) {
    
    const nsAttrName* name = content->GetAttrNameAt(index);
    PRInt32 namespaceID = name->NamespaceID();
    nsIAtom *attrName = name->LocalName();
    
    if (namespaceID == kNameSpaceID_XMLNS ||
        
        
        
        
        
        (namespaceID == kNameSpaceID_None &&
         attrName == nsGkAtoms::xmlns)) {
      content->GetAttr(namespaceID, attrName, uriStr);

      if (!name->GetPrefix()) {
        if (tagNamespaceURI.IsEmpty() && !uriStr.IsEmpty()) {
          
          
          
          
          
          
          
          
          skipAttr = index;
        }
        else {
          
          PushNameSpaceDecl(EmptyString(), uriStr, aOriginalElement);
        }
      }
      else {
        attrName->ToString(nameStr);
        PushNameSpaceDecl(nameStr, uriStr, aOriginalElement);
      }
    }
  }

  PRBool addNSAttr;
    
  MaybeAddNewline(aStr);

  addNSAttr = ConfirmPrefix(tagPrefix, tagNamespaceURI, aOriginalElement,
                            PR_FALSE);
  
  AppendToString(NS_LITERAL_STRING("<"), aStr);
  if (!tagPrefix.IsEmpty()) {
    AppendToString(tagPrefix, aStr);
    AppendToString(NS_LITERAL_STRING(":"), aStr);
  }
  AppendToString(tagLocalName, aStr);
    
  
  
  if (addNSAttr) {
    if (tagPrefix.IsEmpty()) {
      
      SerializeAttr(EmptyString(), xmlnsStr, tagNamespaceURI, aStr, PR_TRUE);
    } else {
      
      SerializeAttr(xmlnsStr, tagPrefix, tagNamespaceURI, aStr, PR_TRUE);
    }
    PushNameSpaceDecl(tagPrefix, tagNamespaceURI, aOriginalElement);
  }

  
  
  
  for (index = 0; index < count; index++) {
    if (skipAttr == index) {
        continue;
    }

    const nsAttrName* name = content->GetAttrNameAt(index);
    PRInt32 namespaceID = name->NamespaceID();
    nsIAtom* attrName = name->LocalName();
    nsIAtom* attrPrefix = name->GetPrefix();

    if (attrPrefix) {
      attrPrefix->ToString(prefixStr);
    }
    else {
      prefixStr.Truncate();
    }

    addNSAttr = PR_FALSE;
    if (kNameSpaceID_XMLNS != namespaceID) {
      nsContentUtils::NameSpaceManager()->GetNameSpaceURI(namespaceID, uriStr);
      addNSAttr = ConfirmPrefix(prefixStr, uriStr, aOriginalElement, PR_TRUE);
    }
    
    content->GetAttr(namespaceID, attrName, valueStr);
    attrName->ToString(nameStr);

    
    
    
    if (!nameStr.IsEmpty() && nameStr.First() == '-')
      continue;

    if (namespaceID == kNameSpaceID_None) {
      if (content->GetNameSpaceID() == kNameSpaceID_XHTML) {
        if (IsShorthandAttr(attrName, content->Tag()) &&
            valueStr.IsEmpty()) {
          valueStr = nameStr;
        }
      }
    }
    SerializeAttr(prefixStr, nameStr, valueStr, aStr, PR_TRUE);
    
    if (addNSAttr) {
      NS_ASSERTION(!prefixStr.IsEmpty(),
                   "Namespaced attributes must have a prefix");
      SerializeAttr(xmlnsStr, prefixStr, uriStr, aStr, PR_TRUE);
      PushNameSpaceDecl(prefixStr, uriStr, aOriginalElement);
    }
  }

  
  PRBool hasChildren;
  if (NS_FAILED(aOriginalElement->HasChildNodes(&hasChildren)) ||
      !hasChildren) {
    AppendToString(NS_LITERAL_STRING("/>"), aStr);    
    MaybeFlagNewline(aElement);
  } else {
    AppendToString(NS_LITERAL_STRING(">"), aStr);    
  }
  
  return NS_OK;
}

NS_IMETHODIMP 
nsXMLContentSerializer::AppendElementEnd(nsIDOMElement *aElement,
                                         nsAString& aStr)
{
  NS_ENSURE_ARG(aElement);

  
  nsCOMPtr<nsIDOMNode> node(do_QueryInterface(aElement));
  PRBool hasChildren;
  if (NS_SUCCEEDED(node->HasChildNodes(&hasChildren)) && !hasChildren) {
    PopNameSpaceDeclsFor(aElement);
  
    return NS_OK;
  }
  
  nsCOMPtr<nsIContent> content(do_QueryInterface(aElement));
  if (!content) return NS_ERROR_FAILURE;

  nsAutoString tagPrefix, tagLocalName, tagNamespaceURI;
  
  aElement->GetPrefix(tagPrefix);
  aElement->GetLocalName(tagLocalName);
  aElement->GetNamespaceURI(tagNamespaceURI);

#ifdef DEBUG
  PRBool debugNeedToPushNamespace =
#endif
  ConfirmPrefix(tagPrefix, tagNamespaceURI, aElement, PR_FALSE);
  NS_ASSERTION(!debugNeedToPushNamespace, "Can't push namespaces in closing tag!");

  AppendToString(NS_LITERAL_STRING("</"), aStr);
  if (!tagPrefix.IsEmpty()) {
    AppendToString(tagPrefix, aStr);
    AppendToString(NS_LITERAL_STRING(":"), aStr);
  }
  AppendToString(tagLocalName, aStr);
  AppendToString(NS_LITERAL_STRING(">"), aStr);
  MaybeFlagNewline(aElement);
  
  PopNameSpaceDeclsFor(aElement);
  
  return NS_OK;
}

void
nsXMLContentSerializer::AppendToString(const PRUnichar* aStr,
                                       PRInt32 aLength,
                                       nsAString& aOutputStr)
{
  PRInt32 length = (aLength == -1) ? nsCRT::strlen(aStr) : aLength;
  
  aOutputStr.Append(aStr, length);
}

void 
nsXMLContentSerializer::AppendToString(const PRUnichar aChar,
                                       nsAString& aOutputStr)
{
  aOutputStr.Append(aChar);
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
nsXMLContentSerializer::AppendToString(const nsAString& aStr,
                                       nsAString& aOutputStr,
                                       PRBool aTranslateEntities,
                                       PRBool aIncrColumn)
{
  if (aTranslateEntities) {
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

    return;
  }
  
  aOutputStr.Append(aStr);
}

PRBool
nsXMLContentSerializer::IsShorthandAttr(const nsIAtom* aAttrName,
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

  return PR_FALSE;
}

void
nsXMLContentSerializer::MaybeAddNewline(nsAString& aStr)
{
  if (mAddNewline) {
    aStr.Append((PRUnichar)'\n');
    mAddNewline = PR_FALSE;
  }
}

void
nsXMLContentSerializer::MaybeFlagNewline(nsIDOMNode* aNode)
{
  nsCOMPtr<nsIDOMNode> parent;
  aNode->GetParentNode(getter_AddRefs(parent));
  if (parent) {
    PRUint16 type;
    parent->GetNodeType(&type);
    mAddNewline = type == nsIDOMNode::DOCUMENT_NODE;
  }
}

NS_IMETHODIMP
nsXMLContentSerializer::AppendDocumentStart(nsIDOMDocument *aDocument,
                                            nsAString& aStr)
{
  NS_ENSURE_ARG_POINTER(aDocument);

  nsCOMPtr<nsIDocument> doc(do_QueryInterface(aDocument));
  if (!doc) {
    return NS_OK;
  }

  nsAutoString version, encoding, standalone;
  doc->GetXMLDeclaration(version, encoding, standalone);

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
  mAddNewline = PR_TRUE;

  return NS_OK;
}
