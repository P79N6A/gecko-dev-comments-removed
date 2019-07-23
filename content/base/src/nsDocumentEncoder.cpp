









































 
#include "nsIDocumentEncoder.h"

#include "nscore.h"
#include "nsIFactory.h"
#include "nsISupports.h"
#include "nsIComponentManager.h" 
#include "nsIServiceManager.h"
#include "nsIDocument.h"
#include "nsIHTMLDocument.h"
#include "nsISelection.h"
#include "nsCOMPtr.h"
#include "nsIContentSerializer.h"
#include "nsIUnicodeEncoder.h"
#include "nsIOutputStream.h"
#include "nsIDOMElement.h"
#include "nsIDOMText.h"
#include "nsIDOMCDATASection.h"
#include "nsIDOMComment.h"
#include "nsIDOMProcessingInstruction.h"
#include "nsIDOMDocumentType.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMRange.h"
#include "nsIDOMDocument.h"
#include "nsICharsetConverterManager.h"
#include "nsGkAtoms.h"
#include "nsIContent.h"
#include "nsIEnumerator.h"
#include "nsISelectionPrivate.h"
#include "nsIParserService.h"
#include "nsIScriptContext.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptSecurityManager.h"
#include "nsContentUtils.h"
#include "nsUnicharUtils.h"
#include "nsReadableUtils.h"
#include "nsTArray.h"

nsresult NS_NewDomSelection(nsISelection **aDomSelection);

enum nsRangeIterationDirection {
  kDirectionOut = -1,
  kDirectionIn = 1
};

class nsDocumentEncoder : public nsIDocumentEncoder
{
public:
  nsDocumentEncoder();
  virtual ~nsDocumentEncoder();

  NS_DECL_ISUPPORTS

  NS_DECL_NSIDOCUMENTENCODER

protected:
  void Initialize();
  nsresult SerializeNodeStart(nsIDOMNode* aNode, PRInt32 aStartOffset,
                              PRInt32 aEndOffset, nsAString& aStr,
                              nsIDOMNode* aOriginalNode = nsnull);
  nsresult SerializeToStringRecursive(nsIDOMNode* aNode,
                                      nsAString& aStr,
                                      PRBool aDontSerializeRoot);
  nsresult SerializeNodeEnd(nsIDOMNode* aNode, nsAString& aStr);
  nsresult SerializeRangeToString(nsIDOMRange *aRange,
                                  nsAString& aOutputString);
  nsresult SerializeRangeNodes(nsIDOMRange* aRange, 
                               nsIDOMNode* aNode, 
                               nsAString& aString,
                               PRInt32 aDepth);
  nsresult SerializeRangeContextStart(const nsTArray<nsIDOMNode*>& aAncestorArray,
                                      nsAString& aString);
  nsresult SerializeRangeContextEnd(const nsTArray<nsIDOMNode*>& aAncestorArray,
                                    nsAString& aString);

  nsresult FlushText(nsAString& aString, PRBool aForce);

  static PRBool IsTag(nsIDOMNode* aNode, nsIAtom* aAtom);
  
  virtual PRBool IncludeInContext(nsIDOMNode *aNode);

  nsCOMPtr<nsIDocument>          mDocument;
  nsCOMPtr<nsISelection>         mSelection;
  nsCOMPtr<nsIDOMRange>          mRange;
  nsCOMPtr<nsIDOMNode>           mNode;
  nsCOMPtr<nsIOutputStream>      mStream;
  nsCOMPtr<nsIContentSerializer> mSerializer;
  nsCOMPtr<nsIUnicodeEncoder>    mUnicodeEncoder;
  nsCOMPtr<nsIDOMNode>           mCommonParent;
  nsCOMPtr<nsIDocumentEncoderNodeFixup> mNodeFixup;
  nsCOMPtr<nsICharsetConverterManager> mCharsetConverterManager;

  nsString          mMimeType;
  nsCString         mCharset;
  PRUint32          mFlags;
  PRUint32          mWrapColumn;
  PRUint32          mStartDepth;
  PRUint32          mEndDepth;
  PRInt32           mStartRootIndex;
  PRInt32           mEndRootIndex;
  nsAutoTArray<nsIDOMNode*, 8> mCommonAncestors;
  nsAutoTArray<nsIContent*, 8> mStartNodes;
  nsAutoTArray<PRInt32, 8>     mStartOffsets;
  nsAutoTArray<nsIContent*, 8> mEndNodes;
  nsAutoTArray<PRInt32, 8>     mEndOffsets;
  PRPackedBool      mHaltRangeHint;  
  PRPackedBool      mIsCopying;  
  PRPackedBool      mNodeIsContainer;
};

NS_IMPL_ADDREF(nsDocumentEncoder)
NS_IMPL_RELEASE(nsDocumentEncoder)

NS_INTERFACE_MAP_BEGIN(nsDocumentEncoder)
   NS_INTERFACE_MAP_ENTRY(nsIDocumentEncoder)
   NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

nsDocumentEncoder::nsDocumentEncoder()
{
  Initialize();
  mMimeType.AssignLiteral("text/plain");

}

void nsDocumentEncoder::Initialize()
{
  mFlags = 0;
  mWrapColumn = 72;
  mStartDepth = 0;
  mEndDepth = 0;
  mStartRootIndex = 0;
  mEndRootIndex = 0;
  mHaltRangeHint = PR_FALSE;
  mNodeIsContainer = PR_FALSE;
}

nsDocumentEncoder::~nsDocumentEncoder()
{
}

NS_IMETHODIMP
nsDocumentEncoder::Init(nsIDOMDocument* aDocument,
                        const nsAString& aMimeType,
                        PRUint32 aFlags)
{
  if (!aDocument)
    return NS_ERROR_INVALID_ARG;

  Initialize();

  mDocument = do_QueryInterface(aDocument);
  NS_ENSURE_TRUE(mDocument, NS_ERROR_FAILURE);

  mMimeType = aMimeType;

  mFlags = aFlags;
  mIsCopying = PR_FALSE;

  return NS_OK;
}

NS_IMETHODIMP
nsDocumentEncoder::SetWrapColumn(PRUint32 aWC)
{
  mWrapColumn = aWC;
  return NS_OK;
}

NS_IMETHODIMP
nsDocumentEncoder::SetSelection(nsISelection* aSelection)
{
  mSelection = aSelection;
  return NS_OK;
}

NS_IMETHODIMP
nsDocumentEncoder::SetRange(nsIDOMRange* aRange)
{
  mRange = aRange;
  return NS_OK;
}

NS_IMETHODIMP
nsDocumentEncoder::SetNode(nsIDOMNode* aNode)
{
  mNodeIsContainer = PR_FALSE;
  mNode = aNode;
  return NS_OK;
}

