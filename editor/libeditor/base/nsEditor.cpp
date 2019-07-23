







































#include "pratom.h"
#include "nsIDOMDocument.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMHTMLElement.h"
#include "nsIDOMNSHTMLElement.h"
#include "nsIDOMEventTarget.h"
#include "nsPIDOMEventTarget.h"
#include "nsIEventListenerManager.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsUnicharUtils.h"
#include "nsReadableUtils.h"

#include "nsIDOMText.h"
#include "nsIDOMElement.h"
#include "nsIDOMAttr.h"
#include "nsIDOMNode.h"
#include "nsIDOMDocumentFragment.h"
#include "nsIDOMNamedNodeMap.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMRange.h"
#include "nsIDOMEventListener.h"
#include "nsIDOMEventGroup.h"
#include "nsIDOMMouseListener.h"
#include "nsIDOMFocusListener.h"
#include "nsIDOMTextListener.h"
#include "nsIDOMCompositionListener.h"
#include "nsIDOMHTMLBRElement.h"
#include "nsIDocument.h"
#include "nsITransactionManager.h"
#include "nsIAbsorbingTransaction.h"
#include "nsIPresShell.h"
#include "nsIViewManager.h"
#include "nsISelection.h"
#include "nsISelectionPrivate.h"
#include "nsISelectionController.h"
#include "nsIEnumerator.h"
#include "nsIAtom.h"
#include "nsCaret.h"
#include "nsIWidget.h"
#include "nsIPlaintextEditor.h"
#include "nsGUIEvent.h"  

#include "nsIFrame.h"  

#include "nsICSSStyleSheet.h"

#include "nsIContent.h"
#include "nsServiceManagerUtils.h"


#include "EditAggregateTxn.h"
#include "PlaceholderTxn.h"
#include "ChangeAttributeTxn.h"
#include "CreateElementTxn.h"
#include "InsertElementTxn.h"
#include "DeleteElementTxn.h"
#include "InsertTextTxn.h"
#include "DeleteTextTxn.h"
#include "DeleteRangeTxn.h"
#include "SplitElementTxn.h"
#include "JoinElementTxn.h"
#include "nsStyleSheetTxns.h"
#include "IMETextTxn.h"
#include "nsString.h"

#include "nsEditor.h"
#include "nsEditorUtils.h"
#include "nsISelectionDisplay.h"
#include "nsIInlineSpellChecker.h"
#include "nsINameSpaceManager.h"
#include "nsIHTMLDocument.h"
#include "nsIParserService.h"

#define NS_ERROR_EDITOR_NO_SELECTION NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_EDITOR,1)
#define NS_ERROR_EDITOR_NO_TEXTNODE  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_EDITOR,2)

#ifdef NS_DEBUG_EDITOR
static PRBool gNoisy = PR_FALSE;
#endif



extern nsIParserService *sParserService;







nsEditor::nsEditor()
:  mModCount(0)
,  mPresShellWeak(nsnull)
,  mViewManager(nsnull)
,  mUpdateCount(0)
,  mSpellcheckCheckboxState(eTriUnset)
,  mPlaceHolderTxn(nsnull)
,  mPlaceHolderName(nsnull)
,  mPlaceHolderBatch(0)
,  mSelState(nsnull)
,  mSavedSel()
,  mRangeUpdater()
,  mAction(nsnull)
,  mDirection(eNone)
,  mIMETextNode(nsnull)
,  mIMETextOffset(0)
,  mIMEBufferLength(0)
,  mInIMEMode(PR_FALSE)
,  mIsIMEComposing(PR_FALSE)
,  mShouldTxnSetSelection(PR_TRUE)
,  mDidPreDestroy(PR_FALSE)
,  mDocDirtyState(-1)
,  mDocWeak(nsnull)
,  mPhonetic(nsnull)
{
  
}

nsEditor::~nsEditor()
{
  mTxnMgr = nsnull;

  delete mPhonetic;
 
  NS_IF_RELEASE(mViewManager);
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsEditor)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsEditor)
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mRootElement)
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mInlineSpellChecker)
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mTxnMgr)
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mIMETextRangeList)
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mIMETextNode)
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMARRAY(mActionListeners)
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMARRAY(mEditorObservers)
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMARRAY(mDocStateListeners)
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mEventTarget)
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mKeyListenerP)
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mMouseListenerP)
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mTextListenerP)
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mCompositionListenerP)
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mDragListenerP)
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mFocusListenerP)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsEditor)
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mRootElement)
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mInlineSpellChecker)
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mTxnMgr)
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mIMETextRangeList)
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mIMETextNode)
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMARRAY(mActionListeners)
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMARRAY(mEditorObservers)
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMARRAY(mDocStateListeners)
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mEventTarget)
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mKeyListenerP)
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mMouseListenerP)
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mTextListenerP)
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mCompositionListenerP)
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mDragListenerP)
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mFocusListenerP)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsEditor)
 NS_INTERFACE_MAP_ENTRY(nsIMutationObserver)
 NS_INTERFACE_MAP_ENTRY(nsIPhonetic)
 NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
 NS_INTERFACE_MAP_ENTRY(nsIEditorIMESupport)
 NS_INTERFACE_MAP_ENTRY(nsIEditor)
 NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIEditor)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF_AMBIGUOUS(nsEditor, nsIEditor)
NS_IMPL_CYCLE_COLLECTING_RELEASE_AMBIGUOUS(nsEditor, nsIEditor)

#ifdef XP_MAC
#pragma mark -
#pragma mark  nsIEditorMethods 
#pragma mark -
#endif


NS_IMETHODIMP
nsEditor::Init(nsIDOMDocument *aDoc, nsIPresShell* aPresShell, nsIContent *aRoot, nsISelectionController *aSelCon, PRUint32 aFlags)
{
  NS_PRECONDITION(nsnull!=aDoc && nsnull!=aPresShell, "bad arg");
  if ((nsnull==aDoc) || (nsnull==aPresShell))
    return NS_ERROR_NULL_POINTER;

  mFlags = aFlags;
  mDocWeak = do_GetWeakReference(aDoc);  
  mPresShellWeak = do_GetWeakReference(aPresShell);   
  mSelConWeak = do_GetWeakReference(aSelCon);   
  nsCOMPtr<nsIPresShell> ps = do_QueryReferent(mPresShellWeak);
  if (!ps) return NS_ERROR_NOT_INITIALIZED;
  
  
  if (aRoot)
    mRootElement = do_QueryInterface(aRoot);

  nsCOMPtr<nsINode> document = do_QueryInterface(aDoc);
  document->AddMutationObserver(this);

  
  
  
  

  mViewManager = ps->GetViewManager();
  if (!mViewManager) {return NS_ERROR_NULL_POINTER;}
  NS_ADDREF(mViewManager);

  mUpdateCount=0;

  
  mIMETextNode = nsnull;
  mIMETextOffset = 0;
  mIMEBufferLength = 0;
  
  
  aSelCon->SetCaretReadOnly(PR_FALSE);
  aSelCon->SetDisplaySelection(nsISelectionController::SELECTION_ON);
  
  aSelCon->SetSelectionFlags(nsISelectionDisplay::DISPLAY_ALL);

#if 1
  
  

  


  nsCOMPtr<nsIPresShell> shell = do_QueryReferent(mSelConWeak);
  if (shell)
    BeginningOfDocument();
#endif

  NS_POSTCONDITION(mDocWeak && mPresShellWeak, "bad state");

  return NS_OK;
}


NS_IMETHODIMP
nsEditor::PostCreate()
{
  
  nsresult rv = SyncRealTimeSpell();
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = CreateEventListeners();
  if (NS_FAILED(rv))
  {
    RemoveEventListeners();

    return rv;
  }

  rv = InstallEventListeners();
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  ResetModificationCount();
  
  
  NotifyDocumentListeners(eDocumentCreated);
  NotifyDocumentListeners(eDocumentStateChanged);
  
  return NS_OK;
}

nsresult
nsEditor::InstallEventListeners()
{
  NS_ENSURE_TRUE(mDocWeak && mPresShellWeak && mKeyListenerP &&
                 mMouseListenerP && mFocusListenerP && mTextListenerP &&
                 mCompositionListenerP && mDragListenerP,
                 NS_ERROR_NOT_INITIALIZED);

  nsCOMPtr<nsPIDOMEventTarget> piTarget = GetPIDOMEventTarget();

  if (!piTarget) {
    RemoveEventListeners();
    return NS_ERROR_FAILURE;
  }

  nsresult rv = NS_OK;

  
  nsCOMPtr<nsIDOMEventGroup> sysGroup;
  piTarget->GetSystemEventGroup(getter_AddRefs(sysGroup));
  nsIEventListenerManager* elmP = piTarget->GetListenerManager(PR_TRUE);

  if (sysGroup && elmP)
  {
    rv = elmP->AddEventListenerByType(mKeyListenerP,
                                      NS_LITERAL_STRING("keypress"),
                                      NS_EVENT_FLAG_BUBBLE |
                                      NS_PRIV_EVENT_UNTRUSTED_PERMITTED,
                                      sysGroup);
    NS_ASSERTION(NS_SUCCEEDED(rv),
                 "failed to register key listener in system group");
  }

  rv |= piTarget->AddEventListenerByIID(mMouseListenerP,
                                        NS_GET_IID(nsIDOMMouseListener));

  if (elmP) {
    
    
    rv |= elmP->AddEventListenerByIID(mFocusListenerP,
                                      NS_GET_IID(nsIDOMFocusListener),
                                      NS_EVENT_FLAG_CAPTURE);
  }

  rv |= piTarget->AddEventListenerByIID(mTextListenerP,
                                        NS_GET_IID(nsIDOMTextListener));

  rv |= piTarget->AddEventListenerByIID(mCompositionListenerP,
                                        NS_GET_IID(nsIDOMCompositionListener));

  nsCOMPtr<nsIDOMEventTarget> target(do_QueryInterface(piTarget));
  if (target) {
    
    rv |= target->AddEventListener(NS_LITERAL_STRING("draggesture"), mDragListenerP, PR_FALSE);
    rv |= target->AddEventListener(NS_LITERAL_STRING("dragenter"), mDragListenerP, PR_FALSE);
    rv |= target->AddEventListener(NS_LITERAL_STRING("dragover"), mDragListenerP, PR_FALSE);
    rv |= target->AddEventListener(NS_LITERAL_STRING("dragleave"), mDragListenerP, PR_FALSE);
    rv |= target->AddEventListener(NS_LITERAL_STRING("drop"), mDragListenerP, PR_FALSE);
  }

  if (NS_FAILED(rv))
  {
    NS_ERROR("failed to register some event listeners");

    RemoveEventListeners();
  }

  return rv;
}

void
nsEditor::RemoveEventListeners()
{
  if (!mDocWeak)
  {
    return;
  }

  nsCOMPtr<nsPIDOMEventTarget> piTarget = GetPIDOMEventTarget();

  if (piTarget)
  {
    
    nsCOMPtr<nsIEventListenerManager> elmP =
      piTarget->GetListenerManager(PR_TRUE);
    if (mKeyListenerP)
    {
      nsCOMPtr<nsIDOMEventGroup> sysGroup;
      piTarget->GetSystemEventGroup(getter_AddRefs(sysGroup));
      if (sysGroup && elmP)
      {
        elmP->RemoveEventListenerByType(mKeyListenerP,
                                        NS_LITERAL_STRING("keypress"),
                                        NS_EVENT_FLAG_BUBBLE |
                                        NS_PRIV_EVENT_UNTRUSTED_PERMITTED,
                                        sysGroup);
      }
    }

    if (mMouseListenerP)
    {
      piTarget->RemoveEventListenerByIID(mMouseListenerP,
                                         NS_GET_IID(nsIDOMMouseListener));
    }

    if (mFocusListenerP && elmP)
    {
      elmP->RemoveEventListenerByIID(mFocusListenerP,
                                     NS_GET_IID(nsIDOMFocusListener),
                                     NS_EVENT_FLAG_CAPTURE);
    }

    if (mTextListenerP)
    {
      piTarget->RemoveEventListenerByIID(mTextListenerP,
                                         NS_GET_IID(nsIDOMTextListener));
    }

    if (mCompositionListenerP)
    {
      piTarget->RemoveEventListenerByIID(mCompositionListenerP,
                                         NS_GET_IID(nsIDOMCompositionListener));
    }

    if (mDragListenerP)
    {
      nsCOMPtr<nsIDOMEventTarget> target(do_QueryInterface(piTarget));
      if (target) {
        target->RemoveEventListener(NS_LITERAL_STRING("draggesture"), mDragListenerP, PR_FALSE);
        target->RemoveEventListener(NS_LITERAL_STRING("dragenter"), mDragListenerP, PR_FALSE);
        target->RemoveEventListener(NS_LITERAL_STRING("dragover"), mDragListenerP, PR_FALSE);
        target->RemoveEventListener(NS_LITERAL_STRING("dragleave"), mDragListenerP, PR_FALSE);
        target->RemoveEventListener(NS_LITERAL_STRING("drop"), mDragListenerP, PR_FALSE);
      }
    }
  }
}

PRBool
nsEditor::GetDesiredSpellCheckState()
{
  
  if (mSpellcheckCheckboxState != eTriUnset) {
    return (mSpellcheckCheckboxState == eTriTrue);
  }

  
  nsresult rv;
  nsCOMPtr<nsIPrefBranch> prefBranch =
    do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  PRInt32 spellcheckLevel = 1;
  if (NS_SUCCEEDED(rv) && prefBranch) {
    prefBranch->GetIntPref("layout.spellcheckDefault", &spellcheckLevel);
  }

  if (spellcheckLevel == 0) {
    return PR_FALSE;                    
  }

  
  
  PRUint32 flags;
  if (NS_SUCCEEDED(GetFlags(&flags)) &&
      flags & (nsIPlaintextEditor::eEditorPasswordMask |
               nsIPlaintextEditor::eEditorReadonlyMask |
               nsIPlaintextEditor::eEditorDisabledMask)) {
    return PR_FALSE;
  }

  nsCOMPtr<nsIPresShell> presShell;
  rv = GetPresShell(getter_AddRefs(presShell));
  if (NS_SUCCEEDED(rv)) {
    nsPresContext* context = presShell->GetPresContext();
    if (context && !context->IsDynamic()) {
        return PR_FALSE;
    }
  }

  
  nsCOMPtr<nsIContent> content = do_QueryInterface(GetRoot());
  if (!content) {
    return PR_FALSE;
  }

  if (content->IsRootOfNativeAnonymousSubtree()) {
    content = content->GetParent();
  }

  nsCOMPtr<nsIDOMNSHTMLElement> element = do_QueryInterface(content);
  if (!element) {
    return PR_FALSE;
  }

  PRBool enable;
  element->GetSpellcheck(&enable);

  return enable;
}

