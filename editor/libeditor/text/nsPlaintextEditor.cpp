







































#include "nsPlaintextEditor.h"
#include "nsCaret.h"
#include "nsTextEditUtils.h"
#include "nsTextEditRules.h"
#include "nsIEditActionListener.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMDocument.h"
#include "nsIDocument.h"
#include "nsIDOMEventTarget.h" 
#include "nsIDOMKeyEvent.h"
#include "nsIDOMMouseListener.h"
#include "nsISelection.h"
#include "nsISelectionPrivate.h"
#include "nsISelectionController.h"
#include "nsGUIEvent.h"
#include "nsCRT.h"

#include "nsIEnumerator.h"
#include "nsIContent.h"
#include "nsIContentIterator.h"
#include "nsIDOMRange.h"
#include "nsISupportsArray.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIDocumentEncoder.h"
#include "nsIPresShell.h"
#include "nsISupportsPrimitives.h"
#include "nsReadableUtils.h"


#include "nsEditorUtils.h"  
#include "nsUnicharUtils.h"
#include "nsContentCID.h"
#include "nsInternetCiter.h"
#include "nsEventDispatcher.h"
#include "nsGkAtoms.h"
#include "nsDebug.h"
#include "mozilla/Preferences.h"


#include "nsIClipboard.h"
#include "nsITransferable.h"
#include "nsCopySupport.h"

#include "mozilla/FunctionTimer.h"

using namespace mozilla;

nsPlaintextEditor::nsPlaintextEditor()
: nsEditor()
, mIgnoreSpuriousDragEvent(PR_FALSE)
, mRules(nsnull)
, mWrapToWindow(PR_FALSE)
, mWrapColumn(0)
, mMaxTextLength(-1)
, mInitTriggerCounter(0)
, mNewlineHandling(nsIPlaintextEditor::eNewlinesPasteToFirst)
#ifdef XP_WIN
, mCaretStyle(1)
#else
, mCaretStyle(0)
#endif
{
} 

nsPlaintextEditor::~nsPlaintextEditor()
{
  
  
  RemoveEventListeners();

  if (mRules)
    mRules->DetachEditor();
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsPlaintextEditor)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsPlaintextEditor, nsEditor)
  if (tmp->mRules)
    tmp->mRules->DetachEditor();
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mRules)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsPlaintextEditor, nsEditor)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mRules)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(nsPlaintextEditor, nsEditor)
NS_IMPL_RELEASE_INHERITED(nsPlaintextEditor, nsEditor)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsPlaintextEditor)
  NS_INTERFACE_MAP_ENTRY(nsIPlaintextEditor)
  NS_INTERFACE_MAP_ENTRY(nsIEditorMailSupport)
NS_INTERFACE_MAP_END_INHERITING(nsEditor)


NS_IMETHODIMP nsPlaintextEditor::Init(nsIDOMDocument *aDoc, 
                                      nsIContent *aRoot,
                                      nsISelectionController *aSelCon,
                                      PRUint32 aFlags)
{
  NS_TIME_FUNCTION;

  NS_PRECONDITION(aDoc, "bad arg");
  NS_ENSURE_TRUE(aDoc, NS_ERROR_NULL_POINTER);
  
  nsresult res = NS_OK, rulesRes = NS_OK;
  
  if (1)
  {
    
    nsAutoEditInitRulesTrigger rulesTrigger(this, rulesRes);
  
    
    res = nsEditor::Init(aDoc, aRoot, aSelCon, aFlags);
  }

  
  
  GetDefaultEditorPrefs(mNewlineHandling, mCaretStyle);

  NS_ENSURE_SUCCESS(rulesRes, rulesRes);
  return res;
}

static PRInt32 sNewlineHandlingPref = -1,
               sCaretStylePref = -1;

static int
EditorPrefsChangedCallback(const char *aPrefName, void *)
{
  if (nsCRT::strcmp(aPrefName, "editor.singleLine.pasteNewlines") == 0) {
    sNewlineHandlingPref =
      Preferences::GetInt("editor.singleLine.pasteNewlines",
                          nsIPlaintextEditor::eNewlinesPasteToFirst);
  } else if (nsCRT::strcmp(aPrefName, "layout.selection.caret_style") == 0) {
    sCaretStylePref = Preferences::GetInt("layout.selection.caret_style",
#ifdef XP_WIN
                                                 1);
    if (sCaretStylePref == 0)
      sCaretStylePref = 1;
#else
                                                 0);
#endif
  }
  return 0;
}


void
nsPlaintextEditor::GetDefaultEditorPrefs(PRInt32 &aNewlineHandling,
                                         PRInt32 &aCaretStyle)
{
  if (sNewlineHandlingPref == -1) {
    Preferences::RegisterCallback(EditorPrefsChangedCallback,
                                  "editor.singleLine.pasteNewlines");
    EditorPrefsChangedCallback("editor.singleLine.pasteNewlines", nsnull);
    Preferences::RegisterCallback(EditorPrefsChangedCallback,
                                  "layout.selection.caret_style");
    EditorPrefsChangedCallback("layout.selection.caret_style", nsnull);
  }

  aNewlineHandling = sNewlineHandlingPref;
  aCaretStyle = sCaretStylePref;
}

void 
nsPlaintextEditor::BeginEditorInit()
{
  mInitTriggerCounter++;
}

nsresult 
nsPlaintextEditor::EndEditorInit()
{
  nsresult res = NS_OK;
  NS_PRECONDITION(mInitTriggerCounter > 0, "ended editor init before we began?");
  mInitTriggerCounter--;
  if (mInitTriggerCounter == 0)
  {
    res = InitRules();
    if (NS_SUCCEEDED(res)) {
      
      
      EnableUndo(PR_FALSE);
      EnableUndo(PR_TRUE);
    }
  }
  return res;
}