NS_IMETHODIMP
nsDocumentEncoder::SetContainerNode(nsIDOMNode *aContainer)
{
  mNodeIsContainer = PR_TRUE;
  mNode = aContainer;
  return NS_OK;
}

NS_IMETHODIMP
nsDocumentEncoder::SetCharset(const nsACString& aCharset)
{
  mCharset = aCharset;
  return NS_OK;
}

NS_IMETHODIMP
nsDocumentEncoder::GetMimeType(nsAString& aMimeType)
{
  aMimeType = mMimeType;
  return NS_OK;
}


PRBool
nsDocumentEncoder::IncludeInContext(nsIDOMNode *aNode)
{
  return PR_FALSE;
}

nsresult
nsDocumentEncoder::SerializeNodeStart(nsIDOMNode* aNode,
                                      PRInt32 aStartOffset,
                                      PRInt32 aEndOffset,
                                      nsAString& aStr,
                                      nsIDOMNode* aOriginalNode)
{
  PRUint16 type;

  nsCOMPtr<nsIDOMNode> node;

  
  if (!aOriginalNode) {
    aOriginalNode = aNode;
    if (mNodeFixup) { 
      PRBool dummy;
      mNodeFixup->FixupNode(aNode, &dummy, getter_AddRefs(node));
    }
  }

  
  
  if (!node)
    node = aNode;

  node->GetNodeType(&type);
  switch (type) {
    case nsIDOMNode::ELEMENT_NODE:
    {
      nsCOMPtr<nsIDOMElement> element = do_QueryInterface(node);
      nsCOMPtr<nsIDOMElement> originalElement = do_QueryInterface(aOriginalNode);
      mSerializer->AppendElementStart(element, originalElement, aStr);
      break;
    }
    case nsIDOMNode::TEXT_NODE:
    {
      nsCOMPtr<nsIDOMText> text = do_QueryInterface(node);
      mSerializer->AppendText(text, aStartOffset, aEndOffset, aStr);
      break;
    }
    case nsIDOMNode::CDATA_SECTION_NODE:
    {
      nsCOMPtr<nsIDOMCDATASection> cdata = do_QueryInterface(node);
      mSerializer->AppendCDATASection(cdata, aStartOffset, aEndOffset, aStr);
      break;
    }
    case nsIDOMNode::PROCESSING_INSTRUCTION_NODE:
    {
      nsCOMPtr<nsIDOMProcessingInstruction> pi = do_QueryInterface(node);
      mSerializer->AppendProcessingInstruction(pi, aStartOffset, aEndOffset,
                                               aStr);
      break;
    }
    case nsIDOMNode::COMMENT_NODE:
    {
      nsCOMPtr<nsIDOMComment> comment = do_QueryInterface(node);
      mSerializer->AppendComment(comment, aStartOffset, aEndOffset, aStr);
      break;
    }
    case nsIDOMNode::DOCUMENT_TYPE_NODE:
    {
      nsCOMPtr<nsIDOMDocumentType> doctype = do_QueryInterface(node);
      mSerializer->AppendDoctype(doctype, aStr);
      break;
    }
  }
  
  return NS_OK;
}

nsresult
nsDocumentEncoder::SerializeNodeEnd(nsIDOMNode* aNode,
                                    nsAString& aStr)
{
  PRUint16 type;

  aNode->GetNodeType(&type);
  switch (type) {
    case nsIDOMNode::ELEMENT_NODE:
    {
      nsCOMPtr<nsIDOMElement> element = do_QueryInterface(aNode);
      mSerializer->AppendElementEnd(element, aStr);
      break;
    }
  }

  return NS_OK;
}

