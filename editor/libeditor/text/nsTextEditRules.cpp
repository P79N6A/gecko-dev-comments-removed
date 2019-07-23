




































#include "nsTextEditRules.h"

#include "nsEditor.h"
#include "nsTextEditUtils.h"
#include "nsCRT.h"

#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsIDOMNode.h"
#include "nsIDOMElement.h"
#include "nsIDOMText.h"
#include "nsIDOMNodeList.h"
#include "nsISelection.h"
#include "nsISelectionPrivate.h"
#include "nsISelectionController.h"
#include "nsIDOMRange.h"
#include "nsIDOMNSRange.h"
#include "nsIDOMCharacterData.h"
#include "nsIContent.h"
#include "nsIContentIterator.h"
#include "nsEditorUtils.h"
#include "EditTxn.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsUnicharUtils.h"
#include "nsILookAndFeel.h"
#include "nsWidgetsCID.h"
#include "DeleteTextTxn.h"
#include "nsNodeIterator.h"
#include "nsIDOMNodeFilter.h"


#include "nsIPresShell.h"
#include "nsFrameSelection.h"

static NS_DEFINE_CID(kLookAndFeelCID, NS_LOOKANDFEEL_CID);

#define CANCEL_OPERATION_IF_READONLY_OR_DISABLED \
  if ((mFlags & nsIPlaintextEditor::eEditorReadonlyMask) || (mFlags & nsIPlaintextEditor::eEditorDisabledMask)) \
  {                     \
    *aCancel = PR_TRUE; \
    return NS_OK;       \
  };


nsresult
NS_NewTextEditRules(nsIEditRules** aInstancePtrResult)
{
  nsTextEditRules * rules = new nsTextEditRules();
  if (rules)
    return rules->QueryInterface(NS_GET_IID(nsIEditRules), (void**) aInstancePtrResult);
  return NS_ERROR_OUT_OF_MEMORY;
}






nsTextEditRules::nsTextEditRules()
: mEditor(nsnull)
, mPasswordText()
, mPasswordIMEText()
, mPasswordIMEIndex(0)
, mFlags(0) 
, mActionNesting(0)
, mLockRulesSniffing(PR_FALSE)
, mDidExplicitlySetInterline(PR_FALSE)
, mTheAction(0)
{
}

nsTextEditRules::~nsTextEditRules()
{
   

  if (mTimer)
    mTimer->Cancel();
}





NS_IMPL_CYCLE_COLLECTION_2(nsTextEditRules, mBogusNode, mCachedSelectionNode)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsTextEditRules)
  NS_INTERFACE_MAP_ENTRY(nsIEditRules)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIEditRules)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsTextEditRules)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsTextEditRules)





NS_IMETHODIMP
nsTextEditRules::Init(nsPlaintextEditor *aEditor, PRUint32 aFlags)
{
  if (!aEditor) { return NS_ERROR_NULL_POINTER; }

  mEditor = aEditor;  
  
  SetFlags(aFlags);
  nsCOMPtr<nsISelection> selection;
  mEditor->GetSelection(getter_AddRefs(selection));
  NS_ASSERTION(selection, "editor cannot get selection");

  
  nsIDOMNode *body = mEditor->GetRoot();

  
  
  nsresult res = CreateBogusNodeIfNeeded(selection);
  if (NS_FAILED(res)) return res;

  if (mFlags & nsIPlaintextEditor::eEditorPlaintextMask)
  {
    
    res = CreateTrailingBRIfNeeded();
    if (NS_FAILED(res)) return res;
  }

  if (body)
  {
    
    nsCOMPtr<nsIDOMRange> wholeDoc =
      do_CreateInstance("@mozilla.org/content/range;1");
    if (!wholeDoc) return NS_ERROR_NULL_POINTER;
    wholeDoc->SetStart(body,0);
    nsCOMPtr<nsIDOMNodeList> list;
    res = body->GetChildNodes(getter_AddRefs(list));
    if (NS_FAILED(res)) return res;
    if (!list) return NS_ERROR_FAILURE;

    PRUint32 listCount;
    res = list->GetLength(&listCount);
    if (NS_FAILED(res)) return res;

    res = wholeDoc->SetEnd(body, listCount);
    if (NS_FAILED(res)) return res;

    
    res = ReplaceNewlines(wholeDoc);
  }

  PRBool deleteBidiImmediately = PR_FALSE;
  nsCOMPtr<nsIPrefBranch> prefBranch =
    do_GetService(NS_PREFSERVICE_CONTRACTID, &res);
  if (NS_SUCCEEDED(res))
    prefBranch->GetBoolPref("bidi.edit.delete_immediately",
                            &deleteBidiImmediately);
  mDeleteBidiImmediately = deleteBidiImmediately;

  return res;
}