NS_IMETHODIMP
nsEditor::PreDestroy(PRBool aDestroyingFrames)
{
  if (mDidPreDestroy)
    return NS_OK;

  
  
  
  
  
  
  if (mInlineSpellChecker)
    mInlineSpellChecker->Cleanup(aDestroyingFrames);

  
  NotifyDocumentListeners(eDocumentToBeDestroyed);

  nsCOMPtr<nsINode> document = do_QueryReferent(mDocWeak);
  if (document)
    document->RemoveMutationObserver(this);

  
  RemoveEventListeners();
  mActionListeners.Clear();
  mEditorObservers.Clear();
  mDocStateListeners.Clear();
  mInlineSpellChecker = nsnull;

  mDidPreDestroy = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
nsEditor::GetFlags(PRUint32 *aFlags)
{
  *aFlags = mFlags;
  return NS_OK;
}

NS_IMETHODIMP
nsEditor::SetFlags(PRUint32 aFlags)
{
  mFlags = aFlags;

  
  SyncRealTimeSpell();
  return NS_OK;
}

NS_IMETHODIMP
nsEditor::GetIsDocumentEditable(PRBool *aIsDocumentEditable)
{
  NS_ENSURE_ARG_POINTER(aIsDocumentEditable);
  nsCOMPtr<nsIDOMDocument> doc;
  GetDocument(getter_AddRefs(doc));
  *aIsDocumentEditable = doc ? PR_TRUE : PR_FALSE;

  return NS_OK;
}

NS_IMETHODIMP 
nsEditor::GetDocument(nsIDOMDocument **aDoc)
{
  if (!aDoc)
    return NS_ERROR_NULL_POINTER;
  *aDoc = nsnull; 
  NS_PRECONDITION(mDocWeak, "bad state, mDocWeak weak pointer not initialized");
  nsCOMPtr<nsIDOMDocument> doc = do_QueryReferent(mDocWeak);
  if (!doc) return NS_ERROR_NOT_INITIALIZED;
  NS_ADDREF(*aDoc = doc);
  return NS_OK;
}


nsresult 
nsEditor::GetPresShell(nsIPresShell **aPS)
{
  if (!aPS)
    return NS_ERROR_NULL_POINTER;
  *aPS = nsnull; 
  NS_PRECONDITION(mPresShellWeak, "bad state, null mPresShellWeak");
  nsCOMPtr<nsIPresShell> ps = do_QueryReferent(mPresShellWeak);
  if (!ps) return NS_ERROR_NOT_INITIALIZED;
  NS_ADDREF(*aPS = ps);
  return NS_OK;
}



NS_IMETHODIMP
nsEditor::GetContentsMIMEType(char * *aContentsMIMEType)
{
  NS_ENSURE_ARG_POINTER(aContentsMIMEType);
  *aContentsMIMEType = ToNewCString(mContentMIMEType);
  return NS_OK;
}

NS_IMETHODIMP
nsEditor::SetContentsMIMEType(const char * aContentsMIMEType)
{
  mContentMIMEType.Assign(aContentsMIMEType ? aContentsMIMEType : "");
  return NS_OK;
}

NS_IMETHODIMP
nsEditor::GetSelectionController(nsISelectionController **aSel)
{
  if (!aSel)
    return NS_ERROR_NULL_POINTER;
  *aSel = nsnull; 
  NS_PRECONDITION(mSelConWeak, "bad state, null mSelConWeak");
  nsCOMPtr<nsISelectionController> selCon = do_QueryReferent(mSelConWeak);
  if (!selCon) return NS_ERROR_NOT_INITIALIZED;
  NS_ADDREF(*aSel = selCon);
  return NS_OK;
}


NS_IMETHODIMP
nsEditor::DeleteSelection(EDirection aAction)
{
  return DeleteSelectionImpl(aAction);
}



NS_IMETHODIMP
nsEditor::GetSelection(nsISelection **aSelection)
{
  if (!aSelection)
    return NS_ERROR_NULL_POINTER;
  *aSelection = nsnull;
  nsCOMPtr<nsISelectionController> selcon = do_QueryReferent(mSelConWeak);
  if (!selcon) return NS_ERROR_NOT_INITIALIZED;
  return selcon->GetSelection(nsISelectionController::SELECTION_NORMAL, aSelection);  
}

NS_IMETHODIMP 
nsEditor::DoTransaction(nsITransaction *aTxn)
{
#ifdef NS_DEBUG_EDITOR
  if (gNoisy) { printf("Editor::DoTransaction ----------\n"); }
#endif

  nsresult result = NS_OK;
  
  if (mPlaceHolderBatch && !mPlaceHolderTxn)
  {
    
    
    
    
    nsRefPtr<EditTxn> editTxn = new PlaceholderTxn();
    if (!editTxn) { return NS_ERROR_OUT_OF_MEMORY; }

    
    nsCOMPtr<nsIAbsorbingTransaction> plcTxn;
    editTxn->QueryInterface(NS_GET_IID(nsIAbsorbingTransaction), getter_AddRefs(plcTxn));
    
    

    
    mPlaceHolderTxn = do_GetWeakReference(plcTxn);
    plcTxn->Init(mPlaceHolderName, mSelState, this);
    mSelState = nsnull;  

    
    nsCOMPtr<nsITransaction> theTxn = do_QueryInterface(plcTxn);
    DoTransaction(theTxn);  

    if (mTxnMgr)
    {
      nsCOMPtr<nsITransaction> topTxn;
      result = mTxnMgr->PeekUndoStack(getter_AddRefs(topTxn));
      if (NS_FAILED(result)) return result;
      if (topTxn)
      {
        plcTxn = do_QueryInterface(topTxn);
        if (plcTxn)
        {
          
          
          
          
          mPlaceHolderTxn = do_GetWeakReference(plcTxn);
        }
      }
    }
  }

  if (aTxn)
  {  
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    nsCOMPtr<nsISelection>selection;
    result = GetSelection(getter_AddRefs(selection));
    if (NS_FAILED(result)) { return result; }
    if (!selection) { return NS_ERROR_NULL_POINTER; }
    nsCOMPtr<nsISelectionPrivate>selPrivate(do_QueryInterface(selection));

    selPrivate->StartBatchChanges();

    if (mTxnMgr) {
      result = mTxnMgr->DoTransaction(aTxn);
    }
    else {
      result = aTxn->DoTransaction();
    }
    if (NS_SUCCEEDED(result)) {
      result = DoAfterDoTransaction(aTxn);
    }

    selPrivate->EndBatchChanges(); 
  }
 
  NS_POSTCONDITION((NS_SUCCEEDED(result)), "transaction did not execute properly");

  return result;
}


NS_IMETHODIMP
nsEditor::EnableUndo(PRBool aEnable)
{
  nsresult result=NS_OK;

  if (PR_TRUE==aEnable)
  {
    if (!mTxnMgr)
    {
      mTxnMgr = do_CreateInstance(NS_TRANSACTIONMANAGER_CONTRACTID, &result);
      if (NS_FAILED(result) || !mTxnMgr) {
        return NS_ERROR_NOT_AVAILABLE;
      }
    }
    mTxnMgr->SetMaxTransactionCount(-1);
  }
  else
  { 
    if (mTxnMgr)
    {
      mTxnMgr->Clear();
      mTxnMgr->SetMaxTransactionCount(0);
    }
  }

  return NS_OK;
}


NS_IMETHODIMP
nsEditor::GetTransactionManager(nsITransactionManager* *aTxnManager)
{
  NS_ENSURE_ARG_POINTER(aTxnManager);
  
  *aTxnManager = NULL;
  if (!mTxnMgr)
    return NS_ERROR_FAILURE;

  NS_ADDREF(*aTxnManager = mTxnMgr);
  return NS_OK;
}

NS_IMETHODIMP
nsEditor::SetTransactionManager(nsITransactionManager *aTxnManager)
{
  NS_ENSURE_TRUE(aTxnManager, NS_ERROR_FAILURE);

  mTxnMgr = aTxnManager;
  return NS_OK;
}

NS_IMETHODIMP 
nsEditor::Undo(PRUint32 aCount)
{
#ifdef NS_DEBUG_EDITOR
  if (gNoisy) { printf("Editor::Undo ----------\n"); }
#endif

  nsresult result = NS_OK;
  ForceCompositionEnd();

  PRBool hasTxnMgr, hasTransaction = PR_FALSE;
  CanUndo(&hasTxnMgr, &hasTransaction);
  if (!hasTransaction)
    return result;

  nsAutoRules beginRulesSniffing(this, kOpUndo, nsIEditor::eNone);

  if ((nsITransactionManager *)nsnull!=mTxnMgr.get())
  {
    PRUint32 i=0;
    for ( ; i<aCount; i++)
    {
      result = mTxnMgr->UndoTransaction();

      if (NS_SUCCEEDED(result))
        result = DoAfterUndoTransaction();
          
      if (NS_FAILED(result))
        break;
    }
  }

  NotifyEditorObservers();  
  return result;
}


NS_IMETHODIMP nsEditor::CanUndo(PRBool *aIsEnabled, PRBool *aCanUndo)
{
  if (!aIsEnabled || !aCanUndo)
     return NS_ERROR_NULL_POINTER;
  *aIsEnabled = ((PRBool)((nsITransactionManager *)0!=mTxnMgr.get()));
  if (*aIsEnabled)
  {
    PRInt32 numTxns=0;
    mTxnMgr->GetNumberOfUndoItems(&numTxns);
    *aCanUndo = ((PRBool)(0!=numTxns));
  }
  else {
    *aCanUndo = PR_FALSE;
  }
  return NS_OK;
}


NS_IMETHODIMP 
nsEditor::Redo(PRUint32 aCount)
{
#ifdef NS_DEBUG_EDITOR
  if (gNoisy) { printf("Editor::Redo ----------\n"); }
#endif

  nsresult result = NS_OK;

  PRBool hasTxnMgr, hasTransaction = PR_FALSE;
  CanRedo(&hasTxnMgr, &hasTransaction);
  if (!hasTransaction)
    return result;

  nsAutoRules beginRulesSniffing(this, kOpRedo, nsIEditor::eNone);

  if ((nsITransactionManager *)nsnull!=mTxnMgr.get())
  {
    PRUint32 i=0;
    for ( ; i<aCount; i++)
    {
      result = mTxnMgr->RedoTransaction();

      if (NS_SUCCEEDED(result))
        result = DoAfterRedoTransaction();

      if (NS_FAILED(result))
        break;
    }
  }

  NotifyEditorObservers();  
  return result;
}


NS_IMETHODIMP nsEditor::CanRedo(PRBool *aIsEnabled, PRBool *aCanRedo)
{
  if (!aIsEnabled || !aCanRedo)
     return NS_ERROR_NULL_POINTER;

  *aIsEnabled = ((PRBool)((nsITransactionManager *)0!=mTxnMgr.get()));
  if (*aIsEnabled)
  {
    PRInt32 numTxns=0;
    mTxnMgr->GetNumberOfRedoItems(&numTxns);
    *aCanRedo = ((PRBool)(0!=numTxns));
  }
  else {
    *aCanRedo = PR_FALSE;
  }
  return NS_OK;
}


NS_IMETHODIMP 
nsEditor::BeginTransaction()
{
  BeginUpdateViewBatch();

  if ((nsITransactionManager *)nsnull!=mTxnMgr.get())
  {
    mTxnMgr->BeginBatch();
  }

  return NS_OK;
}

NS_IMETHODIMP 
nsEditor::EndTransaction()
{
  if ((nsITransactionManager *)nsnull!=mTxnMgr.get())
  {
    mTxnMgr->EndBatch();
  }

  EndUpdateViewBatch();

  return NS_OK;
}









NS_IMETHODIMP 
nsEditor::BeginPlaceHolderTransaction(nsIAtom *aName)
{
  NS_PRECONDITION(mPlaceHolderBatch >= 0, "negative placeholder batch count!");
  if (!mPlaceHolderBatch)
  {
    
    BeginUpdateViewBatch();
    mPlaceHolderTxn = nsnull;
    mPlaceHolderName = aName;
    nsCOMPtr<nsISelection> selection;
    nsresult res = GetSelection(getter_AddRefs(selection));
    if (NS_FAILED(res)) return res;
    mSelState = new nsSelectionState();
    if (!mSelState)
      return NS_ERROR_OUT_OF_MEMORY;

    mSelState->SaveSelection(selection);
  }
  mPlaceHolderBatch++;

  return NS_OK;
}

NS_IMETHODIMP 
nsEditor::EndPlaceHolderTransaction()
{
  NS_PRECONDITION(mPlaceHolderBatch > 0, "zero or negative placeholder batch count when ending batch!");
  if (mPlaceHolderBatch == 1)
  {
    nsCOMPtr<nsISelection>selection;
    GetSelection(getter_AddRefs(selection));

    nsCOMPtr<nsISelectionPrivate>selPrivate(do_QueryInterface(selection));

   
   
   
   
   
   
    if (selPrivate) {
      selPrivate->SetCanCacheFrameOffset(PR_TRUE);
    }
    
    
    EndUpdateViewBatch();
    

    
    
    ScrollSelectionIntoView(PR_FALSE);

    
    if (selPrivate) {
      selPrivate->SetCanCacheFrameOffset(PR_FALSE);
    }

    if (mSelState)
    {
      
      
      delete mSelState;
      mSelState = nsnull;
    }
    if (mPlaceHolderTxn)  
    {
      nsCOMPtr<nsIAbsorbingTransaction> plcTxn = do_QueryReferent(mPlaceHolderTxn);
      if (plcTxn) 
      {
        plcTxn->EndPlaceHolderBatch();
      }
      else  
      {
        
        
        
      }
      
      if (!mInIMEMode) NotifyEditorObservers();
    }
  }
  mPlaceHolderBatch--;
  
  return NS_OK;
}

NS_IMETHODIMP
nsEditor::ShouldTxnSetSelection(PRBool *aResult)
{
  if (!aResult) return NS_ERROR_NULL_POINTER;
  *aResult = mShouldTxnSetSelection;
  return NS_OK;
}

NS_IMETHODIMP  
nsEditor::SetShouldTxnSetSelection(PRBool aShould)
{
  mShouldTxnSetSelection = aShould;
  return NS_OK;
}

NS_IMETHODIMP
nsEditor::GetDocumentIsEmpty(PRBool *aDocumentIsEmpty)
{
  *aDocumentIsEmpty = PR_TRUE;

  nsIDOMElement *rootElement = GetRoot(); 
  if (!rootElement)
    return NS_ERROR_NULL_POINTER; 

  PRBool hasChildNodes;
  nsresult res = rootElement->HasChildNodes(&hasChildNodes);

  *aDocumentIsEmpty = !hasChildNodes;

  return res;
}



NS_IMETHODIMP nsEditor::SelectAll()
{
  if (!mDocWeak || !mPresShellWeak) { return NS_ERROR_NOT_INITIALIZED; }
  ForceCompositionEnd();

  nsCOMPtr<nsISelectionController> selCon = do_QueryReferent(mSelConWeak);
  if (!selCon) return NS_ERROR_NOT_INITIALIZED;
  nsCOMPtr<nsISelection> selection;
  nsresult result = selCon->GetSelection(nsISelectionController::SELECTION_NORMAL, getter_AddRefs(selection));
  if (NS_SUCCEEDED(result) && selection)
  {
    result = SelectEntireDocument(selection);
  }
  return result;
}

NS_IMETHODIMP nsEditor::BeginningOfDocument()
{
  if (!mDocWeak || !mPresShellWeak) { return NS_ERROR_NOT_INITIALIZED; }

  
  nsCOMPtr<nsISelection> selection;
  nsresult result = GetSelection(getter_AddRefs(selection));
  if (NS_FAILED(result))
    return result;
  if (!selection)
    return NS_ERROR_NOT_INITIALIZED;
    
  
  nsIDOMElement *rootElement = GetRoot(); 
  if (!rootElement)
    return NS_ERROR_NULL_POINTER; 
  
  
  nsCOMPtr<nsIDOMNode> firstNode;
  result = GetFirstEditableNode(rootElement, address_of(firstNode));
  if (firstNode)
  {
    
    if (IsTextNode(firstNode)) 
    {
      result = selection->Collapse(firstNode, 0);
    }
    else
    { 
      nsCOMPtr<nsIDOMNode> parentNode;
      result = firstNode->GetParentNode(getter_AddRefs(parentNode));
      if (NS_FAILED(result)) { return result; }
      if (!parentNode) { return NS_ERROR_NULL_POINTER; }
      PRInt32 offsetInParent;
      result = nsEditor::GetChildOffset(firstNode, parentNode, offsetInParent);
      if (NS_FAILED(result)) return result;
      result = selection->Collapse(parentNode, offsetInParent);
    }
  }
  else
  {
    
    result = selection->Collapse(rootElement, 0);
  }
  return result;
}

NS_IMETHODIMP
nsEditor::EndOfDocument() 
{ 
  if (!mDocWeak || !mPresShellWeak) { return NS_ERROR_NOT_INITIALIZED; } 
  nsresult res; 

  
  nsCOMPtr<nsISelection> selection; 
  res = GetSelection(getter_AddRefs(selection)); 
  if (NS_FAILED(res)) return res; 
  if (!selection)   return NS_ERROR_NULL_POINTER; 
  
  
  nsIDOMElement *rootElement = GetRoot(); 
  if (!rootElement)
    return NS_ERROR_NULL_POINTER; 

  
  PRUint32 len; 
  res = GetLengthOfDOMNode(rootElement, len); 
  if (NS_FAILED(res)) return res;

  
  return selection->Collapse(rootElement, (PRInt32)len); 
} 
  
NS_IMETHODIMP
nsEditor::GetDocumentModified(PRBool *outDocModified)
{
  if (!outDocModified)
    return NS_ERROR_NULL_POINTER;

  PRInt32  modCount = 0;
  GetModificationCount(&modCount);

  *outDocModified = (modCount != 0);
  return NS_OK;
}

NS_IMETHODIMP
nsEditor::GetDocumentCharacterSet(nsACString &characterSet)
{
  nsCOMPtr<nsIPresShell> presShell;

  nsresult rv = GetPresShell(getter_AddRefs(presShell));
  if (NS_SUCCEEDED(rv))
  {
    nsIDocument *doc = presShell->GetDocument();
    if (doc) {
      characterSet = doc->GetDocumentCharacterSet();
      return NS_OK;
    }
    rv = NS_ERROR_NULL_POINTER;
  }

  return rv;

}

NS_IMETHODIMP
nsEditor::SetDocumentCharacterSet(const nsACString& characterSet)
{
  nsCOMPtr<nsIPresShell> presShell;
  nsresult rv = GetPresShell(getter_AddRefs(presShell));
  if (NS_SUCCEEDED(rv))
  {
    nsIDocument *doc = presShell->GetDocument();
    if (doc) {
      doc->SetDocumentCharacterSet(characterSet);
      return NS_OK;
    }
    rv = NS_ERROR_NULL_POINTER;
  }

  return rv;
}






NS_IMETHODIMP
nsEditor::GetWrapWidth(PRInt32 *aWrapColumn)
{
  NS_ENSURE_ARG_POINTER(aWrapColumn);
  *aWrapColumn = 72;

  nsresult rv;
  nsCOMPtr<nsIPrefBranch> prefBranch =
    do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  if (NS_SUCCEEDED(rv) && prefBranch)
    (void) prefBranch->GetIntPref("editor.htmlWrapColumn", aWrapColumn);
  return NS_OK;
}

NS_IMETHODIMP
nsEditor::Cut()
{
  return NS_ERROR_NOT_IMPLEMENTED; 
}

NS_IMETHODIMP
nsEditor::CanCut(PRBool *aCanCut)
{
  return NS_ERROR_NOT_IMPLEMENTED; 
}

NS_IMETHODIMP
nsEditor::Copy()
{
  return NS_ERROR_NOT_IMPLEMENTED; 
}

NS_IMETHODIMP
nsEditor::CanCopy(PRBool *aCanCut)
{
  return NS_ERROR_NOT_IMPLEMENTED; 
}

NS_IMETHODIMP
nsEditor::Paste(PRInt32 aSelectionType)
{
  return NS_ERROR_NOT_IMPLEMENTED; 
}

NS_IMETHODIMP
nsEditor::CanPaste(PRInt32 aSelectionType, PRBool *aCanPaste)
{
  return NS_ERROR_NOT_IMPLEMENTED; 
}

NS_IMETHODIMP
nsEditor::CanDrag(nsIDOMEvent *aEvent, PRBool *aCanDrag)
{
  return NS_ERROR_NOT_IMPLEMENTED; 
}

NS_IMETHODIMP
nsEditor::DoDrag(nsIDOMEvent *aEvent)
{
  return NS_ERROR_NOT_IMPLEMENTED; 
}

NS_IMETHODIMP
nsEditor::InsertFromDrop(nsIDOMEvent *aEvent)
{
  return NS_ERROR_NOT_IMPLEMENTED; 
}


NS_IMETHODIMP 
nsEditor::SetAttribute(nsIDOMElement *aElement, const nsAString & aAttribute, const nsAString & aValue)
{
  nsRefPtr<ChangeAttributeTxn> txn;
  nsresult result = CreateTxnForSetAttribute(aElement, aAttribute, aValue,
                                             getter_AddRefs(txn));
  if (NS_SUCCEEDED(result))  {
    result = DoTransaction(txn);  
  }
  return result;
}

NS_IMETHODIMP 
nsEditor::GetAttributeValue(nsIDOMElement *aElement, 
                            const nsAString & aAttribute, 
                            nsAString & aResultValue, 
                            PRBool *aResultIsSet)
{
  if (!aResultIsSet)
    return NS_ERROR_NULL_POINTER;
  *aResultIsSet=PR_FALSE;
  nsresult result=NS_OK;
  if (aElement)
  {
    nsCOMPtr<nsIDOMAttr> attNode;
    result = aElement->GetAttributeNode(aAttribute, getter_AddRefs(attNode));
    if ((NS_SUCCEEDED(result)) && attNode)
    {
      attNode->GetSpecified(aResultIsSet);
      attNode->GetValue(aResultValue);
    }
  }
  return result;
}

NS_IMETHODIMP 
nsEditor::RemoveAttribute(nsIDOMElement *aElement, const nsAString& aAttribute)
{
  nsRefPtr<ChangeAttributeTxn> txn;
  nsresult result = CreateTxnForRemoveAttribute(aElement, aAttribute,
                                                getter_AddRefs(txn));
  if (NS_SUCCEEDED(result))  {
    result = DoTransaction(txn);  
  }
  return result;
}


NS_IMETHODIMP
nsEditor::MarkNodeDirty(nsIDOMNode* aNode)
{  
  
  nsCOMPtr<nsIDOMElement> element (do_QueryInterface(aNode));
  if (element)
    element->SetAttribute(NS_LITERAL_STRING("_moz_dirty"), EmptyString());
  return NS_OK;
}

NS_IMETHODIMP nsEditor::GetInlineSpellChecker(PRBool autoCreate,
                                              nsIInlineSpellChecker ** aInlineSpellChecker)
{
  NS_ENSURE_ARG_POINTER(aInlineSpellChecker);

  if (mDidPreDestroy) {
    
    
    *aInlineSpellChecker = nsnull;
    return autoCreate ? NS_ERROR_NOT_AVAILABLE : NS_OK;
  }

  nsresult rv;
  if (!mInlineSpellChecker && autoCreate) {
    mInlineSpellChecker = do_CreateInstance(MOZ_INLINESPELLCHECKER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (mInlineSpellChecker) {
    rv = mInlineSpellChecker->Init(this);
    if (NS_FAILED(rv))
      mInlineSpellChecker = nsnull;
    NS_ENSURE_SUCCESS(rv, rv);
  }

  NS_IF_ADDREF(*aInlineSpellChecker = mInlineSpellChecker);

  return NS_OK;
}

NS_IMETHODIMP nsEditor::SyncRealTimeSpell()
{
  PRBool enable = GetDesiredSpellCheckState();

  nsCOMPtr<nsIInlineSpellChecker> spellChecker;
  GetInlineSpellChecker(enable, getter_AddRefs(spellChecker));

  if (spellChecker) {
    spellChecker->SetEnableRealTimeSpell(enable);
  }

  return NS_OK;
}

NS_IMETHODIMP nsEditor::SetSpellcheckUserOverride(PRBool enable)
{
  mSpellcheckCheckboxState = enable ? eTriTrue : eTriFalse;

  return SyncRealTimeSpell();
}

#ifdef XP_MAC
#pragma mark -
#pragma mark  main node manipulation routines 
#pragma mark -
#endif

NS_IMETHODIMP nsEditor::CreateNode(const nsAString& aTag,
                                   nsIDOMNode *    aParent,
                                   PRInt32         aPosition,
                                   nsIDOMNode **   aNewNode)
{
  PRInt32 i;

  nsAutoRules beginRulesSniffing(this, kOpCreateNode, nsIEditor::eNext);
  
  for (i = 0; i < mActionListeners.Count(); i++)
    mActionListeners[i]->WillCreateNode(aTag, aParent, aPosition);

  nsRefPtr<CreateElementTxn> txn;
  nsresult result = CreateTxnForCreateElement(aTag, aParent, aPosition,
                                              getter_AddRefs(txn));
  if (NS_SUCCEEDED(result)) 
  {
    result = DoTransaction(txn);  
    if (NS_SUCCEEDED(result)) 
    {
      result = txn->GetNewNode(aNewNode);
      NS_ASSERTION((NS_SUCCEEDED(result)), "GetNewNode can't fail if txn::DoTransaction succeeded.");
    }
  }
  
  mRangeUpdater.SelAdjCreateNode(aParent, aPosition);
  
  for (i = 0; i < mActionListeners.Count(); i++)
    mActionListeners[i]->DidCreateNode(aTag, *aNewNode, aParent, aPosition, result);

  return result;
}


NS_IMETHODIMP nsEditor::InsertNode(nsIDOMNode * aNode,
                                   nsIDOMNode * aParent,
                                   PRInt32      aPosition)
{
  PRInt32 i;
  nsAutoRules beginRulesSniffing(this, kOpInsertNode, nsIEditor::eNext);

  for (i = 0; i < mActionListeners.Count(); i++)
    mActionListeners[i]->WillInsertNode(aNode, aParent, aPosition);

  nsRefPtr<InsertElementTxn> txn;
  nsresult result = CreateTxnForInsertElement(aNode, aParent, aPosition,
                                              getter_AddRefs(txn));
  if (NS_SUCCEEDED(result))  {
    result = DoTransaction(txn);  
  }

  mRangeUpdater.SelAdjInsertNode(aParent, aPosition);

  for (i = 0; i < mActionListeners.Count(); i++)
    mActionListeners[i]->DidInsertNode(aNode, aParent, aPosition, result);

  return result;
}


NS_IMETHODIMP 
nsEditor::SplitNode(nsIDOMNode * aNode,
                    PRInt32      aOffset,
                    nsIDOMNode **aNewLeftNode)
{
  PRInt32 i;
  nsAutoRules beginRulesSniffing(this, kOpSplitNode, nsIEditor::eNext);

  for (i = 0; i < mActionListeners.Count(); i++)
    mActionListeners[i]->WillSplitNode(aNode, aOffset);

  nsRefPtr<SplitElementTxn> txn;
  nsresult result = CreateTxnForSplitNode(aNode, aOffset, getter_AddRefs(txn));
  if (NS_SUCCEEDED(result))  
  {
    result = DoTransaction(txn);
    if (NS_SUCCEEDED(result))
    {
      result = txn->GetNewNode(aNewLeftNode);
      NS_ASSERTION((NS_SUCCEEDED(result)), "result must succeeded for GetNewNode");
    }
  }

  mRangeUpdater.SelAdjSplitNode(aNode, aOffset, *aNewLeftNode);

  for (i = 0; i < mActionListeners.Count(); i++)
  {
    nsIDOMNode *ptr = *aNewLeftNode;
    mActionListeners[i]->DidSplitNode(aNode, aOffset, ptr, result);
  }

  return result;
}



NS_IMETHODIMP
nsEditor::JoinNodes(nsIDOMNode * aLeftNode,
                    nsIDOMNode * aRightNode,
                    nsIDOMNode * aParent)
{
  PRInt32 i, offset;
  nsAutoRules beginRulesSniffing(this, kOpJoinNode, nsIEditor::ePrevious);

  
  
  nsresult result = GetChildOffset(aRightNode, aParent, offset);
  if (NS_FAILED(result)) return result;
  
  PRUint32 oldLeftNodeLen;
  result = GetLengthOfDOMNode(aLeftNode, oldLeftNodeLen);
  if (NS_FAILED(result)) return result;

  for (i = 0; i < mActionListeners.Count(); i++)
    mActionListeners[i]->WillJoinNodes(aLeftNode, aRightNode, aParent);

  nsRefPtr<JoinElementTxn> txn;
  result = CreateTxnForJoinNode(aLeftNode, aRightNode, getter_AddRefs(txn));
  if (NS_SUCCEEDED(result))  {
    result = DoTransaction(txn);  
  }

  mRangeUpdater.SelAdjJoinNodes(aLeftNode, aRightNode, aParent, offset, (PRInt32)oldLeftNodeLen);
  
  for (i = 0; i < mActionListeners.Count(); i++)
    mActionListeners[i]->DidJoinNodes(aLeftNode, aRightNode, aParent, result);

  return result;
}


NS_IMETHODIMP nsEditor::DeleteNode(nsIDOMNode * aElement)
{
  PRInt32 i, offset;
  nsCOMPtr<nsIDOMNode> parent;
  nsAutoRules beginRulesSniffing(this, kOpCreateNode, nsIEditor::ePrevious);

  
  nsresult result = GetNodeLocation(aElement, address_of(parent), &offset);
  if (NS_FAILED(result)) return result;

  for (i = 0; i < mActionListeners.Count(); i++)
    mActionListeners[i]->WillDeleteNode(aElement);

  nsRefPtr<DeleteElementTxn> txn;
  result = CreateTxnForDeleteElement(aElement, getter_AddRefs(txn));
  if (NS_SUCCEEDED(result))  {
    result = DoTransaction(txn);  
  }

  for (i = 0; i < mActionListeners.Count(); i++)
    mActionListeners[i]->DidDeleteNode(aElement, result);

  return result;
}






nsresult
nsEditor::ReplaceContainer(nsIDOMNode *inNode, 
                           nsCOMPtr<nsIDOMNode> *outNode, 
                           const nsAString &aNodeType,
                           const nsAString *aAttribute,
                           const nsAString *aValue,
                           PRBool aCloneAttributes)
{
  if (!inNode || !outNode)
    return NS_ERROR_NULL_POINTER;
  nsCOMPtr<nsIDOMNode> parent;
  PRInt32 offset;
  nsresult res = GetNodeLocation(inNode, address_of(parent), &offset);
  if (NS_FAILED(res)) return res;

  
  nsCOMPtr<nsIContent> newContent;

  
  res = CreateHTMLContent(aNodeType, getter_AddRefs(newContent));
  nsCOMPtr<nsIDOMElement> elem = do_QueryInterface(newContent);
  if (NS_FAILED(res)) return res;
    *outNode = do_QueryInterface(elem);
  
  
  if (aAttribute && aValue && !aAttribute->IsEmpty())
  {
    res = elem->SetAttribute(*aAttribute, *aValue);
    if (NS_FAILED(res)) return res;
  }
  if (aCloneAttributes)
  {
    nsCOMPtr<nsIDOMNode>newNode = do_QueryInterface(elem);
    res = CloneAttributes(newNode, inNode);
    if (NS_FAILED(res)) return res;
  }
  
  
  
  
  nsAutoReplaceContainerSelNotify selStateNotify(mRangeUpdater, inNode, *outNode);
  {
    nsAutoTxnsConserveSelection conserveSelection(this);
    nsCOMPtr<nsIDOMNode> child;
    PRBool bHasMoreChildren;
    inNode->HasChildNodes(&bHasMoreChildren);
    while (bHasMoreChildren)
    {
      inNode->GetFirstChild(getter_AddRefs(child));
      res = DeleteNode(child);
      if (NS_FAILED(res)) return res;

      res = InsertNode(child, *outNode, -1);
      if (NS_FAILED(res)) return res;
      inNode->HasChildNodes(&bHasMoreChildren);
    }
  }
  
  res = InsertNode( *outNode, parent, offset);
  if (NS_FAILED(res)) return res;
  
  
  return DeleteNode(inNode);
}





nsresult
nsEditor::RemoveContainer(nsIDOMNode *inNode)
{
  if (!inNode)
    return NS_ERROR_NULL_POINTER;
  nsCOMPtr<nsIDOMNode> parent;
  PRInt32 offset;
  
  nsresult res = GetNodeLocation(inNode, address_of(parent), &offset);
  if (NS_FAILED(res)) return res;
  
  
  
  PRBool bHasMoreChildren;
  inNode->HasChildNodes(&bHasMoreChildren);
  nsCOMPtr<nsIDOMNodeList> nodeList;
  res = inNode->GetChildNodes(getter_AddRefs(nodeList));
  if (NS_FAILED(res)) return res;
  if (!nodeList) return NS_ERROR_NULL_POINTER;
  PRUint32 nodeOrigLen;
  nodeList->GetLength(&nodeOrigLen);

  
  nsAutoRemoveContainerSelNotify selNotify(mRangeUpdater, inNode, parent, offset, nodeOrigLen);
                                   
  nsCOMPtr<nsIDOMNode> child;
  while (bHasMoreChildren)
  {
    inNode->GetLastChild(getter_AddRefs(child));
    res = DeleteNode(child);
    if (NS_FAILED(res)) return res;
    res = InsertNode(child, parent, offset);
    if (NS_FAILED(res)) return res;
    inNode->HasChildNodes(&bHasMoreChildren);
  }
  return DeleteNode(inNode);
}








nsresult
nsEditor::InsertContainerAbove( nsIDOMNode *inNode, 
                                nsCOMPtr<nsIDOMNode> *outNode, 
                                const nsAString &aNodeType,
                                const nsAString *aAttribute,
                                const nsAString *aValue)
{
  if (!inNode || !outNode)
    return NS_ERROR_NULL_POINTER;
  nsCOMPtr<nsIDOMNode> parent;
  PRInt32 offset;
  nsresult res = GetNodeLocation(inNode, address_of(parent), &offset);
  if (NS_FAILED(res)) return res;

  
  nsCOMPtr<nsIContent> newContent;

  
  res = CreateHTMLContent(aNodeType, getter_AddRefs(newContent));
  nsCOMPtr<nsIDOMElement> elem = do_QueryInterface(newContent);
  if (NS_FAILED(res)) return res;
  *outNode = do_QueryInterface(elem);
  
  
  if (aAttribute && aValue && !aAttribute->IsEmpty())
  {
    res = elem->SetAttribute(*aAttribute, *aValue);
    if (NS_FAILED(res)) return res;
  }
  
  
  nsAutoInsertContainerSelNotify selNotify(mRangeUpdater);
  
  
  res = DeleteNode(inNode);
  if (NS_FAILED(res)) return res;

  {
    nsAutoTxnsConserveSelection conserveSelection(this);
    res = InsertNode(inNode, *outNode, 0);
    if (NS_FAILED(res)) return res;
  }

  
  return InsertNode(*outNode, parent, offset);
}



nsresult
nsEditor::MoveNode(nsIDOMNode *aNode, nsIDOMNode *aParent, PRInt32 aOffset)
{
  if (!aNode || !aParent)
    return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIDOMNode> oldParent;
  PRInt32 oldOffset;
  nsresult res = GetNodeLocation(aNode, address_of(oldParent), &oldOffset);
  
  if (aOffset == -1)
  {
    PRUint32 unsignedOffset;
    
    res = GetLengthOfDOMNode(aParent, unsignedOffset);
    if (NS_FAILED(res)) return res;
    aOffset = (PRInt32)unsignedOffset;
  }
  
  
  if ((aParent == oldParent.get()) && (oldOffset == aOffset)) return NS_OK;
  
  
  nsAutoMoveNodeSelNotify selNotify(mRangeUpdater, oldParent, oldOffset, aParent, aOffset);
  
  
  if ((aParent == oldParent.get()) && (oldOffset < aOffset)) 
  {
    aOffset--;  
  }

  
  res = DeleteNode(aNode);
  if (NS_FAILED(res)) return res;
  return InsertNode(aNode, aParent, aOffset);
}

#ifdef XP_MAC
#pragma mark -
#pragma mark  editor observer maintainance
#pragma mark -
#endif

NS_IMETHODIMP
nsEditor::AddEditorObserver(nsIEditorObserver *aObserver)
{
  
  
  
  if (!aObserver)
    return NS_ERROR_NULL_POINTER;

  
  if (mEditorObservers.IndexOf(aObserver) == -1) 
  {
    if (!mEditorObservers.AppendObject(aObserver))
      return NS_ERROR_FAILURE;
  }

  return NS_OK;
}


NS_IMETHODIMP
nsEditor::RemoveEditorObserver(nsIEditorObserver *aObserver)
{
  if (!aObserver)
    return NS_ERROR_FAILURE;

  if (!mEditorObservers.RemoveObject(aObserver))
    return NS_ERROR_FAILURE;

  return NS_OK;
}

void nsEditor::NotifyEditorObservers(void)
{
  for (PRInt32 i = 0; i < mEditorObservers.Count(); i++)
    mEditorObservers[i]->EditAction();
}

#ifdef XP_MAC
#pragma mark -
#pragma mark  action listener maintainance
#pragma mark -
#endif

NS_IMETHODIMP
nsEditor::AddEditActionListener(nsIEditActionListener *aListener)
{
  if (!aListener)
    return NS_ERROR_NULL_POINTER;

  
  if (mActionListeners.IndexOf(aListener) == -1) 
  {
    if (!mActionListeners.AppendObject(aListener))
      return NS_ERROR_FAILURE;
  }

  return NS_OK;
}


NS_IMETHODIMP
nsEditor::RemoveEditActionListener(nsIEditActionListener *aListener)
{
  if (!aListener)
    return NS_ERROR_FAILURE;

  if (!mActionListeners.RemoveObject(aListener))
    return NS_ERROR_FAILURE;

  return NS_OK;
}


#ifdef XP_MAC
#pragma mark -
#pragma mark  docstate listener maintainance
#pragma mark -
#endif


NS_IMETHODIMP
nsEditor::AddDocumentStateListener(nsIDocumentStateListener *aListener)
{
  if (!aListener)
    return NS_ERROR_NULL_POINTER;

  if (mDocStateListeners.IndexOf(aListener) == -1)
  {
    if (!mDocStateListeners.AppendObject(aListener))
      return NS_ERROR_FAILURE;
  }

  return NS_OK;
}


NS_IMETHODIMP
nsEditor::RemoveDocumentStateListener(nsIDocumentStateListener *aListener)
{
  if (!aListener)
    return NS_ERROR_NULL_POINTER;

  if (!mDocStateListeners.RemoveObject(aListener))
    return NS_ERROR_FAILURE;

  return NS_OK;
}


#ifdef XP_MAC
#pragma mark -
#pragma mark  misc 
#pragma mark -
#endif

NS_IMETHODIMP nsEditor::OutputToString(const nsAString& aFormatType,
                                       PRUint32 aFlags,
                                       nsAString& aOutputString)
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsEditor::OutputToStream(nsIOutputStream* aOutputStream,
                         const nsAString& aFormatType,
                         const nsACString& aCharsetOverride,
                         PRUint32 aFlags)
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsEditor::DumpContentTree()
{
#ifdef DEBUG
  nsCOMPtr<nsIContent> root = do_QueryInterface(mRootElement);
  if (root)  root->List(stdout);
#endif
  return NS_OK;
}


NS_IMETHODIMP
nsEditor::DebugDumpContent()
{
#ifdef DEBUG
  nsCOMPtr<nsIDOMHTMLDocument> doc = do_QueryReferent(mDocWeak);
  if (!doc) return NS_ERROR_NOT_INITIALIZED;

  nsCOMPtr<nsIDOMHTMLElement>bodyElem;
  doc->GetBody(getter_AddRefs(bodyElem));
  nsCOMPtr<nsIContent> content = do_QueryInterface(bodyElem);
  if (content)
    content->List();
#endif
  return NS_OK;
}


NS_IMETHODIMP
nsEditor::DebugUnitTests(PRInt32 *outNumTests, PRInt32 *outNumTestsFailed)
{
#ifdef DEBUG
  NS_NOTREACHED("This should never get called. Overridden by subclasses");
#endif
  return NS_OK;
}

#ifdef XP_MAC
#pragma mark -
#pragma mark  support for selection preservation
#pragma mark -
#endif

PRBool   
nsEditor::ArePreservingSelection()
{
  return !(mSavedSel.IsEmpty());
}

nsresult 
nsEditor::PreserveSelectionAcrossActions(nsISelection *aSel)
{
  mSavedSel.SaveSelection(aSel);
  mRangeUpdater.RegisterSelectionState(mSavedSel);
  return NS_OK;
}

nsresult 
nsEditor::RestorePreservedSelection(nsISelection *aSel)
{
  if (mSavedSel.IsEmpty()) return NS_ERROR_FAILURE;
  mSavedSel.RestoreSelection(aSel);
  StopPreservingSelection();
  return NS_OK;
}

void     
nsEditor::StopPreservingSelection()
{
  mRangeUpdater.DropSelectionState(mSavedSel);
  mSavedSel.MakeEmpty();
}


#ifdef XP_MAC
#pragma mark -
#pragma mark  nsIEditorIMESupport 
#pragma mark -
#endif




NS_IMETHODIMP
nsEditor::QueryComposition(nsTextEventReply* aReply)
{
  nsresult result;
  nsCOMPtr<nsISelection> selection;
  nsCOMPtr<nsISelectionController> selcon = do_QueryReferent(mSelConWeak);
  if (selcon)
    selcon->GetSelection(nsISelectionController::SELECTION_NORMAL, getter_AddRefs(selection));

  if (!mPresShellWeak) return NS_ERROR_NOT_INITIALIZED;
  nsCOMPtr<nsIPresShell> ps = do_QueryReferent(mPresShellWeak);
  if (!ps) return NS_ERROR_NOT_INITIALIZED;
  nsRefPtr<nsCaret> caretP; 
  result = ps->GetCaret(getter_AddRefs(caretP));
  
  if (NS_SUCCEEDED(result) && caretP) {
    if (aReply) {
      caretP->SetCaretDOMSelection(selection);

      
      
      
      
      
      
      
      
      

      PRUint32 flags = 0;

      if (NS_SUCCEEDED(GetFlags(&flags)) &&
          (flags & nsIPlaintextEditor::eEditorUseAsyncUpdatesMask))
      {
        PRBool restoreFlags = PR_FALSE;

        if (NS_SUCCEEDED(SetFlags(flags & (~nsIPlaintextEditor::eEditorUseAsyncUpdatesMask))))
        {
           
           
           

           nsAutoUpdateViewBatch viewBatch(this);
           restoreFlags = PR_TRUE;
        }

        

        if (restoreFlags)
          SetFlags(flags);
      }


      

      nsIView *view = nsnull;
      nsRect rect;
      result =
        caretP->GetCaretCoordinates(nsCaret::eRenderingViewCoordinates,
                                    selection,
                                    &rect,
                                    &(aReply->mCursorIsCollapsed),
                                    &view);
      aReply->mCursorPosition =
        rect.ToOutsidePixels(ps->GetPresContext()->AppUnitsPerDevPixel());
      if (NS_SUCCEEDED(result) && view)
        aReply->mReferenceWidget = view->GetWidget();
    }
  }
  return result;
}

NS_IMETHODIMP
nsEditor::BeginComposition(nsTextEventReply* aReply)
{
#ifdef DEBUG_tague
  printf("nsEditor::StartComposition\n");
#endif
  nsresult ret = QueryComposition(aReply);
  mInIMEMode = PR_TRUE;
  if (mPhonetic)
    mPhonetic->Truncate(0);

  return ret;
}

NS_IMETHODIMP
nsEditor::EndComposition(void)
{
  if (!mInIMEMode) return NS_OK; 
  
  nsresult result = NS_OK;

  
  
  if (mTxnMgr) 
  {
    nsCOMPtr<nsITransaction> txn;
    result = mTxnMgr->PeekUndoStack(getter_AddRefs(txn));  
    nsCOMPtr<nsIAbsorbingTransaction> plcTxn = do_QueryInterface(txn);
    if (plcTxn)
    {
      result = plcTxn->Commit();
    }
  }

  
  mIMETextNode = do_QueryInterface(nsnull);
  mIMETextOffset = 0;
  mIMEBufferLength = 0;
  mInIMEMode = PR_FALSE;
  mIsIMEComposing = PR_FALSE;

  
  NotifyEditorObservers();

  return result;
}

NS_IMETHODIMP
nsEditor::SetCompositionString(const nsAString& aCompositionString, nsIPrivateTextRangeList* aTextRangeList,nsTextEventReply* aReply)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsEditor::GetPhonetic(nsAString& aPhonetic)
{
  if (mPhonetic)
    aPhonetic = *mPhonetic;
  else
    aPhonetic.Truncate(0);

  return NS_OK;
}


static nsresult
GetEditorContentWindow(nsIPresShell *aPresShell, nsIDOMElement *aRoot, nsIWidget **aResult)
{
  if (!aPresShell || !aRoot || !aResult)
    return NS_ERROR_NULL_POINTER;

  *aResult = 0;

  nsCOMPtr<nsIContent> content = do_QueryInterface(aRoot);

  if (!content)
    return NS_ERROR_FAILURE;

  
  nsIFrame *frame = aPresShell->GetPrimaryFrameFor(content);

  if (!frame)
    return NS_ERROR_FAILURE;

  *aResult = frame->GetWindow();
  if (!*aResult)
    return NS_ERROR_FAILURE;

  NS_ADDREF(*aResult);
  return NS_OK;
}

nsresult
nsEditor::GetWidget(nsIWidget **aWidget)
{
  if (!aWidget)
    return NS_ERROR_NULL_POINTER;
  *aWidget = nsnull;
  nsCOMPtr<nsIPresShell> shell;
  nsresult res = GetPresShell(getter_AddRefs(shell));

  if (NS_FAILED(res))
    return res;

  if (!shell)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIWidget> widget;
  res = GetEditorContentWindow(shell, GetRoot(), getter_AddRefs(widget));
  if (NS_FAILED(res))
    return res;
  if (!widget)
    return NS_ERROR_NOT_AVAILABLE;

  NS_ADDREF(*aWidget = widget);

  return NS_OK;
}

NS_IMETHODIMP
nsEditor::ForceCompositionEnd()
{






#if defined(XP_MAC) || defined(XP_MACOSX) || defined(XP_WIN) || defined(XP_OS2)
  if(! mInIMEMode)
    return NS_OK;
#endif

#ifdef XP_UNIX
  if(mFlags & nsIPlaintextEditor::eEditorPasswordMask)
	return NS_OK;
#endif

  nsCOMPtr<nsIWidget> widget;
  nsresult res = GetWidget(getter_AddRefs(widget));
  if (NS_FAILED(res))
    return res;

  if (widget) {
    res = widget->ResetInputState();
    if (NS_FAILED(res)) 
      return res;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsEditor::GetPreferredIMEState(PRUint32 *aState)
{
  NS_ENSURE_ARG_POINTER(aState);
  *aState = nsIContent::IME_STATUS_ENABLE;

  PRUint32 flags;
  nsresult rv = GetFlags(&flags);
  NS_ENSURE_SUCCESS(rv, rv);

  if (flags & (nsIPlaintextEditor::eEditorReadonlyMask |
               nsIPlaintextEditor::eEditorDisabledMask)) {
    *aState = nsIContent::IME_STATUS_DISABLE;
    return NS_OK;
  }

  nsCOMPtr<nsIPresShell> presShell;
  rv = GetPresShell(getter_AddRefs(presShell));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIContent> content = do_QueryInterface(GetRoot());
  NS_ENSURE_TRUE(content, NS_ERROR_FAILURE);

  nsIFrame* frame = presShell->GetPrimaryFrameFor(content);
  NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);

  switch (frame->GetStyleUIReset()->mIMEMode) {
    case NS_STYLE_IME_MODE_AUTO:
      if (flags & (nsIPlaintextEditor::eEditorPasswordMask))
        *aState = nsIContent::IME_STATUS_PASSWORD;
      break;
    case NS_STYLE_IME_MODE_DISABLED:
      
      *aState = nsIContent::IME_STATUS_PASSWORD;
      break;
    case NS_STYLE_IME_MODE_ACTIVE:
      *aState |= nsIContent::IME_STATUS_OPEN;
      break;
    case NS_STYLE_IME_MODE_INACTIVE:
      *aState |= nsIContent::IME_STATUS_CLOSE;
      break;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsEditor::GetComposing(PRBool* aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = IsIMEComposing();
  return NS_OK;
}

#ifdef XP_MAC
#pragma mark -
#pragma mark  public nsEditor methods 
#pragma mark -
#endif



void
nsEditor::ContentAppended(nsIDocument *aDocument, nsIContent* aContainer,
                          PRInt32 aNewIndexInContainer)
{
  ContentInserted(aDocument, aContainer, nsnull, aNewIndexInContainer);
}

void
nsEditor::ContentInserted(nsIDocument *aDocument, nsIContent* aContainer,
                          nsIContent* aChild, PRInt32 aIndexInContainer)
{
  
  
  if (!mRootElement)
  {
    
    
    
    RemoveEventListeners();
    BeginningOfDocument();
    InstallEventListeners();
    SyncRealTimeSpell();
  }
}

void
nsEditor::ContentRemoved(nsIDocument *aDocument, nsIContent* aContainer,
                         nsIContent* aChild, PRInt32 aIndexInContainer)
{
  nsCOMPtr<nsIDOMHTMLElement> elem = do_QueryInterface(aChild);
  if (elem == mRootElement)
  {
    RemoveEventListeners();
    mRootElement = nsnull;
    mEventTarget = nsnull;
    InstallEventListeners();
  }
}

NS_IMETHODIMP 
nsEditor::GetRootElement(nsIDOMElement **aRootElement)
{
  if (!aRootElement)
    return NS_ERROR_NULL_POINTER;

  if (mRootElement)
  {
    
    *aRootElement = mRootElement;
    NS_ADDREF(*aRootElement);
    return NS_OK;
  }

  *aRootElement = 0;

  NS_PRECONDITION(mDocWeak, "bad state, null mDocWeak");
  nsCOMPtr<nsIDOMHTMLDocument> doc = do_QueryReferent(mDocWeak);
  if (!doc) return NS_ERROR_NOT_INITIALIZED;

  
  

  nsCOMPtr<nsIDOMHTMLElement> bodyElement; 
  nsresult result = doc->GetBody(getter_AddRefs(bodyElement));
  if (NS_FAILED(result))
    return result;

  if (!bodyElement)
    return NS_ERROR_NULL_POINTER;

  mRootElement = bodyElement;
  *aRootElement = bodyElement;
  NS_ADDREF(*aRootElement);

  return NS_OK;
}




NS_IMETHODIMP
nsEditor::StartOperation(PRInt32 opID, nsIEditor::EDirection aDirection)
{
  mAction = opID;
  mDirection = aDirection;
  return NS_OK;
}




NS_IMETHODIMP
nsEditor::EndOperation()
{
  mAction = nsnull;
  mDirection = eNone;
  return NS_OK;
}

NS_IMETHODIMP
nsEditor::CloneAttribute(const nsAString & aAttribute,
                         nsIDOMNode *aDestNode, nsIDOMNode *aSourceNode)
{
  if (!aDestNode || !aSourceNode)
    return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIDOMElement> destElement = do_QueryInterface(aDestNode);
  nsCOMPtr<nsIDOMElement> sourceElement = do_QueryInterface(aSourceNode);
  if (!destElement || !sourceElement)
    return NS_ERROR_NO_INTERFACE;

  nsAutoString attrValue;
  PRBool isAttrSet;
  nsresult rv = GetAttributeValue(sourceElement,
                                  aAttribute,
                                  attrValue,
                                  &isAttrSet);
  if (NS_FAILED(rv))
    return rv;
  if (isAttrSet)
    rv = SetAttribute(destElement, aAttribute, attrValue);
  else
    rv = RemoveAttribute(destElement, aAttribute);

  return rv;
}


NS_IMETHODIMP
nsEditor::CloneAttributes(nsIDOMNode *aDestNode, nsIDOMNode *aSourceNode)
{
  if (!aDestNode || !aSourceNode)
    return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIDOMElement> destElement = do_QueryInterface(aDestNode);
  nsCOMPtr<nsIDOMElement> sourceElement = do_QueryInterface(aSourceNode);
  if (!destElement || !sourceElement)
    return NS_ERROR_NO_INTERFACE;

  nsCOMPtr<nsIDOMNamedNodeMap> sourceAttributes;
  sourceElement->GetAttributes(getter_AddRefs(sourceAttributes));
  nsCOMPtr<nsIDOMNamedNodeMap> destAttributes;
  destElement->GetAttributes(getter_AddRefs(destAttributes));
  if (!sourceAttributes || !destAttributes)
    return NS_ERROR_FAILURE;

  nsAutoEditBatch beginBatching(this);

  
  
  nsIDOMElement *rootElement = GetRoot();
  if (!rootElement)
    return NS_ERROR_NULL_POINTER;

  PRBool destInBody = PR_TRUE;
  nsCOMPtr<nsIDOMNode> rootNode = do_QueryInterface(rootElement);
  nsCOMPtr<nsIDOMNode> p = aDestNode;
  while (p && p != rootNode)
  {
    nsCOMPtr<nsIDOMNode> tmp;
    if (NS_FAILED(p->GetParentNode(getter_AddRefs(tmp))) || !tmp)
    {
      destInBody = PR_FALSE;
      break;
    }
    p = tmp;
  }

  PRUint32 sourceCount;
  sourceAttributes->GetLength(&sourceCount);
  PRUint32 i, destCount;
  destAttributes->GetLength(&destCount);
  nsCOMPtr<nsIDOMNode> attrNode;

  
  for (i = 0; i < destCount; i++)
  {
    
    if( NS_SUCCEEDED(destAttributes->Item(0, getter_AddRefs(attrNode))) && attrNode)
    {
      nsCOMPtr<nsIDOMAttr> destAttribute = do_QueryInterface(attrNode);
      if (destAttribute)
      {
        nsAutoString str;
        if (NS_SUCCEEDED(destAttribute->GetName(str)))
        {
          if (destInBody)
            RemoveAttribute(destElement, str);
          else
            destElement->RemoveAttribute(str);
        }
      }
    }
  }

  nsresult result = NS_OK;

  
  for (i = 0; i < sourceCount; i++)
  {
    if( NS_SUCCEEDED(sourceAttributes->Item(i, getter_AddRefs(attrNode))) && attrNode)
    {
      nsCOMPtr<nsIDOMAttr> sourceAttribute = do_QueryInterface(attrNode);
      if (sourceAttribute)
      {
        nsAutoString sourceAttrName;
        if (NS_SUCCEEDED(sourceAttribute->GetName(sourceAttrName)))
        {
          nsAutoString sourceAttrValue;
          



          if (NS_SUCCEEDED(sourceAttribute->GetValue(sourceAttrValue)))
          {
            if (destInBody) {
              result = SetAttributeOrEquivalent(destElement, sourceAttrName, sourceAttrValue, PR_FALSE);
            }
            else {
              
              
              result = SetAttributeOrEquivalent(destElement, sourceAttrName, sourceAttrValue, PR_TRUE);
            }
          } else {
            
#if DEBUG_cmanske
            printf("Attribute in sourceAttribute has empty value in nsEditor::CloneAttributes()\n");
#endif
          }
        }        
      }
    }
  }
  return result;
}

#ifdef XP_MAC
#pragma mark -
#pragma mark  Protected and static methods 
#pragma mark -
#endif

NS_IMETHODIMP nsEditor::ScrollSelectionIntoView(PRBool aScrollToAnchor)
{
  nsCOMPtr<nsISelectionController> selCon;
  if (NS_SUCCEEDED(GetSelectionController(getter_AddRefs(selCon))) && selCon)
  {
    PRInt16 region = nsISelectionController::SELECTION_FOCUS_REGION;

    if (aScrollToAnchor)
      region = nsISelectionController::SELECTION_ANCHOR_REGION;

    PRBool syncScroll = PR_TRUE;
    PRUint32 flags = 0;

    if (NS_SUCCEEDED(GetFlags(&flags)))
    {
      
      
      
      
      syncScroll = !(flags & nsIPlaintextEditor::eEditorUseAsyncUpdatesMask);
    }

    
    
    selCon->ScrollSelectionIntoView(nsISelectionController::SELECTION_NORMAL,
                                    region, syncScroll);
  }

  return NS_OK;
}

NS_IMETHODIMP nsEditor::InsertTextImpl(const nsAString& aStringToInsert, 
                                          nsCOMPtr<nsIDOMNode> *aInOutNode, 
                                          PRInt32 *aInOutOffset,
                                          nsIDOMDocument *aDoc)
{
  
  
  
  
  if (!aInOutNode || !*aInOutNode || !aInOutOffset || !aDoc) return NS_ERROR_NULL_POINTER;
  if (!mInIMEMode && aStringToInsert.IsEmpty()) return NS_OK;
  nsCOMPtr<nsIDOMText> nodeAsText = do_QueryInterface(*aInOutNode);
  PRInt32 offset = *aInOutOffset;
  nsresult res;
  if (mInIMEMode)
  {
    if (!nodeAsText)
    {
      
      res = aDoc->CreateTextNode(EmptyString(), getter_AddRefs(nodeAsText));
      if (NS_FAILED(res)) return res;
      if (!nodeAsText) return NS_ERROR_NULL_POINTER;
      nsCOMPtr<nsIDOMNode> newNode = do_QueryInterface(nodeAsText);
      
      res = InsertNode(newNode, *aInOutNode, offset);
      if (NS_FAILED(res)) return res;
      offset = 0;
    }
    res = InsertTextIntoTextNodeImpl(aStringToInsert, nodeAsText, offset);
    if (NS_FAILED(res)) return res;
  }
  else
  {
    if (nodeAsText)
    {
      
      res = InsertTextIntoTextNodeImpl(aStringToInsert, nodeAsText, offset);
      if (NS_FAILED(res)) return res;
      *aInOutOffset += aStringToInsert.Length();
    }
    else
    {
      
      
      res = aDoc->CreateTextNode(aStringToInsert, getter_AddRefs(nodeAsText));
      if (NS_FAILED(res)) return res;
      if (!nodeAsText) return NS_ERROR_NULL_POINTER;
      nsCOMPtr<nsIDOMNode> newNode = do_QueryInterface(nodeAsText);
      
      res = InsertNode(newNode, *aInOutNode, offset);
      if (NS_FAILED(res)) return res;
      *aInOutNode = newNode;
      *aInOutOffset = aStringToInsert.Length();
    }
  }
  return res;
}


NS_IMETHODIMP nsEditor::InsertTextIntoTextNodeImpl(const nsAString& aStringToInsert, 
                                                     nsIDOMCharacterData *aTextNode, 
                                                     PRInt32 aOffset, PRBool suppressIME)
{
  nsRefPtr<EditTxn> txn;
  nsresult result = NS_OK;
  
  
  if (mIMETextRangeList && mInIMEMode && !suppressIME)
  {
    if (!mIMETextNode)
    {
      mIMETextNode = aTextNode;
      mIMETextOffset = aOffset;
    }
    PRUint16 len ;
    len = mIMETextRangeList->GetLength();
    if (len > 0)
    {
      nsCOMPtr<nsIPrivateTextRange> range;
      for (PRUint16 i = 0; i < len; i++) 
      {
        range = mIMETextRangeList->Item(i);
        if (range)
        {
          PRUint16 type;
          result = range->GetRangeType(&type);
          if (NS_SUCCEEDED(result)) 
          {
            if (type == nsIPrivateTextRange::TEXTRANGE_RAWINPUT) 
            {
              PRUint16 start, end;
              result = range->GetRangeStart(&start);
              if (NS_SUCCEEDED(result)) 
              {
                result = range->GetRangeEnd(&end);
                if (NS_SUCCEEDED(result)) 
                {
                  if (!mPhonetic)
                    mPhonetic = new nsString();
                  if (mPhonetic)
                  {
                    nsAutoString tmp(aStringToInsert);                  
                    tmp.Mid(*mPhonetic, start, end-start);
                  }
                }
              }
            } 
          }
        } 
      } 
    } 

    nsRefPtr<IMETextTxn> imeTxn;
    result = CreateTxnForIMEText(aStringToInsert, getter_AddRefs(imeTxn));
    txn = imeTxn;
  }
  else
  {
    nsRefPtr<InsertTextTxn> insertTxn;
    result = CreateTxnForInsertText(aStringToInsert, aTextNode, aOffset,
                                    getter_AddRefs(insertTxn));
    txn = insertTxn;
  }
  if (NS_FAILED(result)) return result;

  
  PRInt32 i;
  for (i = 0; i < mActionListeners.Count(); i++)
    mActionListeners[i]->WillInsertText(aTextNode, aOffset, aStringToInsert);
  
  
  BeginUpdateViewBatch();
  result = DoTransaction(txn);
  EndUpdateViewBatch();

  mRangeUpdater.SelAdjInsertText(aTextNode, aOffset, aStringToInsert);
  
  
  for (i = 0; i < mActionListeners.Count(); i++)
    mActionListeners[i]->DidInsertText(aTextNode, aOffset, aStringToInsert, result);

  
  
  
  
  
  
  
  
  
  
  if (mInIMEMode && mIMETextNode)
  {
    PRUint32 len;
    mIMETextNode->GetLength(&len);
    if (!len)
    {
      DeleteNode(mIMETextNode);
      mIMETextNode = nsnull;
      static_cast<IMETextTxn*>(txn.get())->MarkFixed();  
    }
  }
  
  return result;
}


NS_IMETHODIMP nsEditor::SelectEntireDocument(nsISelection *aSelection)
{
  if (!aSelection) { return NS_ERROR_NULL_POINTER; }

  nsIDOMElement *rootElement = GetRoot();
  if (!rootElement) { return NS_ERROR_NOT_INITIALIZED; }

  return aSelection->SelectAllChildren(rootElement);
}


nsresult nsEditor::GetFirstEditableNode(nsIDOMNode *aRoot, nsCOMPtr<nsIDOMNode> *outFirstNode)
{
  if (!aRoot || !outFirstNode) return NS_ERROR_NULL_POINTER;
  nsresult rv = NS_OK;
  *outFirstNode = nsnull;

  nsCOMPtr<nsIDOMNode> node = GetLeftmostChild(aRoot);
  if (node && !IsEditable(node))
  {
    nsCOMPtr<nsIDOMNode> next;
    rv = GetNextNode(node, PR_TRUE, address_of(next));
    node = next;
  }
  
  if (node != aRoot)
    *outFirstNode = node;

  return rv;
}

#ifdef XXX_DEAD_CODE

nsresult nsEditor::GetLastEditableNode(nsIDOMNode *aRoot, nsCOMPtr<nsIDOMNode> *outLastNode)
{
  if (!aRoot || !outLastNode) return NS_ERROR_NULL_POINTER;
  nsresult rv = NS_OK;
  *outLastNode = nsnull;

  nsCOMPtr<nsIDOMNode> node = GetRightmostChild(aRoot);
  if (node && !IsEditable(node))
  {
    nsCOMPtr<nsIDOMNode> next;
    rv = GetPriorNode(node, PR_TRUE, address_of(next));
    node = next;
  }

  if (node != aRoot)
    *outLastNode = node;

  return rv;
}
#endif


NS_IMETHODIMP
nsEditor::NotifyDocumentListeners(TDocumentListenerNotification aNotificationType)
{
  PRInt32 numListeners = mDocStateListeners.Count();
  if (!numListeners)
    return NS_OK;    
 
  nsCOMArray<nsIDocumentStateListener> listeners(mDocStateListeners);
  nsresult rv = NS_OK;
  PRInt32 i;

  switch (aNotificationType)
  {
    case eDocumentCreated:
      for (i = 0; i < numListeners;i++)
      {
        rv = listeners[i]->NotifyDocumentCreated();
        if (NS_FAILED(rv))
          break;
      }
      break;
      
    case eDocumentToBeDestroyed:
      for (i = 0; i < numListeners;i++)
      {
        rv = listeners[i]->NotifyDocumentWillBeDestroyed();
        if (NS_FAILED(rv))
          break;
      }
      break;
  
    case eDocumentStateChanged:
      {
        PRBool docIsDirty;
        rv = GetDocumentModified(&docIsDirty);
        if (NS_FAILED(rv)) return rv;
        
        if (docIsDirty == mDocDirtyState)
          return NS_OK;
        
        mDocDirtyState = (PRInt8)docIsDirty;
        
        for (i = 0; i < numListeners;i++)
        {
          rv = listeners[i]->NotifyDocumentStateChanged(mDocDirtyState);
          if (NS_FAILED(rv))
            break;
        }
      }
      break;
    
    default:
      NS_NOTREACHED("Unknown notification");
  }

  return rv;
}


NS_IMETHODIMP nsEditor::CreateTxnForInsertText(const nsAString & aStringToInsert,
                                               nsIDOMCharacterData *aTextNode,
                                               PRInt32 aOffset,
                                               InsertTextTxn ** aTxn)
{
  if (!aTextNode || !aTxn) return NS_ERROR_NULL_POINTER;
  nsresult result;

  *aTxn = new InsertTextTxn();
  if (!*aTxn) return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*aTxn);
  result = (*aTxn)->Init(aTextNode, aOffset, aStringToInsert, this);
  return result;
}


NS_IMETHODIMP nsEditor::DeleteText(nsIDOMCharacterData *aElement,
                              PRUint32             aOffset,
                              PRUint32             aLength)
{
  nsRefPtr<DeleteTextTxn> txn;
  nsresult result = CreateTxnForDeleteText(aElement, aOffset, aLength,
                                           getter_AddRefs(txn));
  nsAutoRules beginRulesSniffing(this, kOpDeleteText, nsIEditor::ePrevious);
  if (NS_SUCCEEDED(result))  
  {
    
    PRInt32 i;
    for (i = 0; i < mActionListeners.Count(); i++)
      mActionListeners[i]->WillDeleteText(aElement, aOffset, aLength);
    
    result = DoTransaction(txn); 
    
    
    for (i = 0; i < mActionListeners.Count(); i++)
      mActionListeners[i]->DidDeleteText(aElement, aOffset, aLength, result);
  }
  return result;
}


NS_IMETHODIMP nsEditor::CreateTxnForDeleteText(nsIDOMCharacterData *aElement,
                                               PRUint32             aOffset,
                                               PRUint32             aLength,
                                               DeleteTextTxn      **aTxn)
{
  if (!aElement)
    return NS_ERROR_NULL_POINTER;

  *aTxn = new DeleteTextTxn();
  if (!*aTxn)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*aTxn);
  return (*aTxn)->Init(this, aElement, aOffset, aLength, &mRangeUpdater);
}




NS_IMETHODIMP nsEditor::CreateTxnForSplitNode(nsIDOMNode *aNode,
                                         PRUint32    aOffset,
                                         SplitElementTxn **aTxn)
{
  if (!aNode)
    return NS_ERROR_NULL_POINTER;

  *aTxn = new SplitElementTxn();
  if (!*aTxn)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*aTxn);

  return (*aTxn)->Init(this, aNode, aOffset);
}

NS_IMETHODIMP nsEditor::CreateTxnForJoinNode(nsIDOMNode  *aLeftNode,
                                             nsIDOMNode  *aRightNode,
                                             JoinElementTxn **aTxn)
{
  if (!aLeftNode || !aRightNode)
    return NS_ERROR_NULL_POINTER;

  *aTxn = new JoinElementTxn();
  if (!*aTxn)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*aTxn);

  return (*aTxn)->Init(this, aLeftNode, aRightNode);
}




#ifdef XP_MAC
#pragma mark -
#pragma mark  nsEditor public static helper methods 
#pragma mark -
#endif



nsresult
nsEditor::SplitNodeImpl(nsIDOMNode * aExistingRightNode,
                        PRInt32      aOffset,
                        nsIDOMNode*  aNewLeftNode,
                        nsIDOMNode*  aParent)
{
#ifdef NS_DEBUG_EDITOR
  if (gNoisy) { printf("SplitNodeImpl: left=%p, right=%p, offset=%d\n", (void*)aNewLeftNode, (void*)aExistingRightNode, aOffset); }
#endif

  NS_ASSERTION(((nsnull!=aExistingRightNode) &&
                (nsnull!=aNewLeftNode) &&
                (nsnull!=aParent)),
                "null arg");
  nsresult result;
  if ((nsnull!=aExistingRightNode) &&
      (nsnull!=aNewLeftNode) &&
      (nsnull!=aParent))
  {
    
    nsCOMPtr<nsISelection> selection;
    result = GetSelection(getter_AddRefs(selection));
    if (NS_FAILED(result)) return result;
    if (!selection) return NS_ERROR_NULL_POINTER;

    
    nsCOMPtr<nsIDOMNode> selStartNode, selEndNode;
    PRInt32 selStartOffset, selEndOffset;
    result = GetStartNodeAndOffset(selection, address_of(selStartNode), &selStartOffset);
    if (NS_FAILED(result)) selStartNode = nsnull;  
    result = GetEndNodeAndOffset(selection, address_of(selEndNode), &selEndOffset);
    if (NS_FAILED(result)) selStartNode = nsnull;  

    nsCOMPtr<nsIDOMNode> resultNode;
    result = aParent->InsertBefore(aNewLeftNode, aExistingRightNode, getter_AddRefs(resultNode));
    
    if (NS_SUCCEEDED(result))
    {
      
      
      
      if (0<=aOffset) 
      {
        
        nsCOMPtr<nsIDOMCharacterData> rightNodeAsText( do_QueryInterface(aExistingRightNode) );
        nsCOMPtr<nsIDOMCharacterData> leftNodeAsText( do_QueryInterface(aNewLeftNode) );
        if (leftNodeAsText && rightNodeAsText)
        {
          
          nsAutoString leftText;
          rightNodeAsText->SubstringData(0, aOffset, leftText);
          rightNodeAsText->DeleteData(0, aOffset);
          
          leftNodeAsText->SetData(leftText);
          
        }
        else
        {  
           
          nsCOMPtr<nsIDOMNodeList> childNodes;
          result = aExistingRightNode->GetChildNodes(getter_AddRefs(childNodes));
          if ((NS_SUCCEEDED(result)) && (childNodes))
          {
            PRInt32 i=aOffset-1;
            for ( ; ((NS_SUCCEEDED(result)) && (0<=i)); i--)
            {
              nsCOMPtr<nsIDOMNode> childNode;
              result = childNodes->Item(i, getter_AddRefs(childNode));
              if ((NS_SUCCEEDED(result)) && (childNode))
              {
                result = aExistingRightNode->RemoveChild(childNode, getter_AddRefs(resultNode));
                
                if (NS_SUCCEEDED(result))
                {
                  nsCOMPtr<nsIDOMNode> firstChild;
                  aNewLeftNode->GetFirstChild(getter_AddRefs(firstChild));
                  result = aNewLeftNode->InsertBefore(childNode, firstChild, getter_AddRefs(resultNode));
                  
                }
              }
            }
          }        
        }
        
        if (GetShouldTxnSetSelection())
        {
          
          selection->Collapse(aNewLeftNode, aOffset);
        }
        else if (selStartNode)   
        {
          
          
          if (selStartNode.get() == aExistingRightNode)
          {
            if (selStartOffset < aOffset)
            {
              selStartNode = aNewLeftNode;
            }
            else
            {
              selStartOffset -= aOffset;
            }
          }
          if (selEndNode.get() == aExistingRightNode)
          {
            if (selEndOffset < aOffset)
            {
              selEndNode = aNewLeftNode;
            }
            else
            {
              selEndOffset -= aOffset;
            }
          }
          selection->Collapse(selStartNode,selStartOffset);
          selection->Extend(selEndNode,selEndOffset);
        }
      }
    }
  }
  else
    result = NS_ERROR_INVALID_ARG;

  return result;
}

nsresult
nsEditor::JoinNodesImpl(nsIDOMNode * aNodeToKeep,
                        nsIDOMNode * aNodeToJoin,
                        nsIDOMNode * aParent,
                        PRBool       aNodeToKeepIsFirst)
{
  NS_ASSERTION(aNodeToKeep && aNodeToJoin && aParent, "null arg");
  nsresult result;
  if (aNodeToKeep && aNodeToJoin && aParent)
  {
    
    nsCOMPtr<nsISelection> selection;
    GetSelection(getter_AddRefs(selection));
    if (!selection) return NS_ERROR_NULL_POINTER;

    
    nsCOMPtr<nsIDOMNode> selStartNode, selEndNode;
    PRInt32 selStartOffset, selEndOffset, joinOffset, keepOffset;
    result = GetStartNodeAndOffset(selection, address_of(selStartNode), &selStartOffset);
    if (NS_FAILED(result)) selStartNode = nsnull;
    result = GetEndNodeAndOffset(selection, address_of(selEndNode), &selEndOffset);
    
    if (NS_FAILED(result)) selStartNode = nsnull;

    nsCOMPtr<nsIDOMNode> leftNode;
    if (aNodeToKeepIsFirst)
      leftNode = aNodeToKeep;
    else
      leftNode = aNodeToJoin;

    PRUint32 firstNodeLength;
    result = GetLengthOfDOMNode(leftNode, firstNodeLength);
    if (NS_FAILED(result)) return result;
    nsCOMPtr<nsIDOMNode> parent;
    result = GetNodeLocation(aNodeToJoin, address_of(parent), &joinOffset);
    if (NS_FAILED(result)) return result;
    result = GetNodeLocation(aNodeToKeep, address_of(parent), &keepOffset);
    if (NS_FAILED(result)) return result;
    
    
    
    
    if (selStartNode)
    {
      if (selStartNode == parent)
      {
        if (aNodeToKeepIsFirst)
        {
          if ((selStartOffset > keepOffset) && (selStartOffset <= joinOffset))
          {
            selStartNode = aNodeToJoin; 
            selStartOffset = 0;
          }
        }
        else
        {
          if ((selStartOffset > joinOffset) && (selStartOffset <= keepOffset))
          {
            selStartNode = aNodeToJoin; 
            selStartOffset = firstNodeLength;
          }
        }
      }
      if (selEndNode == parent)
      {
        if (aNodeToKeepIsFirst)
        {
          if ((selEndOffset > keepOffset) && (selEndOffset <= joinOffset))
          {
            selEndNode = aNodeToJoin; 
            selEndOffset = 0;
          }
        }
        else
        {
          if ((selEndOffset > joinOffset) && (selEndOffset <= keepOffset))
          {
            selEndNode = aNodeToJoin; 
            selEndOffset = firstNodeLength;
          }
        }
      }
    }
    
    
    nsCOMPtr<nsIDOMCharacterData> keepNodeAsText( do_QueryInterface(aNodeToKeep) );
    nsCOMPtr<nsIDOMCharacterData> joinNodeAsText( do_QueryInterface(aNodeToJoin) );
    if (keepNodeAsText && joinNodeAsText)
    {
      nsAutoString rightText;
      nsAutoString leftText;
      if (aNodeToKeepIsFirst)
      {
        keepNodeAsText->GetData(leftText);
        joinNodeAsText->GetData(rightText);
      }
      else
      {
        keepNodeAsText->GetData(rightText);
        joinNodeAsText->GetData(leftText);
      }
      leftText += rightText;
      keepNodeAsText->SetData(leftText);          
    }
    else
    {  
      nsCOMPtr<nsIDOMNodeList> childNodes;
      result = aNodeToJoin->GetChildNodes(getter_AddRefs(childNodes));
      if ((NS_SUCCEEDED(result)) && (childNodes))
      {
        PRInt32 i;  
        PRUint32 childCount=0;
        nsCOMPtr<nsIDOMNode> firstNode; 
        childNodes->GetLength(&childCount);
        if (!aNodeToKeepIsFirst)
        { 
          result = aNodeToKeep->GetFirstChild(getter_AddRefs(firstNode));  
          
        }
        nsCOMPtr<nsIDOMNode> resultNode;
        
        nsCOMPtr<nsIDOMNode> previousChild;
        for (i=childCount-1; ((NS_SUCCEEDED(result)) && (0<=i)); i--)
        {
          nsCOMPtr<nsIDOMNode> childNode;
          result = childNodes->Item(i, getter_AddRefs(childNode));
          if ((NS_SUCCEEDED(result)) && (childNode))
          {
            if (aNodeToKeepIsFirst)
            { 
              
              result = aNodeToKeep->InsertBefore(childNode, previousChild, getter_AddRefs(resultNode));
              previousChild = do_QueryInterface(childNode);
            }
            else
            { 
              result = aNodeToKeep->InsertBefore(childNode, firstNode, getter_AddRefs(resultNode));
              firstNode = do_QueryInterface(childNode);
            }
          }
        }
      }
      else if (!childNodes) {
        result = NS_ERROR_NULL_POINTER;
      }
    }
    if (NS_SUCCEEDED(result))
    { 
      nsCOMPtr<nsIDOMNode> resultNode;
      result = aParent->RemoveChild(aNodeToJoin, getter_AddRefs(resultNode));
      
      if (GetShouldTxnSetSelection())
      {
        
        selection->Collapse(aNodeToKeep, firstNodeLength);
      }
      else if (selStartNode)
      {
        
        
        PRBool bNeedToAdjust = PR_FALSE;
        
        
        if (selStartNode.get() == aNodeToJoin)
        {
          bNeedToAdjust = PR_TRUE;
          selStartNode = aNodeToKeep;
          if (aNodeToKeepIsFirst)
          {
            selStartOffset += firstNodeLength;
          }
        }
        else if ((selStartNode.get() == aNodeToKeep) && !aNodeToKeepIsFirst)
        {
          bNeedToAdjust = PR_TRUE;
          selStartOffset += firstNodeLength;
        }
                
        
        if (selEndNode.get() == aNodeToJoin)
        {
          bNeedToAdjust = PR_TRUE;
          selEndNode = aNodeToKeep;
          if (aNodeToKeepIsFirst)
          {
            selEndOffset += firstNodeLength;
          }
        }
        else if ((selEndNode.get() == aNodeToKeep) && !aNodeToKeepIsFirst)
        {
          bNeedToAdjust = PR_TRUE;
          selEndOffset += firstNodeLength;
        }
        
        
        if (bNeedToAdjust)
        {
          selection->Collapse(selStartNode,selStartOffset);
          selection->Extend(selEndNode,selEndOffset);          
        }
      }
    }
  }
  else
    result = NS_ERROR_INVALID_ARG;

  return result;
}


nsresult 
nsEditor::GetChildOffset(nsIDOMNode *aChild, nsIDOMNode *aParent, PRInt32 &aOffset)
{
  NS_ASSERTION((aChild && aParent), "bad args");

  nsCOMPtr<nsIContent> content = do_QueryInterface(aParent);
  nsCOMPtr<nsIContent> cChild = do_QueryInterface(aChild);
  if (!cChild || !content)
    return NS_ERROR_NULL_POINTER;

  aOffset = content->IndexOf(cChild);

  return NS_OK;
}

nsresult 
nsEditor::GetNodeLocation(nsIDOMNode *inChild, nsCOMPtr<nsIDOMNode> *outParent, PRInt32 *outOffset)
{
  NS_ASSERTION((inChild && outParent && outOffset), "bad args");
  nsresult result = NS_ERROR_NULL_POINTER;
  if (inChild && outParent && outOffset)
  {
    result = inChild->GetParentNode(getter_AddRefs(*outParent));
    if ((NS_SUCCEEDED(result)) && (*outParent))
    {
      result = GetChildOffset(inChild, *outParent, *outOffset);
    }
  }
  return result;
}



nsresult
nsEditor::GetLengthOfDOMNode(nsIDOMNode *aNode, PRUint32 &aCount) 
{
  aCount = 0;
  if (!aNode) { return NS_ERROR_NULL_POINTER; }
  nsresult result=NS_OK;
  nsCOMPtr<nsIDOMCharacterData>nodeAsChar = do_QueryInterface(aNode);
  if (nodeAsChar) {
    nodeAsChar->GetLength(&aCount);
  }
  else
  {
    PRBool hasChildNodes;
    aNode->HasChildNodes(&hasChildNodes);
    if (hasChildNodes)
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
nsEditor::GetPriorNode(nsIDOMNode  *aParentNode, 
                       PRInt32      aOffset, 
                       PRBool       aEditableNode, 
                       nsCOMPtr<nsIDOMNode> *aResultNode,
                       PRBool       bNoBlockCrossing)
{
  
  
  if (!aParentNode || !aResultNode) { return NS_ERROR_NULL_POINTER; }
  *aResultNode = nsnull;
  
  
  if (!aOffset || IsTextNode(aParentNode))
  {
    if (bNoBlockCrossing && IsBlockNode(aParentNode))
    {
      
      return NS_OK;
    }
    return GetPriorNode(aParentNode, aEditableNode, aResultNode, bNoBlockCrossing);
  }

  
  nsCOMPtr<nsIDOMNode> child = GetChildAt(aParentNode, aOffset);
  if (child)
    return GetPriorNode(child, aEditableNode, aResultNode, bNoBlockCrossing);

  
  
  *aResultNode = GetRightmostChild(aParentNode, bNoBlockCrossing);
  if (!*aResultNode || !aEditableNode || IsEditable(*aResultNode))
    return NS_OK;

  
  nsCOMPtr<nsIDOMNode> notEditableNode = do_QueryInterface(*aResultNode);
  return GetPriorNode(notEditableNode, aEditableNode, aResultNode, bNoBlockCrossing);
}


nsresult 
nsEditor::GetNextNode(nsIDOMNode   *aParentNode, 
                       PRInt32      aOffset, 
                       PRBool       aEditableNode, 
                       nsCOMPtr<nsIDOMNode> *aResultNode,
                       PRBool       bNoBlockCrossing)
{
  
  
  if (!aParentNode || !aResultNode) { return NS_ERROR_NULL_POINTER; }
  
  *aResultNode = nsnull;

  
  if (IsTextNode(aParentNode))
  {
    nsCOMPtr<nsIDOMNode> parent;
    nsEditor::GetNodeLocation(aParentNode, address_of(parent), &aOffset);
    aParentNode = parent;
    aOffset++;  
  }
  
  nsCOMPtr<nsIDOMNode> child = GetChildAt(aParentNode, aOffset);
  if (child)
  {
    if (bNoBlockCrossing && IsBlockNode(child))
    {
      *aResultNode = child;  
      return NS_OK;
    }
    *aResultNode = GetLeftmostChild(child, bNoBlockCrossing);
    if (!*aResultNode) 
    {
      *aResultNode = child;
      return NS_OK;
    }
    if (!IsDescendantOfBody(*aResultNode))
    {
      *aResultNode = nsnull;
      return NS_OK;
    }

    if (!aEditableNode || IsEditable(*aResultNode))
      return NS_OK;

    
    nsCOMPtr<nsIDOMNode> notEditableNode = do_QueryInterface(*aResultNode);
    return GetNextNode(notEditableNode, aEditableNode, aResultNode, bNoBlockCrossing);
  }
    
  
  
  if (bNoBlockCrossing && IsBlockNode(aParentNode))
  {
    
    return NS_OK;
  }
  return GetNextNode(aParentNode, aEditableNode, aResultNode, bNoBlockCrossing);
}


nsresult 
nsEditor::GetPriorNode(nsIDOMNode  *aCurrentNode, 
                       PRBool       aEditableNode, 
                       nsCOMPtr<nsIDOMNode> *aResultNode,
                       PRBool       bNoBlockCrossing)
{
  nsresult result;
  if (!aCurrentNode || !aResultNode) { return NS_ERROR_NULL_POINTER; }
  
  *aResultNode = nsnull;  

  if (IsRootNode(aCurrentNode))
  {
    
    
    

    return NS_OK;
  }

  nsCOMPtr<nsIDOMNode> candidate;
  result = GetPriorNodeImpl(aCurrentNode, aEditableNode, address_of(candidate), bNoBlockCrossing);
  if (NS_FAILED(result)) return result;
  
  if (!candidate)
  {
    
    return NS_OK;
  }
  else if (!aEditableNode) *aResultNode = candidate;
  else if (IsEditable(candidate)) *aResultNode = candidate;
  else 
  { 
    nsCOMPtr<nsIDOMNode> notEditableNode = do_QueryInterface(candidate);
    return GetPriorNode(notEditableNode, aEditableNode, aResultNode, bNoBlockCrossing);
  }
  return result;
}

nsresult 
nsEditor::GetPriorNodeImpl(nsIDOMNode  *aCurrentNode, 
                           PRBool       aEditableNode, 
                           nsCOMPtr<nsIDOMNode> *aResultNode,
                           PRBool       bNoBlockCrossing)
{
  

  
  nsCOMPtr<nsIDOMNode> prevSibling;
  nsresult result = aCurrentNode->GetPreviousSibling(getter_AddRefs(prevSibling));
  if ((NS_SUCCEEDED(result)) && prevSibling)
  {
    if (bNoBlockCrossing && IsBlockNode(prevSibling))
    {
      
      *aResultNode = prevSibling;
      return NS_OK;
    }
    *aResultNode = GetRightmostChild(prevSibling, bNoBlockCrossing);
    if (!*aResultNode) 
    { 
      *aResultNode = prevSibling;
      return NS_OK;
    }
    if (!IsDescendantOfBody(*aResultNode))
    {
      *aResultNode = nsnull;
      return NS_OK;
    }
  }
  else
  {
    
    
    nsCOMPtr<nsIDOMNode> parent = do_QueryInterface(aCurrentNode);
    nsCOMPtr<nsIDOMNode> node, notEditableNode;
    do {
      node = parent;
      result = node->GetParentNode(getter_AddRefs(parent));
      if ((NS_SUCCEEDED(result)) && parent)
      {
        if (!IsDescendantOfBody(parent))
        {
          *aResultNode = nsnull;
          return NS_OK;
        }
        if ((bNoBlockCrossing && IsBlockNode(parent)) || IsRootNode(parent))
        {
          
          *aResultNode = nsnull;
          return NS_OK;
        }
        result = parent->GetPreviousSibling(getter_AddRefs(node));
        if ((NS_SUCCEEDED(result)) && node)
        {
          if (bNoBlockCrossing && IsBlockNode(node))
          {
            
            *aResultNode = node;
            return NS_OK;
          }
          *aResultNode = GetRightmostChild(node, bNoBlockCrossing);
          if (!*aResultNode)  *aResultNode = node;
          return NS_OK;
        }
      }
    } while ((NS_SUCCEEDED(result)) && parent && !*aResultNode);
  }
  return result;
}

nsresult 
nsEditor::GetNextNode(nsIDOMNode  *aCurrentNode, 
                      PRBool       aEditableNode, 
                      nsCOMPtr<nsIDOMNode> *aResultNode,
                      PRBool       bNoBlockCrossing)
{
  if (!aCurrentNode || !aResultNode) { return NS_ERROR_NULL_POINTER; }
  
  *aResultNode = nsnull;  

  if (IsRootNode(aCurrentNode))
  {
    
    
    

    return NS_OK;
  }

  nsCOMPtr<nsIDOMNode> candidate;
  nsresult result = GetNextNodeImpl(aCurrentNode, aEditableNode,
                                    address_of(candidate), bNoBlockCrossing);
  if (NS_FAILED(result)) return result;
  
  if (!candidate)
  {
    
    *aResultNode = nsnull;
    return NS_OK;
  }
  else if (!aEditableNode) *aResultNode = candidate;
  else if (IsEditable(candidate)) *aResultNode = candidate;
  else 
  { 
    nsCOMPtr<nsIDOMNode> notEditableNode = do_QueryInterface(candidate);
    return GetNextNode(notEditableNode, aEditableNode, aResultNode, bNoBlockCrossing);
  }
  return result;
}


nsresult 
nsEditor::GetNextNodeImpl(nsIDOMNode  *aCurrentNode, 
                          PRBool       aEditableNode, 
                          nsCOMPtr<nsIDOMNode> *aResultNode,
                          PRBool       bNoBlockCrossing)
{
  

  
  nsCOMPtr<nsIDOMNode> nextSibling;
  nsresult result = aCurrentNode->GetNextSibling(getter_AddRefs(nextSibling));
  if ((NS_SUCCEEDED(result)) && nextSibling)
  {
    if (bNoBlockCrossing && IsBlockNode(nextSibling))
    {
      
      *aResultNode = nextSibling;
      return NS_OK;
    }
    *aResultNode = GetLeftmostChild(nextSibling, bNoBlockCrossing);
    if (!*aResultNode)
    { 
      *aResultNode = nextSibling;
      return NS_OK; 
    }
    if (!IsDescendantOfBody(*aResultNode))
    {
      *aResultNode = nsnull;
      return NS_OK;
    }
  }
  else
  {
    
    
    nsCOMPtr<nsIDOMNode> parent(do_QueryInterface(aCurrentNode));
    nsCOMPtr<nsIDOMNode> node, notEditableNode;
    do {
      node = parent;
      result = node->GetParentNode(getter_AddRefs(parent));
      if ((NS_SUCCEEDED(result)) && parent)
      {
        if (!IsDescendantOfBody(parent))
        {
          *aResultNode = nsnull;
          return NS_OK;
        }
        if ((bNoBlockCrossing && IsBlockNode(parent)) || IsRootNode(parent))
        {
          
          *aResultNode = nsnull;
          return NS_OK;
        }
        result = parent->GetNextSibling(getter_AddRefs(node));
        if ((NS_SUCCEEDED(result)) && node)
        {
          if (bNoBlockCrossing && IsBlockNode(node))
          {
            
            *aResultNode = node;
            return NS_OK;
          }
          *aResultNode = GetLeftmostChild(node, bNoBlockCrossing);
          if (!*aResultNode) *aResultNode = node;
          return NS_OK; 
        }
      }
    } while ((NS_SUCCEEDED(result)) && parent);
  }
  return result;
}


nsCOMPtr<nsIDOMNode>
nsEditor::GetRightmostChild(nsIDOMNode *aCurrentNode, 
                            PRBool bNoBlockCrossing)
{
  if (!aCurrentNode) return nsnull;
  nsCOMPtr<nsIDOMNode> resultNode, temp=aCurrentNode;
  PRBool hasChildren;
  aCurrentNode->HasChildNodes(&hasChildren);
  while (hasChildren)
  {
    temp->GetLastChild(getter_AddRefs(resultNode));
    if (resultNode)
    {
      if (bNoBlockCrossing && IsBlockNode(resultNode))
         return resultNode;
      resultNode->HasChildNodes(&hasChildren);
      temp = resultNode;
    }
    else 
      hasChildren = PR_FALSE;
  }

  return resultNode;
}

nsCOMPtr<nsIDOMNode>
nsEditor::GetLeftmostChild(nsIDOMNode *aCurrentNode,
                           PRBool bNoBlockCrossing)
{
  if (!aCurrentNode) return nsnull;
  nsCOMPtr<nsIDOMNode> resultNode, temp=aCurrentNode;
  PRBool hasChildren;
  aCurrentNode->HasChildNodes(&hasChildren);
  while (hasChildren)
  {
    temp->GetFirstChild(getter_AddRefs(resultNode));
    if (resultNode)
    {
      if (bNoBlockCrossing && IsBlockNode(resultNode))
         return resultNode;
      resultNode->HasChildNodes(&hasChildren);
      temp = resultNode;
    }
    else 
      hasChildren = PR_FALSE;
  }

  return resultNode;
}

PRBool 
nsEditor::IsBlockNode(nsIDOMNode *aNode)
{
  
  
  
  
  return PR_FALSE;
}

PRBool 
nsEditor::CanContainTag(nsIDOMNode* aParent, const nsAString &aChildTag)
{
  nsCOMPtr<nsIDOMElement> parentElement = do_QueryInterface(aParent);
  if (!parentElement) return PR_FALSE;
  
  nsAutoString parentStringTag;
  parentElement->GetTagName(parentStringTag);
  return TagCanContainTag(parentStringTag, aChildTag);
}

PRBool 
nsEditor::TagCanContain(const nsAString &aParentTag, nsIDOMNode* aChild)
{
  nsAutoString childStringTag;
  
  if (IsTextNode(aChild)) 
  {
    childStringTag.AssignLiteral("#text");
  }
  else
  {
    nsCOMPtr<nsIDOMElement> childElement = do_QueryInterface(aChild);
    if (!childElement) return PR_FALSE;
    childElement->GetTagName(childStringTag);
  }
  return TagCanContainTag(aParentTag, childStringTag);
}

PRBool 
nsEditor::TagCanContainTag(const nsAString &aParentTag, const nsAString &aChildTag)
{
  return PR_TRUE;
}

PRBool 
nsEditor::IsRootNode(nsIDOMNode *inNode) 
{
  if (!inNode)
    return PR_FALSE;

  nsIDOMElement *rootElement = GetRoot();

  nsCOMPtr<nsIDOMNode> rootNode = do_QueryInterface(rootElement);

  return inNode == rootNode;
}

PRBool 
nsEditor::IsDescendantOfBody(nsIDOMNode *inNode) 
{
  if (!inNode) return PR_FALSE;
  nsIDOMElement *rootElement = GetRoot();
  if (!rootElement) return PR_FALSE;
  nsCOMPtr<nsIDOMNode> root = do_QueryInterface(rootElement);

  if (inNode == root.get()) return PR_TRUE;
  
  nsCOMPtr<nsIDOMNode> parent, node = do_QueryInterface(inNode);
  
  do
  {
    node->GetParentNode(getter_AddRefs(parent));
    if (parent == root) return PR_TRUE;
    node = parent;
  } while (parent);
  
  return PR_FALSE;
}

PRBool 
nsEditor::IsContainer(nsIDOMNode *aNode)
{
  return aNode ? PR_TRUE : PR_FALSE;
}

PRBool
nsEditor::IsTextInDirtyFrameVisible(nsIDOMNode *aNode)
{
  
  
  
  

  return PR_TRUE;
}

PRBool 
nsEditor::IsEditable(nsIDOMNode *aNode)
{
  if (!aNode) return PR_FALSE;
  nsCOMPtr<nsIPresShell> shell;
  GetPresShell(getter_AddRefs(shell));
  if (!shell)  return PR_FALSE;

  if (IsMozEditorBogusNode(aNode) || !IsModifiableNode(aNode)) return PR_FALSE;

  
  
  nsCOMPtr<nsIContent> content = do_QueryInterface(aNode);
  if (content)
  {
    nsIFrame *resultFrame = shell->GetPrimaryFrameFor(content);
    if (!resultFrame)   
      return PR_FALSE;
    NS_ASSERTION(content->IsNodeOfType(nsINode::eTEXT) ||
                 content->IsNodeOfType(nsINode::eELEMENT),
                 "frame for non element-or-text?");
    if (!content->IsNodeOfType(nsINode::eTEXT))
      return PR_TRUE;  

    
    while (resultFrame) {
      if (resultFrame->GetStateBits() & NS_FRAME_IS_DIRTY) 
      {
        
        
        
        
        
        return IsTextInDirtyFrameVisible(aNode);
      }
      if (resultFrame->GetSize().width > 0) 
        return PR_TRUE;  
      resultFrame = resultFrame->GetNextContinuation();
    }
  }
  return PR_FALSE;  
}

PRBool
nsEditor::IsMozEditorBogusNode(nsIDOMNode *aNode)
{
  if (!aNode)
    return PR_FALSE;

  nsCOMPtr<nsIDOMElement>element = do_QueryInterface(aNode);
  if (element)
  {
    nsAutoString val;
    (void)element->GetAttribute(kMOZEditorBogusNodeAttr, val);
    if (val.Equals(kMOZEditorBogusNodeValue)) {
      return PR_TRUE;
    }
  }
    
  return PR_FALSE;
}

nsresult
nsEditor::CountEditableChildren(nsIDOMNode *aNode, PRUint32 &outCount) 
{
  outCount = 0;
  if (!aNode) { return NS_ERROR_NULL_POINTER; }
  nsresult res=NS_OK;
  PRBool hasChildNodes;
  aNode->HasChildNodes(&hasChildNodes);
  if (hasChildNodes)
  {
    nsCOMPtr<nsIDOMNodeList>nodeList;
    res = aNode->GetChildNodes(getter_AddRefs(nodeList));
    if (NS_SUCCEEDED(res) && nodeList) 
    {
      PRUint32 i;
      PRUint32 len;
      nodeList->GetLength(&len);
      for (i=0 ; i<len; i++)
      {
        nsCOMPtr<nsIDOMNode> child;
        res = nodeList->Item((PRInt32)i, getter_AddRefs(child));
        if ((NS_SUCCEEDED(res)) && (child))
        {
          if (IsEditable(child))
          {
            outCount++;
          }
        }
      }
    }
    else if (!nodeList)
      res = NS_ERROR_NULL_POINTER;
  }
  return res;
}




NS_IMETHODIMP nsEditor::IncrementModificationCount(PRInt32 inNumMods)
{
  PRUint32 oldModCount = mModCount;

  mModCount += inNumMods;

  if ((oldModCount == 0 && mModCount != 0)
   || (oldModCount != 0 && mModCount == 0))
    NotifyDocumentListeners(eDocumentStateChanged);
  return NS_OK;
}


NS_IMETHODIMP nsEditor::GetModificationCount(PRInt32 *outModCount)
{
  NS_ENSURE_ARG_POINTER(outModCount);
  *outModCount = mModCount;
  return NS_OK;
}


NS_IMETHODIMP nsEditor::ResetModificationCount()
{
  PRBool doNotify = (mModCount != 0);

  mModCount = 0;

  if (doNotify)
    NotifyDocumentListeners(eDocumentStateChanged);
  return NS_OK;
}








nsIAtom *
nsEditor::GetTag(nsIDOMNode *aNode)
{
  nsCOMPtr<nsIContent> content = do_QueryInterface(aNode);

  if (!content) 
  {
    NS_ASSERTION(aNode, "null node passed to nsEditor::Tag()");

    return nsnull;
  }
  
  return content->Tag();
}





nsresult 
nsEditor::GetTagString(nsIDOMNode *aNode, nsAString& outString)
{
  if (!aNode) 
  {
    NS_NOTREACHED("null node passed to nsEditor::GetTag()");
    return NS_ERROR_NULL_POINTER;
  }
  
  nsIAtom *atom = GetTag(aNode);
  if (!atom)
  {
    return NS_ERROR_FAILURE;
  }

  atom->ToString(outString);
  return NS_OK;
}





PRBool 
nsEditor::NodesSameType(nsIDOMNode *aNode1, nsIDOMNode *aNode2)
{
  if (!aNode1 || !aNode2) 
  {
    NS_NOTREACHED("null node passed to nsEditor::NodesSameType()");
    return PR_FALSE;
  }
  
  return GetTag(aNode1) == GetTag(aNode2);
}




PRBool
nsEditor::IsTextOrElementNode(nsIDOMNode *aNode)
{
  if (!aNode)
  {
    NS_NOTREACHED("null node passed to IsTextOrElementNode()");
    return PR_FALSE;
  }
  
  PRUint16 nodeType;
  aNode->GetNodeType(&nodeType);
  return ((nodeType == nsIDOMNode::ELEMENT_NODE) || (nodeType == nsIDOMNode::TEXT_NODE));
}






PRBool
nsEditor::IsTextNode(nsIDOMNode *aNode)
{
  if (!aNode)
  {
    NS_NOTREACHED("null node passed to IsTextNode()");
    return PR_FALSE;
  }
  
  PRUint16 nodeType;
  aNode->GetNodeType(&nodeType);
  return (nodeType == nsIDOMNode::TEXT_NODE);
}





PRInt32 
nsEditor::GetIndexOf(nsIDOMNode *parent, nsIDOMNode *child)
{
  nsCOMPtr<nsIContent> content = do_QueryInterface(parent);
  nsCOMPtr<nsIContent> cChild = do_QueryInterface(child);
  NS_PRECONDITION(content, "null content in nsEditor::GetIndexOf");
  NS_PRECONDITION(cChild, "null content in nsEditor::GetIndexOf");

  return content->IndexOf(cChild);
}
  




nsCOMPtr<nsIDOMNode> 
nsEditor::GetChildAt(nsIDOMNode *aParent, PRInt32 aOffset)
{
  nsCOMPtr<nsIDOMNode> resultNode;
  
  nsCOMPtr<nsIContent> parent = do_QueryInterface(aParent);

  if (!parent) 
    return resultNode;

  resultNode = do_QueryInterface(parent->GetChildAt(aOffset));

  return resultNode;
}
  





nsresult 
nsEditor::GetStartNodeAndOffset(nsISelection *aSelection,
                                       nsCOMPtr<nsIDOMNode> *outStartNode,
                                       PRInt32 *outStartOffset)
{
  if (!outStartNode || !outStartOffset || !aSelection) 
    return NS_ERROR_NULL_POINTER;

  

  nsCOMPtr<nsISelectionPrivate>selPrivate(do_QueryInterface(aSelection));
  nsCOMPtr<nsIEnumerator> enumerator;
  nsresult result = selPrivate->GetEnumerator(getter_AddRefs(enumerator));
  if (NS_FAILED(result) || !enumerator)
    return NS_ERROR_FAILURE;
    
  enumerator->First(); 
  nsCOMPtr<nsISupports> currentItem;
  if (NS_FAILED(enumerator->CurrentItem(getter_AddRefs(currentItem))))
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMRange> range( do_QueryInterface(currentItem) );
  if (!range)
    return NS_ERROR_FAILURE;
    
  if (NS_FAILED(range->GetStartContainer(getter_AddRefs(*outStartNode))))
    return NS_ERROR_FAILURE;
    
  if (NS_FAILED(range->GetStartOffset(outStartOffset)))
    return NS_ERROR_FAILURE;
    
  return NS_OK;
}





nsresult 
nsEditor::GetEndNodeAndOffset(nsISelection *aSelection,
                                       nsCOMPtr<nsIDOMNode> *outEndNode,
                                       PRInt32 *outEndOffset)
{
  if (!outEndNode || !outEndOffset) 
    return NS_ERROR_NULL_POINTER;
    
  nsCOMPtr<nsISelectionPrivate>selPrivate(do_QueryInterface(aSelection));
  nsCOMPtr<nsIEnumerator> enumerator;
  nsresult result = selPrivate->GetEnumerator(getter_AddRefs(enumerator));
  if (NS_FAILED(result) || !enumerator)
    return NS_ERROR_FAILURE;
    
  enumerator->First(); 
  nsCOMPtr<nsISupports> currentItem;
  if (NS_FAILED(enumerator->CurrentItem(getter_AddRefs(currentItem))))
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMRange> range( do_QueryInterface(currentItem) );
  if (!range)
    return NS_ERROR_FAILURE;
    
  if (NS_FAILED(range->GetEndContainer(getter_AddRefs(*outEndNode))))
    return NS_ERROR_FAILURE;
    
  if (NS_FAILED(range->GetEndOffset(outEndOffset)))
    return NS_ERROR_FAILURE;
    
  return NS_OK;
}





nsresult 
nsEditor::IsPreformatted(nsIDOMNode *aNode, PRBool *aResult)
{
  nsCOMPtr<nsIContent> content = do_QueryInterface(aNode);
  
  if (!aResult || !content) return NS_ERROR_NULL_POINTER;
  
  nsCOMPtr<nsIPresShell> ps = do_QueryReferent(mPresShellWeak);
  if (!ps) return NS_ERROR_NOT_INITIALIZED;
  
  nsIFrame *frame = ps->GetPrimaryFrameFor(content);

  NS_ASSERTION(frame, "no frame, see bug #188946");
  if (!frame)
  {
    
    
    
    *aResult = PR_FALSE;
    return NS_OK;
  }

  const nsStyleText* styleText = frame->GetStyleText();

  *aResult = styleText->WhiteSpaceIsSignificant();
  return NS_OK;
}












nsresult
nsEditor::SplitNodeDeep(nsIDOMNode *aNode, 
                        nsIDOMNode *aSplitPointParent, 
                        PRInt32 aSplitPointOffset,
                        PRInt32 *outOffset,
                        PRBool  aNoEmptyContainers,
                        nsCOMPtr<nsIDOMNode> *outLeftNode,
                        nsCOMPtr<nsIDOMNode> *outRightNode)
{
  if (!aNode || !aSplitPointParent || !outOffset) return NS_ERROR_NULL_POINTER;
  nsCOMPtr<nsIDOMNode> tempNode, parentNode;  
  PRInt32 offset = aSplitPointOffset;
  nsresult res;
  
  if (outLeftNode)  *outLeftNode  = nsnull;
  if (outRightNode) *outRightNode = nsnull;
  
  nsCOMPtr<nsIDOMNode> nodeToSplit = do_QueryInterface(aSplitPointParent);
  while (nodeToSplit)
  {
    
    
    
    
    
    
    nsCOMPtr<nsIDOMCharacterData> nodeAsText = do_QueryInterface(nodeToSplit);
    PRUint32 len;
    PRBool bDoSplit = PR_FALSE;
    res = GetLengthOfDOMNode(nodeToSplit, len);
    if (NS_FAILED(res)) return res;
    
    if (!(aNoEmptyContainers || nodeAsText) || (offset && (offset != (PRInt32)len)))
    {
      bDoSplit = PR_TRUE;
      res = SplitNode(nodeToSplit, offset, getter_AddRefs(tempNode));
      if (NS_FAILED(res)) return res;
      if (outRightNode) *outRightNode = nodeToSplit;
      if (outLeftNode)  *outLeftNode  = tempNode;
    }

    res = nodeToSplit->GetParentNode(getter_AddRefs(parentNode));
    if (NS_FAILED(res)) return res;
    if (!parentNode) return NS_ERROR_FAILURE;

    if (!bDoSplit && offset)  
    {
      offset = GetIndexOf(parentNode, nodeToSplit) +1;
      if (outLeftNode)  *outLeftNode  = nodeToSplit;
    }
    else
    {
      offset = GetIndexOf(parentNode, nodeToSplit);
      if (outRightNode) *outRightNode = nodeToSplit;
    }
    
    if (nodeToSplit.get() == aNode)  
      break;
      
    nodeToSplit = parentNode;
  }
  
  if (!nodeToSplit)
  {
    NS_NOTREACHED("null node obtained in nsEditor::SplitNodeDeep()");
    return NS_ERROR_FAILURE;
  }
  
  *outOffset = offset;
  
  return NS_OK;
}





nsresult
nsEditor::JoinNodeDeep(nsIDOMNode *aLeftNode, 
                       nsIDOMNode *aRightNode,
                       nsCOMPtr<nsIDOMNode> *aOutJoinNode, 
                       PRInt32 *outOffset)
{
  if (!aLeftNode || !aRightNode || !aOutJoinNode || !outOffset) return NS_ERROR_NULL_POINTER;

  
  
  
  
  nsCOMPtr<nsIDOMNode> leftNodeToJoin = do_QueryInterface(aLeftNode);
  nsCOMPtr<nsIDOMNode> rightNodeToJoin = do_QueryInterface(aRightNode);
  nsCOMPtr<nsIDOMNode> parentNode,tmp;
  nsresult res = NS_OK;
  
  rightNodeToJoin->GetParentNode(getter_AddRefs(parentNode));
  
  while (leftNodeToJoin && rightNodeToJoin && parentNode &&
          NodesSameType(leftNodeToJoin, rightNodeToJoin))
  {
    
    PRUint32 length;
    if (IsTextNode(leftNodeToJoin))
    {
      nsCOMPtr<nsIDOMCharacterData>nodeAsText;
      nodeAsText = do_QueryInterface(leftNodeToJoin);
      nodeAsText->GetLength(&length);
    }
    else
    {
      res = GetLengthOfDOMNode(leftNodeToJoin, length);
      if (NS_FAILED(res)) return res;
    }
    
    *aOutJoinNode = rightNodeToJoin;
    *outOffset = length;
    
    
    res = JoinNodes(leftNodeToJoin, rightNodeToJoin, parentNode);
    if (NS_FAILED(res)) return res;
    
    if (IsTextNode(parentNode)) 
      return NS_OK;

    else
    {
      
      parentNode = rightNodeToJoin;
      leftNodeToJoin = GetChildAt(parentNode, length-1);
      rightNodeToJoin = GetChildAt(parentNode, length);

      
      while (leftNodeToJoin && !IsEditable(leftNodeToJoin))
      {
        leftNodeToJoin->GetPreviousSibling(getter_AddRefs(tmp));
        leftNodeToJoin = tmp;
      }
      if (!leftNodeToJoin) break;
    
      while (rightNodeToJoin && !IsEditable(rightNodeToJoin))
      {
        rightNodeToJoin->GetNextSibling(getter_AddRefs(tmp));
        rightNodeToJoin = tmp;
      }
      if (!rightNodeToJoin) break;
    }
  }
  
  return res;
}

nsresult nsEditor::BeginUpdateViewBatch()
{
  NS_PRECONDITION(mUpdateCount >= 0, "bad state");


  if (0 == mUpdateCount)
  {
    

    nsCOMPtr<nsISelection> selection;
    GetSelection(getter_AddRefs(selection));

    if (selection) 
    {
      nsCOMPtr<nsISelectionPrivate> selPrivate(do_QueryInterface(selection));
      selPrivate->StartBatchChanges();
    }

    
    mBatch.BeginUpdateViewBatch(mViewManager);
  }

  mUpdateCount++;

  return NS_OK;
}


nsresult nsEditor::EndUpdateViewBatch()
{
  NS_PRECONDITION(mUpdateCount > 0, "bad state");
  
  if (mUpdateCount <= 0)
  {
    mUpdateCount = 0;
    return NS_ERROR_FAILURE;
  }

  mUpdateCount--;

  if (0 == mUpdateCount)
  {
    
    
    
    

    nsRefPtr<nsCaret> caret;
    nsCOMPtr<nsIPresShell> presShell;
    GetPresShell(getter_AddRefs(presShell));

    if (presShell)
      presShell->GetCaret(getter_AddRefs(caret));

    StCaretHider caretHider(caret);
        
    PRUint32 flags = 0;

    GetFlags(&flags);

    
    if (mViewManager)
    {
      PRUint32 updateFlag = NS_VMREFRESH_IMMEDIATE;

      
      
      if (flags & nsIPlaintextEditor::eEditorUseAsyncUpdatesMask) {
        updateFlag = NS_VMREFRESH_DEFERRED;
      } else if (presShell) {
        
        
        
        presShell->FlushPendingNotifications(Flush_Layout);
      }
      mBatch.EndUpdateViewBatch(updateFlag);
    }

    

    nsCOMPtr<nsISelection>selection;
    GetSelection(getter_AddRefs(selection));

    if (selection) {
      nsCOMPtr<nsISelectionPrivate>selPrivate(do_QueryInterface(selection));
      selPrivate->EndBatchChanges();
    }
  }

  return NS_OK;
}

PRBool 
nsEditor::GetShouldTxnSetSelection()
{
  return mShouldTxnSetSelection;
}


#ifdef XP_MAC
#pragma mark -
#pragma mark  protected nsEditor methods 
#pragma mark -
#endif


NS_IMETHODIMP 
nsEditor::DeleteSelectionImpl(nsIEditor::EDirection aAction)
{
  nsCOMPtr<nsISelection>selection;
  nsresult res = GetSelection(getter_AddRefs(selection));
  if (NS_FAILED(res)) return res;
  nsRefPtr<EditAggregateTxn> txn;
  nsCOMPtr<nsIDOMNode> deleteNode;
  PRInt32 deleteCharOffset = 0, deleteCharLength = 0;
  res = CreateTxnForDeleteSelection(aAction, getter_AddRefs(txn),
                                    getter_AddRefs(deleteNode),
                                    &deleteCharOffset, &deleteCharLength);
  nsCOMPtr<nsIDOMCharacterData> deleteCharData(do_QueryInterface(deleteNode));

  if (NS_SUCCEEDED(res))  
  {
    nsAutoRules beginRulesSniffing(this, kOpDeleteSelection, aAction);
    PRInt32 i;
    
    if (!deleteNode)
      for (i = 0; i < mActionListeners.Count(); i++)
        mActionListeners[i]->WillDeleteSelection(selection);
    else if (deleteCharData)
      for (i = 0; i < mActionListeners.Count(); i++)
        mActionListeners[i]->WillDeleteText(deleteCharData, deleteCharOffset, 1);
    else
      for (i = 0; i < mActionListeners.Count(); i++)
        mActionListeners[i]->WillDeleteNode(deleteNode);

    
    res = DoTransaction(txn);  

    
    if (!deleteNode)
      for (i = 0; i < mActionListeners.Count(); i++)
        mActionListeners[i]->DidDeleteSelection(selection);
    else if (deleteCharData)
      for (i = 0; i < mActionListeners.Count(); i++)
        mActionListeners[i]->DidDeleteText(deleteCharData, deleteCharOffset, 1, res);
    else
      for (i = 0; i < mActionListeners.Count(); i++)
        mActionListeners[i]->DidDeleteNode(deleteNode, res);
  }

  return res;
}


NS_IMETHODIMP
nsEditor::DeleteSelectionAndCreateNode(const nsAString& aTag,
                                           nsIDOMNode ** aNewNode)
{
  nsCOMPtr<nsIDOMNode> parentSelectedNode;
  PRInt32 offsetOfNewNode;
  nsresult result = DeleteSelectionAndPrepareToCreateNode(parentSelectedNode,
                                                          offsetOfNewNode);
  if (NS_FAILED(result))
    return result;

  nsCOMPtr<nsIDOMNode> newNode;
  result = CreateNode(aTag, parentSelectedNode, offsetOfNewNode,
                      getter_AddRefs(newNode));
  
  *aNewNode = newNode;
  NS_IF_ADDREF(*aNewNode);

  
  nsCOMPtr<nsISelection> selection;
  result = GetSelection(getter_AddRefs(selection));
  if (NS_FAILED(result)) return result;
  if (!selection) return NS_ERROR_NULL_POINTER;
  return selection->Collapse(parentSelectedNode, offsetOfNewNode+1);
}




nsresult
nsEditor::GetIMEBufferLength(PRInt32* length)
{
  *length = mIMEBufferLength;
  return NS_OK;
}

void
nsEditor::SetIsIMEComposing(){  
  
  nsCOMPtr<nsIPrivateTextRange> rangePtr;
  PRUint16 listlen, type;

  mIsIMEComposing = PR_FALSE;
  listlen = mIMETextRangeList->GetLength();

  for (PRUint16 i = 0; i < listlen; i++)
  {
      rangePtr = mIMETextRangeList->Item(i);
      if (!rangePtr) continue;
      nsresult result = rangePtr->GetRangeType(&type);
      if (NS_FAILED(result)) continue;
      if ( type == nsIPrivateTextRange::TEXTRANGE_RAWINPUT ||
           type == nsIPrivateTextRange::TEXTRANGE_CONVERTEDTEXT ||
           type == nsIPrivateTextRange::TEXTRANGE_SELECTEDRAWTEXT ||
           type == nsIPrivateTextRange::TEXTRANGE_SELECTEDCONVERTEDTEXT )
      {
        mIsIMEComposing = PR_TRUE;
#ifdef DEBUG_IME
        printf("nsEditor::mIsIMEComposing = PR_TRUE\n");
#endif
        break;
      }
  }
  return;
}

PRBool
nsEditor::IsIMEComposing() {
  return mIsIMEComposing;
}

NS_IMETHODIMP
nsEditor::DeleteSelectionAndPrepareToCreateNode(nsCOMPtr<nsIDOMNode> &parentSelectedNode, PRInt32& offsetOfNewNode)
{
  nsresult result=NS_ERROR_NOT_INITIALIZED;
  nsCOMPtr<nsISelection> selection;
  result = GetSelection(getter_AddRefs(selection));
  if (NS_FAILED(result)) return result;
  if (!selection) return NS_ERROR_NULL_POINTER;

  PRBool collapsed;
  result = selection->GetIsCollapsed(&collapsed);
  if (NS_SUCCEEDED(result) && !collapsed) 
  {
    result = DeleteSelection(nsIEditor::eNone);
    if (NS_FAILED(result)) {
      return result;
    }
    
    result = GetSelection(getter_AddRefs(selection));
    if (NS_FAILED(result)) {
      return result;
    }
#ifdef NS_DEBUG
    nsCOMPtr<nsIDOMNode>testSelectedNode;
    nsresult debugResult = selection->GetAnchorNode(getter_AddRefs(testSelectedNode));
    
    
    if (testSelectedNode)
    {
      PRBool testCollapsed;
      debugResult = selection->GetIsCollapsed(&testCollapsed);
      NS_ASSERTION((NS_SUCCEEDED(result)), "couldn't get a selection after deletion");
      NS_ASSERTION(testCollapsed, "selection not reset after deletion");
    }
#endif
  }
  
  PRInt32 offsetOfSelectedNode;
  result = selection->GetAnchorNode(getter_AddRefs(parentSelectedNode));
  if (NS_SUCCEEDED(result) && NS_SUCCEEDED(selection->GetAnchorOffset(&offsetOfSelectedNode)) && parentSelectedNode)
  {
    nsCOMPtr<nsIDOMNode> selectedNode;
    PRUint32 selectedNodeContentCount=0;
    nsCOMPtr<nsIDOMCharacterData>selectedParentNodeAsText;
    selectedParentNodeAsText = do_QueryInterface(parentSelectedNode);

    offsetOfNewNode = offsetOfSelectedNode;
    
    


    if (selectedParentNodeAsText) 
    { 
      PRInt32 indexOfTextNodeInParent;
      selectedNode = do_QueryInterface(parentSelectedNode);
      selectedNode->GetParentNode(getter_AddRefs(parentSelectedNode));
      selectedParentNodeAsText->GetLength(&selectedNodeContentCount);
      GetChildOffset(selectedNode, parentSelectedNode, indexOfTextNodeInParent);

      if ((offsetOfSelectedNode!=0) && (((PRUint32)offsetOfSelectedNode)!=selectedNodeContentCount))
      {
        nsCOMPtr<nsIDOMNode> newSiblingNode;
        result = SplitNode(selectedNode, offsetOfSelectedNode, getter_AddRefs(newSiblingNode));
        
        if (NS_SUCCEEDED(result)) {
          result = GetChildOffset(selectedNode, parentSelectedNode, offsetOfNewNode);
        }
      }
      else 
      { 
        if (0==offsetOfSelectedNode) {
          offsetOfNewNode = indexOfTextNodeInParent; 
        }
        else {                 
          GetChildOffset(selectedNode, parentSelectedNode, offsetOfNewNode);
          offsetOfNewNode++;    
        }
      }
    }
    
  }
#ifdef DEBUG
  else {
    printf("InsertLineBreak into an empty document is not yet supported\n");
  }
#endif
  return result;
}



NS_IMETHODIMP 
nsEditor::DoAfterDoTransaction(nsITransaction *aTxn)
{
  nsresult rv = NS_OK;
  
  PRBool  isTransientTransaction;
  rv = aTxn->GetIsTransient(&isTransientTransaction);
  if (NS_FAILED(rv))
    return rv;
  
  if (!isTransientTransaction)
  {
    
    
    
    
    PRInt32 modCount;
    GetModificationCount(&modCount);
    if (modCount < 0)
      modCount = -modCount;
        
    rv = IncrementModificationCount(1);    
  }
  
  return rv;
}


NS_IMETHODIMP 
nsEditor::DoAfterUndoTransaction()
{
  nsresult rv = NS_OK;

  rv = IncrementModificationCount(-1);    

  return rv;
}

NS_IMETHODIMP 
nsEditor::DoAfterRedoTransaction()
{
  return IncrementModificationCount(1);    
}

NS_IMETHODIMP 
nsEditor::CreateTxnForSetAttribute(nsIDOMElement *aElement, 
                                   const nsAString& aAttribute, 
                                   const nsAString& aValue,
                                   ChangeAttributeTxn ** aTxn)
{
  if (!aElement)
    return NS_ERROR_NULL_POINTER;

  *aTxn = new ChangeAttributeTxn();
  if (!*aTxn)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*aTxn);
  return (*aTxn)->Init(this, aElement, aAttribute, aValue, PR_FALSE);
}


NS_IMETHODIMP 
nsEditor::CreateTxnForRemoveAttribute(nsIDOMElement *aElement, 
                                      const nsAString& aAttribute,
                                      ChangeAttributeTxn ** aTxn)
{
  if (!aElement)
    return NS_ERROR_NULL_POINTER;

  *aTxn = new ChangeAttributeTxn();
  if (!*aTxn)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*aTxn);

  return (*aTxn)->Init(this, aElement, aAttribute, EmptyString(), PR_TRUE);
}


NS_IMETHODIMP nsEditor::CreateTxnForCreateElement(const nsAString& aTag,
                                                  nsIDOMNode     *aParent,
                                                  PRInt32         aPosition,
                                                  CreateElementTxn ** aTxn)
{
  if (!aParent)
    return NS_ERROR_NULL_POINTER;

  *aTxn = new CreateElementTxn();
  if (!*aTxn)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*aTxn);

  return (*aTxn)->Init(this, aTag, aParent, aPosition);
}


NS_IMETHODIMP nsEditor::CreateTxnForInsertElement(nsIDOMNode * aNode,
                                                  nsIDOMNode * aParent,
                                                  PRInt32      aPosition,
                                                  InsertElementTxn ** aTxn)
{
  if (!aNode || !aParent)
    return NS_ERROR_NULL_POINTER;

  *aTxn = new InsertElementTxn();
  if (!*aTxn)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*aTxn);

  return (*aTxn)->Init(aNode, aParent, aPosition, this);
}

