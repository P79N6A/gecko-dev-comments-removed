






































#include "nscore.h"
#include "nsLayoutCID.h"
#include "nsIAtom.h"
#include "nsStaticAtom.h"
#include "nsString.h"
#include "nsIEnumerator.h"
#include "nsIContent.h"
#include "nsIContentIterator.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMRange.h"
#include "nsIRangeUtils.h"
#include "nsISelection.h"
#include "nsIPlaintextEditor.h"
#include "nsTextServicesDocument.h"
#include "nsFilteredContentIterator.h"

#include "nsIDOMElement.h"
#include "nsIDOMHTMLElement.h"
#include "nsIDOMHTMLDocument.h"

#include "nsLWBrkCIID.h"
#include "nsIWordBreaker.h"
#include "nsIServiceManager.h"

#define LOCK_DOC(doc)
#define UNLOCK_DOC(doc)


class OffsetEntry
{
public:
  OffsetEntry(nsIDOMNode *aNode, PRInt32 aOffset, PRInt32 aLength)
    : mNode(aNode), mNodeOffset(0), mStrOffset(aOffset), mLength(aLength),
      mIsInsertedText(PR_FALSE), mIsValid(PR_TRUE)
  {
    if (mStrOffset < 1)
      mStrOffset = 0;

    if (mLength < 1)
      mLength = 0;
  }

  virtual ~OffsetEntry()
  {
    mNode       = 0;
    mNodeOffset = 0;
    mStrOffset  = 0;
    mLength     = 0;
    mIsValid    = PR_FALSE;
  }

  nsIDOMNode *mNode;
  PRInt32 mNodeOffset;
  PRInt32 mStrOffset;
  PRInt32 mLength;
  PRBool  mIsInsertedText;
  PRBool  mIsValid;
};

#define TS_ATOM(name_, value_) nsIAtom* nsTextServicesDocument::name_ = 0;
#include "nsTSAtomList.h"
#undef TS_ATOM

nsIRangeUtils* nsTextServicesDocument::sRangeHelper;

nsTextServicesDocument::nsTextServicesDocument()
{
  mRefCnt         = 0;

  mSelStartIndex  = -1;
  mSelStartOffset = -1;
  mSelEndIndex    = -1;
  mSelEndOffset   = -1;

  mIteratorStatus = eIsDone;
}

nsTextServicesDocument::~nsTextServicesDocument()
{
  nsCOMPtr<nsIEditor> editor (do_QueryReferent(mEditor));
  if (editor && mNotifier)
    editor->RemoveEditActionListener(mNotifier);

  ClearOffsetTable(&mOffsetTable);
}


void
nsTextServicesDocument::RegisterAtoms()
{
  static const nsStaticAtom ts_atoms[] = {
#define TS_ATOM(name_, value_) { value_, &name_ },
#include "nsTSAtomList.h"
#undef TS_ATOM
  };

  NS_RegisterStaticAtoms(ts_atoms, NS_ARRAY_LENGTH(ts_atoms));
}


void
nsTextServicesDocument::Shutdown()
{
  NS_IF_RELEASE(sRangeHelper);
}

#define DEBUG_TEXT_SERVICES__DOCUMENT_REFCNT 1

#ifdef DEBUG_TEXT_SERVICES__DOCUMENT_REFCNT

nsrefcnt nsTextServicesDocument::AddRef(void)
{
  return ++mRefCnt;
}

nsrefcnt nsTextServicesDocument::Release(void)
{
  NS_PRECONDITION(0 != mRefCnt, "dup release");
  if (--mRefCnt == 0) {
    NS_DELETEXPCOM(this);
    return 0;
  }
  return mRefCnt;
}

#else

NS_IMPL_ADDREF(nsTextServicesDocument)
NS_IMPL_RELEASE(nsTextServicesDocument)

#endif

NS_IMPL_QUERY_INTERFACE1(nsTextServicesDocument, nsITextServicesDocument)

NS_IMETHODIMP
nsTextServicesDocument::InitWithEditor(nsIEditor *aEditor)
{
  nsresult result = NS_OK;
  nsCOMPtr<nsISelectionController> selCon;
  nsCOMPtr<nsIDOMDocument> doc;

  if (!aEditor)
    return NS_ERROR_NULL_POINTER;

  LOCK_DOC(this);

  
  

  result = aEditor->GetSelectionController(getter_AddRefs(selCon));

  if (NS_FAILED(result))
  {
    UNLOCK_DOC(this);
    return result;
  }

  if (!selCon || (mSelCon && selCon != mSelCon))
  {
    UNLOCK_DOC(this);
    return NS_ERROR_FAILURE;
  }

  if (!mSelCon)
    mSelCon = selCon;

  
  

  result = aEditor->GetDocument(getter_AddRefs(doc));

  if (NS_FAILED(result))
  {
    UNLOCK_DOC(this);
    return result;
  }

  if (!doc || (mDOMDocument && doc != mDOMDocument))
  {
    UNLOCK_DOC(this);
    return NS_ERROR_FAILURE;
  }

  if (!mDOMDocument)
  {
    mDOMDocument = doc;

    result = CreateDocumentContentIterator(getter_AddRefs(mIterator));

    if (NS_FAILED(result))
    {
      UNLOCK_DOC(this);
      return result;
    }

    mIteratorStatus = nsTextServicesDocument::eIsDone;

    result = FirstBlock();

    if (NS_FAILED(result))
    {
      UNLOCK_DOC(this);
      return result;
    }
  }

  mEditor = do_GetWeakReference(aEditor);
  nsTSDNotifier *notifier = new nsTSDNotifier(this);

  if (!notifier)
  {
    UNLOCK_DOC(this);
    return NS_ERROR_OUT_OF_MEMORY;
  }

  mNotifier = do_QueryInterface(notifier);

  result = aEditor->AddEditActionListener(mNotifier);

  UNLOCK_DOC(this);

  return result;
}

NS_IMETHODIMP 
nsTextServicesDocument::GetDocument(nsIDOMDocument **aDoc)
{
  if (!aDoc)
    return NS_ERROR_NULL_POINTER;

  *aDoc = nsnull; 
  if (!mDOMDocument)
    return NS_ERROR_NOT_INITIALIZED;

  *aDoc = mDOMDocument;
  NS_ADDREF(*aDoc);

  return NS_OK;
}

NS_IMETHODIMP
nsTextServicesDocument::SetExtent(nsIDOMRange* aDOMRange)
{
  NS_ENSURE_ARG_POINTER(aDOMRange);
  NS_ENSURE_TRUE(mDOMDocument, NS_ERROR_FAILURE);

  LOCK_DOC(this);

  
  

  nsresult result = aDOMRange->CloneRange(getter_AddRefs(mExtent));

  if (NS_FAILED(result))
  {
    UNLOCK_DOC(this);
    return result;
  }

  

  result = CreateContentIterator(mExtent, getter_AddRefs(mIterator));

  if (NS_FAILED(result))
  {
    UNLOCK_DOC(this);
    return result;
  }

  
  

  mIteratorStatus = nsTextServicesDocument::eIsDone;

  result = FirstBlock();

  UNLOCK_DOC(this);

  return result;
}

NS_IMETHODIMP
nsTextServicesDocument::ExpandRangeToWordBoundaries(nsIDOMRange *aRange)
{
  NS_ENSURE_ARG_POINTER(aRange);

  

  nsCOMPtr<nsIDOMNode> rngStartNode, rngEndNode;
  PRInt32 rngStartOffset, rngEndOffset;

  nsresult result =  GetRangeEndPoints(aRange,
                                       getter_AddRefs(rngStartNode),
                                       &rngStartOffset,
                                       getter_AddRefs(rngEndNode),
                                       &rngEndOffset);

  NS_ENSURE_SUCCESS(result, result);

  

  nsCOMPtr<nsIContentIterator> iter;
  result = CreateContentIterator(aRange, getter_AddRefs(iter));

  NS_ENSURE_SUCCESS(result, result);

  

  TSDIteratorStatus iterStatus;

  result = FirstTextNode(iter, &iterStatus);
  NS_ENSURE_SUCCESS(result, result);

  if (iterStatus == nsTextServicesDocument::eIsDone)
  {
    
    return NS_OK;
  }

  nsINode *firstText = iter->GetCurrentNode();
  NS_ENSURE_TRUE(firstText, NS_ERROR_FAILURE);

  

  result = LastTextNode(iter, &iterStatus);
  NS_ENSURE_SUCCESS(result, result);

  if (iterStatus == nsTextServicesDocument::eIsDone)
  {
    
    
    NS_ASSERTION(PR_FALSE, "Found a first without a last!");
    return NS_ERROR_FAILURE;
  }

  nsINode *lastText = iter->GetCurrentNode();
  NS_ENSURE_TRUE(lastText, NS_ERROR_FAILURE);

  

  nsCOMPtr<nsIDOMNode> firstTextNode = do_QueryInterface(firstText);
  NS_ENSURE_TRUE(firstTextNode, NS_ERROR_FAILURE);

  if (rngStartNode != firstTextNode)
  {
    
    rngStartNode = firstTextNode;
    rngStartOffset = 0;
  }

  nsCOMPtr<nsIDOMNode> lastTextNode = do_QueryInterface(lastText);
  NS_ENSURE_TRUE(lastTextNode, NS_ERROR_FAILURE);

  if (rngEndNode != lastTextNode)
  {
    
    rngEndNode = lastTextNode;
    nsAutoString str;
    result = lastTextNode->GetNodeValue(str);
    rngEndOffset = str.Length();
  }

  
  

  nsCOMPtr<nsIContentIterator> docIter;
  result = CreateDocumentContentIterator(getter_AddRefs(docIter));
  NS_ENSURE_SUCCESS(result, result);

  
  

  result = docIter->PositionAt(firstText);
  NS_ENSURE_SUCCESS(result, result);

  iterStatus = nsTextServicesDocument::eValid;

  nsTArray<OffsetEntry*> offsetTable;
  nsAutoString blockStr;

  result = CreateOffsetTable(&offsetTable, docIter, &iterStatus,
                             nsnull, &blockStr);
  if (NS_FAILED(result))
  {
    ClearOffsetTable(&offsetTable);
    return result;
  }

  nsCOMPtr<nsIDOMNode> wordStartNode, wordEndNode;
  PRInt32 wordStartOffset, wordEndOffset;

  result = FindWordBounds(&offsetTable, &blockStr,
                          rngStartNode, rngStartOffset,
                          getter_AddRefs(wordStartNode), &wordStartOffset,
                          getter_AddRefs(wordEndNode), &wordEndOffset);

  ClearOffsetTable(&offsetTable);

  NS_ENSURE_SUCCESS(result, result);

  rngStartNode = wordStartNode;
  rngStartOffset = wordStartOffset;

  
  

  result = docIter->PositionAt(lastText);
  NS_ENSURE_SUCCESS(result, result);

  iterStatus = nsTextServicesDocument::eValid;

  result = CreateOffsetTable(&offsetTable, docIter, &iterStatus,
                             nsnull, &blockStr);
  if (NS_FAILED(result))
  {
    ClearOffsetTable(&offsetTable);
    return result;
  }

  result = FindWordBounds(&offsetTable, &blockStr,
                          rngEndNode, rngEndOffset,
                          getter_AddRefs(wordStartNode), &wordStartOffset,
                          getter_AddRefs(wordEndNode), &wordEndOffset);

  ClearOffsetTable(&offsetTable);

  NS_ENSURE_SUCCESS(result, result);

  
  
  

  if (rngEndNode != wordStartNode || rngEndOffset != wordStartOffset ||
     (rngEndNode == rngStartNode  && rngEndOffset == rngStartOffset))
  {
    rngEndNode = wordEndNode;
    rngEndOffset = wordEndOffset;
  }

  
  

  result = aRange->SetEnd(rngEndNode, rngEndOffset);
  NS_ENSURE_SUCCESS(result, result);

  return aRange->SetStart(rngStartNode, rngStartOffset);
}