NS_IMETHODIMP
nsTextEditRules::DetachEditor()
{
  if (mTimer)
    mTimer->Cancel();

  mEditor = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsTextEditRules::GetFlags(PRUint32 *aFlags)
{
  if (!aFlags) { return NS_ERROR_NULL_POINTER; }
  *aFlags = mFlags;
  return NS_OK;
}

NS_IMETHODIMP
nsTextEditRules::SetFlags(PRUint32 aFlags)
{
  if (mFlags == aFlags) return NS_OK;
  
  
  
  
  

  mFlags = aFlags;
  return NS_OK;
}

NS_IMETHODIMP
nsTextEditRules::BeforeEdit(PRInt32 action, nsIEditor::EDirection aDirection)
{
  if (mLockRulesSniffing) return NS_OK;
  
  nsAutoLockRulesSniffing lockIt(this);
  mDidExplicitlySetInterline = PR_FALSE;
  
  
  nsCOMPtr<nsISelection> selection;
  nsresult res = mEditor->GetSelection(getter_AddRefs(selection));
  if (NS_FAILED(res)) 
    return res;

  selection->GetAnchorNode(getter_AddRefs(mCachedSelectionNode));
  selection->GetAnchorOffset(&mCachedSelectionOffset);

  if (!mActionNesting)
  {
    
    mTheAction = action;
  }
  mActionNesting++;
  return NS_OK;
}


NS_IMETHODIMP
nsTextEditRules::AfterEdit(PRInt32 action, nsIEditor::EDirection aDirection)
{
  if (mLockRulesSniffing) return NS_OK;
  
  nsAutoLockRulesSniffing lockIt(this);
  
  NS_PRECONDITION(mActionNesting>0, "bad action nesting!");
  nsresult res = NS_OK;
  if (!--mActionNesting)
  {
    nsCOMPtr<nsISelection>selection;
    res = mEditor->GetSelection(getter_AddRefs(selection));
    if (NS_FAILED(res)) return res;
  
    res = mEditor->HandleInlineSpellCheck(action, selection,
                                          mCachedSelectionNode, mCachedSelectionOffset,
                                          nsnull, 0, nsnull, 0);
    if (NS_FAILED(res)) 
      return res;

    
    res = CreateBogusNodeIfNeeded(selection);
    if (NS_FAILED(res)) 
      return res;
    
    
    res = CreateTrailingBRIfNeeded();
    if (NS_FAILED(res)) 
      return res;
    
    





    if (action == nsEditor::kOpInsertText
        || action == nsEditor::kOpInsertIMEText) {
      nsCOMPtr<nsISelectionPrivate> privateSelection(do_QueryInterface(selection));
      nsCOMPtr<nsFrameSelection> frameSelection;
      privateSelection->GetFrameSelection(getter_AddRefs(frameSelection));      
      if (frameSelection) {
        frameSelection->UndefineCaretBidiLevel();
      }
    }
  }
  return res;
}


NS_IMETHODIMP 
nsTextEditRules::WillDoAction(nsISelection *aSelection, 
                              nsRulesInfo *aInfo, 
                              PRBool *aCancel, 
                              PRBool *aHandled)
{
  
  if (!aInfo || !aCancel || !aHandled) { return NS_ERROR_NULL_POINTER; }
#if defined(DEBUG_ftang)
  printf("nsTextEditRules::WillDoAction action= %d", aInfo->action);
#endif

  *aCancel = PR_FALSE;
  *aHandled = PR_FALSE;

  
  nsTextRulesInfo *info = static_cast<nsTextRulesInfo*>(aInfo);
    
  switch (info->action)
  {
    case kInsertBreak:
      return WillInsertBreak(aSelection, aCancel, aHandled);
    case kInsertText:
    case kInsertTextIME:
      return WillInsertText(info->action,
                            aSelection, 
                            aCancel,
                            aHandled, 
                            info->inString,
                            info->outString,
                            info->maxLength);
    case kDeleteSelection:
      return WillDeleteSelection(aSelection, info->collapsedAction, aCancel, aHandled);
    case kUndo:
      return WillUndo(aSelection, aCancel, aHandled);
    case kRedo:
      return WillRedo(aSelection, aCancel, aHandled);
    case kSetTextProperty:
      return WillSetTextProperty(aSelection, aCancel, aHandled);
    case kRemoveTextProperty:
      return WillRemoveTextProperty(aSelection, aCancel, aHandled);
    case kOutputText:
      return WillOutputText(aSelection, 
                            info->outputFormat,
                            info->outString,                            
                            aCancel,
                            aHandled);
    case kInsertElement:  
                          
      return WillInsert(aSelection, aCancel);
  }
  return NS_ERROR_FAILURE;
}
  
NS_IMETHODIMP 
nsTextEditRules::DidDoAction(nsISelection *aSelection,
                             nsRulesInfo *aInfo, nsresult aResult)
{
  
  
  nsAutoTxnsConserveSelection dontSpazMySelection(mEditor);

  if (!aSelection || !aInfo) 
    return NS_ERROR_NULL_POINTER;
    
  
  nsTextRulesInfo *info = static_cast<nsTextRulesInfo*>(aInfo);

  switch (info->action)
  {
   case kInsertBreak:
     return DidInsertBreak(aSelection, aResult);
    case kInsertText:
    case kInsertTextIME:
      return DidInsertText(aSelection, aResult);
    case kDeleteSelection:
      return DidDeleteSelection(aSelection, info->collapsedAction, aResult);
    case kUndo:
      return DidUndo(aSelection, aResult);
    case kRedo:
      return DidRedo(aSelection, aResult);
    case kSetTextProperty:
      return DidSetTextProperty(aSelection, aResult);
    case kRemoveTextProperty:
      return DidRemoveTextProperty(aSelection, aResult);
    case kOutputText:
      return DidOutputText(aSelection, aResult);
  }
  
  return NS_OK;
}


NS_IMETHODIMP
nsTextEditRules::DocumentIsEmpty(PRBool *aDocumentIsEmpty)
{
  if (!aDocumentIsEmpty)
    return NS_ERROR_NULL_POINTER;
  
  *aDocumentIsEmpty = (mBogusNode != nsnull);
  return NS_OK;
}






nsresult
nsTextEditRules::WillInsert(nsISelection *aSelection, PRBool *aCancel)
{
  if (!aSelection || !aCancel)
    return NS_ERROR_NULL_POINTER;
  
  CANCEL_OPERATION_IF_READONLY_OR_DISABLED

  
  *aCancel = PR_FALSE;
  
  
  if (mBogusNode)
  {
    mEditor->DeleteNode(mBogusNode);
    mBogusNode = nsnull;
  }

  return NS_OK;
}

nsresult
nsTextEditRules::DidInsert(nsISelection *aSelection, nsresult aResult)
{
  return NS_OK;
}

nsresult
nsTextEditRules::WillInsertBreak(nsISelection *aSelection, PRBool *aCancel, PRBool *aHandled)
{
  if (!aSelection || !aCancel || !aHandled) { return NS_ERROR_NULL_POINTER; }
  CANCEL_OPERATION_IF_READONLY_OR_DISABLED
  *aHandled = PR_FALSE;
  if (mFlags & nsIPlaintextEditor::eEditorSingleLineMask) {
    *aCancel = PR_TRUE;
  }
  else 
  {
    *aCancel = PR_FALSE;

    
    PRBool bCollapsed;
    nsresult res = aSelection->GetIsCollapsed(&bCollapsed);
    if (NS_FAILED(res)) return res;
    if (!bCollapsed)
    {
      res = mEditor->DeleteSelection(nsIEditor::eNone);
      if (NS_FAILED(res)) return res;
    }

    res = WillInsert(aSelection, aCancel);
    if (NS_FAILED(res)) return res;
    
    
    *aCancel = PR_FALSE;
  
  }
  return NS_OK;
}

nsresult
nsTextEditRules::DidInsertBreak(nsISelection *aSelection, nsresult aResult)
{
  
  
  
  if (!nsIPlaintextEditor::eEditorPlaintextMask & mFlags) return NS_OK;

  
  
  
  PRInt32 selOffset;
  nsCOMPtr<nsIDOMNode> selNode;
  nsresult res;
  res = mEditor->GetStartNodeAndOffset(aSelection, address_of(selNode), &selOffset);
  if (NS_FAILED(res)) return res;
  
  if (selOffset == 0) return NS_OK;  
  nsIDOMElement *rootElem = mEditor->GetRoot();

  nsCOMPtr<nsIDOMNode> root = do_QueryInterface(rootElem);
  if (!root) return NS_ERROR_NULL_POINTER;
  if (selNode != root) return NS_OK; 

  nsCOMPtr<nsIDOMNode> temp = mEditor->GetChildAt(selNode, selOffset);
  if (temp) return NS_OK; 

  nsCOMPtr<nsIDOMNode> nearNode = mEditor->GetChildAt(selNode, selOffset-1);
  if (nearNode && nsTextEditUtils::IsBreak(nearNode) && !nsTextEditUtils::IsMozBR(nearNode))
  {
    nsCOMPtr<nsISelectionPrivate>selPrivate(do_QueryInterface(aSelection));
    
    
    
    nsCOMPtr<nsIDOMNode> brNode;
    res = CreateMozBR(selNode, selOffset, address_of(brNode));
    if (NS_FAILED(res)) return res;

    res = nsEditor::GetNodeLocation(brNode, address_of(selNode), &selOffset);
    if (NS_FAILED(res)) return res;
    selPrivate->SetInterlinePosition(PR_TRUE);
    res = aSelection->Collapse(selNode, selOffset);
    if (NS_FAILED(res)) return res;
  }
  return res;
}


nsresult
nsTextEditRules::WillInsertText(PRInt32          aAction,
                                nsISelection *aSelection, 
                                PRBool          *aCancel,
                                PRBool          *aHandled,
                                const nsAString *inString,
                                nsAString *outString,
                                PRInt32          aMaxLength)
{  
  if (!aSelection || !aCancel || !aHandled) { return NS_ERROR_NULL_POINTER; }

  if (inString->IsEmpty() && (aAction != kInsertTextIME))
  {
    
    
    
    
    
    *aCancel = PR_TRUE;
    *aHandled = PR_FALSE;
    return NS_OK;
  }
  
  
  *aCancel = PR_FALSE;
  *aHandled = PR_TRUE;

  
  
  nsresult res = TruncateInsertionIfNeeded(aSelection, inString, outString, aMaxLength);
  if (NS_FAILED(res)) return res;
  
  PRUint32 start = 0;
  PRUint32 end = 0;  

  
  if (mFlags & nsIPlaintextEditor::eEditorPasswordMask)
  {
    res = mEditor->GetTextSelectionOffsets(aSelection, start, end);
    NS_ASSERTION((NS_SUCCEEDED(res)), "getTextSelectionOffsets failed!");
    if (NS_FAILED(res)) return res;
  }

  
  PRBool bCollapsed;
  res = aSelection->GetIsCollapsed(&bCollapsed);
  if (NS_FAILED(res)) return res;
  if (!bCollapsed)
  {
    res = mEditor->DeleteSelection(nsIEditor::eNone);
    if (NS_FAILED(res)) return res;
  }

  res = WillInsert(aSelection, aCancel);
  if (NS_FAILED(res)) return res;
  
  
  *aCancel = PR_FALSE;
  
  
  
  
  if (mFlags & nsIPlaintextEditor::eEditorPasswordMask)
  {
    if (aAction == kInsertTextIME)  {
      res = RemoveIMETextFromPWBuf(start, outString);
      if (NS_FAILED(res)) return res;
    }
  }

  
  
  
  
  
  
  
  
  
  
  if (nsIPlaintextEditor::eEditorSingleLineMask & mFlags)
  {
    nsAutoString tString(*outString);

    switch(mEditor->mNewlineHandling)
    {
    case nsIPlaintextEditor::eNewlinesReplaceWithSpaces:
      
      tString.Trim(CRLF, PR_FALSE, PR_TRUE);
      tString.ReplaceChar(CRLF, ' ');
      break;
    case nsIPlaintextEditor::eNewlinesStrip:
      tString.StripChars(CRLF);
      break;
    case nsIPlaintextEditor::eNewlinesPasteToFirst:
    default:
      {
        PRInt32 firstCRLF = tString.FindCharInSet(CRLF);

        
        PRInt32 offset = 0;
        while (firstCRLF == offset)
        {
          offset++;
          firstCRLF = tString.FindCharInSet(CRLF, offset);
        }
        if (firstCRLF > 0)
          tString.Truncate(firstCRLF);
        if (offset > 0)
          tString.Cut(0, offset);
      }
      break;
    case nsIPlaintextEditor::eNewlinesReplaceWithCommas:
      tString.Trim(CRLF, PR_TRUE, PR_TRUE);
      tString.ReplaceChar(CRLF, ',');
      break;
    case nsIPlaintextEditor::eNewlinesStripSurroundingWhitespace:
      {
        
        
        PRInt32 firstCRLF = tString.FindCharInSet(CRLF);
        while (firstCRLF >= 0)
        {
          PRUint32 wsBegin = firstCRLF, wsEnd = firstCRLF + 1;
          
          while (wsBegin > 0 && NS_IS_SPACE(tString[wsBegin - 1]))
            --wsBegin;
          while (wsEnd < tString.Length() && NS_IS_SPACE(tString[wsEnd]))
            ++wsEnd;
          
          tString.Cut(wsBegin, wsEnd - wsBegin);
          
          firstCRLF = tString.FindCharInSet(CRLF);
        }
      }
      break;
    case nsIPlaintextEditor::eNewlinesPasteIntact:
      
      tString.Trim(CRLF, PR_TRUE, PR_TRUE);
      break;
    }

    outString->Assign(tString);
  }

  if (mFlags & nsIPlaintextEditor::eEditorPasswordMask)
  {
    
    mPasswordText.Insert(*outString, start);

    nsCOMPtr<nsILookAndFeel> lookAndFeel = do_GetService(kLookAndFeelCID);
    if (lookAndFeel->GetEchoPassword()) {
      if (mPasswordText.Length() > outString->Length()) {
        HideLastPWInput();
      }
      mLastStart = start;
      mLastLength = outString->Length();
      if (mTimer)
      {
        mTimer->Cancel();
      }
      else
      {
        mTimer = do_CreateInstance("@mozilla.org/timer;1", &res);
        if (NS_FAILED(res)) return res;
      }
      mTimer->InitWithCallback(this, 600, nsITimer::TYPE_ONE_SHOT);
    } 
    else 
    {
      res = FillBufWithPWChars(outString, outString->Length());
      if (NS_FAILED(res)) return res;
    }
  }

  
  nsCOMPtr<nsIDOMNode> selNode;
  PRInt32 selOffset;
  res = mEditor->GetStartNodeAndOffset(aSelection, address_of(selNode), &selOffset);
  if (NS_FAILED(res)) return res;

  
  if (!mEditor->IsTextNode(selNode) && !mEditor->CanContainTag(selNode, NS_LITERAL_STRING("#text")))
    return NS_ERROR_FAILURE;

  
  nsCOMPtr<nsIDOMDocument>doc;
  res = mEditor->GetDocument(getter_AddRefs(doc));
  if (NS_FAILED(res)) return res;
  if (!doc) return NS_ERROR_NULL_POINTER;
    
  if (aAction == kInsertTextIME) 
  { 
    res = mEditor->InsertTextImpl(*outString, address_of(selNode), &selOffset, doc);
    if (NS_FAILED(res)) return res;
  }
  else 
  {
    
    nsCOMPtr<nsIDOMNode> curNode = selNode;
    PRInt32 curOffset = selOffset;

    
    
    PRBool isPRE;
    res = mEditor->IsPreformatted(selNode, &isPRE);
    if (NS_FAILED(res)) return res;    

    
    nsAutoTxnsConserveSelection dontSpazMySelection(mEditor);
    nsString tString(*outString);
    const PRUnichar *unicodeBuf = tString.get();
    nsCOMPtr<nsIDOMNode> unused;
    PRInt32 pos = 0;

    
    
    
    if (isPRE)
    {
      while (unicodeBuf && (pos != -1) && ((PRUint32)pos < tString.Length()))
      {
        PRInt32 oldPos = pos;
        PRInt32 subStrLen;
        pos = tString.FindChar(nsCRT::LF, oldPos);
        
        if (pos != -1) 
        {
          subStrLen = pos - oldPos;
          
          if (subStrLen == 0)
            subStrLen = 1;
        }
        else
        {
          subStrLen = tString.Length() - oldPos;
          pos = tString.Length();
        }

        nsDependentSubstring subStr(tString, oldPos, subStrLen);
        
        
        if (subStr.EqualsLiteral(LFSTR))
        {
          if (nsIPlaintextEditor::eEditorSingleLineMask & mFlags)
          {
            NS_ASSERTION((mEditor->mNewlineHandling == nsIPlaintextEditor::eNewlinesPasteIntact),
                  "Newline improperly getting into single-line edit field!");
            res = mEditor->InsertTextImpl(subStr, address_of(curNode), &curOffset, doc);
          }
          else
          {
            res = mEditor->CreateBRImpl(address_of(curNode), &curOffset, address_of(unused), nsIEditor::eNone);

            
            
            

            if (NS_SUCCEEDED(res) && curNode && pos == (PRInt32)(tString.Length() - 1))
            {
              nsCOMPtr<nsIDOMNode> nextChild = mEditor->GetChildAt(curNode, curOffset);

              if (!nextChild)
              {
                
                
                
                
                
                
                

                res = CreateMozBR(curNode, curOffset, address_of(unused));
              }
            }
          }
          pos++;
        }
        else
        {
          res = mEditor->InsertTextImpl(subStr, address_of(curNode), &curOffset, doc);
        }
        if (NS_FAILED(res)) return res;
      }
    }
    else
    {
      char specialChars[] = {TAB, nsCRT::LF, 0};
      while (unicodeBuf && (pos != -1) && ((PRUint32)pos < tString.Length()))
      {
        PRInt32 oldPos = pos;
        PRInt32 subStrLen;
        pos = tString.FindCharInSet(specialChars, oldPos);
        
        if (pos != -1) 
        {
          subStrLen = pos - oldPos;
          
          if (subStrLen == 0)
            subStrLen = 1;
        }
        else
        {
          subStrLen = tString.Length() - oldPos;
          pos = tString.Length();
        }

        nsDependentSubstring subStr(tString, oldPos, subStrLen);
        
        
        if (subStr.EqualsLiteral("\t"))
        {
          res = mEditor->InsertTextImpl(NS_LITERAL_STRING("    "), address_of(curNode), &curOffset, doc);
          pos++;
        }
        
        else if (subStr.EqualsLiteral(LFSTR))
        {
          res = mEditor->CreateBRImpl(address_of(curNode), &curOffset, address_of(unused), nsIEditor::eNone);
          pos++;
        }
        else
        {
          res = mEditor->InsertTextImpl(subStr, address_of(curNode), &curOffset, doc);
        }
        if (NS_FAILED(res)) return res;
      }
    }
    outString->Assign(tString);

    if (curNode) 
    {
      aSelection->Collapse(curNode, curOffset);
      
      
      
      PRBool endsWithLF = !tString.IsEmpty() && tString.get()[tString.Length() - 1] == nsCRT::LF;
      nsCOMPtr<nsISelectionPrivate>selPrivate(do_QueryInterface(aSelection));
      selPrivate->SetInterlinePosition(endsWithLF);
    }
  }
  return res;
}

nsresult
nsTextEditRules::DidInsertText(nsISelection *aSelection, 
                               nsresult aResult)
{
  return DidInsert(aSelection, aResult);
}



nsresult
nsTextEditRules::WillSetTextProperty(nsISelection *aSelection, PRBool *aCancel, PRBool *aHandled)
{
  if (!aSelection || !aCancel || !aHandled) 
    { return NS_ERROR_NULL_POINTER; }

  
  if (nsIPlaintextEditor::eEditorPlaintextMask & mFlags) {
    *aCancel = PR_TRUE;
  }
  return NS_OK;
}

nsresult
nsTextEditRules::DidSetTextProperty(nsISelection *aSelection, nsresult aResult)
{
  return NS_OK;
}

nsresult
nsTextEditRules::WillRemoveTextProperty(nsISelection *aSelection, PRBool *aCancel, PRBool *aHandled)
{
  if (!aSelection || !aCancel || !aHandled) 
    { return NS_ERROR_NULL_POINTER; }

  
  if (nsIPlaintextEditor::eEditorPlaintextMask & mFlags) {
    *aCancel = PR_TRUE;
  }
  return NS_OK;
}

nsresult
nsTextEditRules::DidRemoveTextProperty(nsISelection *aSelection, nsresult aResult)
{
  return NS_OK;
}

nsresult
nsTextEditRules::WillDeleteSelection(nsISelection *aSelection, 
                                     nsIEditor::EDirection aCollapsedAction, 
                                     PRBool *aCancel,
                                     PRBool *aHandled)
{
  if (!aSelection || !aCancel || !aHandled) { return NS_ERROR_NULL_POINTER; }
  CANCEL_OPERATION_IF_READONLY_OR_DISABLED

  
  *aCancel = PR_FALSE;
  *aHandled = PR_FALSE;
  
  
  if (mBogusNode) {
    *aCancel = PR_TRUE;
    return NS_OK;
  }

  nsresult res = NS_OK;

  if (mFlags & nsIPlaintextEditor::eEditorPasswordMask)
  {
    res = mEditor->ExtendSelectionForDelete(aSelection, &aCollapsedAction);
    NS_ENSURE_SUCCESS(res, res);

    
    PRUint32 start, end;
    mEditor->GetTextSelectionOffsets(aSelection, start, end);
    NS_ENSURE_SUCCESS(res, res);
    if (end == start)
    { 
      if (nsIEditor::ePrevious==aCollapsedAction && 0<start) { 
        mPasswordText.Cut(start-1, 1);
      }
      else if (nsIEditor::eNext==aCollapsedAction) {      
        mPasswordText.Cut(start, 1);
      }
      
    }
    else {  
      mPasswordText.Cut(start, end-start);
    }
  }
  else
  {
    nsCOMPtr<nsIDOMNode> startNode;
    PRInt32 startOffset;
    res = mEditor->GetStartNodeAndOffset(aSelection, address_of(startNode), &startOffset);
    if (NS_FAILED(res)) return res;
    if (!startNode) return NS_ERROR_FAILURE;
    
    PRBool bCollapsed;
    res = aSelection->GetIsCollapsed(&bCollapsed);
    if (NS_FAILED(res)) return res;
  
    if (!bCollapsed) return NS_OK;

    
    res = CheckBidiLevelForDeletion(aSelection, startNode, startOffset, aCollapsedAction, aCancel);
    if (NS_FAILED(res)) return res;
    if (*aCancel) return NS_OK;

    res = mEditor->ExtendSelectionForDelete(aSelection, &aCollapsedAction);
    NS_ENSURE_SUCCESS(res, res);
  }

  res = mEditor->DeleteSelectionImpl(aCollapsedAction);
  NS_ENSURE_SUCCESS(res, res);

  *aHandled = PR_TRUE;
  return NS_OK;
}

nsresult
nsTextEditRules::DidDeleteSelection(nsISelection *aSelection, 
                                    nsIEditor::EDirection aCollapsedAction, 
                                    nsresult aResult)
{
  nsCOMPtr<nsIDOMNode> startNode;
  PRInt32 startOffset;
  nsresult res = mEditor->GetStartNodeAndOffset(aSelection, address_of(startNode), &startOffset);
  if (NS_FAILED(res)) return res;
  if (!startNode) return NS_ERROR_FAILURE;
  
  
  if (mEditor->IsTextNode(startNode))
  {
    nsCOMPtr<nsIDOMText> textNode = do_QueryInterface(startNode);
    PRUint32 strLength;
    res = textNode->GetLength(&strLength);
    if (NS_FAILED(res)) return res;
    
    
    if (!strLength)
    {
      res = mEditor->DeleteNode(startNode);
      if (NS_FAILED(res)) return res;
    }
  }
  if (!mDidExplicitlySetInterline)
  {
    
    
    nsCOMPtr<nsISelectionPrivate> selPriv = do_QueryInterface(aSelection);
    if (selPriv) res = selPriv->SetInterlinePosition(PR_TRUE);
  }
  return res;
}

nsresult
nsTextEditRules::WillUndo(nsISelection *aSelection, PRBool *aCancel, PRBool *aHandled)
{
  if (!aSelection || !aCancel || !aHandled) { return NS_ERROR_NULL_POINTER; }
  CANCEL_OPERATION_IF_READONLY_OR_DISABLED
  
  *aCancel = PR_FALSE;
  *aHandled = PR_FALSE;
  return NS_OK;
}






nsresult
nsTextEditRules:: DidUndo(nsISelection *aSelection, nsresult aResult)
{
  nsresult res = aResult;  
  if (!aSelection) { return NS_ERROR_NULL_POINTER; }
  if (NS_SUCCEEDED(res)) 
  {
    if (mBogusNode) {
      mBogusNode = nsnull;
    }
    else
    {
      nsIDOMElement *theRoot = mEditor->GetRoot();
      if (!theRoot) return NS_ERROR_FAILURE;
      nsCOMPtr<nsIDOMNode> node = mEditor->GetLeftmostChild(theRoot);
      if (node && mEditor->IsMozEditorBogusNode(node))
        mBogusNode = node;
    }
  }
  return res;
}

nsresult
nsTextEditRules::WillRedo(nsISelection *aSelection, PRBool *aCancel, PRBool *aHandled)
{
  if (!aSelection || !aCancel || !aHandled) { return NS_ERROR_NULL_POINTER; }
  CANCEL_OPERATION_IF_READONLY_OR_DISABLED
  
  *aCancel = PR_FALSE;
  *aHandled = PR_FALSE;
  return NS_OK;
}

nsresult
nsTextEditRules::DidRedo(nsISelection *aSelection, nsresult aResult)
{
  nsresult res = aResult;  
  if (!aSelection) { return NS_ERROR_NULL_POINTER; }
  if (NS_SUCCEEDED(res)) 
  {
    if (mBogusNode) {
      mBogusNode = nsnull;
    }
    else
    {
      nsIDOMElement *theRoot = mEditor->GetRoot();
      if (!theRoot) return NS_ERROR_FAILURE;
      
      nsCOMPtr<nsIDOMNodeList> nodeList;
      res = theRoot->GetElementsByTagName(NS_LITERAL_STRING("br"),
                                          getter_AddRefs(nodeList));
      if (NS_FAILED(res)) return res;
      if (nodeList)
      {
        PRUint32 len;
        nodeList->GetLength(&len);
        
        if (len != 1) return NS_OK;  
        nsCOMPtr<nsIDOMNode> node;
        nodeList->Item(0, getter_AddRefs(node));
        if (!node) return NS_ERROR_NULL_POINTER;
        if (mEditor->IsMozEditorBogusNode(node))
          mBogusNode = node;
      }
    }
  }
  return res;
}

nsresult
nsTextEditRules::WillOutputText(nsISelection *aSelection, 
                                const nsAString  *aOutputFormat,
                                nsAString *aOutString,                                
                                PRBool   *aCancel,
                                PRBool   *aHandled)
{
  
  if (!aOutString || !aOutputFormat || !aCancel || !aHandled) 
    { return NS_ERROR_NULL_POINTER; }

  
  *aCancel = PR_FALSE;
  *aHandled = PR_FALSE;

  nsAutoString outputFormat(*aOutputFormat);
  ToLowerCase(outputFormat);
  if (outputFormat.EqualsLiteral("text/plain"))
  { 
    if (mFlags & nsIPlaintextEditor::eEditorPasswordMask)
    {
      *aOutString = mPasswordText;
      *aHandled = PR_TRUE;
    }
    else if (mBogusNode)
    { 
      aOutString->Truncate();
      *aHandled = PR_TRUE;
    }
  }
  return NS_OK;
}

nsresult
nsTextEditRules::DidOutputText(nsISelection *aSelection, nsresult aResult)
{
  return NS_OK;
}

nsresult
nsTextEditRules::ReplaceNewlines(nsIDOMRange *aRange)
{
  if (!aRange) return NS_ERROR_NULL_POINTER;
  
  
  
  

  nsresult res;
  nsCOMPtr<nsIContentIterator> iter =
       do_CreateInstance("@mozilla.org/content/post-content-iterator;1", &res);
  if (NS_FAILED(res)) return res;

  res = iter->Init(aRange);
  if (NS_FAILED(res)) return res;
  
  nsCOMArray<nsIDOMCharacterData> arrayOfNodes;
  
  
  while (!iter->IsDone())
  {
    nsCOMPtr<nsIDOMNode> node = do_QueryInterface(iter->GetCurrentNode());
    if (!node)
      return NS_ERROR_FAILURE;

    if (mEditor->IsTextNode(node) && mEditor->IsEditable(node))
    {
      PRBool isPRE;
      res = mEditor->IsPreformatted(node, &isPRE);
      if (NS_FAILED(res)) return res;
      if (isPRE)
      {
        nsCOMPtr<nsIDOMCharacterData> data = do_QueryInterface(node);
        arrayOfNodes.AppendObject(data);
      }
    }
    iter->Next();
  }
  
  
  
  
  PRInt32 j, nodeCount = arrayOfNodes.Count();
  for (j = 0; j < nodeCount; j++)
  {
    nsCOMPtr<nsIDOMNode> brNode;
    nsCOMPtr<nsIDOMCharacterData> textNode = arrayOfNodes[0];
    arrayOfNodes.RemoveObjectAt(0);
    
    PRInt32 offset;
    nsAutoString tempString;
    do 
    {
      textNode->GetData(tempString);
      offset = tempString.FindChar(nsCRT::LF);
      if (offset == -1) break; 
      
      
      nsRefPtr<DeleteTextTxn> txn;
      
      
      
      res = mEditor->CreateTxnForDeleteText(textNode, offset, 1,
                                            getter_AddRefs(txn));
      if (NS_FAILED(res))  return res; 
      if (!txn)  return NS_ERROR_OUT_OF_MEMORY;
      res = mEditor->DoTransaction(txn); 
      if (NS_FAILED(res))  return res; 
      
      
      res = mEditor->CreateBR(textNode, offset, address_of(brNode));
      if (NS_FAILED(res)) return res;
    } while (1);  
  }
  return res;
}

nsresult
nsTextEditRules::CreateTrailingBRIfNeeded()
{
  
  if (mFlags & nsIPlaintextEditor::eEditorSingleLineMask)
    return NS_OK;
  nsIDOMNode *body = mEditor->GetRoot();
  if (!body)
    return NS_ERROR_NULL_POINTER;
  nsCOMPtr<nsIDOMNode> lastChild;
  nsresult res = body->GetLastChild(getter_AddRefs(lastChild));
  
  if (NS_FAILED(res)) return res;  
  if (!lastChild) return NS_ERROR_NULL_POINTER;

  if (!nsTextEditUtils::IsBreak(lastChild))
  {
    nsAutoTxnsConserveSelection dontSpazMySelection(mEditor);
    PRUint32 rootLen;
    res = mEditor->GetLengthOfDOMNode(body, rootLen);
    if (NS_FAILED(res)) return res; 
    nsCOMPtr<nsIDOMNode> unused;
    res = CreateMozBR(body, rootLen, address_of(unused));
  }
  return res;
}

nsresult
nsTextEditRules::CreateBogusNodeIfNeeded(nsISelection *aSelection)
{
  if (!aSelection) { return NS_ERROR_NULL_POINTER; }
  if (!mEditor) { return NS_ERROR_NULL_POINTER; }
  if (mBogusNode) return NS_OK;  

  
  nsAutoRules beginRulesSniffing(mEditor, nsEditor::kOpIgnore, nsIEditor::eNone);

  nsIDOMNode* body = mEditor->GetRoot();
  if (!body)
  {
    
    

    return NS_OK;
  }

  
  
  
  PRBool needsBogusContent=PR_TRUE;
  nsCOMPtr<nsIDOMNode> bodyChild;
  nsresult res = body->GetFirstChild(getter_AddRefs(bodyChild));        
  while ((NS_SUCCEEDED(res)) && bodyChild)
  { 
    if (mEditor->IsMozEditorBogusNode(bodyChild) ||
        !mEditor->IsEditable(body) ||
        mEditor->IsEditable(bodyChild))
    {
      needsBogusContent = PR_FALSE;
      break;
    }
    nsCOMPtr<nsIDOMNode>temp;
    bodyChild->GetNextSibling(getter_AddRefs(temp));
    bodyChild = do_QueryInterface(temp);
  }
  if (needsBogusContent)
  {
    
    nsCOMPtr<nsIContent> newContent;
    res = mEditor->CreateHTMLContent(NS_LITERAL_STRING("br"), getter_AddRefs(newContent));
    if (NS_FAILED(res)) return res;
    nsCOMPtr<nsIDOMElement>brElement = do_QueryInterface(newContent);

    
    mBogusNode = brElement;
    if (!mBogusNode) return NS_ERROR_NULL_POINTER;

    
    brElement->SetAttribute( kMOZEditorBogusNodeAttr,
                             kMOZEditorBogusNodeValue );
    
    
    res = mEditor->InsertNode(mBogusNode, body, 0);
    if (NS_FAILED(res)) return res;

    
    aSelection->Collapse(body, 0);
  }
  return res;
}


nsresult
nsTextEditRules::TruncateInsertionIfNeeded(nsISelection *aSelection, 
                                           const nsAString  *aInString,
                                           nsAString  *aOutString,
                                           PRInt32          aMaxLength)
{
  if (!aSelection || !aInString || !aOutString) {return NS_ERROR_NULL_POINTER;}
  
  nsresult res = NS_OK;
  *aOutString = *aInString;
  
  if ((-1 != aMaxLength) && (mFlags & nsIPlaintextEditor::eEditorPlaintextMask)
      && !mEditor->IsIMEComposing() )
  {
    
    
    
    
    
    
    
    
    
    
    
    
    PRInt32 docLength;
    res = mEditor->GetTextLength(&docLength);
    if (NS_FAILED(res)) { return res; }

    PRUint32 start, end;
    res = mEditor->GetTextSelectionOffsets(aSelection, start, end);
    if (NS_FAILED(res)) { return res; }

    PRInt32 oldCompStrLength;
    res = mEditor->GetIMEBufferLength(&oldCompStrLength);
    if (NS_FAILED(res)) { return res; }

    const PRInt32 selectionLength = end - start;
    const PRInt32 resultingDocLength = docLength - selectionLength - oldCompStrLength;
    if (resultingDocLength >= aMaxLength)
    {
      aOutString->Truncate();
    }
    else
    {
      PRInt32 inCount = aOutString->Length();
      if (inCount + resultingDocLength > aMaxLength)
      {
        aOutString->Truncate(aMaxLength - resultingDocLength);
      }
    }
  }
  return res;
}

nsresult
nsTextEditRules::ResetIMETextPWBuf()
{
  mPasswordIMEText.Truncate();
  return NS_OK;
}

nsresult
nsTextEditRules::RemoveIMETextFromPWBuf(PRUint32 &aStart, nsAString *aIMEString)
{
  if (!aIMEString) {
    return NS_ERROR_NULL_POINTER;
  }

  
  if (mPasswordIMEText.IsEmpty()) {
    mPasswordIMEIndex = aStart;
  }
  else {
    
    mPasswordText.Cut(mPasswordIMEIndex, mPasswordIMEText.Length());
    aStart = mPasswordIMEIndex;
  }

  mPasswordIMEText.Assign(*aIMEString);
  return NS_OK;
}

NS_IMETHODIMP nsTextEditRules::Notify(class nsITimer *) {
  return HideLastPWInput();
}

nsresult nsTextEditRules::HideLastPWInput() {
  nsCOMPtr<nsIDOMNode> selNode;
  nsCOMPtr<nsISelection> selection;
  PRInt32 selOffset;
  PRUint32 start, end;
  nsresult res = mEditor->GetSelection(getter_AddRefs(selection));
  if (NS_FAILED(res)) return res;
  res = mEditor->GetTextSelectionOffsets(selection, start, end);
  if (NS_FAILED(res)) return res;
  res = mEditor->GetStartNodeAndOffset(selection, address_of(selNode), &selOffset);
  if (NS_FAILED(res)) return res;
  if (!mEditor->IsTextNode(selNode)) {
    
    nsCOMPtr<nsINode> node = do_QueryInterface(selNode);
    
    if (!node) return NS_OK;
    
    nsNodeIterator iter(node, nsIDOMNodeFilter::SHOW_TEXT, nsnull, PR_TRUE);
    while (!mEditor->IsTextNode(selNode)) {
      if (NS_FAILED(res = iter.NextNode(getter_AddRefs(selNode))) || 
          selNode == nsnull) {
        return NS_SUCCEEDED(res) ? NS_ERROR_NULL_POINTER : res;
      }
    }
  }
  nsCOMPtr<nsIDOMCharacterData> nodeAsText(do_QueryInterface(selNode));
  if (!nodeAsText) return NS_ERROR_FAILURE;
  nsAutoString hiddenText;
  FillBufWithPWChars(&hiddenText, mLastLength);
  nodeAsText->ReplaceData(mLastStart, mLastLength, hiddenText);
  selection->Collapse(selNode, start);
  if (start != end)
    selection->Extend(selNode, end);
  return NS_OK;
}

nsresult
nsTextEditRules::FillBufWithPWChars(nsAString *aOutString, PRInt32 aLength)
{
  if (!aOutString) {return NS_ERROR_NULL_POINTER;}

  
  PRUnichar passwordChar = PRUnichar('*');
  nsCOMPtr<nsILookAndFeel> lookAndFeel = do_GetService(kLookAndFeelCID);
  if (lookAndFeel)
  {
    passwordChar = lookAndFeel->GetPasswordCharacter();
  }

  PRInt32 i;
  aOutString->Truncate();
  for (i=0; i < aLength; i++)
    aOutString->Append(passwordChar);

  return NS_OK;
}





nsresult 
nsTextEditRules::CreateMozBR(nsIDOMNode *inParent, PRInt32 inOffset, nsCOMPtr<nsIDOMNode> *outBRNode)
{
  if (!inParent || !outBRNode) return NS_ERROR_NULL_POINTER;

  nsresult res = mEditor->CreateBR(inParent, inOffset, outBRNode);
  if (NS_FAILED(res)) return res;

  
  nsCOMPtr<nsIDOMElement> brElem = do_QueryInterface(*outBRNode);
  if (brElem)
  {
    res = mEditor->SetAttribute(brElem, NS_LITERAL_STRING("type"), NS_LITERAL_STRING("_moz"));
    if (NS_FAILED(res)) return res;
  }
  return res;
}