nsresult
nsDocumentEncoder::SerializeToStringRecursive(nsIDOMNode* aNode,
                                              nsAString& aStr,
                                              PRBool aDontSerializeRoot)
{
  nsresult rv = NS_OK;
  PRBool serializeClonedChildren = PR_FALSE;
  nsCOMPtr<nsIDOMNode> maybeFixedNode;
  
  if (mNodeFixup)
    mNodeFixup->FixupNode(aNode, &serializeClonedChildren, getter_AddRefs(maybeFixedNode));

  if (!maybeFixedNode)
    maybeFixedNode = aNode;

  if (!aDontSerializeRoot) {
    rv = SerializeNodeStart(maybeFixedNode, 0, -1, aStr, aNode);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsIDOMNode *node;
  if (serializeClonedChildren)
    node = maybeFixedNode;
  else  
    node = aNode;

  PRBool hasChildren = PR_FALSE;
  node->HasChildNodes(&hasChildren);

  if (hasChildren) {
    nsCOMPtr<nsIDOMNodeList> childNodes;
    rv = node->GetChildNodes(getter_AddRefs(childNodes));
    NS_ENSURE_TRUE(childNodes, NS_SUCCEEDED(rv) ? NS_ERROR_FAILURE : rv);

    PRInt32 index, count;

    childNodes->GetLength((PRUint32*)&count);
    for (index = 0; index < count; index++) {
      nsCOMPtr<nsIDOMNode> child;

      rv = childNodes->Item(index, getter_AddRefs(child));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = SerializeToStringRecursive(child, aStr, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);     
    }
  }

  if (!aDontSerializeRoot) {
    rv = SerializeNodeEnd(node, aStr);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return FlushText(aStr, PR_FALSE);
}

PRBool 
nsDocumentEncoder::IsTag(nsIDOMNode* aNode, nsIAtom* aAtom)
{
  nsCOMPtr<nsIContent> content = do_QueryInterface(aNode);
  return content && content->Tag() == aAtom;
}

static nsresult
ConvertAndWrite(const nsAString& aString,
                nsIOutputStream* aStream,
                nsIUnicodeEncoder* aEncoder)
{
  NS_ENSURE_ARG_POINTER(aStream);
  NS_ENSURE_ARG_POINTER(aEncoder);
  nsresult rv;
  PRInt32 charLength, startCharLength;
  const nsPromiseFlatString& flat = PromiseFlatString(aString);
  const PRUnichar* unicodeBuf = flat.get();
  PRInt32 unicodeLength = aString.Length();
  PRInt32 startLength = unicodeLength;

  rv = aEncoder->GetMaxLength(unicodeBuf, unicodeLength, &charLength);
  startCharLength = charLength;
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString charXferString;
  if (!EnsureStringLength(charXferString, charLength))
    return NS_ERROR_OUT_OF_MEMORY;

  char* charXferBuf = charXferString.BeginWriting();
  nsresult convert_rv = NS_OK;

  do {
    unicodeLength = startLength;
    charLength = startCharLength;

    convert_rv = aEncoder->Convert(unicodeBuf, &unicodeLength, charXferBuf, &charLength);
    NS_ENSURE_SUCCESS(convert_rv, convert_rv);

    
    

    charXferBuf[charLength] = '\0';

    PRUint32 written;
    rv = aStream->Write(charXferBuf, charLength, &written);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    if (convert_rv == NS_ERROR_UENC_NOMAPPING) {
      
      
      char finish_buf[33];
      charLength = sizeof(finish_buf) - 1;
      rv = aEncoder->Finish(finish_buf, &charLength);
      NS_ENSURE_SUCCESS(rv, rv);

      
      

      finish_buf[charLength] = '\0';

      rv = aStream->Write(finish_buf, charLength, &written);
      NS_ENSURE_SUCCESS(rv, rv);

      nsCAutoString entString("&#");
      if (NS_IS_HIGH_SURROGATE(unicodeBuf[unicodeLength - 1]) && 
          unicodeLength < startLength && NS_IS_LOW_SURROGATE(unicodeBuf[unicodeLength]))  {
        entString.AppendInt(SURROGATE_TO_UCS4(unicodeBuf[unicodeLength - 1],
                                              unicodeBuf[unicodeLength]));
        unicodeLength += 1;
      }
      else
        entString.AppendInt(unicodeBuf[unicodeLength - 1]);
      entString.Append(';');

      
      
      

      rv = aStream->Write(entString.get(), entString.Length(), &written);
      NS_ENSURE_SUCCESS(rv, rv);

      unicodeBuf += unicodeLength;
      startLength -= unicodeLength;
    }
  } while (convert_rv == NS_ERROR_UENC_NOMAPPING);

  return rv;
}

nsresult
nsDocumentEncoder::FlushText(nsAString& aString, PRBool aForce)
{
  if (!mStream)
    return NS_OK;

  nsresult rv = NS_OK;

  if (aString.Length() > 1024 || aForce) {
    rv = ConvertAndWrite(aString, mStream, mUnicodeEncoder);

    aString.Truncate();
  }

  return rv;
}

#if 0 
      
static nsresult ChildAt(nsIDOMNode* aNode, PRInt32 aIndex, nsIDOMNode*& aChild)
{
  nsCOMPtr<nsIContent> content(do_QueryInterface(aNode));

  aChild = nsnull;

  NS_ENSURE_TRUE(content, NS_ERROR_FAILURE);

  nsIContent *child = content->GetChildAt(aIndex);

  if (child)
    return CallQueryInterface(child, &aChild);

  return NS_OK;
}

static PRInt32 IndexOf(nsIDOMNode* aParent, nsIDOMNode* aChild)
{
  nsCOMPtr<nsIContent> parent(do_QueryInterface(aParent));
  nsCOMPtr<nsIContent> child(do_QueryInterface(aChild));

  if (!parent)
    return -1;

  return parent->IndexOf(child);
}

static inline PRInt32 GetIndex(nsTArray<PRInt32>& aIndexArray)
{
  PRInt32 count = aIndexArray.Length();

  if (count) {
    return aIndexArray.ElementAt(count - 1);
  }

  return 0;
}

static nsresult GetNextNode(nsIDOMNode* aNode, nsTArray<PRInt32>& aIndexArray,
                            nsIDOMNode*& aNextNode,
                            nsRangeIterationDirection& aDirection)
{
  PRBool hasChildren;

  aNextNode = nsnull;

  aNode->HasChildNodes(&hasChildren);

  if (hasChildren && aDirection == kDirectionIn) {
    ChildAt(aNode, 0, aNextNode);
    NS_ENSURE_TRUE(aNextNode, NS_ERROR_FAILURE);

    aIndexArray.AppendElement(0);

    aDirection = kDirectionIn;
  } else if (aDirection == kDirectionIn) {
    aNextNode = aNode;

    NS_ADDREF(aNextNode);

    aDirection = kDirectionOut;
  } else {
    nsCOMPtr<nsIDOMNode> parent;

    aNode->GetParentNode(getter_AddRefs(parent));
    NS_ENSURE_TRUE(parent, NS_ERROR_FAILURE);

    PRInt32 count = aIndexArray.Length();

    if (count) {
      PRInt32 indx = aIndexArray.ElementAt(count - 1);

      ChildAt(parent, indx + 1, aNextNode);

      if (aNextNode)
        aIndexArray.ElementAt(count - 1) = indx + 1;
      else
        aIndexArray.RemoveElementAt(count - 1);
    } else {
      PRInt32 indx = IndexOf(parent, aNode);

      if (indx >= 0) {
        ChildAt(parent, indx + 1, aNextNode);

        if (aNextNode)
          aIndexArray.AppendElement(indx + 1);
      }
    }

    if (aNextNode) {
      aDirection = kDirectionIn;
    } else {
      aDirection = kDirectionOut;

      aNextNode = parent;

      NS_ADDREF(aNextNode);
    }
  }

  return NS_OK;
}
#endif

static PRBool IsTextNode(nsIDOMNode *aNode)
{
  if (!aNode) return PR_FALSE;
  PRUint16 nodeType;
  aNode->GetNodeType(&nodeType);
  if (nodeType == nsIDOMNode::TEXT_NODE ||
      nodeType == nsIDOMNode::CDATA_SECTION_NODE)
    return PR_TRUE;
  return PR_FALSE;
}

static nsresult GetLengthOfDOMNode(nsIDOMNode *aNode, PRUint32 &aCount) 
{
  aCount = 0;
  if (!aNode) { return NS_ERROR_NULL_POINTER; }
  nsresult result=NS_OK;
  nsCOMPtr<nsIDOMCharacterData>nodeAsChar;
  nodeAsChar = do_QueryInterface(aNode);
  if (nodeAsChar) {
    nodeAsChar->GetLength(&aCount);
  }
  else
  {
    PRBool hasChildNodes;
    aNode->HasChildNodes(&hasChildNodes);
    if (PR_TRUE==hasChildNodes)
    {
      nsCOMPtr<nsIDOMNodeList>nodeList;
      result = aNode->GetChildNodes(getter_AddRefs(nodeList));
      if (NS_SUCCEEDED(result) && nodeList) {
        nodeList->GetLength(&aCount);
      }
    }
  }
  return result;
}

nsresult
nsDocumentEncoder::SerializeRangeNodes(nsIDOMRange* aRange, 
                                       nsIDOMNode* aNode, 
                                       nsAString& aString,
                                       PRInt32 aDepth)
{
  nsCOMPtr<nsIContent> content = do_QueryInterface(aNode);
  NS_ENSURE_TRUE(content, NS_ERROR_FAILURE);

  nsresult rv=NS_OK;
  
  
  nsCOMPtr<nsIContent> startNode, endNode;
  PRInt32 start = mStartRootIndex - aDepth;
  if (start >= 0 && start <= mStartNodes.Length())
    startNode = mStartNodes[start];

  PRInt32 end = mEndRootIndex - aDepth;
  if (end >= 0 && end <= mEndNodes.Length())
    endNode = mEndNodes[end];

  if ((startNode != content) && (endNode != content))
  {
    
    
    rv = SerializeToStringRecursive(aNode, aString, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else
  {
    
    
    if (IsTextNode(aNode))
    {
      if (startNode == content)
      {
        PRInt32 startOffset;
        aRange->GetStartOffset(&startOffset);
        rv = SerializeNodeStart(aNode, startOffset, -1, aString);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else
      {
        PRInt32 endOffset;
        aRange->GetEndOffset(&endOffset);
        rv = SerializeNodeStart(aNode, 0, endOffset, aString);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
    else
    {
      if (aNode != mCommonParent)
      {
        if (IncludeInContext(aNode))
        {
          
          
          mHaltRangeHint = PR_TRUE;
        }
        if ((startNode == content) && !mHaltRangeHint) mStartDepth++;
        if ((endNode == content) && !mHaltRangeHint) mEndDepth++;
      
        
        rv = SerializeNodeStart(aNode, 0, -1, aString);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      
      
      
      nsCOMPtr<nsIDOMNode> childAsNode;
      PRInt32 startOffset = 0, endOffset = -1;
      if (startNode == content && mStartRootIndex >= aDepth)
        startOffset = mStartOffsets[mStartRootIndex - aDepth];
      if (endNode == content && mEndRootIndex >= aDepth)
        endOffset = mEndOffsets[mEndRootIndex - aDepth];
      
      PRInt32 j;
      PRUint32 childCount = content->GetChildCount();

      if (startOffset == -1) startOffset = 0;
      if (endOffset == -1) endOffset = childCount;
      else
      {
        
        
        
        
        
        
        nsCOMPtr<nsIDOMNode> endParent;
        aRange->GetEndContainer(getter_AddRefs(endParent));
        if (aNode != endParent)
        {
          endOffset++;
        }
      }
      
      for (j=startOffset; j<endOffset; j++)
      {
        childAsNode = do_QueryInterface(content->GetChildAt(j));

        if ((j==startOffset) || (j==endOffset-1))
          rv = SerializeRangeNodes(aRange, childAsNode, aString, aDepth+1);
        else
          rv = SerializeToStringRecursive(childAsNode, aString, PR_FALSE);

        NS_ENSURE_SUCCESS(rv, rv);
      }

      
      if (aNode != mCommonParent)
      {
        rv = SerializeNodeEnd(aNode, aString);
        NS_ENSURE_SUCCESS(rv, rv); 
      }
    }
  }
  return NS_OK;
}

nsresult
nsDocumentEncoder::SerializeRangeContextStart(const nsTArray<nsIDOMNode*>& aAncestorArray,
                                              nsAString& aString)
{
  PRInt32 i = aAncestorArray.Length();
  nsresult rv = NS_OK;

  while (i > 0) {
    nsIDOMNode *node = aAncestorArray.ElementAt(--i);

    if (!node)
      break;

    if (IncludeInContext(node)) {
      rv = SerializeNodeStart(node, 0, -1, aString);

      if (NS_FAILED(rv))
        break;
    }
  }

  return rv;
}

nsresult
nsDocumentEncoder::SerializeRangeContextEnd(const nsTArray<nsIDOMNode*>& aAncestorArray,
                                            nsAString& aString)
{
  PRInt32 i = 0;
  PRInt32 count = aAncestorArray.Length();
  nsresult rv = NS_OK;

  while (i < count) {
    nsIDOMNode *node = aAncestorArray.ElementAt(i++);

    if (!node)
      break;

    if (IncludeInContext(node)) {
      rv = SerializeNodeEnd(node, aString);

      if (NS_FAILED(rv))
        break;
    }
  }

  return rv;
}

nsresult
nsDocumentEncoder::SerializeRangeToString(nsIDOMRange *aRange,
                                          nsAString& aOutputString)
{
  if (!aRange)
    return NS_OK;

  PRBool collapsed;

  aRange->GetCollapsed(&collapsed);

  if (collapsed)
    return NS_OK;

  nsCOMPtr<nsIDOMNode> startParent, endParent;
  PRInt32 startOffset, endOffset;
  
  aRange->GetCommonAncestorContainer(getter_AddRefs(mCommonParent));

  if (!mCommonParent)
    return NS_OK;
  
  aRange->GetStartContainer(getter_AddRefs(startParent));
  NS_ENSURE_TRUE(startParent, NS_ERROR_FAILURE);
  aRange->GetStartOffset(&startOffset);

  aRange->GetEndContainer(getter_AddRefs(endParent));
  NS_ENSURE_TRUE(endParent, NS_ERROR_FAILURE);
  aRange->GetEndOffset(&endOffset);

  mCommonAncestors.Clear();
  mStartNodes.Clear();
  mStartOffsets.Clear();
  mEndNodes.Clear();
  mEndOffsets.Clear();

  nsContentUtils::GetAncestors(mCommonParent, &mCommonAncestors);
  nsContentUtils::GetAncestorsAndOffsets(startParent, startOffset,
                                         &mStartNodes, &mStartOffsets);
  nsContentUtils::GetAncestorsAndOffsets(endParent, endOffset,
                                         &mEndNodes, &mEndOffsets);

  nsCOMPtr<nsIContent> commonContent = do_QueryInterface(mCommonParent);
  mStartRootIndex = mStartNodes.IndexOf(commonContent);
  mEndRootIndex = mEndNodes.IndexOf(commonContent);
  
  nsresult rv = NS_OK;

  rv = SerializeRangeContextStart(mCommonAncestors, aOutputString);
  NS_ENSURE_SUCCESS(rv, rv);

  if ((startParent == endParent) && IsTextNode(startParent))
  {
    rv = SerializeNodeStart(startParent, startOffset, endOffset, aOutputString);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else
  {
    rv = SerializeRangeNodes(aRange, mCommonParent, aOutputString, 0);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  rv = SerializeRangeContextEnd(mCommonAncestors, aOutputString);
  NS_ENSURE_SUCCESS(rv, rv);

  return rv;
}

NS_IMETHODIMP
nsDocumentEncoder::EncodeToString(nsAString& aOutputString)
{
  if (!mDocument)
    return NS_ERROR_NOT_INITIALIZED;

  aOutputString.Truncate();

  nsCAutoString progId(NS_CONTENTSERIALIZER_CONTRACTID_PREFIX);
  AppendUTF16toUTF8(mMimeType, progId);

  mSerializer = do_CreateInstance(progId.get());
  NS_ENSURE_TRUE(mSerializer, NS_ERROR_NOT_IMPLEMENTED);

  nsresult rv = NS_OK;

  nsCOMPtr<nsIAtom> charsetAtom;
  if (!mCharset.IsEmpty()) {
    if (!mCharsetConverterManager) {
      mCharsetConverterManager = do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  
  PRBool rewriteEncodingDeclaration = !(mSelection || mRange || mNode) && !(mFlags & OutputDontRewriteEncodingDeclaration);
  mSerializer->Init(mFlags, mWrapColumn, mCharset.get(), mIsCopying, rewriteEncodingDeclaration);

  if (mSelection) {
    nsCOMPtr<nsIDOMRange> range;
    PRInt32 i, count = 0;

    rv = mSelection->GetRangeCount(&count);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIDOMNode> node, prevNode;
    for (i = 0; i < count; i++) {
      mSelection->GetRangeAt(i, getter_AddRefs(range));

      
      
      
      range->GetStartContainer(getter_AddRefs(node));
      NS_ENSURE_TRUE(node, NS_ERROR_FAILURE);
      if (node != prevNode) {
        if (prevNode) {
          rv = SerializeNodeEnd(prevNode, aOutputString);
          NS_ENSURE_SUCCESS(rv, rv);
          prevNode = nsnull;
        }
        nsCOMPtr<nsIContent> content = do_QueryInterface(node);
        if (content && content->Tag() == nsGkAtoms::tr) {
          rv = SerializeNodeStart(node, 0, -1, aOutputString);
          NS_ENSURE_SUCCESS(rv, rv);
          prevNode = node;
        }
      }

      rv = SerializeRangeToString(range, aOutputString);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    if (prevNode) {
      rv = SerializeNodeEnd(prevNode, aOutputString);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    mSelection = nsnull;
  } else if (mRange) {
      rv = SerializeRangeToString(mRange, aOutputString);

      mRange = nsnull;
  } else if (mNode) {
    rv = SerializeToStringRecursive(mNode, aOutputString, mNodeIsContainer);
    mNode = nsnull;
  } else {
    nsCOMPtr<nsIDOMDocument> domdoc(do_QueryInterface(mDocument));
    rv = mSerializer->AppendDocumentStart(domdoc, aOutputString);

    if (NS_SUCCEEDED(rv)) {
      nsCOMPtr<nsIDOMNode> doc(do_QueryInterface(mDocument));

      rv = SerializeToStringRecursive(doc, aOutputString, PR_FALSE);
    }
  }

  NS_ENSURE_SUCCESS(rv, rv);
  rv = mSerializer->Flush(aOutputString);

  return rv;
}

NS_IMETHODIMP
nsDocumentEncoder::EncodeToStream(nsIOutputStream* aStream)
{
  nsresult rv = NS_OK;

  if (!mDocument)
    return NS_ERROR_NOT_INITIALIZED;

  if (!mCharsetConverterManager) {
    mCharsetConverterManager = do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = mCharsetConverterManager->GetUnicodeEncoder(mCharset.get(),
                                                   getter_AddRefs(mUnicodeEncoder));
  NS_ENSURE_SUCCESS(rv, rv);

  if (mMimeType.LowerCaseEqualsLiteral("text/plain")) {
    rv = mUnicodeEncoder->SetOutputErrorBehavior(nsIUnicodeEncoder::kOnError_Replace, nsnull, '?');
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mStream = aStream;

  nsAutoString buf;

  rv = EncodeToString(buf);

  
  FlushText(buf, PR_TRUE);

  mStream = nsnull;
  mUnicodeEncoder = nsnull;

  return rv;
}

NS_IMETHODIMP
nsDocumentEncoder::EncodeToStringWithContext(nsAString& aContextString,
                                             nsAString& aInfoString,
                                             nsAString& aEncodedString)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocumentEncoder::SetNodeFixup(nsIDocumentEncoderNodeFixup *aFixup)
{
  mNodeFixup = aFixup;
  return NS_OK;
}


nsresult NS_NewTextEncoder(nsIDocumentEncoder** aResult); 

nsresult
NS_NewTextEncoder(nsIDocumentEncoder** aResult)
{
  *aResult = new nsDocumentEncoder;
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;
 NS_ADDREF(*aResult);
 return NS_OK;
}

class nsHTMLCopyEncoder : public nsDocumentEncoder
{
public:

  nsHTMLCopyEncoder();
  virtual ~nsHTMLCopyEncoder();

  NS_IMETHOD Init(nsIDOMDocument* aDocument, const nsAString& aMimeType, PRUint32 aFlags);

  
  NS_IMETHOD SetSelection(nsISelection* aSelection);
  NS_IMETHOD EncodeToStringWithContext(nsAString& aContextString,
                                       nsAString& aInfoString,
                                       nsAString& aEncodedString);

protected:

  enum Endpoint
  {
    kStart,
    kEnd
  };
  
  nsresult PromoteRange(nsIDOMRange *inRange);
  nsresult PromoteAncestorChain(nsCOMPtr<nsIDOMNode> *ioNode, 
                                PRInt32 *ioStartOffset, 
                                PRInt32 *ioEndOffset);
  nsresult GetPromotedPoint(Endpoint aWhere, nsIDOMNode *aNode, PRInt32 aOffset, 
                            nsCOMPtr<nsIDOMNode> *outNode, PRInt32 *outOffset, nsIDOMNode *aCommon);
  nsCOMPtr<nsIDOMNode> GetChildAt(nsIDOMNode *aParent, PRInt32 aOffset);
  PRBool IsMozBR(nsIDOMNode* aNode);
  nsresult GetNodeLocation(nsIDOMNode *inChild, nsCOMPtr<nsIDOMNode> *outParent, PRInt32 *outOffset);
  PRBool IsRoot(nsIDOMNode* aNode);
  PRBool IsFirstNode(nsIDOMNode *aNode);
  PRBool IsLastNode(nsIDOMNode *aNode);
  PRBool IsEmptyTextContent(nsIDOMNode* aNode);
  virtual PRBool IncludeInContext(nsIDOMNode *aNode);

  PRBool mIsTextWidget;
};

nsHTMLCopyEncoder::nsHTMLCopyEncoder()
{
  mIsTextWidget = PR_FALSE;
}

nsHTMLCopyEncoder::~nsHTMLCopyEncoder()
{
}

NS_IMETHODIMP
nsHTMLCopyEncoder::Init(nsIDOMDocument* aDocument,
                        const nsAString& aMimetype,
                        PRUint32 aFlags)
{
  if (!aDocument)
    return NS_ERROR_INVALID_ARG;

  mIsTextWidget = PR_FALSE;
  Initialize();

  mIsCopying = PR_TRUE;
  mDocument = do_QueryInterface(aDocument);
  NS_ENSURE_TRUE(mDocument, NS_ERROR_FAILURE);

  mMimeType.AssignLiteral("text/html");
  
  
  
  mFlags = aFlags | OutputAbsoluteLinks;

  if (!mDocument->IsScriptEnabled())
    mFlags |= OutputNoScriptContent;

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLCopyEncoder::SetSelection(nsISelection* aSelection)
{
  
  
  
  
  if (!aSelection) 
    return NS_ERROR_NULL_POINTER;
  
  nsCOMPtr<nsIDOMRange> range;
  nsCOMPtr<nsIDOMNode> commonParent;
  PRInt32 count = 0;

  nsresult rv = aSelection->GetRangeCount(&count);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (!count)
    return NS_ERROR_FAILURE;
  
  
  
  
  rv = aSelection->GetRangeAt(0, getter_AddRefs(range));
  NS_ENSURE_SUCCESS(rv, rv);
  if (!range)
    return NS_ERROR_NULL_POINTER;
  range->GetCommonAncestorContainer(getter_AddRefs(commonParent));

  for (nsCOMPtr<nsIContent> selContent(do_QueryInterface(commonParent));
       selContent;
       selContent = selContent->GetParent())
  {
    
    nsIAtom *atom = selContent->Tag();
    if (atom == nsGkAtoms::input ||
        atom == nsGkAtoms::textarea)
    {
      mIsTextWidget = PR_TRUE;
      break;
    }
    else if (atom == nsGkAtoms::body)
    {
      
      
      
      nsCOMPtr<nsIDOMElement> bodyElem = do_QueryInterface(selContent);
      nsAutoString wsVal;
      rv = bodyElem->GetAttribute(NS_LITERAL_STRING("style"), wsVal);
      if (NS_SUCCEEDED(rv) && (kNotFound != wsVal.Find(NS_LITERAL_STRING("pre-wrap"))))
      {
        mIsTextWidget = PR_TRUE;
        break;
      }
    }
  }
  
  
  nsCOMPtr<nsIHTMLDocument> htmlDoc = do_QueryInterface(mDocument);
  if (!(htmlDoc && mDocument->IsHTML()))
    mIsTextWidget = PR_TRUE;
  
  
  if (mIsTextWidget) 
  {
    mSelection = aSelection;
    mMimeType.AssignLiteral("text/plain");
    return NS_OK;
  }
  
  
  
  
  NS_NewDomSelection(getter_AddRefs(mSelection));
  NS_ENSURE_TRUE(mSelection, NS_ERROR_FAILURE);
  nsCOMPtr<nsISelectionPrivate> privSelection( do_QueryInterface(aSelection) );
  NS_ENSURE_TRUE(privSelection, NS_ERROR_FAILURE);
  
  
  nsCOMPtr<nsIEnumerator> enumerator;
  rv = privSelection->GetEnumerator(getter_AddRefs(enumerator));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(enumerator, NS_ERROR_FAILURE);

  
  enumerator->First(); 
  nsCOMPtr<nsISupports> currentItem;
  while ((NS_ENUMERATOR_FALSE == enumerator->IsDone()))
  {
    rv = enumerator->CurrentItem(getter_AddRefs(currentItem));
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(currentItem, NS_ERROR_FAILURE);
    
    range = do_QueryInterface(currentItem);
    NS_ENSURE_TRUE(range, NS_ERROR_FAILURE);
    nsCOMPtr<nsIDOMRange> myRange;
    range->CloneRange(getter_AddRefs(myRange));
    NS_ENSURE_TRUE(myRange, NS_ERROR_FAILURE);

    
    rv = PromoteRange(myRange);
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = mSelection->AddRange(myRange);
    NS_ENSURE_SUCCESS(rv, rv);

    enumerator->Next();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLCopyEncoder::EncodeToStringWithContext(nsAString& aContextString,
                                             nsAString& aInfoString,
                                             nsAString& aEncodedString)
{
  nsresult rv = EncodeToString(aEncodedString);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (mIsTextWidget) return NS_OK;

  
  
  
  
  
  

  
  PRInt32 count = mCommonAncestors.Length();
  PRInt32 i;
  nsCOMPtr<nsIDOMNode> node;
  if (count > 0)
    node = mCommonAncestors.ElementAt(0);

  if (node && IsTextNode(node)) 
  {
    mCommonAncestors.RemoveElementAt(0);
    
    if (mStartDepth) mStartDepth--;
    if (mEndDepth) mEndDepth--;
    
    count--;
  }
  
  i = count;
  while (i > 0)
  {
    node = mCommonAncestors.ElementAt(--i);
    SerializeNodeStart(node, 0, -1, aContextString);
  }
  
  while (i < count)
  {
    node = mCommonAncestors.ElementAt(i++);
    SerializeNodeEnd(node, aContextString);
  }

  
  
  
  nsAutoString infoString;
  infoString.AppendInt(mStartDepth);
  infoString.Append(PRUnichar(','));
  infoString.AppendInt(mEndDepth);
  aInfoString = infoString;
  
  return NS_OK;
}


PRBool
nsHTMLCopyEncoder::IncludeInContext(nsIDOMNode *aNode)
{
  nsCOMPtr<nsIContent> content(do_QueryInterface(aNode));

  if (!content)
    return PR_FALSE;

  nsIAtom *tag = content->Tag();

  return (tag == nsGkAtoms::b        ||
          tag == nsGkAtoms::i        ||
          tag == nsGkAtoms::u        ||
          tag == nsGkAtoms::a        ||
          tag == nsGkAtoms::tt       ||
          tag == nsGkAtoms::s        ||
          tag == nsGkAtoms::big      ||
          tag == nsGkAtoms::small    ||
          tag == nsGkAtoms::strike   ||
          tag == nsGkAtoms::em       ||
          tag == nsGkAtoms::strong   ||
          tag == nsGkAtoms::dfn      ||
          tag == nsGkAtoms::code     ||
          tag == nsGkAtoms::cite     ||
          tag == nsGkAtoms::variable ||
          tag == nsGkAtoms::abbr     ||
          tag == nsGkAtoms::font     ||
          tag == nsGkAtoms::script   ||
          tag == nsGkAtoms::span     ||
          tag == nsGkAtoms::pre      ||
          tag == nsGkAtoms::h1       ||
          tag == nsGkAtoms::h2       ||
          tag == nsGkAtoms::h3       ||
          tag == nsGkAtoms::h4       ||
          tag == nsGkAtoms::h5       ||
          tag == nsGkAtoms::h6);
}


nsresult 
nsHTMLCopyEncoder::PromoteRange(nsIDOMRange *inRange)
{
  if (!inRange) return NS_ERROR_NULL_POINTER;
  nsresult rv;
  nsCOMPtr<nsIDOMNode> startNode, endNode, common;
  PRInt32 startOffset, endOffset;
  
  rv = inRange->GetCommonAncestorContainer(getter_AddRefs(common));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = inRange->GetStartContainer(getter_AddRefs(startNode));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = inRange->GetStartOffset(&startOffset);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = inRange->GetEndContainer(getter_AddRefs(endNode));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = inRange->GetEndOffset(&endOffset);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<nsIDOMNode> opStartNode;
  nsCOMPtr<nsIDOMNode> opEndNode;
  PRInt32 opStartOffset, opEndOffset;
  nsCOMPtr<nsIDOMRange> opRange;
  
  
  rv = GetPromotedPoint( kStart, startNode, startOffset, address_of(opStartNode), &opStartOffset, common);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = GetPromotedPoint( kEnd, endNode, endOffset, address_of(opEndNode), &opEndOffset, common);
  NS_ENSURE_SUCCESS(rv, rv);
  
  
  if ( (opStartNode == common) && (opEndNode == common) )
  {
    rv = PromoteAncestorChain(address_of(opStartNode), &opStartOffset, &opEndOffset);
    NS_ENSURE_SUCCESS(rv, rv);
    opEndNode = opStartNode;
  }
  
  
  rv = inRange->SetStart(opStartNode, opStartOffset);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = inRange->SetEnd(opEndNode, opEndOffset);
  return rv;
} 





nsresult
nsHTMLCopyEncoder::PromoteAncestorChain(nsCOMPtr<nsIDOMNode> *ioNode, 
                                        PRInt32 *ioStartOffset, 
                                        PRInt32 *ioEndOffset) 
{
  if (!ioNode || !ioStartOffset || !ioEndOffset) return NS_ERROR_NULL_POINTER;

  nsresult rv = NS_OK;
  PRBool done = PR_FALSE;

  nsCOMPtr<nsIDOMNode> frontNode, endNode, parent;
  PRInt32 frontOffset, endOffset;

  
  nsCOMPtr<nsINode> node = do_QueryInterface(*ioNode);
  PRBool isEditable = node->IsEditable();
  
  
  while (!done)
  {
    rv = (*ioNode)->GetParentNode(getter_AddRefs(parent));
    if ((NS_FAILED(rv)) || !parent)
      done = PR_TRUE;
    else
    {
      
      
      rv = GetPromotedPoint( kStart, *ioNode, *ioStartOffset, address_of(frontNode), &frontOffset, parent);
      NS_ENSURE_SUCCESS(rv, rv);
      
      rv = GetPromotedPoint( kEnd, *ioNode, *ioEndOffset, address_of(endNode), &endOffset, parent);
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsINode> frontINode = do_QueryInterface(frontNode);
      
      
      if ( (frontNode != parent) || (endNode != parent) || (frontINode->IsEditable() != isEditable) )
        done = PR_TRUE;
      else
      {
        *ioNode = frontNode;  
        *ioStartOffset = frontOffset;
        *ioEndOffset = endOffset;
      }
    }
  }
  return rv;
}

nsresult
nsHTMLCopyEncoder::GetPromotedPoint(Endpoint aWhere, nsIDOMNode *aNode, PRInt32 aOffset, 
                                  nsCOMPtr<nsIDOMNode> *outNode, PRInt32 *outOffset, nsIDOMNode *common)
{
  nsresult rv = NS_OK;
  nsCOMPtr<nsIDOMNode> node = aNode;
  nsCOMPtr<nsIDOMNode> parent = aNode;
  PRInt32 offset = aOffset;
  PRBool  bResetPromotion = PR_FALSE;
  
  
  *outNode = node;
  *outOffset = offset;

  if (common == node) 
    return NS_OK;
    
  if (aWhere == kStart)
  {
    
    if (IsTextNode(aNode))  
    {
      
      if (offset >  0) 
      {
        
        
        
        nsCOMPtr<nsIDOMCharacterData> nodeAsText = do_QueryInterface(aNode);
        nsAutoString text;
        nodeAsText->SubstringData(0, offset, text);
        text.CompressWhitespace();
        if (!text.IsEmpty())
          return NS_OK;
        bResetPromotion = PR_TRUE;
      }
      
      rv = GetNodeLocation(aNode, address_of(parent), &offset);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else
    {
      node = GetChildAt(parent,offset);
    }
    if (!node) node = parent;

    
    
    if (!IsRoot(node) && (parent != common))
    {
      rv = GetNodeLocation(node, address_of(parent), &offset);
      NS_ENSURE_SUCCESS(rv, rv);
      if (offset == -1) return NS_OK; 
      nsIParserService *parserService = nsContentUtils::GetParserService();
      if (!parserService)
        return NS_ERROR_OUT_OF_MEMORY;
      while ((IsFirstNode(node)) && (!IsRoot(parent)) && (parent != common))
      {
        if (bResetPromotion)
        {
          nsCOMPtr<nsIContent> content = do_QueryInterface(parent);
          if (content)
          {
            PRBool isBlock = PR_FALSE;
            parserService->IsBlock(parserService->HTMLAtomTagToId(content->Tag()), isBlock);
            if (isBlock)
            {
              bResetPromotion = PR_FALSE;
            }
          }   
        }
         
        node = parent;
        rv = GetNodeLocation(node, address_of(parent), &offset);
        NS_ENSURE_SUCCESS(rv, rv);
        if (offset == -1)  
        {
          
          parent = node;
          offset = 0;
          break;
        }
      } 
      if (bResetPromotion)
      {
        *outNode = aNode;
        *outOffset = aOffset;
      }
      else
      {
        *outNode = parent;
        *outOffset = offset;
      }
      return rv;
    }
  }
  
  if (aWhere == kEnd)
  {
    
    if (IsTextNode(aNode))  
    {
      
      PRUint32 len;
      GetLengthOfDOMNode(aNode, len);
      if (offset < (PRInt32)len)
      {
        
        
        
        nsCOMPtr<nsIDOMCharacterData> nodeAsText = do_QueryInterface(aNode);
        nsAutoString text;
        nodeAsText->SubstringData(offset, len-offset, text);
        text.CompressWhitespace();
        if (!text.IsEmpty())
          return NS_OK;
        bResetPromotion = PR_TRUE;
      }
      rv = GetNodeLocation(aNode, address_of(parent), &offset);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else
    {
      if (offset) offset--; 
      node = GetChildAt(parent,offset);
    }
    if (!node) node = parent;
    
    
    
    if (!IsRoot(node) && (parent != common))
    {
      rv = GetNodeLocation(node, address_of(parent), &offset);
      NS_ENSURE_SUCCESS(rv, rv);
      if (offset == -1) return NS_OK; 
      nsIParserService *parserService = nsContentUtils::GetParserService();
      if (!parserService)
        return NS_ERROR_OUT_OF_MEMORY;
      while ((IsLastNode(node)) && (!IsRoot(parent)) && (parent != common))
      {
        if (bResetPromotion)
        {
          nsCOMPtr<nsIContent> content = do_QueryInterface(parent);
          if (content)
          {
            PRBool isBlock = PR_FALSE;
            parserService->IsBlock(parserService->HTMLAtomTagToId(content->Tag()), isBlock);
            if (isBlock)
            {
              bResetPromotion = PR_FALSE;
            }
          }   
        }
          
        node = parent;
        rv = GetNodeLocation(node, address_of(parent), &offset);
        NS_ENSURE_SUCCESS(rv, rv);
        if (offset == -1)  
        {
          
          parent = node;
          offset = 0;
          break;
        }
      } 
      if (bResetPromotion)
      {
        *outNode = aNode;
        *outOffset = aOffset;
      }
      else
      {
        *outNode = parent;
        offset++;  
        *outOffset = offset;
      }
      return rv;
    }
  }
  
  return rv;
}

nsCOMPtr<nsIDOMNode> 
nsHTMLCopyEncoder::GetChildAt(nsIDOMNode *aParent, PRInt32 aOffset)
{
  nsCOMPtr<nsIDOMNode> resultNode;
  
  if (!aParent) 
    return resultNode;
  
  nsCOMPtr<nsIContent> content = do_QueryInterface(aParent);
  NS_PRECONDITION(content, "null content in nsHTMLCopyEncoder::GetChildAt");

  resultNode = do_QueryInterface(content->GetChildAt(aOffset));

  return resultNode;
}

PRBool 
nsHTMLCopyEncoder::IsMozBR(nsIDOMNode* aNode)
{
  if (IsTag(aNode, nsGkAtoms::br))
  {
    nsCOMPtr<nsIDOMElement> elem = do_QueryInterface(aNode);
    if (elem)
    {
      nsAutoString typeAttrName(NS_LITERAL_STRING("type"));
      nsAutoString typeAttrVal;
      nsresult rv = elem->GetAttribute(typeAttrName, typeAttrVal);
      ToLowerCase(typeAttrVal);
      if (NS_SUCCEEDED(rv) && (typeAttrVal.EqualsLiteral("_moz")))
        return PR_TRUE;
    }
    return PR_FALSE;
  }
  return PR_FALSE;
}

nsresult 
nsHTMLCopyEncoder::GetNodeLocation(nsIDOMNode *inChild,
                                   nsCOMPtr<nsIDOMNode> *outParent,
                                   PRInt32 *outOffset)
{
  NS_ASSERTION((inChild && outParent && outOffset), "bad args");
  nsresult result = NS_ERROR_NULL_POINTER;
  if (inChild && outParent && outOffset)
  {
    result = inChild->GetParentNode(getter_AddRefs(*outParent));
    if ((NS_SUCCEEDED(result)) && (*outParent))
    {
      nsCOMPtr<nsIContent> content = do_QueryInterface(*outParent);
      nsCOMPtr<nsIContent> cChild = do_QueryInterface(inChild);
      if (!cChild || !content)
        return NS_ERROR_NULL_POINTER;

      *outOffset = content->IndexOf(cChild);
    }
  }
  return result;
}

PRBool
nsHTMLCopyEncoder::IsRoot(nsIDOMNode* aNode)
{
  if (aNode)
  {
    if (mIsTextWidget) 
      return (IsTag(aNode, nsGkAtoms::div));
    else
      return (IsTag(aNode, nsGkAtoms::body) || 
              IsTag(aNode, nsGkAtoms::td)   ||
              IsTag(aNode, nsGkAtoms::th));
  }
  return PR_FALSE;
}

PRBool
nsHTMLCopyEncoder::IsFirstNode(nsIDOMNode *aNode)
{
  nsCOMPtr<nsIDOMNode> parent;
  PRInt32 offset, j=0;
  nsresult rv = GetNodeLocation(aNode, address_of(parent), &offset);
  if (NS_FAILED(rv)) 
  {
    NS_NOTREACHED("failure in IsFirstNode");
    return PR_FALSE;
  }
  if (offset == 0)  
    return PR_TRUE;
  if (!parent)  
    return PR_TRUE;
  
  
  
  
  
  
  nsCOMPtr<nsIDOMNodeList> childList;
  nsCOMPtr<nsIDOMNode> child;

  rv = parent->GetChildNodes(getter_AddRefs(childList));
  if (NS_FAILED(rv) || !childList) 
  {
    NS_NOTREACHED("failure in IsFirstNode");
    return PR_TRUE;
  }
  while (j < offset)
  {
    childList->Item(j, getter_AddRefs(child));
    if (!IsEmptyTextContent(child)) 
      return PR_FALSE;
    j++;
  }
  return PR_TRUE;
}


PRBool
nsHTMLCopyEncoder::IsLastNode(nsIDOMNode *aNode)
{
  nsCOMPtr<nsIDOMNode> parent;
  PRInt32 offset,j;
  PRUint32 numChildren;
  nsresult rv = GetNodeLocation(aNode, address_of(parent), &offset);
  if (NS_FAILED(rv)) 
  {
    NS_NOTREACHED("failure in IsLastNode");
    return PR_FALSE;
  }
  GetLengthOfDOMNode(parent, numChildren); 
  if (offset+1 == (PRInt32)numChildren) 
    return PR_TRUE;
  if (!parent)
    return PR_TRUE;
  
  
  
  
  
  j = (PRInt32)numChildren-1;
  nsCOMPtr<nsIDOMNodeList>childList;
  nsCOMPtr<nsIDOMNode> child;
  rv = parent->GetChildNodes(getter_AddRefs(childList));
  if (NS_FAILED(rv) || !childList) 
  {
    NS_NOTREACHED("failure in IsLastNode");
    return PR_TRUE;
  }
  while (j > offset)
  {
    childList->Item(j, getter_AddRefs(child));
    j--;
    if (IsMozBR(child))  
      continue;
    if (!IsEmptyTextContent(child)) 
      return PR_FALSE;
  }
  return PR_TRUE;
}

PRBool
nsHTMLCopyEncoder::IsEmptyTextContent(nsIDOMNode* aNode)
{
  nsCOMPtr<nsIContent> cont = do_QueryInterface(aNode);
  return cont && cont->TextIsOnlyWhitespace();
}

nsresult NS_NewHTMLCopyTextEncoder(nsIDocumentEncoder** aResult); 

nsresult
NS_NewHTMLCopyTextEncoder(nsIDocumentEncoder** aResult)
{
  *aResult = new nsHTMLCopyEncoder;
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;
 NS_ADDREF(*aResult);
 return NS_OK;
}
