




#include <stddef.h>                     

#include "mozilla/Assertions.h"         
#include "mozilla/mozalloc.h"           
#include "nsAString.h"                  
#include "nsAutoPtr.h"                  
#include "nsContentUtils.h"             
#include "nsDebug.h"                    
#include "nsDependentSubstring.h"       
#include "nsError.h"                    
#include "nsFilteredContentIterator.h"  
#include "nsIContent.h"                 
#include "nsIContentIterator.h"         
#include "nsID.h"                       
#include "nsIDOMDocument.h"             
#include "nsIDOMElement.h"              
#include "nsIDOMHTMLDocument.h"         
#include "nsIDOMHTMLElement.h"          
#include "nsIDOMNode.h"                 
#include "nsIDOMRange.h"                
#include "nsIEditor.h"                  
#include "nsINode.h"                    
#include "nsIPlaintextEditor.h"         
#include "nsISelection.h"               
#include "nsISelectionController.h"     
#include "nsISupportsBase.h"            
#include "nsISupportsUtils.h"           
#include "nsITextServicesFilter.h"      
#include "nsIWordBreaker.h"             
#include "nsRange.h"                    
#include "nsStaticAtom.h"               
#include "nsString.h"                   
#include "nsTextServicesDocument.h"
#include "nscore.h"                     

#define LOCK_DOC(doc)
#define UNLOCK_DOC(doc)

using namespace mozilla;

class OffsetEntry
{
public:
  OffsetEntry(nsIDOMNode *aNode, int32_t aOffset, int32_t aLength)
    : mNode(aNode), mNodeOffset(0), mStrOffset(aOffset), mLength(aLength),
      mIsInsertedText(false), mIsValid(true)
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
    mIsValid    = false;
  }

  nsIDOMNode *mNode;
  int32_t mNodeOffset;
  int32_t mStrOffset;
  int32_t mLength;
  bool    mIsInsertedText;
  bool    mIsValid;
};

#define TS_ATOM(name_, value_) nsIAtom* nsTextServicesDocument::name_ = 0;
#include "nsTSAtomList.h" 
#undef TS_ATOM

nsTextServicesDocument::nsTextServicesDocument()
{
  mSelStartIndex  = -1;
  mSelStartOffset = -1;
  mSelEndIndex    = -1;
  mSelEndOffset   = -1;

  mIteratorStatus = eIsDone;
}

nsTextServicesDocument::~nsTextServicesDocument()
{
  ClearOffsetTable(&mOffsetTable);
}

#define TS_ATOM(name_, value_) NS_STATIC_ATOM_BUFFER(name_##_buffer, value_)
#include "nsTSAtomList.h" 
#undef TS_ATOM


void
nsTextServicesDocument::RegisterAtoms()
{
  static const nsStaticAtom ts_atoms[] = {
#define TS_ATOM(name_, value_) NS_STATIC_ATOM(name_##_buffer, &name_),
#include "nsTSAtomList.h" 
#undef TS_ATOM
  };

  NS_RegisterStaticAtoms(ts_atoms);
}

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsTextServicesDocument)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsTextServicesDocument)

NS_INTERFACE_MAP_BEGIN(nsTextServicesDocument)
  NS_INTERFACE_MAP_ENTRY(nsITextServicesDocument)
  NS_INTERFACE_MAP_ENTRY(nsIEditActionListener)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsITextServicesDocument)
  NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION(nsTextServicesDocument)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION(nsTextServicesDocument,
                         mDOMDocument,
                         mSelCon,
                         mIterator,
                         mPrevTextBlock,
                         mNextTextBlock,
                         mExtent,
                         mTxtSvcFilter)

NS_IMETHODIMP
nsTextServicesDocument::InitWithEditor(nsIEditor *aEditor)
{
  nsresult result = NS_OK;
  nsCOMPtr<nsISelectionController> selCon;
  nsCOMPtr<nsIDOMDocument> doc;

  NS_ENSURE_TRUE(aEditor, NS_ERROR_NULL_POINTER);

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

  result = aEditor->AddEditActionListener(this);

  UNLOCK_DOC(this);

  return result;
}

NS_IMETHODIMP 
nsTextServicesDocument::GetDocument(nsIDOMDocument **aDoc)
{
  NS_ENSURE_TRUE(aDoc, NS_ERROR_NULL_POINTER);

  *aDoc = nullptr; 
  NS_ENSURE_TRUE(mDOMDocument, NS_ERROR_NOT_INITIALIZED);

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
  int32_t rngStartOffset, rngEndOffset;

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
    
    
    NS_ASSERTION(false, "Found a first without a last!");
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
                             nullptr, &blockStr);
  if (NS_FAILED(result))
  {
    ClearOffsetTable(&offsetTable);
    return result;
  }

  nsCOMPtr<nsIDOMNode> wordStartNode, wordEndNode;
  int32_t wordStartOffset, wordEndOffset;

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
                             nullptr, &blockStr);
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

  NS_ENSURE_TRUE(aStr, NS_ERROR_NULL_POINTER);

  aStr->Truncate();

  NS_ENSURE_TRUE(mIterator, NS_ERROR_FAILURE);

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
    mPrevTextBlock  = nullptr;
    result = GetFirstTextNodeInNextBlock(getter_AddRefs(mNextTextBlock));
  }
  else
  {
    

    mPrevTextBlock  = nullptr;
    mNextTextBlock  = nullptr;
  }

  UNLOCK_DOC(this);

  return result;
}