NS_IMETHODIMP nsEditor::CreateTxnForDeleteElement(nsIDOMNode * aElement,
                                             DeleteElementTxn ** aTxn)
{
  if (!aElement)
    return NS_ERROR_NULL_POINTER;

  *aTxn = new DeleteElementTxn();
  if (!*aTxn)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*aTxn);

  return (*aTxn)->Init(this, aElement, &mRangeUpdater);
}

NS_IMETHODIMP 
nsEditor::CreateTxnForIMEText(const nsAString& aStringToInsert,
                              IMETextTxn ** aTxn)
{
  NS_ASSERTION(aTxn, "illegal value- null ptr- aTxn");
     
  *aTxn = new IMETextTxn();
  if (!*aTxn)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*aTxn);

  return (*aTxn)->Init(mIMETextNode, mIMETextOffset, mIMEBufferLength,
                       mIMETextRangeList, aStringToInsert, mSelConWeak);
}


NS_IMETHODIMP 
nsEditor::CreateTxnForAddStyleSheet(nsICSSStyleSheet* aSheet, AddStyleSheetTxn* *aTxn)
{
  *aTxn = new AddStyleSheetTxn();
  if (! *aTxn)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*aTxn);

  return (*aTxn)->Init(this, aSheet);
}



NS_IMETHODIMP 
nsEditor::CreateTxnForRemoveStyleSheet(nsICSSStyleSheet* aSheet, RemoveStyleSheetTxn* *aTxn)
{
  *aTxn = new RemoveStyleSheetTxn();
  if (! *aTxn)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*aTxn);

  return (*aTxn)->Init(this, aSheet);
}