NS_IMETHODIMP 
nsPlaintextEditor::SetDocumentCharacterSet(const nsACString & characterSet) 
{ 
  nsresult result; 

  result = nsEditor::SetDocumentCharacterSet(characterSet); 

  
  if (NS_SUCCEEDED(result)) { 
    nsCOMPtr<nsIDOMDocument>domdoc; 
    result = GetDocument(getter_AddRefs(domdoc)); 
    if (NS_SUCCEEDED(result) && domdoc) { 
      nsCOMPtr<nsIDOMNodeList>metaList; 
      nsCOMPtr<nsIDOMElement>metaElement; 
      PRBool newMetaCharset = PR_TRUE; 

      
      result = domdoc->GetElementsByTagName(NS_LITERAL_STRING("meta"), getter_AddRefs(metaList)); 
      if (NS_SUCCEEDED(result) && metaList) { 
        PRUint32 listLength = 0; 
        (void) metaList->GetLength(&listLength); 

        nsCOMPtr<nsIDOMNode>metaNode; 
        for (PRUint32 i = 0; i < listLength; i++) { 
          metaList->Item(i, getter_AddRefs(metaNode)); 
          if (!metaNode) continue; 
          metaElement = do_QueryInterface(metaNode); 
          if (!metaElement) continue; 

          nsAutoString currentValue; 
          if (NS_FAILED(metaElement->GetAttribute(NS_LITERAL_STRING("http-equiv"), currentValue))) continue; 

          if (FindInReadable(NS_LITERAL_STRING("content-type"),
                             currentValue,
                             nsCaseInsensitiveStringComparator())) { 
            NS_NAMED_LITERAL_STRING(content, "content");
            if (NS_FAILED(metaElement->GetAttribute(content, currentValue))) continue; 

            NS_NAMED_LITERAL_STRING(charsetEquals, "charset=");
            nsAString::const_iterator originalStart, start, end;
            originalStart = currentValue.BeginReading(start);
            currentValue.EndReading(end);
            if (FindInReadable(charsetEquals, start, end,
                               nsCaseInsensitiveStringComparator())) {

              
              result = nsEditor::SetAttribute(metaElement, content,
                                              Substring(originalStart, start) +
                                              charsetEquals + NS_ConvertASCIItoUTF16(characterSet)); 
              if (NS_SUCCEEDED(result)) 
                newMetaCharset = PR_FALSE; 
              break; 
            } 
          } 
        } 
      } 

      if (newMetaCharset) { 
        nsCOMPtr<nsIDOMNodeList>headList; 
        result = domdoc->GetElementsByTagName(NS_LITERAL_STRING("head"),getter_AddRefs(headList)); 
        if (NS_SUCCEEDED(result) && headList) { 
          nsCOMPtr<nsIDOMNode>headNode; 
          headList->Item(0, getter_AddRefs(headNode)); 
          if (headNode) { 
            nsCOMPtr<nsIDOMNode>resultNode; 
            
            result = CreateNode(NS_LITERAL_STRING("meta"), headNode, 0, getter_AddRefs(resultNode)); 
            NS_ENSURE_SUCCESS(result, NS_ERROR_FAILURE); 

            
            if (resultNode && !characterSet.IsEmpty()) { 
              metaElement = do_QueryInterface(resultNode); 
              if (metaElement) { 
                
                result = metaElement->SetAttribute(NS_LITERAL_STRING("http-equiv"), NS_LITERAL_STRING("Content-Type")); 
                if (NS_SUCCEEDED(result)) { 
                  
                  result = metaElement->SetAttribute(NS_LITERAL_STRING("content"),
                                                     NS_LITERAL_STRING("text/html;charset=") + NS_ConvertASCIItoUTF16(characterSet)); 
                } 
              } 
            } 
          } 
        } 
      } 
    } 
  } 

  return result; 
} 


NS_IMETHODIMP nsPlaintextEditor::InitRules()
{
  
  mRules = new nsTextEditRules();
  return mRules->Init(this);
}


NS_IMETHODIMP
nsPlaintextEditor::GetIsDocumentEditable(PRBool *aIsDocumentEditable)
{
  NS_ENSURE_ARG_POINTER(aIsDocumentEditable);

  nsCOMPtr<nsIDOMDocument> doc;
  GetDocument(getter_AddRefs(doc));
  *aIsDocumentEditable = doc ? IsModifiable() : PR_FALSE;

  return NS_OK;
}

PRBool nsPlaintextEditor::IsModifiable()
{
  return !IsReadonly();
}

nsresult
nsPlaintextEditor::HandleKeyPressEvent(nsIDOMKeyEvent* aKeyEvent)
{
  
  
  
  
  
  

  if (IsReadonly() || IsDisabled()) {
    
    return nsEditor::HandleKeyPressEvent(aKeyEvent);
  }

  nsKeyEvent* nativeKeyEvent = GetNativeKeyEvent(aKeyEvent);
  NS_ENSURE_TRUE(nativeKeyEvent, NS_ERROR_UNEXPECTED);
  NS_ASSERTION(nativeKeyEvent->message == NS_KEY_PRESS,
               "HandleKeyPressEvent gets non-keypress event");

  switch (nativeKeyEvent->keyCode) {
    case nsIDOMKeyEvent::DOM_VK_META:
    case nsIDOMKeyEvent::DOM_VK_SHIFT:
    case nsIDOMKeyEvent::DOM_VK_CONTROL:
    case nsIDOMKeyEvent::DOM_VK_ALT:
    case nsIDOMKeyEvent::DOM_VK_BACK_SPACE:
    case nsIDOMKeyEvent::DOM_VK_DELETE:
      
      return nsEditor::HandleKeyPressEvent(aKeyEvent);
    case nsIDOMKeyEvent::DOM_VK_TAB: {
      if (IsTabbable()) {
        return NS_OK; 
      }

      if (nativeKeyEvent->isShift || nativeKeyEvent->isControl ||
          nativeKeyEvent->isAlt || nativeKeyEvent->isMeta) {
        return NS_OK;
      }

      
      aKeyEvent->PreventDefault();
      return TypedText(NS_LITERAL_STRING("\t"), eTypedText);
    }
    case nsIDOMKeyEvent::DOM_VK_RETURN:
    case nsIDOMKeyEvent::DOM_VK_ENTER:
      if (IsSingleLineEditor() || nativeKeyEvent->isControl ||
          nativeKeyEvent->isAlt || nativeKeyEvent->isMeta) {
        return NS_OK;
      }
      aKeyEvent->PreventDefault();
      return TypedText(EmptyString(), eTypedBreak);
  }

  
  
  if (nativeKeyEvent->charCode == 0 || nativeKeyEvent->isControl ||
      nativeKeyEvent->isAlt || nativeKeyEvent->isMeta) {
    
    return NS_OK;
  }
  aKeyEvent->PreventDefault();
  nsAutoString str(nativeKeyEvent->charCode);
  return TypedText(str, eTypedText);
}







NS_IMETHODIMP nsPlaintextEditor::TypedText(const nsAString& aString,
                                      PRInt32 aAction)
{
  nsAutoPlaceHolderBatch batch(this, nsGkAtoms::TypingTxnName);

  switch (aAction)
  {
    case eTypedText:
      {
        return InsertText(aString);
      }
    case eTypedBreak:
      {
        return InsertLineBreak();
      } 
  } 
  return NS_ERROR_FAILURE; 
}