NS_IMETHODIMP
nsTextServicesDocument::SetFilter(nsITextServicesFilter *aFilter)
{
  
  mTxtSvcFilter = aFilter;

  return NS_OK;
}

NS_IMETHODIMP
nsTextServicesDocument::GetCurrentTextBlock(nsString *aStr)
{
  nsresult result;

  if (!aStr)
    return NS_ERROR_NULL_POINTER;

  aStr->Truncate();

  if (!mIterator)
    return NS_ERROR_FAILURE;

  LOCK_DOC(this);

  result = CreateOffsetTable(&mOffsetTable, mIterator, &mIteratorStatus,
                             mExtent, aStr);

  UNLOCK_DOC(this);

  return result;
}

NS_IMETHODIMP
nsTextServicesDocument::FirstBlock()
{
  NS_ENSURE_TRUE(mIterator, NS_ERROR_FAILURE);

  LOCK_DOC(this);

  nsresult result = FirstTextNode(mIterator, &mIteratorStatus);

  if (NS_FAILED(result))
  {
    UNLOCK_DOC(this);
    return result;
  }

  
  

  if (mIteratorStatus == nsTextServicesDocument::eValid)
  {
    mPrevTextBlock  = nsnull;
    result = GetFirstTextNodeInNextBlock(getter_AddRefs(mNextTextBlock));
  }
  else
  {
    

    mPrevTextBlock  = nsnull;
    mNextTextBlock  = nsnull;
  }

  UNLOCK_DOC(this);

  return result;
}