NS_IMETHODIMP
nsEditor::CreateTxnForDeleteSelection(nsIEditor::EDirection aAction,
                                      EditAggregateTxn ** aTxn,
                                      nsIDOMNode ** aNode,
                                      PRInt32 *aOffset,
                                      PRInt32 *aLength)
{
  if (!aTxn)
    return NS_ERROR_NULL_POINTER;
  *aTxn = nsnull;

  nsCOMPtr<nsISelectionController> selCon = do_QueryReferent(mSelConWeak);
  if (!selCon) return NS_ERROR_NOT_INITIALIZED;
  nsCOMPtr<nsISelection> selection;
  nsresult result = selCon->GetSelection(nsISelectionController::SELECTION_NORMAL,
                                         getter_AddRefs(selection));
  if ((NS_SUCCEEDED(result)) && selection)
  {
    
    PRBool isCollapsed;
    result = (selection->GetIsCollapsed(&isCollapsed));
    if (NS_SUCCEEDED(result) && isCollapsed && aAction == eNone)
      return NS_OK;

    
    *aTxn = new EditAggregateTxn();
    if (!*aTxn) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    NS_ADDREF(*aTxn);

    nsCOMPtr<nsISelectionPrivate>selPrivate(do_QueryInterface(selection));
    nsCOMPtr<nsIEnumerator> enumerator;
    result = selPrivate->GetEnumerator(getter_AddRefs(enumerator));
    if (NS_SUCCEEDED(result) && enumerator)
    {
      for (enumerator->First(); NS_OK!=enumerator->IsDone(); enumerator->Next())
      {
        nsCOMPtr<nsISupports> currentItem;
        result = enumerator->CurrentItem(getter_AddRefs(currentItem));
        if ((NS_SUCCEEDED(result)) && (currentItem))
        {
          nsCOMPtr<nsIDOMRange> range( do_QueryInterface(currentItem) );
          range->GetCollapsed(&isCollapsed);
          if (!isCollapsed)
          {
            nsRefPtr<DeleteRangeTxn> txn = new DeleteRangeTxn();
            if (txn)
            {
              txn->Init(this, range, &mRangeUpdater);
              (*aTxn)->AppendChild(txn);
            }
            else
              result = NS_ERROR_OUT_OF_MEMORY;
          }
          
          
          else if (aAction != eNone)
          { 
            result = CreateTxnForDeleteInsertionPoint(range, aAction, *aTxn, aNode, aOffset, aLength);
          }
        }
      }
    }
  }

  
  if (NS_FAILED(result))
  {
    NS_IF_RELEASE(*aTxn);
  }

  return result;
}