NS_IMETHODIMP nsPlaintextEditor::CreateBRImpl(nsCOMPtr<nsIDOMNode> *aInOutParent, PRInt32 *aInOutOffset, nsCOMPtr<nsIDOMNode> *outBRNode, EDirection aSelect)
{
  NS_ENSURE_SUCCESS(aInOutParent && *aInOutParent && aInOutOffset && outBRNode, NS_ERROR_NULL_POINTER);
  *outBRNode = nsnull;
  nsresult res;
  
  
  nsCOMPtr<nsIDOMNode> node = *aInOutParent;
  PRInt32 theOffset = *aInOutOffset;
  nsCOMPtr<nsIDOMCharacterData> nodeAsText = do_QueryInterface(node);
  NS_NAMED_LITERAL_STRING(brType, "br");
  nsCOMPtr<nsIDOMNode> brNode;
  if (nodeAsText)  
  {
    nsCOMPtr<nsIDOMNode> tmp;
    PRInt32 offset;
    PRUint32 len;
    nodeAsText->GetLength(&len);
    GetNodeLocation(node, address_of(tmp), &offset);
    NS_ENSURE_TRUE(tmp, NS_ERROR_FAILURE);
    if (!theOffset)
    {
      
    }
    else if (theOffset == (PRInt32)len)
    {
      
      offset++;
    }
    else
    {
      
      res = SplitNode(node, theOffset, getter_AddRefs(tmp));
      NS_ENSURE_SUCCESS(res, res);
      res = GetNodeLocation(node, address_of(tmp), &offset);
      NS_ENSURE_SUCCESS(res, res);
    }
    
    res = CreateNode(brType, tmp, offset, getter_AddRefs(brNode));
    NS_ENSURE_SUCCESS(res, res);
    *aInOutParent = tmp;
    *aInOutOffset = offset+1;
  }
  else
  {
    res = CreateNode(brType, node, theOffset, getter_AddRefs(brNode));
    NS_ENSURE_SUCCESS(res, res);
    (*aInOutOffset)++;
  }

  *outBRNode = brNode;
  if (*outBRNode && (aSelect != eNone))
  {
    nsCOMPtr<nsIDOMNode> parent;
    PRInt32 offset;
    res = GetNodeLocation(*outBRNode, address_of(parent), &offset);
    NS_ENSURE_SUCCESS(res, res);

    nsCOMPtr<nsISelection> selection;
    res = GetSelection(getter_AddRefs(selection));
    NS_ENSURE_SUCCESS(res, res);
    nsCOMPtr<nsISelectionPrivate> selPriv(do_QueryInterface(selection));
    if (aSelect == eNext)
    {
      
      selPriv->SetInterlinePosition(PR_TRUE);
      res = selection->Collapse(parent, offset+1);
    }
    else if (aSelect == ePrevious)
    {
      
      selPriv->SetInterlinePosition(PR_TRUE);
      res = selection->Collapse(parent, offset);
    }
  }
  return NS_OK;
}


NS_IMETHODIMP nsPlaintextEditor::CreateBR(nsIDOMNode *aNode, PRInt32 aOffset, nsCOMPtr<nsIDOMNode> *outBRNode, EDirection aSelect)
{
  nsCOMPtr<nsIDOMNode> parent = aNode;
  PRInt32 offset = aOffset;
  return CreateBRImpl(address_of(parent), &offset, outBRNode, aSelect);
}

NS_IMETHODIMP nsPlaintextEditor::InsertBR(nsCOMPtr<nsIDOMNode> *outBRNode)
{
  NS_ENSURE_TRUE(outBRNode, NS_ERROR_NULL_POINTER);
  *outBRNode = nsnull;

  
  nsAutoRules beginRulesSniffing(this, kOpInsertText, nsIEditor::eNext);

  nsCOMPtr<nsISelection> selection;
  nsresult res = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);
  PRBool bCollapsed;
  res = selection->GetIsCollapsed(&bCollapsed);
  NS_ENSURE_SUCCESS(res, res);
  if (!bCollapsed)
  {
    res = DeleteSelection(nsIEditor::eNone);
    NS_ENSURE_SUCCESS(res, res);
  }
  nsCOMPtr<nsIDOMNode> selNode;
  PRInt32 selOffset;
  res = GetStartNodeAndOffset(selection, getter_AddRefs(selNode), &selOffset);
  NS_ENSURE_SUCCESS(res, res);
  
  res = CreateBR(selNode, selOffset, outBRNode);
  NS_ENSURE_SUCCESS(res, res);
    
  
  res = GetNodeLocation(*outBRNode, address_of(selNode), &selOffset);
  NS_ENSURE_SUCCESS(res, res);
  nsCOMPtr<nsISelectionPrivate> selPriv(do_QueryInterface(selection));
  selPriv->SetInterlinePosition(PR_TRUE);
  return selection->Collapse(selNode, selOffset+1);
}