NS_IMETHODIMP
nsTextServicesDocument::LastSelectedBlock(TSDBlockSelectionStatus *aSelStatus,
                                          int32_t *aSelOffset,
                                          int32_t *aSelLength)
{
  nsresult result = NS_OK;

  NS_ENSURE_TRUE(aSelStatus && aSelOffset && aSelLength, NS_ERROR_NULL_POINTER);

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
  bool isCollapsed = false;

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
  int32_t                      i, rangeCount, offset;

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
                                 mExtent, nullptr);

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
        result = SetSelectionInternal(*aSelOffset, *aSelLength, false);
    }
    else
    {
      
      
      
      

      result = CreateDocumentContentRootToNodeOffsetRange(parent, offset, false, getter_AddRefs(range));

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

        content = nullptr;

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
                                 mExtent, nullptr);

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
      if (iter->GetCurrentNode()->NodeType() == nsIDOMNode::TEXT_NODE) {
        
        
        
        nsCOMPtr<nsIContent> content = iter->GetCurrentNode()->AsContent();

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
                                   mExtent, nullptr);

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

  result = CreateDocumentContentRootToNodeOffsetRange(parent, offset, false, getter_AddRefs(range));

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
    if (iter->GetCurrentNode()->NodeType() == nsIDOMNode::TEXT_NODE) {
      
      
      nsCOMPtr<nsIContent> content = iter->GetCurrentNode()->AsContent();

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
                                 mExtent, nullptr);

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

  NS_ENSURE_TRUE(mIterator, NS_ERROR_FAILURE);

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
    

    mPrevTextBlock = nullptr;
    mNextTextBlock = nullptr;
  }

  UNLOCK_DOC(this);

  return result;
}

NS_IMETHODIMP
nsTextServicesDocument::NextBlock()
{
  nsresult result = NS_OK;

  NS_ENSURE_TRUE(mIterator, NS_ERROR_FAILURE);

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
    

    mPrevTextBlock = nullptr;
    mNextTextBlock = nullptr;
  }


  UNLOCK_DOC(this);

  return result;
}

NS_IMETHODIMP
nsTextServicesDocument::IsDone(bool *aIsDone)
{
  NS_ENSURE_TRUE(aIsDone, NS_ERROR_NULL_POINTER);

  *aIsDone = false;

  NS_ENSURE_TRUE(mIterator, NS_ERROR_FAILURE);

  LOCK_DOC(this);

  *aIsDone = (mIteratorStatus == nsTextServicesDocument::eIsDone) ? true : false;

  UNLOCK_DOC(this);

  return NS_OK;
}

NS_IMETHODIMP
nsTextServicesDocument::SetSelection(int32_t aOffset, int32_t aLength)
{
  nsresult result;

  NS_ENSURE_TRUE(mSelCon && aOffset >= 0 && aLength >= 0, NS_ERROR_FAILURE);

  LOCK_DOC(this);

  result = SetSelectionInternal(aOffset, aLength, true);

  UNLOCK_DOC(this);

  
  
  

  return result;
}