nsresult
nsEditor::CreateTxnForDeleteCharacter(nsIDOMCharacterData  *aData,
                                      PRUint32              aOffset,
                                      nsIEditor::EDirection aDirection,
                                      DeleteTextTxn       **aTxn)
{
  NS_ASSERTION(aDirection == eNext || aDirection == ePrevious,
               "invalid direction");
  nsAutoString data;
  aData->GetData(data);
  PRUint32 segOffset, segLength = 1;
  if (aDirection == eNext) {
    segOffset = aOffset;
    if (NS_IS_HIGH_SURROGATE(data[segOffset]) &&
        segOffset + 1 < data.Length() &&
        NS_IS_LOW_SURROGATE(data[segOffset+1])) {
      
      ++segLength;
    }
  } else {
    segOffset = aOffset - 1;
    if (NS_IS_LOW_SURROGATE(data[segOffset]) &&
        segOffset > 0 &&
        NS_IS_HIGH_SURROGATE(data[segOffset-1])) {
      ++segLength;
      --segOffset;
    }
  }
  return CreateTxnForDeleteText(aData, segOffset, segLength, aTxn);
}


NS_IMETHODIMP
nsEditor::CreateTxnForDeleteInsertionPoint(nsIDOMRange          *aRange, 
                                           nsIEditor::EDirection aAction,
                                           EditAggregateTxn     *aTxn,
                                           nsIDOMNode          **aNode,
                                           PRInt32              *aOffset,
                                           PRInt32              *aLength)
{
  NS_ASSERTION(aAction == eNext || aAction == ePrevious, "invalid action");

  
  nsCOMPtr<nsIDOMNode> node;
  nsresult result = aRange->GetStartContainer(getter_AddRefs(node));
  if (NS_FAILED(result))
    return result;

  PRInt32 offset;
  result = aRange->GetStartOffset(&offset);
  if (NS_FAILED(result))
    return result;

  
  nsCOMPtr<nsIDOMCharacterData> nodeAsText = do_QueryInterface(node);

  PRUint32 count=0;

  if (nodeAsText)
    nodeAsText->GetLength(&count);
  else
  { 
    
    nsCOMPtr<nsIDOMNodeList>childList;
    result = node->GetChildNodes(getter_AddRefs(childList));
    if ((NS_SUCCEEDED(result)) && childList)
      childList->GetLength(&count);
  }

  PRBool isFirst = (0 == offset);
  PRBool isLast  = (count == (PRUint32)offset);

  
  

  
  
  if ((ePrevious==aAction) && (PR_TRUE==isFirst))
  { 
    nsCOMPtr<nsIDOMNode> priorNode;
    result = GetPriorNode(node, PR_TRUE, address_of(priorNode));
    if ((NS_SUCCEEDED(result)) && priorNode)
    { 
      
      nsCOMPtr<nsIDOMCharacterData> priorNodeAsText = do_QueryInterface(priorNode);
      if (priorNodeAsText)
      {
        PRUint32 length=0;
        priorNodeAsText->GetLength(&length);
        if (0<length)
        {
          DeleteTextTxn *txn;
          result = CreateTxnForDeleteCharacter(priorNodeAsText, length,
                                               ePrevious, &txn);
          if (NS_SUCCEEDED(result)) {
            aTxn->AppendChild(txn);
            NS_ADDREF(*aNode = priorNode);
            *aOffset = txn->GetOffset();
            *aLength = txn->GetNumCharsToDelete();
            NS_RELEASE(txn);
          }
        }
        else
        { 
          printf("ERROR: found a text node with 0 characters\n");
          result = NS_ERROR_UNEXPECTED;
        }
      }
      else
      { 
        DeleteElementTxn *txn;
        result = CreateTxnForDeleteElement(priorNode, &txn);
        if (NS_SUCCEEDED(result)) {
          aTxn->AppendChild(txn);
          NS_RELEASE(txn);
          NS_ADDREF(*aNode = priorNode);
        }
      }
    }
  }
  else if ((nsIEditor::eNext==aAction) && (PR_TRUE==isLast))
  { 
    nsCOMPtr<nsIDOMNode> nextNode;
    result = GetNextNode(node, PR_TRUE, address_of(nextNode));
    if ((NS_SUCCEEDED(result)) && nextNode)
    { 
      
      nsCOMPtr<nsIDOMCharacterData> nextNodeAsText = do_QueryInterface(nextNode);
      if (nextNodeAsText)
      {
        PRUint32 length=0;
        nextNodeAsText->GetLength(&length);
        if (0<length)
        {
          DeleteTextTxn *txn;
          result = CreateTxnForDeleteCharacter(nextNodeAsText, 0, eNext, &txn);
          if (NS_SUCCEEDED(result)) {
            aTxn->AppendChild(txn);
            NS_ADDREF(*aNode = nextNode);
            *aOffset = txn->GetOffset();
            *aLength = txn->GetNumCharsToDelete();
            NS_RELEASE(txn);
          }
        }
        else
        { 
          printf("ERROR: found a text node with 0 characters\n");
          result = NS_ERROR_UNEXPECTED;
        }
      }
      else
      { 
        DeleteElementTxn *txn;
        result = CreateTxnForDeleteElement(nextNode, &txn);
        if (NS_SUCCEEDED(result)) {
          aTxn->AppendChild(txn);
          NS_RELEASE(txn);
          NS_ADDREF(*aNode = nextNode);
        }
      }
    }
  }
  else
  {
    if (nodeAsText)
    { 
      nsRefPtr<DeleteTextTxn> txn;
      result = CreateTxnForDeleteCharacter(nodeAsText, offset, aAction,
                                           getter_AddRefs(txn));
      if (NS_SUCCEEDED(result)) {
        aTxn->AppendChild(txn);
        NS_ADDREF(*aNode = node);
        *aOffset = txn->GetOffset();
        *aLength = txn->GetNumCharsToDelete();
      }
    }
    else
    { 
      nsCOMPtr<nsIDOMNode> selectedNode;
      if (ePrevious==aAction)
      {
        result = GetPriorNode(node, offset, PR_TRUE, address_of(selectedNode));
      }
      else if (eNext==aAction)
      {
        result = GetNextNode(node, offset, PR_TRUE, address_of(selectedNode));
      }
      if (NS_FAILED(result)) { return result; }
      if (selectedNode) 
      {
        nsCOMPtr<nsIDOMCharacterData> selectedNodeAsText =
                                             do_QueryInterface(selectedNode);
        if (selectedNodeAsText)
        { 
          PRUint32 position = 0;    
          if (ePrevious==aAction)
          {
            selectedNodeAsText->GetLength(&position);
          }
          nsRefPtr<DeleteTextTxn> delTextTxn;
          result = CreateTxnForDeleteCharacter(selectedNodeAsText, position,
                                               aAction,
                                               getter_AddRefs(delTextTxn));
          if (NS_FAILED(result))  { return result; }
          if (!delTextTxn) { return NS_ERROR_NULL_POINTER; }
          aTxn->AppendChild(delTextTxn);
          NS_ADDREF(*aNode = selectedNode);
          *aOffset = delTextTxn->GetOffset();
          *aLength = delTextTxn->GetNumCharsToDelete();
        }
        else
        {
          nsRefPtr<DeleteElementTxn> delElementTxn;
          result = CreateTxnForDeleteElement(selectedNode,
                                             getter_AddRefs(delElementTxn));
          if (NS_FAILED(result))  { return result; }
          if (!delElementTxn) { return NS_ERROR_NULL_POINTER; }
          aTxn->AppendChild(delElementTxn);
          NS_ADDREF(*aNode = selectedNode);
        }
      }
    }
  }
  return result;
}