nsresult
nsPlaintextEditor::GetTextSelectionOffsets(nsISelection *aSelection,
                                           PRUint32 &aOutStartOffset, 
                                           PRUint32 &aOutEndOffset)
{
  NS_ASSERTION(aSelection, "null selection");

  nsresult rv;
  nsCOMPtr<nsIDOMNode> startNode, endNode;
  PRInt32 startNodeOffset, endNodeOffset;
  aSelection->GetAnchorNode(getter_AddRefs(startNode));
  aSelection->GetAnchorOffset(&startNodeOffset);
  aSelection->GetFocusNode(getter_AddRefs(endNode));
  aSelection->GetFocusOffset(&endNodeOffset);

  nsIDOMElement* rootNode = GetRoot();
  NS_ENSURE_TRUE(rootNode, NS_ERROR_NULL_POINTER);

  PRInt32 startOffset = -1;
  PRInt32 endOffset = -1;

  nsCOMPtr<nsIContentIterator> iter =
    do_CreateInstance("@mozilla.org/content/post-content-iterator;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
    
#ifdef NS_DEBUG
  PRInt32 nodeCount = 0; 
#endif
  PRUint32 totalLength = 0;
  nsCOMPtr<nsIContent> rootContent = do_QueryInterface(rootNode);
  iter->Init(rootContent);
  for (; !iter->IsDone() && (startOffset == -1 || endOffset == -1); iter->Next()) {
    nsCOMPtr<nsIDOMNode> currentNode = do_QueryInterface(iter->GetCurrentNode());
    nsCOMPtr<nsIDOMCharacterData> textNode = do_QueryInterface(currentNode);
    if (textNode) {
      
      
      
      PRBool editable = IsEditable(currentNode);
      if (currentNode == startNode) {
        startOffset = totalLength + (editable ? startNodeOffset : 0);
      }
      if (currentNode == endNode) {
        endOffset = totalLength + (editable ? endNodeOffset : 0);
      }
      if (editable) {
        PRUint32 length;
        textNode->GetLength(&length);
        totalLength += length;
      }
    }
#ifdef NS_DEBUG
    
    
    
    if (!SameCOMIdentity(currentNode, rootNode)) {
      ++nodeCount;
    }
#endif
  }

  if (endOffset == -1) {
    NS_ASSERTION(endNode == rootNode, "failed to find the end node");
    NS_ASSERTION(IsPasswordEditor() ||
                 (endNodeOffset == nodeCount-1 || endNodeOffset == 0),
                 "invalid end node offset");
    endOffset = endNodeOffset == 0 ? 0 : totalLength;
  }
  if (startOffset == -1) {
    NS_ASSERTION(startNode == rootNode, "failed to find the start node");
    NS_ASSERTION(startNodeOffset == nodeCount-1 || startNodeOffset == 0,
                 "invalid start node offset");
    startOffset = startNodeOffset == 0 ? 0 : totalLength;
  }

  
  if (startOffset <= endOffset) {
    aOutStartOffset = startOffset;
    aOutEndOffset = endOffset;
  }
  else {
    aOutStartOffset = endOffset;
    aOutEndOffset = startOffset;
  }

  return NS_OK;
}

nsresult
nsPlaintextEditor::ExtendSelectionForDelete(nsISelection *aSelection,
                                            nsIEditor::EDirection *aAction)
{
  nsresult result;

  PRBool bCollapsed;
  result = aSelection->GetIsCollapsed(&bCollapsed);
  NS_ENSURE_SUCCESS(result, result);

  if (*aAction == eNextWord || *aAction == ePreviousWord
      || (*aAction == eNext && bCollapsed)
      || (*aAction == ePrevious && bCollapsed)
      || *aAction == eToBeginningOfLine || *aAction == eToEndOfLine)
  {
    nsCOMPtr<nsISelectionController> selCont;
    GetSelectionController(getter_AddRefs(selCont));
    NS_ENSURE_TRUE(selCont, NS_ERROR_NO_INTERFACE);

    switch (*aAction)
    {
      case eNextWord:
        result = selCont->WordExtendForDelete(PR_TRUE);
        
        
        *aAction = eNone;
        break;
      case ePreviousWord:
        result = selCont->WordExtendForDelete(PR_FALSE);
        *aAction = eNone;
        break;
      case eNext:
        result = selCont->CharacterExtendForDelete();
        
        break;
      case ePrevious: {
        
        
        
        
        nsCOMPtr<nsIDOMNode> node;
        PRInt32 offset;
        result = GetStartNodeAndOffset(aSelection, getter_AddRefs(node), &offset);
        NS_ENSURE_SUCCESS(result, result);
        NS_ENSURE_TRUE(node, NS_ERROR_FAILURE);

        if (IsTextNode(node)) {
          nsCOMPtr<nsIDOMCharacterData> charData = do_QueryInterface(node);
          if (charData) {
            nsAutoString data;
            result = charData->GetData(data);
            NS_ENSURE_SUCCESS(result, result);

            if (offset > 1 &&
                NS_IS_LOW_SURROGATE(data[offset - 1]) &&
                NS_IS_HIGH_SURROGATE(data[offset - 2])) {
              result = selCont->CharacterExtendForBackspace();
            }
          }
        }
        break;
      }
      case eToBeginningOfLine:
        selCont->IntraLineMove(PR_TRUE, PR_FALSE);          
        result = selCont->IntraLineMove(PR_FALSE, PR_TRUE); 
        *aAction = eNone;
        break;
      case eToEndOfLine:
        result = selCont->IntraLineMove(PR_TRUE, PR_TRUE);
        *aAction = eNext;
        break;
      default:       
        result = NS_OK;
        break;
    }
  }
  return result;
}

NS_IMETHODIMP nsPlaintextEditor::DeleteSelection(nsIEditor::EDirection aAction)
{
  if (!mRules) { return NS_ERROR_NOT_INITIALIZED; }

  
  nsCOMPtr<nsIEditRules> kungFuDeathGrip(mRules);

  nsresult result;

  FireTrustedInputEvent trusted(this, aAction != eNone);

  
  nsAutoPlaceHolderBatch batch(this, nsGkAtoms::DeleteTxnName);
  nsAutoRules beginRulesSniffing(this, kOpDeleteSelection, aAction);

  
  nsCOMPtr<nsISelection> selection;
  result = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(result, result);
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);

  
  
  
  
  
  PRBool bCollapsed;
  result  = selection->GetIsCollapsed(&bCollapsed);
  NS_ENSURE_SUCCESS(result, result);
  if (!bCollapsed &&
      (aAction == eNextWord || aAction == ePreviousWord ||
       aAction == eToBeginningOfLine || aAction == eToEndOfLine))
  {
    if (mCaretStyle == 1)
    {
      result = selection->CollapseToStart();
      NS_ENSURE_SUCCESS(result, result);
    }
    else
    { 
      aAction = eNone;
    }
  }

  nsTextRulesInfo ruleInfo(nsTextEditRules::kDeleteSelection);
  ruleInfo.collapsedAction = aAction;
  PRBool cancel, handled;
  result = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  NS_ENSURE_SUCCESS(result, result);
  if (!cancel && !handled)
  {
    result = DeleteSelectionImpl(aAction);
  }
  if (!cancel)
  {
    
    result = mRules->DidDoAction(selection, &ruleInfo, result);
  }

  return result;
}

NS_IMETHODIMP nsPlaintextEditor::InsertText(const nsAString &aStringToInsert)
{
  if (!mRules) { return NS_ERROR_NOT_INITIALIZED; }

  
  nsCOMPtr<nsIEditRules> kungFuDeathGrip(mRules);

  PRInt32 theAction = nsTextEditRules::kInsertText;
  PRInt32 opID = kOpInsertText;
  if (mInIMEMode) 
  {
    theAction = nsTextEditRules::kInsertTextIME;
    opID = kOpInsertIMEText;
  }
  nsAutoPlaceHolderBatch batch(this, nsnull); 
  nsAutoRules beginRulesSniffing(this, opID, nsIEditor::eNext);

  
  nsCOMPtr<nsISelection> selection;
  nsresult result = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(result, result);
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);
  nsAutoString resultString;
  
  
  
  nsTextRulesInfo ruleInfo(theAction);
  ruleInfo.inString = &aStringToInsert;
  ruleInfo.outString = &resultString;
  ruleInfo.maxLength = mMaxTextLength;

  PRBool cancel, handled;
  result = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  NS_ENSURE_SUCCESS(result, result);
  if (!cancel && !handled)
  {
    
  }
  if (!cancel)
  {
    
    result = mRules->DidDoAction(selection, &ruleInfo, result);
  }
  return result;
}