NS_IMETHODIMP
nsTextServicesDocument::LastSelectedBlock(TSDBlockSelectionStatus *aSelStatus,
                                          PRInt32 *aSelOffset,
                                          PRInt32 *aSelLength)
{
  nsresult result = NS_OK;

  if (!aSelStatus || !aSelOffset || !aSelLength)
    return NS_ERROR_NULL_POINTER;

  LOCK_DOC(this);

  mIteratorStatus = nsTextServicesDocument::eIsDone;

  *aSelStatus = nsITextServicesDocument::eBlockNotFound;
  *aSelOffset = *aSelLength = -1;

  if (!mSelCon || !mIterator)
  {
    UNLOCK_DOC(this);
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsISelection> selection;
  PRBool isCollapsed = PR_FALSE;

  result = mSelCon->GetSelection(nsISelectionController::SELECTION_NORMAL, getter_AddRefs(selection));

  if (NS_FAILED(result))
  {
    UNLOCK_DOC(this);
    return result;
  }

  result = selection->GetIsCollapsed(&isCollapsed);

  if (NS_FAILED(result))
  {
    UNLOCK_DOC(this);
    return result;
  }

  nsCOMPtr<nsIContentIterator> iter;
  nsCOMPtr<nsIDOMRange>        range;
  nsCOMPtr<nsIDOMNode>         parent;
  PRInt32                      i, rangeCount, offset;

  if (isCollapsed)
  {
    
    
    
    

    result = selection->GetRangeAt(0, getter_AddRefs(range));

    if (NS_FAILED(result))
    {
      UNLOCK_DOC(this);
      return result;
    }

    if (!range)
    {
      UNLOCK_DOC(this);
      return NS_ERROR_FAILURE;
    }

    result = range->GetStartContainer(getter_AddRefs(parent));

    if (NS_FAILED(result))
    {
      UNLOCK_DOC(this);
      return result;
    }

    if (!parent)
    {
      UNLOCK_DOC(this);
      return NS_ERROR_FAILURE;
    }

    result = range->GetStartOffset(&offset);

    if (NS_FAILED(result))
    {
      UNLOCK_DOC(this);
      return result;
    }

    if (IsTextNode(parent))
    {
      
      
      

      nsCOMPtr<nsIContent> content(do_QueryInterface(parent));

      if (!content)
      {
        UNLOCK_DOC(this);
        return NS_ERROR_FAILURE;
      }

      result = mIterator->PositionAt(content);

      if (NS_FAILED(result))
      {
        UNLOCK_DOC(this);
        return result;
      }

      result = FirstTextNodeInCurrentBlock(mIterator);

      if (NS_FAILED(result))
      {
        UNLOCK_DOC(this);
        return result;
      }

      mIteratorStatus = nsTextServicesDocument::eValid;

      result = CreateOffsetTable(&mOffsetTable, mIterator, &mIteratorStatus,
                                 mExtent, nsnull);

      if (NS_FAILED(result))
      {
        UNLOCK_DOC(this);
        return result;
      }

      result = GetSelection(aSelStatus, aSelOffset, aSelLength);

      if (NS_FAILED(result))
      {
        UNLOCK_DOC(this);
        return result;
      }

      if (*aSelStatus == nsITextServicesDocument::eBlockContains)
        result = SetSelectionInternal(*aSelOffset, *aSelLength, PR_FALSE);
    }
    else
    {
      
      
      
      

      result = CreateDocumentContentRootToNodeOffsetRange(parent, offset, PR_FALSE, getter_AddRefs(range));

      if (NS_FAILED(result))
      {
        UNLOCK_DOC(this);
        return result;
      }

      result = range->GetCollapsed(&isCollapsed);

      if (NS_FAILED(result))
      {
        UNLOCK_DOC(this);
        return result;
      }

      if (isCollapsed)
      {
        
        

        UNLOCK_DOC(this);
        return NS_OK;
      }

      result = CreateContentIterator(range, getter_AddRefs(iter));

      if (NS_FAILED(result))
      {
        UNLOCK_DOC(this);
        return result;
      }

      iter->First();

      nsCOMPtr<nsIContent> content;
      while (!iter->IsDone())
      {
        content = do_QueryInterface(iter->GetCurrentNode());

        if (IsTextNode(content))
          break;

        content = nsnull;

        iter->Next();
      }

      if (!content)
      {
        UNLOCK_DOC(this);
        return NS_OK;
      }

      result = mIterator->PositionAt(content);

      if (NS_FAILED(result))
      {
        UNLOCK_DOC(this);
        return result;
      }

      result = FirstTextNodeInCurrentBlock(mIterator);

      if (NS_FAILED(result))
      {
        UNLOCK_DOC(this);
        return result;
      }

      mIteratorStatus = nsTextServicesDocument::eValid;

      result = CreateOffsetTable(&mOffsetTable, mIterator, &mIteratorStatus,
                                 mExtent, nsnull);

      if (NS_FAILED(result))
      {
        UNLOCK_DOC(this);
        return result;
      }

      result = GetSelection(aSelStatus, aSelOffset, aSelLength);

      if (NS_FAILED(result))
      {
        UNLOCK_DOC(this);
        return result;
      }
    }

    UNLOCK_DOC(this);

    return result;
  }

  
  
  
  
  

  result = selection->GetRangeCount(&rangeCount);

  if (NS_FAILED(result))
  {
    UNLOCK_DOC(this);
    return result;
  }

  NS_ASSERTION(rangeCount > 0, "Unexpected range count!");

  if (rangeCount <= 0)
  {
    UNLOCK_DOC(this);
    return NS_OK;
  }

  
  

  for (i = rangeCount - 1; i >= 0; i--)
  {
    

    result = selection->GetRangeAt(i, getter_AddRefs(range));

    if (NS_FAILED(result))
    {
      UNLOCK_DOC(this);
      return result;
    }

    

    result = CreateContentIterator(range, getter_AddRefs(iter));

    if (NS_FAILED(result))
    {
      UNLOCK_DOC(this);
      return result;
    }

    iter->Last();

    

    while (!iter->IsDone())
    {
      nsCOMPtr<nsIContent> content = do_QueryInterface(iter->GetCurrentNode());

      if (IsTextNode(content))
      {
        
        
        

        result = mIterator->PositionAt(content);

        if (NS_FAILED(result))
        {
          UNLOCK_DOC(this);
          return result;
        }

        result = FirstTextNodeInCurrentBlock(mIterator);

        if (NS_FAILED(result))
        {
          UNLOCK_DOC(this);
          return result;
        }

        mIteratorStatus = nsTextServicesDocument::eValid;

        result = CreateOffsetTable(&mOffsetTable, mIterator, &mIteratorStatus,
                                   mExtent, nsnull);

        if (NS_FAILED(result))
        {
          UNLOCK_DOC(this);
          return result;
        }

        result = GetSelection(aSelStatus, aSelOffset, aSelLength);

        UNLOCK_DOC(this);

        return result;

      }

      iter->Prev();
    }
  }

  
  
  
  

  result = selection->GetRangeAt(rangeCount - 1, getter_AddRefs(range));

  if (NS_FAILED(result))
  {
    UNLOCK_DOC(this);
    return result;
  }

  if (!range)
  {
    UNLOCK_DOC(this);
    return NS_ERROR_FAILURE;
  }

  result = range->GetEndContainer(getter_AddRefs(parent));

  if (NS_FAILED(result))
  {
    UNLOCK_DOC(this);
    return result;
  }

  if (!parent)
  {
    UNLOCK_DOC(this);
    return NS_ERROR_FAILURE;
  }

  result = range->GetEndOffset(&offset);

  if (NS_FAILED(result))
  {
    UNLOCK_DOC(this);
    return result;
  }

  result = CreateDocumentContentRootToNodeOffsetRange(parent, offset, PR_FALSE, getter_AddRefs(range));

  if (NS_FAILED(result))
  {
    UNLOCK_DOC(this);
    return result;
  }

  result = range->GetCollapsed(&isCollapsed);

  if (NS_FAILED(result))
  {
    UNLOCK_DOC(this);
    return result;
  }

  if (isCollapsed)
  {
    
    

    UNLOCK_DOC(this);
    return NS_OK;
  }

  result = CreateContentIterator(range, getter_AddRefs(iter));

  if (NS_FAILED(result))
  {
    UNLOCK_DOC(this);
    return result;
  }

  iter->First();

  while (!iter->IsDone())
  {
    nsCOMPtr<nsIContent> content = do_QueryInterface(iter->GetCurrentNode());

    if (IsTextNode(content))
    {
      
      

      result = mIterator->PositionAt(content);

      if (NS_FAILED(result))
      {
        UNLOCK_DOC(this);
        return result;
      }

      result = FirstTextNodeInCurrentBlock(mIterator);

      if (NS_FAILED(result))
      {
        UNLOCK_DOC(this);
        return result;
      }


      mIteratorStatus = nsTextServicesDocument::eValid;

      result = CreateOffsetTable(&mOffsetTable, mIterator, &mIteratorStatus,
                                 mExtent, nsnull);

      if (NS_FAILED(result))
      {
        UNLOCK_DOC(this);
        return result;
      }

      result = GetSelection(aSelStatus, aSelOffset, aSelLength);

      UNLOCK_DOC(this);

      return result;
    }

    iter->Next();
  }

  
  

  UNLOCK_DOC(this);

  return NS_OK;
}

NS_IMETHODIMP
nsTextServicesDocument::PrevBlock()
{
  nsresult result = NS_OK;

  if (!mIterator)
    return NS_ERROR_FAILURE;

  LOCK_DOC(this);

  if (mIteratorStatus == nsTextServicesDocument::eIsDone)
    return NS_OK;

  switch (mIteratorStatus)
  {
    case nsTextServicesDocument::eValid:
    case nsTextServicesDocument::eNext:

      result = FirstTextNodeInPrevBlock(mIterator);

      if (NS_FAILED(result))
      {
        mIteratorStatus = nsTextServicesDocument::eIsDone;
        UNLOCK_DOC(this);
        return result;
      }

      if (mIterator->IsDone())
      {
        mIteratorStatus = nsTextServicesDocument::eIsDone;
        UNLOCK_DOC(this);
        return NS_OK;
      }

      mIteratorStatus = nsTextServicesDocument::eValid;
      break;

    case nsTextServicesDocument::ePrev:

      
      

      mIteratorStatus = nsTextServicesDocument::eValid;
      break;

    default:

      mIteratorStatus = nsTextServicesDocument::eIsDone;
      break;
  }

  
  

  if (mIteratorStatus == nsTextServicesDocument::eValid)
  {
    result = GetFirstTextNodeInPrevBlock(getter_AddRefs(mPrevTextBlock));
    result = GetFirstTextNodeInNextBlock(getter_AddRefs(mNextTextBlock));
  }
  else
  {
    

    mPrevTextBlock = nsnull;
    mNextTextBlock = nsnull;
  }

  UNLOCK_DOC(this);

  return result;
}

NS_IMETHODIMP
nsTextServicesDocument::NextBlock()
{
  nsresult result = NS_OK;

  if (!mIterator)
    return NS_ERROR_FAILURE;

  LOCK_DOC(this);

  if (mIteratorStatus == nsTextServicesDocument::eIsDone)
    return NS_OK;

  switch (mIteratorStatus)
  {
    case nsTextServicesDocument::eValid:

      

      result = FirstTextNodeInNextBlock(mIterator);

      if (NS_FAILED(result))
      {
        mIteratorStatus = nsTextServicesDocument::eIsDone;
        UNLOCK_DOC(this);
        return result;
      }

      if (mIterator->IsDone())
      {
        mIteratorStatus = nsTextServicesDocument::eIsDone;
        UNLOCK_DOC(this);
        return NS_OK;
      }

      mIteratorStatus = nsTextServicesDocument::eValid;
      break;

    case nsTextServicesDocument::eNext:

      
      

      mIteratorStatus = nsTextServicesDocument::eValid;
      break;

    case nsTextServicesDocument::ePrev:

      
      
      

    default:

      mIteratorStatus = nsTextServicesDocument::eIsDone;
      break;
  }

  
  

  if (mIteratorStatus == nsTextServicesDocument::eValid)
  {
    result = GetFirstTextNodeInPrevBlock(getter_AddRefs(mPrevTextBlock));
    result = GetFirstTextNodeInNextBlock(getter_AddRefs(mNextTextBlock));
  }
  else
  {
    

    mPrevTextBlock = nsnull;
    mNextTextBlock = nsnull;
  }


  UNLOCK_DOC(this);

  return result;
}

NS_IMETHODIMP
nsTextServicesDocument::IsDone(PRBool *aIsDone)
{
  if (!aIsDone)
    return NS_ERROR_NULL_POINTER;

  *aIsDone = PR_FALSE;

  if (!mIterator)
    return NS_ERROR_FAILURE;

  LOCK_DOC(this);

  *aIsDone = (mIteratorStatus == nsTextServicesDocument::eIsDone) ? PR_TRUE : PR_FALSE;

  UNLOCK_DOC(this);

  return NS_OK;
}

NS_IMETHODIMP
nsTextServicesDocument::SetSelection(PRInt32 aOffset, PRInt32 aLength)
{
  nsresult result;

  if (!mSelCon || aOffset < 0 || aLength < 0)
    return NS_ERROR_FAILURE;

  LOCK_DOC(this);

  result = SetSelectionInternal(aOffset, aLength, PR_TRUE);

  UNLOCK_DOC(this);

  
  
  

  return result;
}

NS_IMETHODIMP
nsTextServicesDocument::ScrollSelectionIntoView()
{
  nsresult result;

  if (!mSelCon)
    return NS_ERROR_FAILURE;

  LOCK_DOC(this);

  
  
  result = mSelCon->ScrollSelectionIntoView(nsISelectionController::SELECTION_NORMAL, nsISelectionController::SELECTION_FOCUS_REGION, PR_TRUE);

  UNLOCK_DOC(this);

  return result;
}

NS_IMETHODIMP
nsTextServicesDocument::DeleteSelection()
{
  nsresult result = NS_OK;

  
  nsCOMPtr<nsIEditor> editor (do_QueryReferent(mEditor));
  NS_ASSERTION(editor, "DeleteSelection called without an editor present!"); 
  NS_ASSERTION(SelectionIsValid(), "DeleteSelection called without a valid selection!"); 

  if (!editor || !SelectionIsValid())
    return NS_ERROR_FAILURE;

  if (SelectionIsCollapsed())
    return NS_OK;

  LOCK_DOC(this);

  
  
  
  
  

  
  
  

  nsCOMPtr<nsIDOMNode> origStartNode, origEndNode;
  PRInt32 origStartOffset = 0, origEndOffset = 0;

  if (mExtent)
  {
    result = GetRangeEndPoints(mExtent,
                               getter_AddRefs(origStartNode), &origStartOffset,
                               getter_AddRefs(origEndNode), &origEndOffset);

    if (NS_FAILED(result))
    {
      UNLOCK_DOC(this);
      return result;
    }
  }

  PRInt32 i, selLength;
  OffsetEntry *entry, *newEntry;

  for (i = mSelStartIndex; i <= mSelEndIndex; i++)
  {
    entry = mOffsetTable[i];

    if (i == mSelStartIndex)
    {
      
      
      

      if (entry->mIsInsertedText)
      {
        
        
        
        

        selLength = 0;
      }
      else
        selLength = entry->mLength - (mSelStartOffset - entry->mStrOffset);

      if (selLength > 0 && mSelStartOffset > entry->mStrOffset)
      {
        
        
        
        

        result = SplitOffsetEntry(i, selLength);

        if (NS_FAILED(result))
        {
          UNLOCK_DOC(this);
          return result;
        }

        

        ++mSelStartIndex;
        ++mSelEndIndex;
        ++i;

        entry = mOffsetTable[i];
      }


      if (selLength > 0 && mSelStartIndex < mSelEndIndex)
      {
        
        

        entry->mIsValid = PR_FALSE;
      }
    }

  
  
  
  
  

    if (i == mSelEndIndex)
    {
      if (entry->mIsInsertedText)
      {
        
        
        
        

        entry->mIsValid = PR_FALSE;
      }
      else
      {
        
        
        

        selLength = mSelEndOffset - entry->mStrOffset;

        if (selLength > 0 && mSelEndOffset < entry->mStrOffset + entry->mLength)
        {
          
          

          result = SplitOffsetEntry(i, entry->mLength - selLength);

          if (NS_FAILED(result))
          {
            UNLOCK_DOC(this);
            return result;
          }

          

          newEntry = mOffsetTable[i+1];
          newEntry->mNodeOffset = entry->mNodeOffset;
        }


        if (selLength > 0 && mSelEndOffset == entry->mStrOffset + entry->mLength)
        {
          
          

          entry->mIsValid = PR_FALSE;
        }
      }
    }

    if (i != mSelStartIndex && i != mSelEndIndex)
    {
      
      

      entry->mIsValid = PR_FALSE;
    }
  }

  

  AdjustContentIterator();

  

  result = editor->DeleteSelection(nsIEditor::ePrevious);

  if (NS_FAILED(result))
  {
    UNLOCK_DOC(this);
    return result;
  }

  
  
  

  if (origStartNode && origEndNode)
  {
    nsCOMPtr<nsIDOMNode> curStartNode, curEndNode;
    PRInt32 curStartOffset = 0, curEndOffset = 0;

    result = GetRangeEndPoints(mExtent,
                               getter_AddRefs(curStartNode), &curStartOffset,
                               getter_AddRefs(curEndNode), &curEndOffset);

    if (NS_FAILED(result))
    {
      UNLOCK_DOC(this);
      return result;
    }

    if (origStartNode != curStartNode || origEndNode != curEndNode)
    {
      
      

      nsCOMPtr<nsIContent> curContent;

      if (mIteratorStatus != nsTextServicesDocument::eIsDone)
      {
        
        
        

        curContent = do_QueryInterface(mIterator->GetCurrentNode());
      }

      

      result = CreateContentIterator(mExtent, getter_AddRefs(mIterator));

      if (NS_FAILED(result))
      {
        UNLOCK_DOC(this);
        return result;
      }

      
      

      if (curContent)
      {
        result = mIterator->PositionAt(curContent);

        if (NS_FAILED(result))
          mIteratorStatus = eIsDone;
        else
          mIteratorStatus = eValid;
      }
    }
  }

  entry = 0;

  
  

  for (i = mSelStartIndex; !entry && i >= 0; i--)
  {
    entry = mOffsetTable[i];

    if (!entry->mIsValid)
      entry = 0;
    else
    {
      mSelStartIndex  = mSelEndIndex  = i;
      mSelStartOffset = mSelEndOffset = entry->mStrOffset + entry->mLength;
    }
  }

  
  

  for (i = mSelEndIndex; !entry && i < PRInt32(mOffsetTable.Length()); i++)
  {
    entry = mOffsetTable[i];

    if (!entry->mIsValid)
      entry = 0;
    else
    {
      mSelStartIndex = mSelEndIndex = i;
      mSelStartOffset = mSelEndOffset = entry->mStrOffset;
    }
  }

  if (entry)
    result = SetSelection(mSelStartOffset, 0);
  else
  {
    
    

    mSelStartIndex  = mSelEndIndex  = -1;
    mSelStartOffset = mSelEndOffset = -1;
  }

  

  result = RemoveInvalidOffsetEntries();

  
  
  
  
  

  UNLOCK_DOC(this);

  return result;
}

NS_IMETHODIMP
nsTextServicesDocument::InsertText(const nsString *aText)
{
  nsresult result = NS_OK;

  nsCOMPtr<nsIEditor> editor (do_QueryReferent(mEditor));
  NS_ASSERTION(editor, "InsertText called without an editor present!"); 

  if (!editor || !SelectionIsValid())
    return NS_ERROR_FAILURE;

  if (!aText)
    return NS_ERROR_NULL_POINTER;

  
  
  
  
  
  

  PRBool collapsedSelection = SelectionIsCollapsed();
  PRInt32 savedSelOffset = mSelStartOffset;
  PRInt32 savedSelLength = mSelEndOffset - mSelStartOffset;

  if (!collapsedSelection)
  {
    
    

    result = SetSelection(mSelStartOffset, 0);

    if (NS_FAILED(result))
      return result;
  }


  LOCK_DOC(this);

  result = editor->BeginTransaction();

  if (NS_FAILED(result))
  {
    UNLOCK_DOC(this);
    return result;
  }

  nsCOMPtr<nsIPlaintextEditor> textEditor (do_QueryInterface(editor, &result));
  if (textEditor)
    result = textEditor->InsertText(*aText);

  if (NS_FAILED(result))
  {
    editor->EndTransaction();
    UNLOCK_DOC(this);
    return result;
  }

  
  
  
  
  

  PRInt32 strLength = aText->Length();
  PRUint32 i;

  nsCOMPtr<nsISelection> selection;
  OffsetEntry *itEntry;
  OffsetEntry *entry = mOffsetTable[mSelStartIndex];
  void *node         = entry->mNode;

  NS_ASSERTION((entry->mIsValid), "Invalid insertion point!");

  if (entry->mStrOffset == mSelStartOffset)
  {
    if (entry->mIsInsertedText)
    {
      
      

      entry->mLength += strLength;
    }
    else
    {
      
      

      itEntry = new OffsetEntry(entry->mNode, entry->mStrOffset, strLength);

      if (!itEntry)
      {
        editor->EndTransaction();
        UNLOCK_DOC(this);
        return NS_ERROR_OUT_OF_MEMORY;
      }

      itEntry->mIsInsertedText = PR_TRUE;
      itEntry->mNodeOffset = entry->mNodeOffset;

      if (!mOffsetTable.InsertElementAt(mSelStartIndex, itEntry))
      {
        editor->EndTransaction();
        UNLOCK_DOC(this);
        return NS_ERROR_FAILURE;
      }
    }
  }
  else if ((entry->mStrOffset + entry->mLength) == mSelStartOffset)
  {
    
    
    
    

    i       = mSelStartIndex + 1;
    itEntry = 0;

    if (mOffsetTable.Length() > i)
    {
      itEntry = mOffsetTable[i];

      if (!itEntry)
      {
        editor->EndTransaction();
        UNLOCK_DOC(this);
        return NS_ERROR_FAILURE;
      }

      
      

      if (!itEntry->mIsInsertedText || itEntry->mStrOffset != mSelStartOffset)
        itEntry = 0;
    }

    if (!itEntry)
    {
      
      

      itEntry = new OffsetEntry(entry->mNode, mSelStartOffset, 0);

      if (!itEntry)
      {
        editor->EndTransaction();
        UNLOCK_DOC(this);
        return NS_ERROR_OUT_OF_MEMORY;
      }

      itEntry->mNodeOffset = entry->mNodeOffset + entry->mLength;
      itEntry->mIsInsertedText = PR_TRUE;

      if (!mOffsetTable.InsertElementAt(i, itEntry))
      {
        delete itEntry;
        return NS_ERROR_FAILURE;
      }
    }

    
    
    

    itEntry->mLength += strLength;

    mSelStartIndex = mSelEndIndex = i;
          
    result = mSelCon->GetSelection(nsISelectionController::SELECTION_NORMAL, getter_AddRefs(selection));

    if (NS_FAILED(result))
    {
      editor->EndTransaction();
      UNLOCK_DOC(this);
      return result;
    }

    result = selection->Collapse(itEntry->mNode, itEntry->mNodeOffset + itEntry->mLength);
        
    if (NS_FAILED(result))
    {
      editor->EndTransaction();
      UNLOCK_DOC(this);
      return result;
    }
  }
  else if ((entry->mStrOffset + entry->mLength) > mSelStartOffset)
  {
    
    
    

    i = entry->mLength - (mSelStartOffset - entry->mStrOffset);

    result = SplitOffsetEntry(mSelStartIndex, i);

    if (NS_FAILED(result))
    {
      editor->EndTransaction();
      UNLOCK_DOC(this);
      return result;
    }

    itEntry = new OffsetEntry(entry->mNode, mSelStartOffset, strLength);

    if (!itEntry)
    {
      editor->EndTransaction();
      UNLOCK_DOC(this);
      return NS_ERROR_OUT_OF_MEMORY;
    }

    itEntry->mIsInsertedText = PR_TRUE;
    itEntry->mNodeOffset     = entry->mNodeOffset + entry->mLength;

    if (!mOffsetTable.InsertElementAt(mSelStartIndex + 1, itEntry))
    {
      editor->EndTransaction();
      UNLOCK_DOC(this);
      return NS_ERROR_FAILURE;
    }

    mSelEndIndex = ++mSelStartIndex;
  }

  
  
  

  for (i = mSelStartIndex + 1; i < mOffsetTable.Length(); i++)
  {
    entry = mOffsetTable[i];

    if (entry->mNode == node)
    {
      if (entry->mIsValid)
        entry->mNodeOffset += strLength;
    }
    else
      break;
  }

  
  
  
  
  

  if (!collapsedSelection)
  {
    result = SetSelection(savedSelOffset, savedSelLength);

    if (NS_FAILED(result))
    {
      editor->EndTransaction();
      UNLOCK_DOC(this);
      return result;
    }

    result = DeleteSelection();
  
    if (NS_FAILED(result))
    {
      editor->EndTransaction();
      UNLOCK_DOC(this);
      return result;
    }
  }

  result = editor->EndTransaction();

  UNLOCK_DOC(this);

  return result;
}

nsresult
nsTextServicesDocument::InsertNode(nsIDOMNode *aNode,
                                   nsIDOMNode *aParent,
                                   PRInt32 aPosition)
{
  return NS_OK;
}

nsresult
nsTextServicesDocument::DeleteNode(nsIDOMNode *aChild)
{
  NS_ENSURE_TRUE(mIterator, NS_ERROR_FAILURE);

  
  
  
  

  LOCK_DOC(this);

  PRInt32 nodeIndex, tcount;
  PRBool hasEntry;
  OffsetEntry *entry;

  nsresult result = NodeHasOffsetEntry(&mOffsetTable, aChild, &hasEntry, &nodeIndex);

  if (NS_FAILED(result))
  {
    UNLOCK_DOC(this);
    return result;
  }

  if (!hasEntry)
  {
    
    

    UNLOCK_DOC(this);
    return NS_OK;
  }

  nsCOMPtr<nsIDOMNode> node = do_QueryInterface(mIterator->GetCurrentNode());

  if (node && node == aChild &&
      mIteratorStatus != nsTextServicesDocument::eIsDone)
  {
    
    
    
    
    

    NS_ERROR("DeleteNode called for current iterator node."); 
  }

  tcount = mOffsetTable.Length();

  while (nodeIndex < tcount)
  {
    entry = mOffsetTable[nodeIndex];

    if (!entry)
    {
      UNLOCK_DOC(this);
      return NS_ERROR_FAILURE;
    }

    if (entry->mNode == aChild)
    {
      entry->mIsValid = PR_FALSE;
    }

    nodeIndex++;
  }

  UNLOCK_DOC(this);

  return NS_OK;
}

nsresult
nsTextServicesDocument::SplitNode(nsIDOMNode *aExistingRightNode,
                                  PRInt32 aOffset,
                                  nsIDOMNode *aNewLeftNode)
{
  
  
  
  
  return NS_OK;
}

nsresult
nsTextServicesDocument::JoinNodes(nsIDOMNode  *aLeftNode,
                                  nsIDOMNode  *aRightNode,
                                  nsIDOMNode  *aParent)
{
  PRInt32 i;
  PRUint16 type;
  nsresult result;

  
  
  
  

  

  result = aLeftNode->GetNodeType(&type);

  if (NS_FAILED(result))
    return PR_FALSE;

  if (nsIDOMNode::TEXT_NODE != type)
  {
    NS_ERROR("JoinNode called with a non-text left node!");
    return NS_ERROR_FAILURE;
  }

  result = aRightNode->GetNodeType(&type);

  if (NS_FAILED(result))
    return PR_FALSE;

  if (nsIDOMNode::TEXT_NODE != type)
  {
    NS_ERROR("JoinNode called with a non-text right node!");
    return NS_ERROR_FAILURE;
  }

  
  

  PRInt32 leftIndex, rightIndex;
  PRBool leftHasEntry, rightHasEntry;

  result = NodeHasOffsetEntry(&mOffsetTable, aLeftNode, &leftHasEntry, &leftIndex);

  if (NS_FAILED(result))
    return result;

  if (!leftHasEntry)
  {
    
    
    return NS_OK;
  }

  result = NodeHasOffsetEntry(&mOffsetTable, aRightNode, &rightHasEntry, &rightIndex);

  if (NS_FAILED(result))
    return result;

  if (!rightHasEntry)
  {
    
    
    return NS_OK;
  }

  NS_ASSERTION(leftIndex < rightIndex, "Indexes out of order.");

  if (leftIndex > rightIndex)
  {
    
    return NS_ERROR_FAILURE;
  }

  LOCK_DOC(this);

  OffsetEntry *entry = mOffsetTable[rightIndex];
  NS_ASSERTION(entry->mNodeOffset == 0, "Unexpected offset value for rightIndex.");

  
  

  nsAutoString str;
  result = aLeftNode->GetNodeValue(str);
  PRInt32 nodeLength = str.Length();

  for (i = leftIndex; i < rightIndex; i++)
  {
    entry = mOffsetTable[i];

    if (entry->mNode == aLeftNode)
    {
      if (entry->mIsValid)
        entry->mNode = aRightNode;
    }
    else
      break;
  }

  
  

  for (i = rightIndex; i < PRInt32(mOffsetTable.Length()); i++)
  {
    entry = mOffsetTable[i];

    if (entry->mNode == aRightNode)
    {
      if (entry->mIsValid)
        entry->mNodeOffset += nodeLength;
    }
    else
      break;
  }

  
  

  nsCOMPtr<nsIContent> leftContent = do_QueryInterface(aLeftNode);
  nsCOMPtr<nsIContent> rightContent = do_QueryInterface(aRightNode);

  if (!leftContent || !rightContent)
  {
    UNLOCK_DOC(this);
    return NS_ERROR_FAILURE;
  }

  if (mIterator->GetCurrentNode() == leftContent)
    result = mIterator->PositionAt(rightContent);

  UNLOCK_DOC(this);

  return NS_OK;
}

nsresult
nsTextServicesDocument::CreateContentIterator(nsIDOMRange *aRange, nsIContentIterator **aIterator)
{
  nsresult result;

  if (!aRange || !aIterator)
    return NS_ERROR_NULL_POINTER;

  *aIterator = 0;

  
  
  
  nsFilteredContentIterator* filter = new nsFilteredContentIterator(mTxtSvcFilter);
  *aIterator = static_cast<nsIContentIterator *>(filter);
  if (*aIterator) {
    NS_IF_ADDREF(*aIterator);
    result = filter ? NS_OK : NS_ERROR_FAILURE;
  } else {
    delete filter;
    result = NS_ERROR_FAILURE;
  }
  NS_ENSURE_SUCCESS(result, result);

  if (!*aIterator)
    return NS_ERROR_NULL_POINTER;

  result = (*aIterator)->Init(aRange);

  if (NS_FAILED(result))
  {
    NS_RELEASE((*aIterator));
    *aIterator = 0;
    return result;
  }

  return NS_OK;
}

nsresult
nsTextServicesDocument::GetDocumentContentRootNode(nsIDOMNode **aNode)
{
  nsresult result;

  if (!aNode)
    return NS_ERROR_NULL_POINTER;

  *aNode = 0;

  if (!mDOMDocument)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMHTMLDocument> htmlDoc = do_QueryInterface(mDOMDocument);

  if (htmlDoc)
  {
    

    nsCOMPtr<nsIDOMHTMLElement> bodyElement;

    result = htmlDoc->GetBody(getter_AddRefs(bodyElement));

    if (NS_FAILED(result))
      return result;

    if (!bodyElement)
      return NS_ERROR_FAILURE;

    result = bodyElement->QueryInterface(NS_GET_IID(nsIDOMNode), (void **)aNode);
  }
  else
  {
    

    nsCOMPtr<nsIDOMElement> docElement;

    result = mDOMDocument->GetDocumentElement(getter_AddRefs(docElement));

    if (NS_FAILED(result))
      return result;

    if (!docElement)
      return NS_ERROR_FAILURE;

    result = docElement->QueryInterface(NS_GET_IID(nsIDOMNode), (void **)aNode);
  }

  return result;
}

nsresult
nsTextServicesDocument::CreateDocumentContentRange(nsIDOMRange **aRange)
{
  nsresult result;

  if (!aRange)
    return NS_ERROR_NULL_POINTER;

  *aRange = 0;

  nsCOMPtr<nsIDOMNode>node;

  result = GetDocumentContentRootNode(getter_AddRefs(node));

  if (NS_FAILED(result))
    return result;

  if (!node)
    return NS_ERROR_NULL_POINTER;

  result = CallCreateInstance("@mozilla.org/content/range;1", aRange);
  if (NS_FAILED(result))
    return result;

  if (!*aRange)
    return NS_ERROR_NULL_POINTER;

  result = (*aRange)->SelectNodeContents(node);

  if (NS_FAILED(result))
  {
    NS_RELEASE((*aRange));
    *aRange = 0;
    return result;
  }

  return NS_OK;
}

nsresult
nsTextServicesDocument::CreateDocumentContentRootToNodeOffsetRange(nsIDOMNode *aParent, PRInt32 aOffset, PRBool aToStart, nsIDOMRange **aRange)
{
  nsresult result;

  if (!aParent || !aRange)
    return NS_ERROR_NULL_POINTER;

  *aRange = 0;

  NS_ASSERTION(aOffset >= 0, "Invalid offset!");

  if (aOffset < 0)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMNode> bodyNode; 

  result = GetDocumentContentRootNode(getter_AddRefs(bodyNode));

  if (NS_FAILED(result))
    return result;

  if (!bodyNode)
    return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIDOMNode> startNode;
  nsCOMPtr<nsIDOMNode> endNode;
  PRInt32 startOffset, endOffset;

  if (aToStart)
  {
    
    

    startNode   = bodyNode;
    startOffset = 0;
    endNode     = do_QueryInterface(aParent);
    endOffset   = aOffset;
  }
  else
  {
    
    

    nsCOMPtr<nsIDOMNodeList> nodeList;
    PRUint32 nodeListLength;

    startNode   = do_QueryInterface(aParent);
    startOffset = aOffset;
    endNode     = bodyNode;
    endOffset   = 0;

    result = bodyNode->GetChildNodes(getter_AddRefs(nodeList));

    if (NS_FAILED(result))
      return NS_ERROR_FAILURE;

    if (nodeList)
    {
      result = nodeList->GetLength(&nodeListLength);

      if (NS_FAILED(result))
        return NS_ERROR_FAILURE;

      endOffset = (PRInt32)nodeListLength;
    }
  }

  result = CallCreateInstance("@mozilla.org/content/range;1", aRange);
  if (NS_FAILED(result))
    return result;

  if (!*aRange)
    return NS_ERROR_NULL_POINTER;

  result = (*aRange)->SetStart(startNode, startOffset);

  if (NS_SUCCEEDED(result))
    result = (*aRange)->SetEnd(endNode, endOffset);

  if (NS_FAILED(result))
  {
    NS_RELEASE((*aRange));
    *aRange = 0;
  }

  return result;
}

nsresult
nsTextServicesDocument::CreateDocumentContentIterator(nsIContentIterator **aIterator)
{
  nsresult result;

  if (!aIterator)
    return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIDOMRange> range;

  result = CreateDocumentContentRange(getter_AddRefs(range));

  if (NS_FAILED(result))
    return result;

  result = CreateContentIterator(range, aIterator);

  return result;
}

nsresult
nsTextServicesDocument::AdjustContentIterator()
{
  nsresult result = NS_OK;

  if (!mIterator)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMNode> node(do_QueryInterface(mIterator->GetCurrentNode()));

  if (!node)
    return NS_ERROR_FAILURE;

  nsIDOMNode *nodePtr = node.get();
  PRInt32 tcount      = mOffsetTable.Length();

  nsIDOMNode *prevValidNode = 0;
  nsIDOMNode *nextValidNode = 0;
  PRBool foundEntry = PR_FALSE;
  OffsetEntry *entry;

  for (PRInt32 i = 0; i < tcount && !nextValidNode; i++)
  {
    entry = mOffsetTable[i];

    if (!entry)
      return NS_ERROR_FAILURE;

    if (entry->mNode == nodePtr)
    {
      if (entry->mIsValid)
      {
        
        

        return NS_OK;
      }
      else
      {
        
        
        

        foundEntry = PR_TRUE;
      }
    }

    if (entry->mIsValid)
    {
      if (!foundEntry)
        prevValidNode = entry->mNode;
      else
        nextValidNode = entry->mNode;
    }
  }

  nsCOMPtr<nsIContent> content;

  if (prevValidNode)
    content = do_QueryInterface(prevValidNode);
  else if (nextValidNode)
    content = do_QueryInterface(nextValidNode);

  if (content)
  {
    result = mIterator->PositionAt(content);

    if (NS_FAILED(result))
      mIteratorStatus = eIsDone;
    else
      mIteratorStatus = eValid;

    return result;
  }

  
  
  
  

  if (mNextTextBlock)
  {
    result = mIterator->PositionAt(mNextTextBlock);

    if (NS_FAILED(result))
    {
      mIteratorStatus = eIsDone;
      return result;
    }

    mIteratorStatus = eNext;
  }
  else if (mPrevTextBlock)
  {
    result = mIterator->PositionAt(mPrevTextBlock);

    if (NS_FAILED(result))
    {
      mIteratorStatus = eIsDone;
      return result;
    }

    mIteratorStatus = ePrev;
  }
  else
    mIteratorStatus = eIsDone;

  return NS_OK;
}

PRBool
nsTextServicesDocument::DidSkip(nsIContentIterator* aFilteredIter)
{
  
  
  
  
  
  if (aFilteredIter) {
    nsFilteredContentIterator* filter = static_cast<nsFilteredContentIterator *>(aFilteredIter);
    if (filter && filter->DidSkip()) {
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}

void
nsTextServicesDocument::ClearDidSkip(nsIContentIterator* aFilteredIter)
{
  
  if (aFilteredIter) {
    nsFilteredContentIterator* filter = static_cast<nsFilteredContentIterator *>(aFilteredIter);
    filter->ClearDidSkip();
  }
}

PRBool
nsTextServicesDocument::IsBlockNode(nsIContent *aContent)
{
  nsIAtom *atom = aContent->Tag();

  return (sAAtom       != atom &&
          sAddressAtom != atom &&
          sBigAtom     != atom &&
          sBlinkAtom   != atom &&
          sBAtom       != atom &&
          sCiteAtom    != atom &&
          sCodeAtom    != atom &&
          sDfnAtom     != atom &&
          sEmAtom      != atom &&
          sFontAtom    != atom &&
          sIAtom       != atom &&
          sKbdAtom     != atom &&
          sKeygenAtom  != atom &&
          sNobrAtom    != atom &&
          sSAtom       != atom &&
          sSampAtom    != atom &&
          sSmallAtom   != atom &&
          sSpacerAtom  != atom &&
          sSpanAtom    != atom &&
          sStrikeAtom  != atom &&
          sStrongAtom  != atom &&
          sSubAtom     != atom &&
          sSupAtom     != atom &&
          sTtAtom      != atom &&
          sUAtom       != atom &&
          sVarAtom     != atom &&
          sWbrAtom     != atom);
}

PRBool
nsTextServicesDocument::HasSameBlockNodeParent(nsIContent *aContent1, nsIContent *aContent2)
{
  nsIContent* p1 = aContent1->GetParent();
  nsIContent* p2 = aContent2->GetParent();

  

  if (p1 == p2)
    return PR_TRUE;

  

  while (p1 && !IsBlockNode(p1))
  {
    p1 = p1->GetParent();
  }

  while (p2 && !IsBlockNode(p2))
  {
    p2 = p2->GetParent();
  }

  return p1 == p2;
}

PRBool
nsTextServicesDocument::IsTextNode(nsIContent *aContent)
{
  if (!aContent)
    return PR_FALSE;

  nsCOMPtr<nsIDOMNode> node = do_QueryInterface(aContent);

  return IsTextNode(node);
}

PRBool
nsTextServicesDocument::IsTextNode(nsIDOMNode *aNode)
{
  if (!aNode)
    return PR_FALSE;

  PRUint16 type;

  nsresult result = aNode->GetNodeType(&type);

  if (NS_FAILED(result))
    return PR_FALSE;

  return nsIDOMNode::TEXT_NODE == type;
}

nsresult
nsTextServicesDocument::SetSelectionInternal(PRInt32 aOffset, PRInt32 aLength, PRBool aDoUpdate)
{
  nsresult result = NS_OK;

  if (!mSelCon || aOffset < 0 || aLength < 0)
    return NS_ERROR_FAILURE;

  nsIDOMNode *sNode = 0, *eNode = 0;
  PRInt32 i, sOffset = 0, eOffset = 0;
  OffsetEntry *entry;

  

  for (i = 0; !sNode && i < PRInt32(mOffsetTable.Length()); i++)
  {
    entry = mOffsetTable[i];
    if (entry->mIsValid)
    {
      if (entry->mIsInsertedText)
      {
        
        
        

        if (entry->mStrOffset == aOffset)
        {
          sNode   = entry->mNode;
          sOffset = entry->mNodeOffset + entry->mLength;
        }
      }
      else if (aOffset >= entry->mStrOffset)
      {
        PRBool foundEntry = PR_FALSE;
        PRInt32 strEndOffset = entry->mStrOffset + entry->mLength;

        if (aOffset < strEndOffset)
          foundEntry = PR_TRUE;
        else if (aOffset == strEndOffset)
        {
          
          
          
          

          if ((i+1) < PRInt32(mOffsetTable.Length()))
          {
            OffsetEntry *nextEntry = mOffsetTable[i+1];

            if (!nextEntry->mIsValid || nextEntry->mStrOffset != aOffset)
            {
              
              
              foundEntry = PR_TRUE;
            }
          }
        }

        if (foundEntry)
        {
          sNode   = entry->mNode;
          sOffset = entry->mNodeOffset + aOffset - entry->mStrOffset;
        }
      }

      if (sNode)
      {
        mSelStartIndex  = i;
        mSelStartOffset = aOffset;
      }
    }
  }

  if (!sNode)
    return NS_ERROR_FAILURE;

  
  

  nsCOMPtr<nsISelection> selection;

  if (aDoUpdate)
  {
    result = mSelCon->GetSelection(nsISelectionController::SELECTION_NORMAL, getter_AddRefs(selection));

    if (NS_FAILED(result))
      return result;

    result = selection->Collapse(sNode, sOffset);

    if (NS_FAILED(result))
      return result;
   }

  if (aLength <= 0)
  {
    

    mSelEndIndex  = mSelStartIndex;
    mSelEndOffset = mSelStartOffset;

   
   
   

    return NS_OK;
  }

  

  PRInt32 endOffset = aOffset + aLength;

  for (i = mOffsetTable.Length() - 1; !eNode && i >= 0; i--)
  {
    entry = mOffsetTable[i];
    
    if (entry->mIsValid)
    {
      if (entry->mIsInsertedText)
      {
        if (entry->mStrOffset == eOffset)
        {
          
          

          eNode   = entry->mNode;
          eOffset = entry->mNodeOffset + entry->mLength;
        }
      }
      else if (endOffset >= entry->mStrOffset && endOffset <= entry->mStrOffset + entry->mLength)
      {
        eNode   = entry->mNode;
        eOffset = entry->mNodeOffset + endOffset - entry->mStrOffset;
      }

      if (eNode)
      {
        mSelEndIndex  = i;
        mSelEndOffset = endOffset;
      }
    }
  }

  if (aDoUpdate && eNode)
  {
    result = selection->Extend(eNode, eOffset);

    if (NS_FAILED(result))
      return result;
  }

  
  
  

  return result;
}

nsresult
nsTextServicesDocument::GetSelection(nsITextServicesDocument::TSDBlockSelectionStatus *aSelStatus, PRInt32 *aSelOffset, PRInt32 *aSelLength)
{
  nsresult result;

  if (!aSelStatus || !aSelOffset || !aSelLength)
    return NS_ERROR_NULL_POINTER;

  *aSelStatus = nsITextServicesDocument::eBlockNotFound;
  *aSelOffset = -1;
  *aSelLength = -1;

  if (!mDOMDocument || !mSelCon)
    return NS_ERROR_FAILURE;

  if (mIteratorStatus == nsTextServicesDocument::eIsDone)
    return NS_OK;

  nsCOMPtr<nsISelection> selection;
  PRBool isCollapsed;

  result = mSelCon->GetSelection(nsISelectionController::SELECTION_NORMAL, getter_AddRefs(selection));

  if (NS_FAILED(result))
    return result;

  if (!selection)
    return NS_ERROR_FAILURE;

  result = selection->GetIsCollapsed(&isCollapsed);

  if (NS_FAILED(result))
    return result;

  
  

  

  if (isCollapsed)
    result = GetCollapsedSelection(aSelStatus, aSelOffset, aSelLength);
  else
    result = GetUncollapsedSelection(aSelStatus, aSelOffset, aSelLength);

  

  return result;
}

nsresult
nsTextServicesDocument::GetCollapsedSelection(nsITextServicesDocument::TSDBlockSelectionStatus *aSelStatus, PRInt32 *aSelOffset, PRInt32 *aSelLength)
{
  nsresult result;
  nsCOMPtr<nsISelection> selection;

  result = mSelCon->GetSelection(nsISelectionController::SELECTION_NORMAL, getter_AddRefs(selection));

  if (NS_FAILED(result))
    return result;

  if (!selection)
    return NS_ERROR_FAILURE;

  
  

  nsCOMPtr<nsIDOMRange> range;
  OffsetEntry *entry;
  nsCOMPtr<nsIDOMNode> parent;
  PRInt32 offset, tableCount, i;
  PRInt32 e1s1, e2s1;

  OffsetEntry *eStart, *eEnd;
  PRInt32 eStartOffset, eEndOffset;


  *aSelStatus = nsITextServicesDocument::eBlockOutside;
  *aSelOffset = *aSelLength = -1;

  tableCount = mOffsetTable.Length();

  if (tableCount == 0)
    return NS_OK;

  
  

  eStart = mOffsetTable[0];

  if (tableCount > 1)
    eEnd = mOffsetTable[tableCount - 1];
  else
    eEnd = eStart;

  eStartOffset = eStart->mNodeOffset;
  eEndOffset   = eEnd->mNodeOffset + eEnd->mLength;

  result = selection->GetRangeAt(0, getter_AddRefs(range));

  if (NS_FAILED(result))
    return result;

  result = range->GetStartContainer(getter_AddRefs(parent));

  if (NS_FAILED(result))
    return result;

  result = range->GetStartOffset(&offset);

  if (NS_FAILED(result))
    return result;

  result = ComparePoints(eStart->mNode, eStartOffset, parent, offset, &e1s1);

  if (NS_FAILED(result))
    return result;

  result = ComparePoints(eEnd->mNode, eEndOffset, parent, offset, &e2s1);

  if (NS_FAILED(result))
    return result;

  if (e1s1 > 0 || e2s1 < 0)
  {
    
    

    return NS_OK;
  }

  if (IsTextNode(parent))
  {
    
    
    

    for (i = 0; i < tableCount; i++)
    {
      entry = mOffsetTable[i];

      if (!entry)
        return NS_ERROR_FAILURE;

      if (entry->mNode == parent.get() &&
          entry->mNodeOffset <= offset && offset <= (entry->mNodeOffset + entry->mLength))
      {
        *aSelStatus = nsITextServicesDocument::eBlockContains;
        *aSelOffset = entry->mStrOffset + (offset - entry->mNodeOffset);
        *aSelLength = 0;

        return NS_OK;
      }
    }

    
    

    return NS_ERROR_FAILURE;
  }

  
  
  
  
  
  

  nsCOMPtr<nsIDOMNode> node, saveNode;
  nsCOMPtr<nsIDOMNodeList> children;
  nsCOMPtr<nsIContentIterator> iter;
  PRBool hasChildren;

  result = CreateRange(eStart->mNode, eStartOffset, eEnd->mNode, eEndOffset, getter_AddRefs(range));

  if (NS_FAILED(result))
    return result;

  result = CreateContentIterator(range, getter_AddRefs(iter));

  if (NS_FAILED(result))
    return result;

  result = parent->HasChildNodes(&hasChildren);

  if (NS_FAILED(result))
    return result;

  if (hasChildren)
  {
    
    

    
    

    PRUint32 childIndex = (PRUint32)offset;

    result = parent->GetChildNodes(getter_AddRefs(children));

    if (NS_FAILED(result))
      return result;

    if (!children)
      return NS_ERROR_FAILURE;

    if (childIndex > 0)
    {
      PRUint32 numChildren;

      result = children->GetLength(&numChildren);

      if (NS_FAILED(result))
        return result;

      NS_ASSERTION(childIndex <= numChildren, "Invalid selection offset!");

      if (childIndex > numChildren)
        childIndex = numChildren;

      childIndex -= 1;
    }

    result = children->Item(childIndex, getter_AddRefs(saveNode));

    if (NS_FAILED(result))
      return result;

    nsCOMPtr<nsIContent> content(do_QueryInterface(saveNode));

    if (!content)
      return NS_ERROR_FAILURE;

    result = iter->PositionAt(content);

    if (NS_FAILED(result))
      return result;
  }
  else
  {
    
    

    nsCOMPtr<nsIContent> content(do_QueryInterface(parent));

    if (!content)
      return NS_ERROR_FAILURE;

    result = iter->PositionAt(content);

    if (NS_FAILED(result))
      return result;

    saveNode = parent;
  }

  
  
  

  while (!iter->IsDone())
  {
    nsCOMPtr<nsIContent> content = do_QueryInterface(iter->GetCurrentNode());

    if (IsTextNode(content))
    {
      node = do_QueryInterface(content);

      if (!node)
        return NS_ERROR_FAILURE;

      break;
    }

    node = nsnull;

    iter->Prev();
  }

  if (node)
  {
    
    

    nsAutoString str;
    result = node->GetNodeValue(str);

    if (NS_FAILED(result))
      return result;

    offset = str.Length();
  }
  else
  {
    

    
    
    

    {
      nsCOMPtr<nsIContent> content(do_QueryInterface(saveNode));

      result = iter->PositionAt(content);

      if (NS_FAILED(result))
        return result;
    }

    while (!iter->IsDone())
    {
      nsCOMPtr<nsIContent> content = do_QueryInterface(iter->GetCurrentNode());

      if (IsTextNode(content))
      {
        node = do_QueryInterface(content);

        if (!node)
          return NS_ERROR_FAILURE;

        break;
      }

      node = nsnull;

      iter->Next();
    }

    if (!node)
      return NS_ERROR_FAILURE;

    
    

    offset = 0;
  }

  for (i = 0; i < tableCount; i++)
  {
    entry = mOffsetTable[i];

    if (!entry)
      return NS_ERROR_FAILURE;

    if (entry->mNode == node.get() &&
        entry->mNodeOffset <= offset && offset <= (entry->mNodeOffset + entry->mLength))
    {
      *aSelStatus = nsITextServicesDocument::eBlockContains;
      *aSelOffset = entry->mStrOffset + (offset - entry->mNodeOffset);
      *aSelLength = 0;

      
      
      
      
      

      result = SetSelectionInternal(*aSelOffset, *aSelLength, PR_TRUE);

      return result;
    }
  }

  return NS_ERROR_FAILURE;
}

nsresult
nsTextServicesDocument::GetUncollapsedSelection(nsITextServicesDocument::TSDBlockSelectionStatus *aSelStatus, PRInt32 *aSelOffset, PRInt32 *aSelLength)
{
  nsresult result;

  nsCOMPtr<nsISelection> selection;
  nsCOMPtr<nsIDOMRange> range;
  OffsetEntry *entry;

  result = mSelCon->GetSelection(nsISelectionController::SELECTION_NORMAL, getter_AddRefs(selection));

  if (NS_FAILED(result))
    return result;

  if (!selection)
    return NS_ERROR_FAILURE;

  
  
  

  nsCOMPtr<nsIDOMNode> startParent, endParent;
  PRInt32 startOffset, endOffset;
  PRInt32 rangeCount, tableCount, i;
  PRInt32 e1s1, e1s2, e2s1, e2s2;

  OffsetEntry *eStart, *eEnd;
  PRInt32 eStartOffset, eEndOffset;

  tableCount = mOffsetTable.Length();

  
  

  eStart = mOffsetTable[0];

  if (tableCount > 1)
    eEnd = mOffsetTable[tableCount - 1];
  else
    eEnd = eStart;

  eStartOffset = eStart->mNodeOffset;
  eEndOffset   = eEnd->mNodeOffset + eEnd->mLength;

  result = selection->GetRangeCount(&rangeCount);

  if (NS_FAILED(result))
    return result;

  
  

  for (i = 0; i < rangeCount; i++)
  {
    result = selection->GetRangeAt(i, getter_AddRefs(range));

    if (NS_FAILED(result))
      return result;

    result = GetRangeEndPoints(range,
                               getter_AddRefs(startParent), &startOffset,
                               getter_AddRefs(endParent), &endOffset);

    if (NS_FAILED(result))
      return result;

    result = ComparePoints(eStart->mNode, eStartOffset, endParent, endOffset, &e1s2);

    if (NS_FAILED(result))
      return result;

    result = ComparePoints(eEnd->mNode, eEndOffset, startParent, startOffset, &e2s1);

    if (NS_FAILED(result))
      return result;

    

    if (e1s2 <= 0 && e2s1 >= 0)
      break;
  }

  

  if (rangeCount < 1 || e1s2 > 0 || e2s1 < 0)
  {
    *aSelStatus = nsITextServicesDocument::eBlockOutside;
    *aSelOffset = *aSelLength = -1;
    return NS_OK;
  }

  

  result = ComparePoints(eStart->mNode, eStartOffset, startParent, startOffset, &e1s1);

  if (NS_FAILED(result))
    return result;

  result = ComparePoints(eEnd->mNode, eEndOffset, endParent, endOffset, &e2s2);

  if (NS_FAILED(result))
    return result;

  if (rangeCount > 1)
  {
    
    
    

    *aSelStatus = nsITextServicesDocument::eBlockPartial;
  }
  else if (e1s1 > 0 && e2s2 < 0)
  {
    
    

    *aSelStatus = nsITextServicesDocument::eBlockInside;
  }
  else if (e1s1 <= 0 && e2s2 >= 0)
  {
    
    

    *aSelStatus = nsITextServicesDocument::eBlockContains;
  }
  else
  {
    

    *aSelStatus = nsITextServicesDocument::eBlockPartial;
  }

  
  

  nsCOMPtr<nsIDOMNode> p1, p2;
  PRInt32     o1,  o2;

  
  

  if (e1s1 >= 0)
  {
    p1 = do_QueryInterface(eStart->mNode);
    o1 = eStartOffset;
  }
  else
  {
    p1 = startParent;
    o1 = startOffset;
  }

  
  

  if (e2s2 <= 0)
  {
    p2 = do_QueryInterface(eEnd->mNode);
    o2 = eEndOffset;
  }
  else
  {
    p2 = endParent;
    o2 = endOffset;
  }

  result = CreateRange(p1, o1, p2, o2, getter_AddRefs(range));

  if (NS_FAILED(result))
    return result;

  
  

  nsCOMPtr<nsIContentIterator> iter;

  result = CreateContentIterator(range, getter_AddRefs(iter));

  if (NS_FAILED(result))
    return result;

  
  
  PRBool found;
  nsCOMPtr<nsIContent> content;

  iter->First();

  if (!IsTextNode(p1))
  {
    found = PR_FALSE;

    while (!iter->IsDone())
    {
      content = do_QueryInterface(iter->GetCurrentNode());

      if (IsTextNode(content))
      {
        p1 = do_QueryInterface(content);

        if (!p1)
          return NS_ERROR_FAILURE;

        o1 = 0;
        found = PR_TRUE;

        break;
      }

      iter->Next();
    }

    if (!found)
      return NS_ERROR_FAILURE;
  }

  

  iter->Last();

  if (! IsTextNode(p2))
  {
    found = PR_FALSE;

    while (!iter->IsDone())
    {
      content = do_QueryInterface(iter->GetCurrentNode());

      if (IsTextNode(content))
      {
        p2 = do_QueryInterface(content);

        if (!p2)
          return NS_ERROR_FAILURE;

        nsString str;

        result = p2->GetNodeValue(str);

        if (NS_FAILED(result))
          return result;

        o2 = str.Length();
        found = PR_TRUE;

        break;
      }

      iter->Prev();
    }

    if (!found)
      return NS_ERROR_FAILURE;
  }

  found    = PR_FALSE;
  *aSelLength = 0;

  for (i = 0; i < tableCount; i++)
  {
    entry = mOffsetTable[i];

    if (!entry)
      return NS_ERROR_FAILURE;

    if (!found)
    {
      if (entry->mNode == p1.get() &&
          entry->mNodeOffset <= o1 && o1 <= (entry->mNodeOffset + entry->mLength))
      {
        *aSelOffset = entry->mStrOffset + (o1 - entry->mNodeOffset);

        if (p1 == p2 &&
            entry->mNodeOffset <= o2 && o2 <= (entry->mNodeOffset + entry->mLength))
        {
          
          

          *aSelLength = o2 - o1;
          break;
        }
        else
        {
          
          

          *aSelLength = entry->mLength - (o1 - entry->mNodeOffset);
        }

        found = PR_TRUE;
      }
    }
    else 
    {
      if (entry->mNode == p2.get() &&
          entry->mNodeOffset <= o2 && o2 <= (entry->mNodeOffset + entry->mLength))
      {
        
        

        *aSelLength += o2 - entry->mNodeOffset;
        break;
      }
      else
      {
        

        *aSelLength += entry->mLength;
      }
    }
  }

  return result;
}

PRBool
nsTextServicesDocument::SelectionIsCollapsed()
{
  return(mSelStartIndex == mSelEndIndex && mSelStartOffset == mSelEndOffset);
}

PRBool
nsTextServicesDocument::SelectionIsValid()
{
  return(mSelStartIndex >= 0);
}

nsresult
nsTextServicesDocument::ComparePoints(nsIDOMNode* aParent1, PRInt32 aOffset1,
                                      nsIDOMNode* aParent2, PRInt32 aOffset2,
                                      PRInt32 *aResult)
{
  nsresult result;
  
  if (!sRangeHelper) {
    result = CallGetService("@mozilla.org/content/range-utils;1",
                            &sRangeHelper);
    if (!sRangeHelper)
      return result;
  }

  *aResult = sRangeHelper->ComparePoints(aParent1, aOffset1,
                                         aParent2, aOffset2);
  return NS_OK;
}

nsresult
nsTextServicesDocument::GetRangeEndPoints(nsIDOMRange *aRange,
                                          nsIDOMNode **aStartParent, PRInt32 *aStartOffset,
                                          nsIDOMNode **aEndParent, PRInt32 *aEndOffset)
{
  nsresult result;

  if (!aRange || !aStartParent || !aStartOffset || !aEndParent || !aEndOffset)
    return NS_ERROR_NULL_POINTER;

  result = aRange->GetStartContainer(aStartParent);

  if (NS_FAILED(result))
    return result;

  if (!aStartParent)
    return NS_ERROR_FAILURE;

  result = aRange->GetStartOffset(aStartOffset);

  if (NS_FAILED(result))
    return result;

  result = aRange->GetEndContainer(aEndParent);

  if (NS_FAILED(result))
    return result;

  if (!aEndParent)
    return NS_ERROR_FAILURE;

  result = aRange->GetEndOffset(aEndOffset);

  return result;
}


nsresult
nsTextServicesDocument::CreateRange(nsIDOMNode *aStartParent, PRInt32 aStartOffset,
                                    nsIDOMNode *aEndParent, PRInt32 aEndOffset,
                                    nsIDOMRange **aRange)
{
  nsresult result;

  result = CallCreateInstance("@mozilla.org/content/range;1", aRange);
  if (NS_FAILED(result))
    return result;

  if (!*aRange)
    return NS_ERROR_NULL_POINTER;

  result = (*aRange)->SetStart(aStartParent, aStartOffset);

  if (NS_SUCCEEDED(result))
    result = (*aRange)->SetEnd(aEndParent, aEndOffset);

  if (NS_FAILED(result))
  {
    NS_RELEASE((*aRange));
    *aRange = 0;
  }

  return result;
}

nsresult
nsTextServicesDocument::FirstTextNode(nsIContentIterator *aIterator,
                                      TSDIteratorStatus *aIteratorStatus)
{
  if (aIteratorStatus)
    *aIteratorStatus = nsTextServicesDocument::eIsDone;

  aIterator->First();

  while (!aIterator->IsDone())
  {
    nsCOMPtr<nsIContent> content = do_QueryInterface(aIterator->GetCurrentNode());

    if (IsTextNode(content))
    {
      if (aIteratorStatus)
        *aIteratorStatus = nsTextServicesDocument::eValid;
      break;
    }

    aIterator->Next();
  }

  return NS_OK;
}

nsresult
nsTextServicesDocument::LastTextNode(nsIContentIterator *aIterator,
                                     TSDIteratorStatus *aIteratorStatus)
{
  if (aIteratorStatus)
    *aIteratorStatus = nsTextServicesDocument::eIsDone;

  aIterator->Last();

  while (!aIterator->IsDone())
  {
    nsCOMPtr<nsIContent> content = do_QueryInterface(aIterator->GetCurrentNode());

    if (IsTextNode(content))
    {
      if (aIteratorStatus)
        *aIteratorStatus = nsTextServicesDocument::eValid;
      break;
    }

    aIterator->Prev();
  }

  return NS_OK;
}

nsresult
nsTextServicesDocument::FirstTextNodeInCurrentBlock(nsIContentIterator *iter)
{
  if (!iter)
    return NS_ERROR_NULL_POINTER;

  ClearDidSkip(iter);

  nsCOMPtr<nsIContent> last;

  
  

  while (!iter->IsDone())
  {
    nsCOMPtr<nsIContent> content = do_QueryInterface(iter->GetCurrentNode());

    if (IsTextNode(content))
    {
      if (!last || HasSameBlockNodeParent(content, last))
        last = content;
      else
      {
        
        
        break;
      }
    }
    else if (last && IsBlockNode(content))
      break;

    iter->Prev();

    if (DidSkip(iter))
      break;
  }
  
  if (last)
    iter->PositionAt(last);

  

  return NS_OK;
}

nsresult
nsTextServicesDocument::FirstTextNodeInPrevBlock(nsIContentIterator *aIterator)
{
  nsCOMPtr<nsIContent> content;
  nsresult result;

  if (!aIterator)
    return NS_ERROR_NULL_POINTER;

  

  
  

  result = FirstTextNodeInCurrentBlock(aIterator);

  if (NS_FAILED(result))
    return NS_ERROR_FAILURE;

  

  aIterator->Prev();

  if (aIterator->IsDone())
    return NS_ERROR_FAILURE;

  

  return FirstTextNodeInCurrentBlock(aIterator);
}

nsresult
nsTextServicesDocument::FirstTextNodeInNextBlock(nsIContentIterator *aIterator)
{
  nsCOMPtr<nsIContent> prev;
  PRBool crossedBlockBoundary = PR_FALSE;

  if (!aIterator)
    return NS_ERROR_NULL_POINTER;

  ClearDidSkip(aIterator);

  while (!aIterator->IsDone())
  {
    nsCOMPtr<nsIContent> content = do_QueryInterface(aIterator->GetCurrentNode());

    if (IsTextNode(content))
    {
      if (!crossedBlockBoundary && (!prev || HasSameBlockNodeParent(prev, content)))
        prev = content;
      else
        break;
    }
    else if (!crossedBlockBoundary && IsBlockNode(content))
      crossedBlockBoundary = PR_TRUE;

    aIterator->Next();

    if (!crossedBlockBoundary && DidSkip(aIterator))
      crossedBlockBoundary = PR_TRUE;
  }

  return NS_OK;
}

nsresult
nsTextServicesDocument::GetFirstTextNodeInPrevBlock(nsIContent **aContent)
{
  nsresult result;

  if (!aContent)
    return NS_ERROR_NULL_POINTER;

  *aContent = 0;

  
  

  nsINode* node = mIterator->GetCurrentNode();

  result = FirstTextNodeInPrevBlock(mIterator);

  if (NS_FAILED(result))
  {
    
    mIterator->PositionAt(node);
    return result;
  }

  if (!mIterator->IsDone())
  {
    nsCOMPtr<nsIContent> current = do_QueryInterface(mIterator->GetCurrentNode());
    current.swap(*aContent);
  }

  

  return mIterator->PositionAt(node);
}

nsresult
nsTextServicesDocument::GetFirstTextNodeInNextBlock(nsIContent **aContent)
{
  nsresult result;

  if (!aContent)
    return NS_ERROR_NULL_POINTER;

  *aContent = 0;

  
  

  nsINode* node = mIterator->GetCurrentNode();

  result = FirstTextNodeInNextBlock(mIterator);

  if (NS_FAILED(result))
  {
    
    mIterator->PositionAt(node);
    return result;
  }

  if (!mIterator->IsDone())
  {
    nsCOMPtr<nsIContent> current = do_QueryInterface(mIterator->GetCurrentNode());
    current.swap(*aContent);
  }

  
  return mIterator->PositionAt(node);
}

nsresult
nsTextServicesDocument::CreateOffsetTable(nsTArray<OffsetEntry*> *aOffsetTable,
                                          nsIContentIterator *aIterator,
                                          TSDIteratorStatus *aIteratorStatus,
                                          nsIDOMRange *aIterRange,
                                          nsString *aStr)
{
  nsresult result = NS_OK;

  nsCOMPtr<nsIContent> first;
  nsCOMPtr<nsIContent> prev;

  if (!aIterator)
    return NS_ERROR_NULL_POINTER;

  ClearOffsetTable(aOffsetTable);

  if (aStr)
    aStr->Truncate();

  if (*aIteratorStatus == nsTextServicesDocument::eIsDone)
    return NS_OK;

  
  
  
  
  nsCOMPtr<nsIDOMNode> rngStartNode, rngEndNode;
  PRInt32 rngStartOffset = 0, rngEndOffset = 0;

  if (aIterRange)
  {
    result = GetRangeEndPoints(aIterRange,
                               getter_AddRefs(rngStartNode), &rngStartOffset,
                               getter_AddRefs(rngEndNode), &rngEndOffset);

    NS_ENSURE_SUCCESS(result, result);
  }

  
  
  

  result = FirstTextNodeInCurrentBlock(aIterator);

  if (NS_FAILED(result))
    return result;

  PRInt32 offset = 0;

  ClearDidSkip(aIterator);

  while (!aIterator->IsDone())
  {
    nsCOMPtr<nsIContent> content = do_QueryInterface(aIterator->GetCurrentNode());

    if (IsTextNode(content))
    {
      if (!prev || HasSameBlockNodeParent(prev, content))
      {
        nsCOMPtr<nsIDOMNode> node = do_QueryInterface(content);

        if (node)
        {
          nsString str;

          result = node->GetNodeValue(str);

          if (NS_FAILED(result))
            return result;

          

          OffsetEntry *entry = new OffsetEntry(node, offset, str.Length());

          if (!entry)
            return NS_ERROR_OUT_OF_MEMORY;

          aOffsetTable->AppendElement(entry);

          
          
          
          

          PRInt32 startOffset = 0;
          PRInt32 endOffset   = str.Length();
          PRBool adjustStr    = PR_FALSE;

          if (entry->mNode == rngStartNode)
          {
            entry->mNodeOffset = startOffset = rngStartOffset;
            adjustStr = PR_TRUE;
          }

          if (entry->mNode == rngEndNode)
          {
            endOffset = rngEndOffset;
            adjustStr = PR_TRUE;
          }

          if (adjustStr)
          {
            entry->mLength = endOffset - startOffset;
            str = Substring(str, startOffset, entry->mLength);
          }

          offset += str.Length();

          if (aStr)
          {
            

            if (!first)
              *aStr = str;
            else
              *aStr += str;
          }
        }

        prev = content;

        if (!first)
          first = content;
      }
      else
        break;

    }
    else if (IsBlockNode(content))
      break;

    aIterator->Next();

    if (DidSkip(aIterator))
      break;
  }

  if (first)
  {
    
    

    aIterator->PositionAt(first);
  }
  else
  {
    
    
    

    *aIteratorStatus = nsTextServicesDocument::eIsDone;
  }

  return result;
}

nsresult
nsTextServicesDocument::RemoveInvalidOffsetEntries()
{
  OffsetEntry *entry;
  PRInt32 i = 0;

  while (PRUint32(i) < mOffsetTable.Length())
  {
    entry = mOffsetTable[i];

    if (!entry->mIsValid)
    {
      mOffsetTable.RemoveElementAt(i);

      if (mSelStartIndex >= 0 && mSelStartIndex >= i)
      {
        
        
        

        NS_ASSERTION(i != mSelStartIndex, "Invalid selection index.");

        --mSelStartIndex;
        --mSelEndIndex;
      }
    }
    else
      i++;
  }

  return NS_OK;
}

nsresult
nsTextServicesDocument::ClearOffsetTable(nsTArray<OffsetEntry*> *aOffsetTable)
{
  PRUint32 i;

  for (i = 0; i < aOffsetTable->Length(); i++)
  {
    delete aOffsetTable->ElementAt(i);
  }

  aOffsetTable->Clear();

  return NS_OK;
}

nsresult
nsTextServicesDocument::SplitOffsetEntry(PRInt32 aTableIndex, PRInt32 aNewEntryLength)
{
  OffsetEntry *entry = mOffsetTable[aTableIndex];

  NS_ASSERTION((aNewEntryLength > 0), "aNewEntryLength <= 0");
  NS_ASSERTION((aNewEntryLength < entry->mLength), "aNewEntryLength >= mLength");

  if (aNewEntryLength < 1 || aNewEntryLength >= entry->mLength)
    return NS_ERROR_FAILURE;

  PRInt32 oldLength = entry->mLength - aNewEntryLength;

  OffsetEntry *newEntry = new OffsetEntry(entry->mNode,
                                          entry->mStrOffset + oldLength,
                                          aNewEntryLength);

  if (!newEntry)
    return NS_ERROR_OUT_OF_MEMORY;

  if (!mOffsetTable.InsertElementAt(aTableIndex + 1, newEntry))
  {
    delete newEntry;
    return NS_ERROR_FAILURE;
  }

   

   entry->mLength        = oldLength;
   newEntry->mNodeOffset = entry->mNodeOffset + oldLength;

  return NS_OK;
}

nsresult
nsTextServicesDocument::NodeHasOffsetEntry(nsTArray<OffsetEntry*> *aOffsetTable, nsIDOMNode *aNode, PRBool *aHasEntry, PRInt32 *aEntryIndex)
{
  OffsetEntry *entry;
  PRUint32 i;

  if (!aNode || !aHasEntry || !aEntryIndex)
    return NS_ERROR_NULL_POINTER;

  for (i = 0; i < aOffsetTable->Length(); i++)
  {
    entry = (*aOffsetTable)[i];

    if (!entry)
      return NS_ERROR_FAILURE;

    if (entry->mNode == aNode)
    {
      *aHasEntry   = PR_TRUE;
      *aEntryIndex = i;

      return NS_OK;
    }
  }

  *aHasEntry   = PR_FALSE;
  *aEntryIndex = -1;

  return NS_OK;
}


#ifdef XP_MAC
#define IS_NBSP_CHAR(c) (((unsigned char)0xca)==(c))
#else
#define IS_NBSP_CHAR(c) (((unsigned char)0xa0)==(c))
#endif

nsresult
nsTextServicesDocument::FindWordBounds(nsTArray<OffsetEntry*> *aOffsetTable,
                                       nsString *aBlockStr,
                                       nsIDOMNode *aNode,
                                       PRInt32 aNodeOffset,
                                       nsIDOMNode **aWordStartNode,
                                       PRInt32 *aWordStartOffset,
                                       nsIDOMNode **aWordEndNode,
                                       PRInt32 *aWordEndOffset)
{
  

  if (aWordStartNode)
    *aWordStartNode = nsnull;
  if (aWordStartOffset)
    *aWordStartOffset = 0;
  if (aWordEndNode)
    *aWordEndNode = nsnull;
  if (aWordEndOffset)
    *aWordEndOffset = 0;

  PRInt32 entryIndex;
  PRBool hasEntry;

  
  
  

  nsresult result = NodeHasOffsetEntry(aOffsetTable, aNode, &hasEntry, &entryIndex);
  NS_ENSURE_SUCCESS(result, result);
  NS_ENSURE_TRUE(hasEntry, NS_ERROR_FAILURE);

  

  OffsetEntry *entry = (*aOffsetTable)[entryIndex];
  PRUint32 strOffset = entry->mStrOffset + aNodeOffset - entry->mNodeOffset;

  
  

  const PRUnichar *str = aBlockStr->get();
  PRUint32 strLen = aBlockStr->Length();

  nsIWordBreaker *aWordBreaker;

  result = CallGetService(NS_WBRK_CONTRACTID, &aWordBreaker);
  NS_ENSURE_SUCCESS(result, result);

  nsWordRange res = aWordBreaker->FindWord(str, strLen, strOffset);
  NS_IF_RELEASE(aWordBreaker);
  if(res.mBegin > strLen)
  {
    if(!str)
      return NS_ERROR_NULL_POINTER;
    else
      return NS_ERROR_ILLEGAL_VALUE;
  }

  
  while ((res.mBegin <= res.mEnd) && (IS_NBSP_CHAR(str[res.mBegin]))) 
    res.mBegin++;
  if (str[res.mEnd] == (unsigned char)0x20)
  {
    PRUint32 realEndWord = res.mEnd - 1;
    while ((realEndWord > res.mBegin) && (IS_NBSP_CHAR(str[realEndWord]))) 
      realEndWord--;
    if (realEndWord < res.mEnd - 1) 
      res.mEnd = realEndWord + 1;
  }

  
  
  

  PRInt32 i, lastIndex = aOffsetTable->Length() - 1;

  for (i=0; i <= lastIndex; i++)
  {
    entry = (*aOffsetTable)[i];

    PRInt32 strEndOffset = entry->mStrOffset + entry->mLength;

    
    
    
    

    if (entry->mStrOffset <= res.mBegin &&
       (res.mBegin < strEndOffset || (res.mBegin == strEndOffset && i == lastIndex)))
    {
      if (aWordStartNode)
      {
        *aWordStartNode = entry->mNode;
        NS_IF_ADDREF(*aWordStartNode);
      }

      if (aWordStartOffset)
        *aWordStartOffset = entry->mNodeOffset + res.mBegin - entry->mStrOffset;

      if (!aWordEndNode && !aWordEndOffset)
      {
        
        

        break;
      }
    }

    
    

    if (entry->mStrOffset <= res.mEnd && res.mEnd <= strEndOffset)
    {
      if (res.mBegin == res.mEnd && res.mEnd == strEndOffset && i != lastIndex)
      {
        
        

        continue;
      }

      if (aWordEndNode)
      {
        *aWordEndNode = entry->mNode;
        NS_IF_ADDREF(*aWordEndNode);
      }

      if (aWordEndOffset)
        *aWordEndOffset = entry->mNodeOffset + res.mEnd - entry->mStrOffset;

      break;
    }
  }


  return NS_OK;
}

#ifdef DEBUG_kin
void
nsTextServicesDocument::PrintOffsetTable()
{
  OffsetEntry *entry;
  PRUint32 i;

  for (i = 0; i < mOffsetTable.Length(); i++)
  {
    entry = mOffsetTable[i];
    printf("ENTRY %4d: %p  %c  %c  %4d  %4d  %4d\n",
           i, entry->mNode,  entry->mIsValid ? 'V' : 'N',
           entry->mIsInsertedText ? 'I' : 'B',
           entry->mNodeOffset, entry->mStrOffset, entry->mLength);
  }

  fflush(stdout);
}

void
nsTextServicesDocument::PrintContentNode(nsIContent *aContent)
{
  nsString tmpStr, str;
  nsresult result;

  aContent->Tag()->ToString(tmpStr);
  printf("%s", NS_LossyConvertUTF16toASCII(tmpStr).get());

  nsCOMPtr<nsIDOMNode> node = do_QueryInterface(aContent);

  if (node)
  {
    PRUint16 type;

    result = node->GetNodeType(&type);

    if (NS_FAILED(result))
      return;

    if (nsIDOMNode::TEXT_NODE == type)
    {
      result = node->GetNodeValue(str);

      if (NS_FAILED(result))
        return;

      printf(":  \"%s\"", NS_LossyConvertUTF16toASCII(str).get());
    }
  }

  printf("\n");
  fflush(stdout);
}
#endif