nsresult 
nsEditor::CreateRange(nsIDOMNode *aStartParent, PRInt32 aStartOffset,
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
nsEditor::AppendNodeToSelectionAsRange(nsIDOMNode *aNode)
{
  if (!aNode) return NS_ERROR_NULL_POINTER;
  nsCOMPtr<nsISelection> selection;
  nsresult res = GetSelection(getter_AddRefs(selection));
  if (NS_FAILED(res)) return res;
  if(!selection) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMNode> parentNode;
  res = aNode->GetParentNode(getter_AddRefs(parentNode));
  if (NS_FAILED(res)) return res;
  if (!parentNode) return NS_ERROR_NULL_POINTER;
  
  PRInt32 offset;
  res = GetChildOffset(aNode, parentNode, offset);
  if (NS_FAILED(res)) return res;
  
  nsCOMPtr<nsIDOMRange> range;
  res = CreateRange(parentNode, offset, parentNode, offset+1, getter_AddRefs(range));
  if (NS_FAILED(res)) return res;
  if (!range) return NS_ERROR_NULL_POINTER;

  return selection->AddRange(range);
}

nsresult nsEditor::ClearSelection()
{
  nsCOMPtr<nsISelection> selection;
  nsresult res = nsEditor::GetSelection(getter_AddRefs(selection));
  if (NS_FAILED(res)) return res;
  if (!selection) return NS_ERROR_FAILURE;
  return selection->RemoveAllRanges();  
}

nsresult
nsEditor::CreateHTMLContent(const nsAString& aTag, nsIContent** aContent)
{
  nsCOMPtr<nsIDOMDocument> tempDoc;
  GetDocument(getter_AddRefs(tempDoc));

  nsCOMPtr<nsIDocument> doc = do_QueryInterface(tempDoc);
  if (!doc)
    return NS_ERROR_FAILURE;

  
  
  if (aTag.IsEmpty()) {
    NS_ERROR("Don't pass an empty tag to nsEditor::CreateHTMLContent, "
             "check caller.");
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIAtom> tag = do_GetAtom(aTag);
  if (!tag)
    return NS_ERROR_OUT_OF_MEMORY;

  return doc->CreateElem(tag, nsnull, kNameSpaceID_XHTML, PR_FALSE, aContent);
}

nsresult
nsEditor::SetAttributeOrEquivalent(nsIDOMElement * aElement,
                                   const nsAString & aAttribute,
                                   const nsAString & aValue,
                                   PRBool aSuppressTransaction)
{
  return SetAttribute(aElement, aAttribute, aValue);
}

nsresult
nsEditor::RemoveAttributeOrEquivalent(nsIDOMElement * aElement,
                                      const nsAString & aAttribute,
                                      PRBool aSuppressTransaction)
{
  return RemoveAttribute(aElement, aAttribute);
}

nsresult
nsEditor::HandleInlineSpellCheck(PRInt32 action,
                                   nsISelection *aSelection,
                                   nsIDOMNode *previousSelectedNode,
                                   PRInt32 previousSelectedOffset,
                                   nsIDOMNode *aStartNode,
                                   PRInt32 aStartOffset,
                                   nsIDOMNode *aEndNode,
                                   PRInt32 aEndOffset)
{
  return mInlineSpellChecker ? mInlineSpellChecker->SpellCheckAfterEditorChange(action,
                                                       aSelection,
                                                       previousSelectedNode,
                                                       previousSelectedOffset,
                                                       aStartNode,
                                                       aStartOffset,
                                                       aEndNode,
                                                       aEndOffset) : NS_OK;
}

already_AddRefed<nsPIDOMEventTarget>
nsEditor::GetPIDOMEventTarget()
{
  nsPIDOMEventTarget* piTarget = mEventTarget;
  if (piTarget)
  {
    NS_ADDREF(piTarget);
    return piTarget;
  }

  nsIDOMElement *rootElement = GetRoot();

  
  

  nsCOMPtr<nsIContent> content = do_QueryInterface(rootElement);

  if (content && content->IsRootOfNativeAnonymousSubtree())
  {
    mEventTarget = do_QueryInterface(content->GetParent());
    piTarget = mEventTarget;
    NS_IF_ADDREF(piTarget);
  }
  else
  {
    
    
    
    if (mDocWeak)
    {
      CallQueryReferent(mDocWeak.get(), &piTarget);
    }
    else
    {
      NS_ERROR("not initialized yet");
    }
  }

  return piTarget;
}

nsIDOMElement *
nsEditor::GetRoot()
{
  if (!mRootElement)
  {
    nsCOMPtr<nsIDOMElement> root;

    
    GetRootElement(getter_AddRefs(root));
  }

  return mRootElement;
}

NS_IMETHODIMP
nsEditor::SwitchTextDirection()
{
  
  nsIDOMElement *rootElement = GetRoot();

  nsresult rv;
  nsCOMPtr<nsIContent> content = do_QueryInterface(rootElement, &rv);
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIPresShell> presShell;
  rv = GetPresShell(getter_AddRefs(presShell));
  if (NS_FAILED(rv))
    return rv;  

  nsIFrame *frame = presShell->GetPrimaryFrameFor(content);
  if (!frame)
    return NS_ERROR_FAILURE; 

  
  if (frame->GetStyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL)
    rv = rootElement->SetAttribute(NS_LITERAL_STRING("dir"), NS_LITERAL_STRING("ltr"));
  else
    rv = rootElement->SetAttribute(NS_LITERAL_STRING("dir"), NS_LITERAL_STRING("rtl"));

  return rv;
}

#if DEBUG_JOE
void
nsEditor::DumpNode(nsIDOMNode *aNode, PRInt32 indent)
{
  PRInt32 i;
  for (i=0; i<indent; i++)
    printf("  ");
  
  nsCOMPtr<nsIDOMElement> element = do_QueryInterface(aNode);
  nsCOMPtr<nsIDOMDocumentFragment> docfrag = do_QueryInterface(aNode);
  
  if (element || docfrag)
  { 
    if (element)
    {
      nsAutoString tag;
      element->GetTagName(tag);
      printf("<%s>\n", NS_LossyConvertUTF16toASCII(tag).get());
    }
    else
    {
      printf("<document fragment>\n");
    }
    nsCOMPtr<nsIDOMNodeList> childList;
    aNode->GetChildNodes(getter_AddRefs(childList));
    if (!childList) return NS_ERROR_NULL_POINTER;
    PRUint32 numChildren;
    childList->GetLength(&numChildren);
    nsCOMPtr<nsIDOMNode> child, tmp;
    aNode->GetFirstChild(getter_AddRefs(child));
    for (i=0; i<numChildren; i++)
    {
      DumpNode(child, indent+1);
      child->GetNextSibling(getter_AddRefs(tmp));
      child = tmp;
    }
  }
  else if (IsTextNode(aNode))
  {
    nsCOMPtr<nsIDOMCharacterData> textNode = do_QueryInterface(aNode);
    nsAutoString str;
    textNode->GetData(str);
    nsCAutoString cstr;
    LossyCopyUTF16toASCII(str, cstr);
    cstr.ReplaceChar('\n', ' ');
    printf("<textnode> %s\n", cstr.get());
  }
}
#endif

PRBool
nsEditor::IsModifiableNode(nsIDOMNode *aNode)
{
  return PR_TRUE;
}