NS_IMETHODIMP nsPlaintextEditor::InsertLineBreak()
{
  if (!mRules) { return NS_ERROR_NOT_INITIALIZED; }

  
  nsCOMPtr<nsIEditRules> kungFuDeathGrip(mRules);

  nsAutoEditBatch beginBatching(this);
  nsAutoRules beginRulesSniffing(this, kOpInsertBreak, nsIEditor::eNext);

  
  nsCOMPtr<nsISelection> selection;
  nsresult res;
  res = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);

  
  
  nsCOMPtr<nsIPresShell> shell = GetPresShell();
  NS_ENSURE_TRUE(shell, NS_ERROR_NOT_INITIALIZED);
  shell->MaybeInvalidateCaretPosition();

  nsTextRulesInfo ruleInfo(nsTextEditRules::kInsertBreak);
  ruleInfo.maxLength = mMaxTextLength;
  PRBool cancel, handled;
  res = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  NS_ENSURE_SUCCESS(res, res);
  if (!cancel && !handled)
  {
    
    nsCOMPtr<nsIDOMNode> selNode;
    PRInt32 selOffset;
    res = GetStartNodeAndOffset(selection, getter_AddRefs(selNode), &selOffset);
    NS_ENSURE_SUCCESS(res, res);

    
    if (!IsTextNode(selNode) && !CanContainTag(selNode, NS_LITERAL_STRING("#text")))
      return NS_ERROR_FAILURE;

    
    nsCOMPtr<nsIDOMDocument> doc;
    res = GetDocument(getter_AddRefs(doc));
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_TRUE(doc, NS_ERROR_NULL_POINTER);

    
    nsAutoTxnsConserveSelection dontSpazMySelection(this);

    
    res = InsertTextImpl(NS_LITERAL_STRING("\n"), address_of(selNode),
                         &selOffset, doc);
    if (!selNode) res = NS_ERROR_NULL_POINTER; 
    if (NS_SUCCEEDED(res))
    {
      
      res = selection->Collapse(selNode, selOffset);

      if (NS_SUCCEEDED(res))
      {
        
        nsCOMPtr<nsIDOMNode> endNode;
        PRInt32 endOffset;
        res = GetEndNodeAndOffset(selection, getter_AddRefs(endNode), &endOffset);

        if (NS_SUCCEEDED(res) && endNode == selNode && endOffset == selOffset)
        {
          
          
          
          
          nsCOMPtr<nsISelectionPrivate> selPriv(do_QueryInterface(selection));
          selPriv->SetInterlinePosition(PR_TRUE);
        }
      }
    }
  }
  if (!cancel)
  {
    
    res = mRules->DidDoAction(selection, &ruleInfo, res);
  }

  return res;
}

nsresult
nsPlaintextEditor::BeginIMEComposition()
{
  NS_ENSURE_TRUE(!mInIMEMode, NS_OK);

  if (IsPasswordEditor()) {
    NS_ENSURE_TRUE(mRules, NS_ERROR_NULL_POINTER);
    
    nsCOMPtr<nsIEditRules> kungFuDeathGrip(mRules);

    nsTextEditRules *textEditRules =
      static_cast<nsTextEditRules*>(mRules.get());
    textEditRules->ResetIMETextPWBuf();
  }

  return nsEditor::BeginIMEComposition();
}

nsresult
nsPlaintextEditor::UpdateIMEComposition(const nsAString& aCompositionString,
                                        nsIPrivateTextRangeList* aTextRangeList)
{
  if (!aTextRangeList && !aCompositionString.IsEmpty()) {
    NS_ERROR("aTextRangeList is null but the composition string is not null");
    return NS_ERROR_NULL_POINTER;
  }

  nsCOMPtr<nsIPresShell> ps = GetPresShell();
  NS_ENSURE_TRUE(ps, NS_ERROR_NOT_INITIALIZED);

  nsCOMPtr<nsISelection> selection;
  nsresult rv = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<nsCaret> caretP = ps->GetCaret();

  
  
  
  

  
  
  
  

  
  
  
  

  if (!aCompositionString.IsEmpty() || (mIMETextNode && aTextRangeList)) {
    mIMETextRangeList = aTextRangeList;

    nsAutoPlaceHolderBatch batch(this, nsGkAtoms::IMETxnName);

    SetIsIMEComposing(); 

    rv = InsertText(aCompositionString);

    mIMEBufferLength = aCompositionString.Length();

    if (caretP) {
      caretP->SetCaretDOMSelection(selection);
    }

    
    if (aCompositionString.IsEmpty()) {
      mIMETextNode = nsnull;
    }
  }

  return rv;
}

NS_IMETHODIMP
nsPlaintextEditor::GetDocumentIsEmpty(PRBool *aDocumentIsEmpty)
{
  NS_ENSURE_TRUE(aDocumentIsEmpty, NS_ERROR_NULL_POINTER);
  
  NS_ENSURE_TRUE(mRules, NS_ERROR_NOT_INITIALIZED);

  
  nsCOMPtr<nsIEditRules> kungFuDeathGrip(mRules);
  
  return mRules->DocumentIsEmpty(aDocumentIsEmpty);
}