NS_IMETHODIMP
nsTextServicesDocument::ScrollSelectionIntoView()
{
  nsresult result;

  NS_ENSURE_TRUE(mSelCon, NS_ERROR_FAILURE);

  LOCK_DOC(this);

  
  
  result = mSelCon->ScrollSelectionIntoView(nsISelectionController::SELECTION_NORMAL, nsISelectionController::SELECTION_FOCUS_REGION,
                                            nsISelectionController::SCROLL_SYNCHRONOUS);

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
  int32_t origStartOffset = 0, origEndOffset = 0;

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

  int32_t i, selLength;
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
        
        

        entry->mIsValid = false;
      }
    }

  
  
  
  
  

    if (i == mSelEndIndex)
    {
      if (entry->mIsInsertedText)
      {
        
        
        
        

        entry->mIsValid = false;
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
          
          

          entry->mIsValid = false;
        }
      }
    }

    if (i != mSelStartIndex && i != mSelEndIndex)
    {
      
      

      entry->mIsValid = false;
    }
  }

  

  AdjustContentIterator();

  

  result = editor->DeleteSelection(nsIEditor::ePrevious, nsIEditor::eStrip);

  if (NS_FAILED(result))
  {
    UNLOCK_DOC(this);
    return result;
  }

  
  
  

  if (origStartNode && origEndNode)
  {
    nsCOMPtr<nsIDOMNode> curStartNode, curEndNode;
    int32_t curStartOffset = 0, curEndOffset = 0;

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

      if (mIteratorStatus != nsTextServicesDocument::eIsDone) {
        
        
        

        curContent = mIterator->GetCurrentNode()
                     ? mIterator->GetCurrentNode()->AsContent()
                     : nullptr;
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

  
  

  for (i = mSelEndIndex; !entry && i < int32_t(mOffsetTable.Length()); i++)
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

  NS_ENSURE_TRUE(aText, NS_ERROR_NULL_POINTER);

  
  
  
  
  
  

  bool collapsedSelection = SelectionIsCollapsed();
  int32_t savedSelOffset = mSelStartOffset;
  int32_t savedSelLength = mSelEndOffset - mSelStartOffset;

  if (!collapsedSelection)
  {
    
    

    result = SetSelection(mSelStartOffset, 0);

    NS_ENSURE_SUCCESS(result, result);
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

  
  
  
  
  

  int32_t strLength = aText->Length();
  uint32_t i;

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

      itEntry->mIsInsertedText = true;
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
      itEntry->mIsInsertedText = true;

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

    itEntry->mIsInsertedText = true;
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

NS_IMETHODIMP
nsTextServicesDocument::DidInsertNode(nsIDOMNode *aNode,
                                      nsIDOMNode *aParent,
                                      int32_t     aPosition,
                                      nsresult    aResult)
{
  return NS_OK;
}

NS_IMETHODIMP
nsTextServicesDocument::DidDeleteNode(nsIDOMNode *aChild, nsresult aResult)
{
  NS_ENSURE_SUCCESS(aResult, NS_OK);

  NS_ENSURE_TRUE(mIterator, NS_ERROR_FAILURE);

  
  
  
  

  LOCK_DOC(this);

  int32_t nodeIndex = 0;
  bool hasEntry = false;
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

  int32_t tcount = mOffsetTable.Length();

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
      entry->mIsValid = false;
    }

    nodeIndex++;
  }

  UNLOCK_DOC(this);

  return NS_OK;
}

NS_IMETHODIMP
nsTextServicesDocument::DidSplitNode(nsIDOMNode *aExistingRightNode,
                                     int32_t     aOffset,
                                     nsIDOMNode *aNewLeftNode,
                                     nsresult    aResult)
{
  
  
  
  
  return NS_OK;
}

NS_IMETHODIMP
nsTextServicesDocument::DidJoinNodes(nsIDOMNode  *aLeftNode,
                                     nsIDOMNode  *aRightNode,
                                     nsIDOMNode  *aParent,
                                     nsresult     aResult)
{
  NS_ENSURE_SUCCESS(aResult, NS_OK);

  int32_t i;
  uint16_t type;
  nsresult result;

  
  
  
  

  

  result = aLeftNode->GetNodeType(&type);
  NS_ENSURE_SUCCESS(result, NS_OK);
  if (nsIDOMNode::TEXT_NODE != type) {
    return NS_OK;
  }

  result = aRightNode->GetNodeType(&type);
  NS_ENSURE_SUCCESS(result, NS_OK);
  if (nsIDOMNode::TEXT_NODE != type) {
    return NS_OK;
  }

  
  

  int32_t leftIndex = 0;
  int32_t rightIndex = 0;
  bool leftHasEntry = false;
  bool rightHasEntry = false;

  result = NodeHasOffsetEntry(&mOffsetTable, aLeftNode, &leftHasEntry, &leftIndex);

  NS_ENSURE_SUCCESS(result, result);

  if (!leftHasEntry)
  {
    
    
    return NS_OK;
  }

  result = NodeHasOffsetEntry(&mOffsetTable, aRightNode, &rightHasEntry, &rightIndex);

  NS_ENSURE_SUCCESS(result, result);

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
  int32_t nodeLength = str.Length();

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

  
  

  for (i = rightIndex; i < int32_t(mOffsetTable.Length()); i++)
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
  NS_ENSURE_TRUE(aRange && aIterator, NS_ERROR_NULL_POINTER);

  *aIterator = nullptr;

  
  
  
  nsRefPtr<nsFilteredContentIterator> filter = new nsFilteredContentIterator(mTxtSvcFilter);

  nsresult result = filter->Init(aRange);
  if (NS_FAILED(result)) {
    return result;
  }

  filter.forget(aIterator);
  return NS_OK;
}

nsresult
nsTextServicesDocument::GetDocumentContentRootNode(nsIDOMNode **aNode)
{
  nsresult result;

  NS_ENSURE_TRUE(aNode, NS_ERROR_NULL_POINTER);

  *aNode = 0;

  NS_ENSURE_TRUE(mDOMDocument, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDOMHTMLDocument> htmlDoc = do_QueryInterface(mDOMDocument);

  if (htmlDoc)
  {
    

    nsCOMPtr<nsIDOMHTMLElement> bodyElement;

    result = htmlDoc->GetBody(getter_AddRefs(bodyElement));

    NS_ENSURE_SUCCESS(result, result);

    NS_ENSURE_TRUE(bodyElement, NS_ERROR_FAILURE);

    result = bodyElement->QueryInterface(NS_GET_IID(nsIDOMNode), (void **)aNode);
  }
  else
  {
    

    nsCOMPtr<nsIDOMElement> docElement;

    result = mDOMDocument->GetDocumentElement(getter_AddRefs(docElement));

    NS_ENSURE_SUCCESS(result, result);

    NS_ENSURE_TRUE(docElement, NS_ERROR_FAILURE);

    result = docElement->QueryInterface(NS_GET_IID(nsIDOMNode), (void **)aNode);
  }

  return result;
}

nsresult
nsTextServicesDocument::CreateDocumentContentRange(nsIDOMRange **aRange)
{
  *aRange = nullptr;

  nsCOMPtr<nsIDOMNode> node;
  nsresult rv = GetDocumentContentRootNode(getter_AddRefs(node));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(node, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsINode> nativeNode = do_QueryInterface(node);
  NS_ENSURE_STATE(nativeNode);

  nsRefPtr<nsRange> range = new nsRange(nativeNode);

  rv = range->SelectNodeContents(node);
  NS_ENSURE_SUCCESS(rv, rv);

  range.forget(aRange);
  return NS_OK;
}

nsresult
nsTextServicesDocument::CreateDocumentContentRootToNodeOffsetRange(nsIDOMNode *aParent, int32_t aOffset, bool aToStart, nsIDOMRange **aRange)
{
  NS_ENSURE_TRUE(aParent && aRange, NS_ERROR_NULL_POINTER);

  *aRange = 0;

  NS_ASSERTION(aOffset >= 0, "Invalid offset!");

  if (aOffset < 0)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMNode> bodyNode; 
  nsresult rv = GetDocumentContentRootNode(getter_AddRefs(bodyNode));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(bodyNode, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsIDOMNode> startNode;
  nsCOMPtr<nsIDOMNode> endNode;
  int32_t startOffset, endOffset;

  if (aToStart) {
    
    

    startNode   = bodyNode;
    startOffset = 0;
    endNode     = aParent;
    endOffset   = aOffset;
  } else {
    
    

    startNode   = aParent;
    startOffset = aOffset;
    endNode     = bodyNode;

    nsCOMPtr<nsINode> body = do_QueryInterface(bodyNode);
    endOffset = body ? int32_t(body->GetChildCount()) : 0;
  }

  return nsRange::CreateRange(startNode, startOffset, endNode, endOffset,
                              aRange);
}

nsresult
nsTextServicesDocument::CreateDocumentContentIterator(nsIContentIterator **aIterator)
{
  nsresult result;

  NS_ENSURE_TRUE(aIterator, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsIDOMRange> range;

  result = CreateDocumentContentRange(getter_AddRefs(range));

  NS_ENSURE_SUCCESS(result, result);

  result = CreateContentIterator(range, aIterator);

  return result;
}

nsresult
nsTextServicesDocument::AdjustContentIterator()
{
  nsresult result = NS_OK;

  NS_ENSURE_TRUE(mIterator, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDOMNode> node(do_QueryInterface(mIterator->GetCurrentNode()));

  NS_ENSURE_TRUE(node, NS_ERROR_FAILURE);

  nsIDOMNode *nodePtr = node.get();
  int32_t tcount      = mOffsetTable.Length();

  nsIDOMNode *prevValidNode = 0;
  nsIDOMNode *nextValidNode = 0;
  bool foundEntry = false;
  OffsetEntry *entry;

  for (int32_t i = 0; i < tcount && !nextValidNode; i++)
  {
    entry = mOffsetTable[i];

    NS_ENSURE_TRUE(entry, NS_ERROR_FAILURE);

    if (entry->mNode == nodePtr)
    {
      if (entry->mIsValid)
      {
        
        

        return NS_OK;
      }
      else
      {
        
        
        

        foundEntry = true;
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

bool
nsTextServicesDocument::DidSkip(nsIContentIterator* aFilteredIter)
{
  
  
  
  
  
  if (aFilteredIter) {
    nsFilteredContentIterator* filter = static_cast<nsFilteredContentIterator *>(aFilteredIter);
    if (filter && filter->DidSkip()) {
      return true;
    }
  }
  return false;
}

void
nsTextServicesDocument::ClearDidSkip(nsIContentIterator* aFilteredIter)
{
  
  if (aFilteredIter) {
    nsFilteredContentIterator* filter = static_cast<nsFilteredContentIterator *>(aFilteredIter);
    filter->ClearDidSkip();
  }
}

bool
nsTextServicesDocument::IsBlockNode(nsIContent *aContent)
{
  if (!aContent) {
    NS_ERROR("How did a null pointer get passed to IsBlockNode?");
    return false;
  }

  nsIAtom *atom = aContent->Tag();

  return (sAAtom       != atom &&
          sAddressAtom != atom &&
          sBigAtom     != atom &&
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

bool
nsTextServicesDocument::HasSameBlockNodeParent(nsIContent *aContent1, nsIContent *aContent2)
{
  nsIContent* p1 = aContent1->GetParent();
  nsIContent* p2 = aContent2->GetParent();

  

  if (p1 == p2)
    return true;

  

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

bool
nsTextServicesDocument::IsTextNode(nsIContent *aContent)
{
  NS_ENSURE_TRUE(aContent, false);
  return nsIDOMNode::TEXT_NODE == aContent->NodeType();
}

bool
nsTextServicesDocument::IsTextNode(nsIDOMNode *aNode)
{
  NS_ENSURE_TRUE(aNode, false);

  nsCOMPtr<nsIContent> content = do_QueryInterface(aNode);
  return IsTextNode(content);
}

nsresult
nsTextServicesDocument::SetSelectionInternal(int32_t aOffset, int32_t aLength, bool aDoUpdate)
{
  nsresult result = NS_OK;

  NS_ENSURE_TRUE(mSelCon && aOffset >= 0 && aLength >= 0, NS_ERROR_FAILURE);

  nsIDOMNode *sNode = 0, *eNode = 0;
  int32_t i, sOffset = 0, eOffset = 0;
  OffsetEntry *entry;

  

  for (i = 0; !sNode && i < int32_t(mOffsetTable.Length()); i++)
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
        bool foundEntry = false;
        int32_t strEndOffset = entry->mStrOffset + entry->mLength;

        if (aOffset < strEndOffset)
          foundEntry = true;
        else if (aOffset == strEndOffset)
        {
          
          
          
          

          if ((i+1) < int32_t(mOffsetTable.Length()))
          {
            OffsetEntry *nextEntry = mOffsetTable[i+1];

            if (!nextEntry->mIsValid || nextEntry->mStrOffset != aOffset)
            {
              
              
              foundEntry = true;
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

  NS_ENSURE_TRUE(sNode, NS_ERROR_FAILURE);

  
  

  nsCOMPtr<nsISelection> selection;

  if (aDoUpdate)
  {
    result = mSelCon->GetSelection(nsISelectionController::SELECTION_NORMAL, getter_AddRefs(selection));

    NS_ENSURE_SUCCESS(result, result);

    result = selection->Collapse(sNode, sOffset);

    NS_ENSURE_SUCCESS(result, result);
   }

  if (aLength <= 0)
  {
    

    mSelEndIndex  = mSelStartIndex;
    mSelEndOffset = mSelStartOffset;

   
   
   

    return NS_OK;
  }

  

  int32_t endOffset = aOffset + aLength;

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

    NS_ENSURE_SUCCESS(result, result);
  }

  
  
  

  return result;
}

nsresult
nsTextServicesDocument::GetSelection(nsITextServicesDocument::TSDBlockSelectionStatus *aSelStatus, int32_t *aSelOffset, int32_t *aSelLength)
{
  nsresult result;

  NS_ENSURE_TRUE(aSelStatus && aSelOffset && aSelLength, NS_ERROR_NULL_POINTER);

  *aSelStatus = nsITextServicesDocument::eBlockNotFound;
  *aSelOffset = -1;
  *aSelLength = -1;

  NS_ENSURE_TRUE(mDOMDocument && mSelCon, NS_ERROR_FAILURE);

  if (mIteratorStatus == nsTextServicesDocument::eIsDone)
    return NS_OK;

  nsCOMPtr<nsISelection> selection;
  bool isCollapsed;

  result = mSelCon->GetSelection(nsISelectionController::SELECTION_NORMAL, getter_AddRefs(selection));

  NS_ENSURE_SUCCESS(result, result);

  NS_ENSURE_TRUE(selection, NS_ERROR_FAILURE);

  result = selection->GetIsCollapsed(&isCollapsed);

  NS_ENSURE_SUCCESS(result, result);

  
  

  

  if (isCollapsed)
    result = GetCollapsedSelection(aSelStatus, aSelOffset, aSelLength);
  else
    result = GetUncollapsedSelection(aSelStatus, aSelOffset, aSelLength);

  

  return result;
}

nsresult
nsTextServicesDocument::GetCollapsedSelection(nsITextServicesDocument::TSDBlockSelectionStatus *aSelStatus, int32_t *aSelOffset, int32_t *aSelLength)
{
  nsCOMPtr<nsISelection> selection;
  nsresult result = mSelCon->GetSelection(nsISelectionController::SELECTION_NORMAL, getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(result, result);
  NS_ENSURE_TRUE(selection, NS_ERROR_FAILURE);

  
  
  *aSelStatus = nsITextServicesDocument::eBlockOutside;
  *aSelOffset = *aSelLength = -1;

  int32_t tableCount = mOffsetTable.Length();

  if (tableCount == 0)
    return NS_OK;

  
  

  OffsetEntry* eStart = mOffsetTable[0];
  OffsetEntry* eEnd;
  if (tableCount > 1)
    eEnd = mOffsetTable[tableCount - 1];
  else
    eEnd = eStart;

  int32_t eStartOffset = eStart->mNodeOffset;
  int32_t eEndOffset   = eEnd->mNodeOffset + eEnd->mLength;

  nsCOMPtr<nsIDOMRange> range;
  result = selection->GetRangeAt(0, getter_AddRefs(range));
  NS_ENSURE_SUCCESS(result, result);

  nsCOMPtr<nsIDOMNode> domParent;
  result = range->GetStartContainer(getter_AddRefs(domParent));
  NS_ENSURE_SUCCESS(result, result);

  nsCOMPtr<nsINode> parent = do_QueryInterface(domParent);
  MOZ_ASSERT(parent);

  int32_t offset;
  result = range->GetStartOffset(&offset);
  NS_ENSURE_SUCCESS(result, result);

  int32_t e1s1 = nsContentUtils::ComparePoints(eStart->mNode, eStartOffset,
                                               domParent, offset);
  int32_t e2s1 = nsContentUtils::ComparePoints(eEnd->mNode, eEndOffset,
                                               domParent, offset);

  if (e1s1 > 0 || e2s1 < 0) {
    
    return NS_OK;
  }

  if (parent->NodeType() == nsIDOMNode::TEXT_NODE) {
    
    
    

    for (int32_t i = 0; i < tableCount; i++) {
      OffsetEntry* entry = mOffsetTable[i];
      NS_ENSURE_TRUE(entry, NS_ERROR_FAILURE);

      if (entry->mNode == domParent.get() &&
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

  
  
  
  
  
  

  result = CreateRange(eStart->mNode, eStartOffset, eEnd->mNode, eEndOffset, getter_AddRefs(range));
  NS_ENSURE_SUCCESS(result, result);

  nsCOMPtr<nsIContentIterator> iter;
  result = CreateContentIterator(range, getter_AddRefs(iter));
  NS_ENSURE_SUCCESS(result, result);

  nsIContent* saveNode;
  if (parent->HasChildren()) {
    
    

    
    

    uint32_t childIndex = (uint32_t)offset;

    if (childIndex > 0) {
      uint32_t numChildren = parent->GetChildCount();
      NS_ASSERTION(childIndex <= numChildren, "Invalid selection offset!");

      if (childIndex > numChildren) {
        childIndex = numChildren;
      }

      childIndex -= 1;
    }

    nsIContent* content = parent->GetChildAt(childIndex);
    NS_ENSURE_TRUE(content, NS_ERROR_FAILURE);

    result = iter->PositionAt(content);
    NS_ENSURE_SUCCESS(result, result);

    saveNode = content;
  } else {
    
    
    NS_ENSURE_TRUE(parent->IsContent(), NS_ERROR_FAILURE);
    nsCOMPtr<nsIContent> content = parent->AsContent();

    result = iter->PositionAt(content);
    NS_ENSURE_SUCCESS(result, result);

    saveNode = content;
  }

  
  
  

  nsIContent* node = nullptr;
  while (!iter->IsDone()) {
    nsINode* current = iter->GetCurrentNode();
    if (current->NodeType() == nsIDOMNode::TEXT_NODE) {
      node = static_cast<nsIContent*>(current);
      break;
    }

    iter->Prev();
  }

  if (node) {
    
    
    offset = node->TextLength();
  } else {
    

    
    
    

    result = iter->PositionAt(saveNode);
    NS_ENSURE_SUCCESS(result, result);

    node = nullptr;
    while (!iter->IsDone()) {
      nsINode* current = iter->GetCurrentNode();

      if (current->NodeType() == nsIDOMNode::TEXT_NODE) {
        node = static_cast<nsIContent*>(current);
        break;
      }

      iter->Next();
    }

    NS_ENSURE_TRUE(node, NS_ERROR_FAILURE);

    
    

    offset = 0;
  }

  for (int32_t i = 0; i < tableCount; i++) {
    OffsetEntry* entry = mOffsetTable[i];
    NS_ENSURE_TRUE(entry, NS_ERROR_FAILURE);

    if (entry->mNode == node->AsDOMNode() &&
        entry->mNodeOffset <= offset && offset <= (entry->mNodeOffset + entry->mLength))
    {
      *aSelStatus = nsITextServicesDocument::eBlockContains;
      *aSelOffset = entry->mStrOffset + (offset - entry->mNodeOffset);
      *aSelLength = 0;

      
      
      
      
      

      result = SetSelectionInternal(*aSelOffset, *aSelLength, true);

      return result;
    }
  }

  return NS_ERROR_FAILURE;
}

nsresult
nsTextServicesDocument::GetUncollapsedSelection(nsITextServicesDocument::TSDBlockSelectionStatus *aSelStatus, int32_t *aSelOffset, int32_t *aSelLength)
{
  nsresult result;

  nsCOMPtr<nsISelection> selection;
  nsCOMPtr<nsIDOMRange> range;
  OffsetEntry *entry;

  result = mSelCon->GetSelection(nsISelectionController::SELECTION_NORMAL, getter_AddRefs(selection));

  NS_ENSURE_SUCCESS(result, result);

  NS_ENSURE_TRUE(selection, NS_ERROR_FAILURE);

  
  
  

  nsCOMPtr<nsIDOMNode> startParent, endParent;
  int32_t startOffset, endOffset;
  int32_t rangeCount, tableCount, i;
  int32_t e1s1, e1s2, e2s1, e2s2;

  OffsetEntry *eStart, *eEnd;
  int32_t eStartOffset, eEndOffset;

  tableCount = mOffsetTable.Length();

  
  

  eStart = mOffsetTable[0];

  if (tableCount > 1)
    eEnd = mOffsetTable[tableCount - 1];
  else
    eEnd = eStart;

  eStartOffset = eStart->mNodeOffset;
  eEndOffset   = eEnd->mNodeOffset + eEnd->mLength;

  result = selection->GetRangeCount(&rangeCount);

  NS_ENSURE_SUCCESS(result, result);

  
  

  for (i = 0; i < rangeCount; i++)
  {
    result = selection->GetRangeAt(i, getter_AddRefs(range));

    NS_ENSURE_SUCCESS(result, result);

    result = GetRangeEndPoints(range,
                               getter_AddRefs(startParent), &startOffset,
                               getter_AddRefs(endParent), &endOffset);

    NS_ENSURE_SUCCESS(result, result);

    e1s2 = nsContentUtils::ComparePoints(eStart->mNode, eStartOffset,
                                         endParent, endOffset);
    e2s1 = nsContentUtils::ComparePoints(eEnd->mNode, eEndOffset,
                                         startParent, startOffset);

    

    if (e1s2 <= 0 && e2s1 >= 0)
      break;
  }

  

  if (rangeCount < 1 || e1s2 > 0 || e2s1 < 0)
  {
    *aSelStatus = nsITextServicesDocument::eBlockOutside;
    *aSelOffset = *aSelLength = -1;
    return NS_OK;
  }

  

  e1s1 = nsContentUtils::ComparePoints(eStart->mNode, eStartOffset,
                                       startParent, startOffset);
  e2s2 = nsContentUtils::ComparePoints(eEnd->mNode, eEndOffset,
                                       endParent, endOffset);

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
  int32_t     o1,  o2;

  
  

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

  NS_ENSURE_SUCCESS(result, result);

  
  

  nsCOMPtr<nsIContentIterator> iter;

  result = CreateContentIterator(range, getter_AddRefs(iter));

  NS_ENSURE_SUCCESS(result, result);

  
  
  bool found;
  nsCOMPtr<nsIContent> content;

  iter->First();

  if (!IsTextNode(p1))
  {
    found = false;

    while (!iter->IsDone())
    {
      content = do_QueryInterface(iter->GetCurrentNode());

      if (IsTextNode(content))
      {
        p1 = do_QueryInterface(content);

        NS_ENSURE_TRUE(p1, NS_ERROR_FAILURE);

        o1 = 0;
        found = true;

        break;
      }

      iter->Next();
    }

    NS_ENSURE_TRUE(found, NS_ERROR_FAILURE);
  }

  

  iter->Last();

  if (! IsTextNode(p2))
  {
    found = false;

    while (!iter->IsDone())
    {
      content = do_QueryInterface(iter->GetCurrentNode());

      if (IsTextNode(content))
      {
        p2 = do_QueryInterface(content);

        NS_ENSURE_TRUE(p2, NS_ERROR_FAILURE);

        nsString str;

        result = p2->GetNodeValue(str);

        NS_ENSURE_SUCCESS(result, result);

        o2 = str.Length();
        found = true;

        break;
      }

      iter->Prev();
    }

    NS_ENSURE_TRUE(found, NS_ERROR_FAILURE);
  }

  found    = false;
  *aSelLength = 0;

  for (i = 0; i < tableCount; i++)
  {
    entry = mOffsetTable[i];

    NS_ENSURE_TRUE(entry, NS_ERROR_FAILURE);

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

        found = true;
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

bool
nsTextServicesDocument::SelectionIsCollapsed()
{
  return(mSelStartIndex == mSelEndIndex && mSelStartOffset == mSelEndOffset);
}

bool
nsTextServicesDocument::SelectionIsValid()
{
  return(mSelStartIndex >= 0);
}

nsresult
nsTextServicesDocument::GetRangeEndPoints(nsIDOMRange *aRange,
                                          nsIDOMNode **aStartParent, int32_t *aStartOffset,
                                          nsIDOMNode **aEndParent, int32_t *aEndOffset)
{
  nsresult result;

  NS_ENSURE_TRUE(aRange && aStartParent && aStartOffset && aEndParent && aEndOffset, NS_ERROR_NULL_POINTER);

  result = aRange->GetStartContainer(aStartParent);

  NS_ENSURE_SUCCESS(result, result);

  NS_ENSURE_TRUE(aStartParent, NS_ERROR_FAILURE);

  result = aRange->GetStartOffset(aStartOffset);

  NS_ENSURE_SUCCESS(result, result);

  result = aRange->GetEndContainer(aEndParent);

  NS_ENSURE_SUCCESS(result, result);

  NS_ENSURE_TRUE(aEndParent, NS_ERROR_FAILURE);

  result = aRange->GetEndOffset(aEndOffset);

  return result;
}


nsresult
nsTextServicesDocument::CreateRange(nsIDOMNode *aStartParent, int32_t aStartOffset,
                                    nsIDOMNode *aEndParent, int32_t aEndOffset,
                                    nsIDOMRange **aRange)
{
  return nsRange::CreateRange(aStartParent, aStartOffset, aEndParent,
                              aEndOffset, aRange);
}

nsresult
nsTextServicesDocument::FirstTextNode(nsIContentIterator *aIterator,
                                      TSDIteratorStatus *aIteratorStatus)
{
  if (aIteratorStatus)
    *aIteratorStatus = nsTextServicesDocument::eIsDone;

  aIterator->First();

  while (!aIterator->IsDone()) {
    if (aIterator->GetCurrentNode()->NodeType() == nsIDOMNode::TEXT_NODE) {
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

  while (!aIterator->IsDone()) {
    if (aIterator->GetCurrentNode()->NodeType() == nsIDOMNode::TEXT_NODE) {
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
  NS_ENSURE_TRUE(iter, NS_ERROR_NULL_POINTER);

  ClearDidSkip(iter);

  nsCOMPtr<nsIContent> last;

  
  

  while (!iter->IsDone())
  {
    nsCOMPtr<nsIContent> content = iter->GetCurrentNode()->IsContent()
                                   ? iter->GetCurrentNode()->AsContent()
                                   : nullptr;

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

  NS_ENSURE_TRUE(aIterator, NS_ERROR_NULL_POINTER);

  

  
  

  result = FirstTextNodeInCurrentBlock(aIterator);

  NS_ENSURE_SUCCESS(result, NS_ERROR_FAILURE);

  

  aIterator->Prev();

  if (aIterator->IsDone())
    return NS_ERROR_FAILURE;

  

  return FirstTextNodeInCurrentBlock(aIterator);
}

nsresult
nsTextServicesDocument::FirstTextNodeInNextBlock(nsIContentIterator *aIterator)
{
  nsCOMPtr<nsIContent> prev;
  bool crossedBlockBoundary = false;

  NS_ENSURE_TRUE(aIterator, NS_ERROR_NULL_POINTER);

  ClearDidSkip(aIterator);

  while (!aIterator->IsDone())
  {
    nsCOMPtr<nsIContent> content = aIterator->GetCurrentNode()->IsContent()
                                   ? aIterator->GetCurrentNode()->AsContent()
                                   : nullptr;

    if (IsTextNode(content))
    {
      if (!crossedBlockBoundary && (!prev || HasSameBlockNodeParent(prev, content)))
        prev = content;
      else
        break;
    }
    else if (!crossedBlockBoundary && IsBlockNode(content))
      crossedBlockBoundary = true;

    aIterator->Next();

    if (!crossedBlockBoundary && DidSkip(aIterator))
      crossedBlockBoundary = true;
  }

  return NS_OK;
}

nsresult
nsTextServicesDocument::GetFirstTextNodeInPrevBlock(nsIContent **aContent)
{
  nsresult result;

  NS_ENSURE_TRUE(aContent, NS_ERROR_NULL_POINTER);

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
    nsCOMPtr<nsIContent> current = mIterator->GetCurrentNode()->IsContent()
                                   ? mIterator->GetCurrentNode()->AsContent()
                                   : nullptr;
    current.forget(aContent);
  }

  

  return mIterator->PositionAt(node);
}

nsresult
nsTextServicesDocument::GetFirstTextNodeInNextBlock(nsIContent **aContent)
{
  nsresult result;

  NS_ENSURE_TRUE(aContent, NS_ERROR_NULL_POINTER);

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
    nsCOMPtr<nsIContent> current = mIterator->GetCurrentNode()->IsContent()
                                   ? mIterator->GetCurrentNode()->AsContent()
                                   : nullptr;
    current.forget(aContent);
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

  NS_ENSURE_TRUE(aIterator, NS_ERROR_NULL_POINTER);

  ClearOffsetTable(aOffsetTable);

  if (aStr)
    aStr->Truncate();

  if (*aIteratorStatus == nsTextServicesDocument::eIsDone)
    return NS_OK;

  
  
  
  
  nsCOMPtr<nsIDOMNode> rngStartNode, rngEndNode;
  int32_t rngStartOffset = 0, rngEndOffset = 0;

  if (aIterRange)
  {
    result = GetRangeEndPoints(aIterRange,
                               getter_AddRefs(rngStartNode), &rngStartOffset,
                               getter_AddRefs(rngEndNode), &rngEndOffset);

    NS_ENSURE_SUCCESS(result, result);
  }

  
  
  

  result = FirstTextNodeInCurrentBlock(aIterator);

  NS_ENSURE_SUCCESS(result, result);

  int32_t offset = 0;

  ClearDidSkip(aIterator);

  while (!aIterator->IsDone())
  {
    nsCOMPtr<nsIContent> content = aIterator->GetCurrentNode()->IsContent()
                                   ? aIterator->GetCurrentNode()->AsContent()
                                   : nullptr;

    if (IsTextNode(content))
    {
      if (!prev || HasSameBlockNodeParent(prev, content))
      {
        nsCOMPtr<nsIDOMNode> node = do_QueryInterface(content);

        if (node)
        {
          nsString str;

          result = node->GetNodeValue(str);

          NS_ENSURE_SUCCESS(result, result);

          

          OffsetEntry *entry = new OffsetEntry(node, offset, str.Length());
          aOffsetTable->AppendElement(entry);

          
          
          
          

          int32_t startOffset = 0;
          int32_t endOffset   = str.Length();
          bool adjustStr    = false;

          if (entry->mNode == rngStartNode)
          {
            entry->mNodeOffset = startOffset = rngStartOffset;
            adjustStr = true;
          }

          if (entry->mNode == rngEndNode)
          {
            endOffset = rngEndOffset;
            adjustStr = true;
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
  int32_t i = 0;

  while (uint32_t(i) < mOffsetTable.Length())
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
  uint32_t i;

  for (i = 0; i < aOffsetTable->Length(); i++)
  {
    delete aOffsetTable->ElementAt(i);
  }

  aOffsetTable->Clear();

  return NS_OK;
}

nsresult
nsTextServicesDocument::SplitOffsetEntry(int32_t aTableIndex, int32_t aNewEntryLength)
{
  OffsetEntry *entry = mOffsetTable[aTableIndex];

  NS_ASSERTION((aNewEntryLength > 0), "aNewEntryLength <= 0");
  NS_ASSERTION((aNewEntryLength < entry->mLength), "aNewEntryLength >= mLength");

  if (aNewEntryLength < 1 || aNewEntryLength >= entry->mLength)
    return NS_ERROR_FAILURE;

  int32_t oldLength = entry->mLength - aNewEntryLength;

  OffsetEntry *newEntry = new OffsetEntry(entry->mNode,
                                          entry->mStrOffset + oldLength,
                                          aNewEntryLength);

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
nsTextServicesDocument::NodeHasOffsetEntry(nsTArray<OffsetEntry*> *aOffsetTable, nsIDOMNode *aNode, bool *aHasEntry, int32_t *aEntryIndex)
{
  OffsetEntry *entry;
  uint32_t i;

  NS_ENSURE_TRUE(aNode && aHasEntry && aEntryIndex, NS_ERROR_NULL_POINTER);

  for (i = 0; i < aOffsetTable->Length(); i++)
  {
    entry = (*aOffsetTable)[i];

    NS_ENSURE_TRUE(entry, NS_ERROR_FAILURE);

    if (entry->mNode == aNode)
    {
      *aHasEntry   = true;
      *aEntryIndex = i;

      return NS_OK;
    }
  }

  *aHasEntry   = false;
  *aEntryIndex = -1;

  return NS_OK;
}


#define IS_NBSP_CHAR(c) (((unsigned char)0xa0)==(c))

nsresult
nsTextServicesDocument::FindWordBounds(nsTArray<OffsetEntry*> *aOffsetTable,
                                       nsString *aBlockStr,
                                       nsIDOMNode *aNode,
                                       int32_t aNodeOffset,
                                       nsIDOMNode **aWordStartNode,
                                       int32_t *aWordStartOffset,
                                       nsIDOMNode **aWordEndNode,
                                       int32_t *aWordEndOffset)
{
  

  if (aWordStartNode)
    *aWordStartNode = nullptr;
  if (aWordStartOffset)
    *aWordStartOffset = 0;
  if (aWordEndNode)
    *aWordEndNode = nullptr;
  if (aWordEndOffset)
    *aWordEndOffset = 0;

  int32_t entryIndex = 0;
  bool hasEntry = false;

  
  
  

  nsresult result = NodeHasOffsetEntry(aOffsetTable, aNode, &hasEntry, &entryIndex);
  NS_ENSURE_SUCCESS(result, result);
  NS_ENSURE_TRUE(hasEntry, NS_ERROR_FAILURE);

  

  OffsetEntry *entry = (*aOffsetTable)[entryIndex];
  uint32_t strOffset = entry->mStrOffset + aNodeOffset - entry->mNodeOffset;

  
  

  const char16_t *str = aBlockStr->get();
  uint32_t strLen = aBlockStr->Length();

  nsIWordBreaker* wordBreaker = nsContentUtils::WordBreaker();
  nsWordRange res = wordBreaker->FindWord(str, strLen, strOffset);
  if (res.mBegin > strLen) {
    return str ? NS_ERROR_ILLEGAL_VALUE : NS_ERROR_NULL_POINTER;
  }

  
  while ((res.mBegin <= res.mEnd) && (IS_NBSP_CHAR(str[res.mBegin]))) 
    res.mBegin++;
  if (str[res.mEnd] == (unsigned char)0x20)
  {
    uint32_t realEndWord = res.mEnd - 1;
    while ((realEndWord > res.mBegin) && (IS_NBSP_CHAR(str[realEndWord]))) 
      realEndWord--;
    if (realEndWord < res.mEnd - 1) 
      res.mEnd = realEndWord + 1;
  }

  
  
  

  int32_t i, lastIndex = aOffsetTable->Length() - 1;

  for (i=0; i <= lastIndex; i++)
  {
    entry = (*aOffsetTable)[i];

    int32_t strEndOffset = entry->mStrOffset + entry->mLength;

    
    
    
    

    if (uint32_t(entry->mStrOffset) <= res.mBegin &&
        (res.mBegin < uint32_t(strEndOffset) ||
        (res.mBegin == uint32_t(strEndOffset) && i == lastIndex)))
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

    
    

    if (uint32_t(entry->mStrOffset) <= res.mEnd && res.mEnd <= uint32_t(strEndOffset))
    {
      if (res.mBegin == res.mEnd && res.mEnd == uint32_t(strEndOffset) && i != lastIndex)
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

NS_IMETHODIMP
nsTextServicesDocument::WillInsertNode(nsIDOMNode *aNode,
                              nsIDOMNode *aParent,
                              int32_t     aPosition)
{
  return NS_OK;
}

NS_IMETHODIMP
nsTextServicesDocument::WillDeleteNode(nsIDOMNode *aChild)
{
  return NS_OK;
}

NS_IMETHODIMP
nsTextServicesDocument::WillSplitNode(nsIDOMNode *aExistingRightNode,
                             int32_t     aOffset)
{
  return NS_OK;
}

NS_IMETHODIMP
nsTextServicesDocument::WillJoinNodes(nsIDOMNode  *aLeftNode,
                             nsIDOMNode  *aRightNode,
                             nsIDOMNode  *aParent)
{
  return NS_OK;
}






NS_IMETHODIMP
nsTextServicesDocument::WillCreateNode(const nsAString& aTag, nsIDOMNode *aParent, int32_t aPosition)
{
  return NS_OK;
}

NS_IMETHODIMP
nsTextServicesDocument::DidCreateNode(const nsAString& aTag, nsIDOMNode *aNode, nsIDOMNode *aParent, int32_t aPosition, nsresult aResult)
{
  return NS_OK;
}

NS_IMETHODIMP
nsTextServicesDocument::WillInsertText(nsIDOMCharacterData *aTextNode, int32_t aOffset, const nsAString &aString)
{
  return NS_OK;
}

NS_IMETHODIMP
nsTextServicesDocument::DidInsertText(nsIDOMCharacterData *aTextNode, int32_t aOffset, const nsAString &aString, nsresult aResult)
{
  return NS_OK;
}

NS_IMETHODIMP
nsTextServicesDocument::WillDeleteText(nsIDOMCharacterData *aTextNode, int32_t aOffset, int32_t aLength)
{
  return NS_OK;
}

NS_IMETHODIMP
nsTextServicesDocument::DidDeleteText(nsIDOMCharacterData *aTextNode, int32_t aOffset, int32_t aLength, nsresult aResult)
{
  return NS_OK;
}

NS_IMETHODIMP
nsTextServicesDocument::WillDeleteSelection(nsISelection *aSelection)
{
  return NS_OK;
}

NS_IMETHODIMP
nsTextServicesDocument::DidDeleteSelection(nsISelection *aSelection)
{
  return NS_OK;
}