NS_IMETHODIMP
nsPlaintextEditor::GetTextLength(PRInt32 *aCount)
{
  NS_ASSERTION(aCount, "null pointer");

  
  *aCount = 0;
  
  
  PRBool docEmpty;
  nsresult rv = GetDocumentIsEmpty(&docEmpty);
  NS_ENSURE_SUCCESS(rv, rv);
  if (docEmpty)
    return NS_OK;

  nsIDOMElement* rootNode = GetRoot();
  NS_ENSURE_TRUE(rootNode, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsIContentIterator> iter =
    do_CreateInstance("@mozilla.org/content/post-content-iterator;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 totalLength = 0;
  nsCOMPtr<nsIContent> rootContent = do_QueryInterface(rootNode);
  iter->Init(rootContent);
  for (; !iter->IsDone(); iter->Next()) {
    nsCOMPtr<nsIDOMNode> currentNode = do_QueryInterface(iter->GetCurrentNode());
    nsCOMPtr<nsIDOMCharacterData> textNode = do_QueryInterface(currentNode);
    if (textNode && IsEditable(currentNode)) {
      PRUint32 length;
      textNode->GetLength(&length);
      totalLength += length;
    }
  }

  *aCount = totalLength;
  return NS_OK;
}

NS_IMETHODIMP
nsPlaintextEditor::SetMaxTextLength(PRInt32 aMaxTextLength)
{
  mMaxTextLength = aMaxTextLength;
  return NS_OK;
}

NS_IMETHODIMP
nsPlaintextEditor::GetMaxTextLength(PRInt32* aMaxTextLength)
{
  NS_ENSURE_TRUE(aMaxTextLength, NS_ERROR_INVALID_POINTER);
  *aMaxTextLength = mMaxTextLength;
  return NS_OK;
}




NS_IMETHODIMP 
nsPlaintextEditor::GetWrapWidth(PRInt32 *aWrapColumn)
{
  NS_ENSURE_TRUE( aWrapColumn, NS_ERROR_NULL_POINTER);

  *aWrapColumn = mWrapColumn;
  return NS_OK;
}





static void CutStyle(const char* stylename, nsString& styleValue)
{
  
  PRInt32 styleStart = styleValue.Find(stylename, PR_TRUE);
  if (styleStart >= 0)
  {
    PRInt32 styleEnd = styleValue.Find(";", PR_FALSE, styleStart);
    if (styleEnd > styleStart)
      styleValue.Cut(styleStart, styleEnd - styleStart + 1);
    else
      styleValue.Cut(styleStart, styleValue.Length() - styleStart);
  }
}




NS_IMETHODIMP 
nsPlaintextEditor::SetWrapWidth(PRInt32 aWrapColumn)
{
  SetWrapColumn(aWrapColumn);

  
  
  if (!IsPlaintextEditor())
    return NS_OK;

  
  
  nsIDOMElement *rootElement = GetRoot();
  NS_ENSURE_TRUE(rootElement, NS_ERROR_NULL_POINTER);

  
  NS_NAMED_LITERAL_STRING(styleName, "style");
  nsAutoString styleValue;
  nsresult res = rootElement->GetAttribute(styleName, styleValue);
  NS_ENSURE_SUCCESS(res, res);

  
  CutStyle("white-space", styleValue);
  CutStyle("width", styleValue);
  CutStyle("font-family", styleValue);

  
  
  if (!styleValue.IsEmpty())
  {
    styleValue.Trim("; \t", PR_FALSE, PR_TRUE);
    styleValue.AppendLiteral("; ");
  }

  
  
  
  if (IsWrapHackEnabled() && aWrapColumn >= 0)
    styleValue.AppendLiteral("font-family: -moz-fixed; ");

  
  
  
  
  if (IsMailEditor())
  {
    mWrapToWindow =
      Preferences::GetBool("mail.compose.wrap_to_window_width", mWrapToWindow);
  }

  
  if (aWrapColumn > 0 && !mWrapToWindow)        
  {
    styleValue.AppendLiteral("white-space: pre-wrap; width: ");
    styleValue.AppendInt(aWrapColumn);
    styleValue.AppendLiteral("ch;");
  }
  else if (mWrapToWindow || aWrapColumn == 0)
    styleValue.AppendLiteral("white-space: pre-wrap;");
  else
    styleValue.AppendLiteral("white-space: pre;");

  return rootElement->SetAttribute(styleName, styleValue);
}

NS_IMETHODIMP 
nsPlaintextEditor::SetWrapColumn(PRInt32 aWrapColumn)
{
  mWrapColumn = aWrapColumn;
  return NS_OK;
}




NS_IMETHODIMP 
nsPlaintextEditor::GetNewlineHandling(PRInt32 *aNewlineHandling)
{
  NS_ENSURE_ARG_POINTER(aNewlineHandling);

  *aNewlineHandling = mNewlineHandling;
  return NS_OK;
}




NS_IMETHODIMP 
nsPlaintextEditor::SetNewlineHandling(PRInt32 aNewlineHandling)
{
  mNewlineHandling = aNewlineHandling;
  
  return NS_OK;
}

NS_IMETHODIMP 
nsPlaintextEditor::Undo(PRUint32 aCount)
{
  
  nsCOMPtr<nsIEditRules> kungFuDeathGrip(mRules);

  FireTrustedInputEvent trusted(this);

  nsAutoUpdateViewBatch beginViewBatching(this);

  ForceCompositionEnd();

  nsAutoRules beginRulesSniffing(this, kOpUndo, nsIEditor::eNone);

  nsTextRulesInfo ruleInfo(nsTextEditRules::kUndo);
  nsCOMPtr<nsISelection> selection;
  GetSelection(getter_AddRefs(selection));
  PRBool cancel, handled;
  nsresult result = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  
  if (!cancel && NS_SUCCEEDED(result))
  {
    result = nsEditor::Undo(aCount);
    result = mRules->DidDoAction(selection, &ruleInfo, result);
  } 
   
  return result;
}

NS_IMETHODIMP 
nsPlaintextEditor::Redo(PRUint32 aCount)
{
  
  nsCOMPtr<nsIEditRules> kungFuDeathGrip(mRules);

  FireTrustedInputEvent trusted(this);

  nsAutoUpdateViewBatch beginViewBatching(this);

  ForceCompositionEnd();

  nsAutoRules beginRulesSniffing(this, kOpRedo, nsIEditor::eNone);

  nsTextRulesInfo ruleInfo(nsTextEditRules::kRedo);
  nsCOMPtr<nsISelection> selection;
  GetSelection(getter_AddRefs(selection));
  PRBool cancel, handled;
  nsresult result = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  
  if (!cancel && NS_SUCCEEDED(result))
  {
    result = nsEditor::Redo(aCount);
    result = mRules->DidDoAction(selection, &ruleInfo, result);
  } 
   
  return result;
}

PRBool
nsPlaintextEditor::CanCutOrCopy()
{
  nsCOMPtr<nsISelection> selection;
  if (NS_FAILED(GetSelection(getter_AddRefs(selection))))
    return PR_FALSE;

  PRBool isCollapsed;
  selection->GetIsCollapsed(&isCollapsed);
  return !isCollapsed;
}

PRBool
nsPlaintextEditor::FireClipboardEvent(PRInt32 aType)
{
  if (aType == NS_PASTE)
    ForceCompositionEnd();

  nsCOMPtr<nsIPresShell> presShell = GetPresShell();
  NS_ENSURE_TRUE(presShell, PR_FALSE);

  nsCOMPtr<nsISelection> selection;
  if (NS_FAILED(GetSelection(getter_AddRefs(selection))))
    return PR_FALSE;

  if (!nsCopySupport::FireClipboardEvent(aType, presShell, selection))
    return PR_FALSE;

  
  
  return !mDidPreDestroy;
}

NS_IMETHODIMP nsPlaintextEditor::Cut()
{
  FireTrustedInputEvent trusted(this);

  if (FireClipboardEvent(NS_CUT))
    return DeleteSelection(eNone);
  return NS_OK;
}

NS_IMETHODIMP nsPlaintextEditor::CanCut(PRBool *aCanCut)
{
  NS_ENSURE_ARG_POINTER(aCanCut);
  *aCanCut = IsModifiable() && CanCutOrCopy();
  return NS_OK;
}

NS_IMETHODIMP nsPlaintextEditor::Copy()
{
  FireClipboardEvent(NS_COPY);
  return NS_OK;
}

NS_IMETHODIMP nsPlaintextEditor::CanCopy(PRBool *aCanCopy)
{
  NS_ENSURE_ARG_POINTER(aCanCopy);
  *aCanCopy = CanCutOrCopy();
  return NS_OK;
}


NS_IMETHODIMP
nsPlaintextEditor::GetAndInitDocEncoder(const nsAString& aFormatType,
                                        PRUint32 aFlags,
                                        const nsACString& aCharset,
                                        nsIDocumentEncoder** encoder)
{
  nsresult rv = NS_OK;

  nsCAutoString formatType(NS_DOC_ENCODER_CONTRACTID_BASE);
  formatType.AppendWithConversion(aFormatType);
  nsCOMPtr<nsIDocumentEncoder> docEncoder (do_CreateInstance(formatType.get(), &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDocument> doc = do_QueryReferent(mDocWeak);
  nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(doc);
  NS_ASSERTION(domDoc, "Need a document");

  rv = docEncoder->Init(domDoc, aFormatType, aFlags);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!aCharset.IsEmpty()
    && !(aCharset.EqualsLiteral("null")))
    docEncoder->SetCharset(aCharset);

  PRInt32 wc;
  (void) GetWrapWidth(&wc);
  if (wc >= 0)
    (void) docEncoder->SetWrapColumn(wc);

  
  
  
  if (aFlags & nsIDocumentEncoder::OutputSelectionOnly)
  {
    nsCOMPtr<nsISelection> selection;
    rv = GetSelection(getter_AddRefs(selection));
    if (NS_SUCCEEDED(rv) && selection)
      rv = docEncoder->SetSelection(selection);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  
  else
  {
    nsIDOMElement *rootElement = GetRoot();
    NS_ENSURE_TRUE(rootElement, NS_ERROR_FAILURE);
    if (!nsTextEditUtils::IsBody(rootElement))
    {
      rv = docEncoder->SetContainerNode(rootElement);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  NS_ADDREF(*encoder = docEncoder);
  return rv;
}


NS_IMETHODIMP 
nsPlaintextEditor::OutputToString(const nsAString& aFormatType,
                                  PRUint32 aFlags,
                                  nsAString& aOutputString)
{
  
  nsCOMPtr<nsIEditRules> kungFuDeathGrip(mRules);

  nsString resultString;
  nsTextRulesInfo ruleInfo(nsTextEditRules::kOutputText);
  ruleInfo.outString = &resultString;
  
  nsAutoString str(aFormatType);
  ruleInfo.outputFormat = &str;
  PRBool cancel, handled;
  nsresult rv = mRules->WillDoAction(nsnull, &ruleInfo, &cancel, &handled);
  if (cancel || NS_FAILED(rv)) { return rv; }
  if (handled)
  { 
    aOutputString.Assign(*(ruleInfo.outString));
    return rv;
  }

  nsCAutoString charsetStr;
  rv = GetDocumentCharacterSet(charsetStr);
  if(NS_FAILED(rv) || charsetStr.IsEmpty())
    charsetStr.AssignLiteral("ISO-8859-1");

  nsCOMPtr<nsIDocumentEncoder> encoder;
  rv = GetAndInitDocEncoder(aFormatType, aFlags, charsetStr, getter_AddRefs(encoder));
  NS_ENSURE_SUCCESS(rv, rv);
  return encoder->EncodeToString(aOutputString);
}

NS_IMETHODIMP
nsPlaintextEditor::OutputToStream(nsIOutputStream* aOutputStream,
                             const nsAString& aFormatType,
                             const nsACString& aCharset,
                             PRUint32 aFlags)
{
  nsresult rv;

  
  
  
  if (aFormatType.EqualsLiteral("text/plain"))
  {
    PRBool docEmpty;
    rv = GetDocumentIsEmpty(&docEmpty);
    NS_ENSURE_SUCCESS(rv, rv);
    
    if (docEmpty)
       return NS_OK;    
  }

  nsCOMPtr<nsIDocumentEncoder> encoder;
  rv = GetAndInitDocEncoder(aFormatType, aFlags, aCharset,
                            getter_AddRefs(encoder));

  NS_ENSURE_SUCCESS(rv, rv);

  return encoder->EncodeToStream(aOutputStream);
}

NS_IMETHODIMP
nsPlaintextEditor::InsertTextWithQuotations(const nsAString &aStringToInsert)
{
  return InsertText(aStringToInsert);
}

NS_IMETHODIMP
nsPlaintextEditor::PasteAsQuotation(PRInt32 aSelectionType)
{
  
  nsresult rv;
  nsCOMPtr<nsIClipboard> clipboard(do_GetService("@mozilla.org/widget/clipboard;1", &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsITransferable> trans = do_CreateInstance("@mozilla.org/widget/transferable;1", &rv);
  if (NS_SUCCEEDED(rv) && trans)
  {
    
    trans->AddDataFlavor(kUnicodeMime);

    
    clipboard->GetData(trans, aSelectionType);

    
    
    
    nsCOMPtr<nsISupports> genericDataObj;
    PRUint32 len;
    char* flav = nsnull;
    rv = trans->GetAnyTransferData(&flav, getter_AddRefs(genericDataObj),
                                   &len);
    if (NS_FAILED(rv) || !flav)
    {
#ifdef DEBUG_akkana
      printf("PasteAsPlaintextQuotation: GetAnyTransferData failed, %d\n", rv);
#endif
      return rv;
    }
#ifdef DEBUG_clipboard
    printf("Got flavor [%s]\n", flav);
#endif
    if (0 == nsCRT::strcmp(flav, kUnicodeMime))
    {
      nsCOMPtr<nsISupportsString> textDataObj ( do_QueryInterface(genericDataObj) );
      if (textDataObj && len > 0)
      {
        nsAutoString stuffToPaste;
        textDataObj->GetData ( stuffToPaste );
        nsAutoEditBatch beginBatching(this);
        rv = InsertAsQuotation(stuffToPaste, 0);
      }
    }
    NS_Free(flav);
  }

  return rv;
}

NS_IMETHODIMP
nsPlaintextEditor::InsertAsQuotation(const nsAString& aQuotedText,
                                     nsIDOMNode **aNodeInserted)
{
  
  nsCOMPtr<nsIEditRules> kungFuDeathGrip(mRules);

  
  nsString quotedStuff;
  nsresult rv = nsInternetCiter::GetCiteString(aQuotedText, quotedStuff);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (!aQuotedText.IsEmpty() && (aQuotedText.Last() != PRUnichar('\n')))
    quotedStuff.Append(PRUnichar('\n'));

  
  nsCOMPtr<nsISelection> selection;
  rv = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);

  nsAutoEditBatch beginBatching(this);
  nsAutoRules beginRulesSniffing(this, kOpInsertText, nsIEditor::eNext);

  
  nsTextRulesInfo ruleInfo(nsTextEditRules::kInsertElement);
  PRBool cancel, handled;
  rv = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  NS_ENSURE_SUCCESS(rv, rv);
  if (cancel) return NS_OK; 
  if (!handled)
  {
    rv = InsertText(quotedStuff);

    
    if (aNodeInserted && NS_SUCCEEDED(rv))
    {
      *aNodeInserted = 0;
      
    }
  }
  return rv;
}

NS_IMETHODIMP
nsPlaintextEditor::PasteAsCitedQuotation(const nsAString& aCitation,
                                         PRInt32 aSelectionType)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsPlaintextEditor::InsertAsCitedQuotation(const nsAString& aQuotedText,
                                          const nsAString& aCitation,
                                          PRBool aInsertHTML,
                                          nsIDOMNode **aNodeInserted)
{
  return InsertAsQuotation(aQuotedText, aNodeInserted);
}

nsresult
nsPlaintextEditor::SharedOutputString(PRUint32 aFlags,
                                      PRBool* aIsCollapsed,
                                      nsAString& aResult)
{
  nsCOMPtr<nsISelection> selection;
  nsresult rv = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(selection, NS_ERROR_NOT_INITIALIZED);

  rv = selection->GetIsCollapsed(aIsCollapsed);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!*aIsCollapsed)
    aFlags |= nsIDocumentEncoder::OutputSelectionOnly;
  

  return OutputToString(NS_LITERAL_STRING("text/plain"), aFlags, aResult);
}

NS_IMETHODIMP
nsPlaintextEditor::Rewrap(PRBool aRespectNewlines)
{
  PRInt32 wrapCol;
  nsresult rv = GetWrapWidth(&wrapCol);
  NS_ENSURE_SUCCESS(rv, NS_OK);

  
  if (wrapCol <= 0)
    wrapCol = 72;

#ifdef DEBUG_akkana
  printf("nsPlaintextEditor::Rewrap to %ld columns\n", (long)wrapCol);
#endif

  nsAutoString current;
  PRBool isCollapsed;
  rv = SharedOutputString(nsIDocumentEncoder::OutputFormatted
                          | nsIDocumentEncoder::OutputLFLineBreak,
                          &isCollapsed, current);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString wrapped;
  PRUint32 firstLineOffset = 0;   
  rv = nsInternetCiter::Rewrap(current, wrapCol, firstLineOffset, aRespectNewlines,
                     wrapped);
  NS_ENSURE_SUCCESS(rv, rv);

  if (isCollapsed)    
    SelectAll();

  return InsertTextWithQuotations(wrapped);
}

NS_IMETHODIMP    
nsPlaintextEditor::StripCites()
{
#ifdef DEBUG_akkana
  printf("nsPlaintextEditor::StripCites()\n");
#endif

  nsAutoString current;
  PRBool isCollapsed;
  nsresult rv = SharedOutputString(nsIDocumentEncoder::OutputFormatted,
                                   &isCollapsed, current);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString stripped;
  rv = nsInternetCiter::StripCites(current, stripped);
  NS_ENSURE_SUCCESS(rv, rv);

  if (isCollapsed)    
  {
    rv = SelectAll();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return InsertText(stripped);
}

NS_IMETHODIMP
nsPlaintextEditor::GetEmbeddedObjects(nsISupportsArray** aNodeList)
{
  *aNodeList = 0;
  return NS_OK;
}




NS_IMETHODIMP
nsPlaintextEditor::StartOperation(PRInt32 opID, nsIEditor::EDirection aDirection)
{
  
  nsCOMPtr<nsIEditRules> kungFuDeathGrip(mRules);

  nsEditor::StartOperation(opID, aDirection);  
  if (mRules) return mRules->BeforeEdit(mAction, mDirection);
  return NS_OK;
}




NS_IMETHODIMP
nsPlaintextEditor::EndOperation()
{
  
  nsCOMPtr<nsIEditRules> kungFuDeathGrip(mRules);

  
  nsresult res = NS_OK;
  if (mRules) res = mRules->AfterEdit(mAction, mDirection);
  nsEditor::EndOperation();  
  return res;
}  


NS_IMETHODIMP 
nsPlaintextEditor::SelectEntireDocument(nsISelection *aSelection)
{
  if (!aSelection || !mRules) { return NS_ERROR_NULL_POINTER; }

  
  nsCOMPtr<nsIEditRules> kungFuDeathGrip(mRules);

  
  PRBool bDocIsEmpty;
  if (NS_SUCCEEDED(mRules->DocumentIsEmpty(&bDocIsEmpty)) && bDocIsEmpty)
  {
    
    nsIDOMElement *rootElement = GetRoot();
    NS_ENSURE_TRUE(rootElement, NS_ERROR_FAILURE);

    
    return aSelection->Collapse(rootElement, 0);
  }

  nsresult rv = nsEditor::SelectEntireDocument(aSelection);
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRInt32 selOffset;
  nsCOMPtr<nsIDOMNode> selNode;
  rv = GetEndNodeAndOffset(aSelection, getter_AddRefs(selNode), &selOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMNode> childNode = GetChildAt(selNode, selOffset - 1);

  if (childNode && nsTextEditUtils::IsMozBR(childNode)) {
    nsCOMPtr<nsIDOMNode> parentNode;
    PRInt32 parentOffset;
    rv = GetNodeLocation(childNode, address_of(parentNode), &parentOffset);
    NS_ENSURE_SUCCESS(rv, rv);

    return aSelection->Extend(parentNode, parentOffset);
  }

  return NS_OK;
}

already_AddRefed<nsIDOMEventTarget>
nsPlaintextEditor::GetDOMEventTarget()
{
  NS_IF_ADDREF(mEventTarget);
  return mEventTarget.get();
}


nsresult
nsPlaintextEditor::SetAttributeOrEquivalent(nsIDOMElement * aElement,
                                            const nsAString & aAttribute,
                                            const nsAString & aValue,
                                            PRBool aSuppressTransaction)
{
  return nsEditor::SetAttribute(aElement, aAttribute, aValue);
}

nsresult
nsPlaintextEditor::RemoveAttributeOrEquivalent(nsIDOMElement * aElement,
                                               const nsAString & aAttribute,
                                               PRBool aSuppressTransaction)
{
  return nsEditor::RemoveAttribute(aElement, aAttribute);
}
