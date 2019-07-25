








































#include "pratom.h"
#include "nsIDOMDocument.h"
#include "nsIDOMHTMLElement.h"
#include "nsIDOMNSHTMLElement.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMNSEvent.h"
#include "nsIDOMEventTarget.h"
#include "nsIMEStateManager.h"
#include "nsFocusManager.h"
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
#include "nsIDOMHTMLBRElement.h"
#include "nsIDOMEventTarget.h"
#include "nsIDocument.h"
#include "nsITransactionManager.h"
#include "nsIAbsorbingTransaction.h"
#include "nsIPresShell.h"
#include "nsISelection.h"
#include "nsISelectionPrivate.h"
#include "nsISelectionController.h"
#include "nsIEnumerator.h"
#include "nsEditProperty.h"
#include "nsIAtom.h"
#include "nsCaret.h"
#include "nsIWidget.h"
#include "nsIPlaintextEditor.h"
#include "nsIPrivateDOMEvent.h"
#include "nsGUIEvent.h"

#include "nsIFrame.h"  

#include "nsCSSStyleSheet.h"

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
#include "nsEditorEventListener.h"
#include "nsISelectionDisplay.h"
#include "nsIInlineSpellChecker.h"
#include "nsINameSpaceManager.h"
#include "nsIHTMLDocument.h"
#include "nsIParserService.h"

#include "nsITransferable.h"
#include "nsComputedDOMStyle.h"
#include "nsTextEditUtils.h"

#include "mozilla/FunctionTimer.h"
#include "mozilla/Preferences.h"

#define NS_ERROR_EDITOR_NO_SELECTION NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_EDITOR,1)
#define NS_ERROR_EDITOR_NO_TEXTNODE  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_EDITOR,2)

#ifdef NS_DEBUG_EDITOR
static PRBool gNoisy = PR_FALSE;
#endif

#ifdef DEBUG
#include "nsIDOMHTMLDocument.h"
#endif

using namespace mozilla;


extern nsIParserService *sParserService;







nsEditor::nsEditor()
:  mModCount(0)
,  mFlags(0)
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
,  mDidPostCreate(PR_FALSE)
,  mDocDirtyState(-1)
,  mDocWeak(nsnull)
,  mPhonetic(nsnull)
,  mLastKeypressEventWasTrusted(eTriUnset)
{
  
}

nsEditor::~nsEditor()
{
  NS_ASSERTION(!mDocWeak || mDidPreDestroy, "Why PreDestroy hasn't been called?");

  mTxnMgr = nsnull;

  delete mPhonetic;
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
 NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mEventListener)
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
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mEventListener)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsEditor)
 NS_INTERFACE_MAP_ENTRY(nsIPhonetic)
 NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
 NS_INTERFACE_MAP_ENTRY(nsIEditorIMESupport)
 NS_INTERFACE_MAP_ENTRY(nsIEditor)
 NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIEditor)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsEditor)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsEditor)


NS_IMETHODIMP
nsEditor::Init(nsIDOMDocument *aDoc, nsIContent *aRoot, nsISelectionController *aSelCon, PRUint32 aFlags)
{
  NS_PRECONDITION(aDoc, "bad arg");
  if (!aDoc)
    return NS_ERROR_NULL_POINTER;

  
  
  
  
#ifdef DEBUG
  nsresult rv =
#endif
  SetFlags(aFlags);
  NS_ASSERTION(NS_SUCCEEDED(rv), "SetFlags() failed");

  mDocWeak = do_GetWeakReference(aDoc);  
  
  
  
  nsCOMPtr<nsISelectionController> selCon;
  if (aSelCon) {
    mSelConWeak = do_GetWeakReference(aSelCon);   
    selCon = aSelCon;
  } else {
    nsCOMPtr<nsIPresShell> presShell = GetPresShell();
    selCon = do_QueryInterface(presShell);
  }
  NS_ASSERTION(selCon, "Selection controller should be available at this point");

  
  if (aRoot)
    mRootElement = do_QueryInterface(aRoot);

  mUpdateCount=0;

  
  mIMETextNode = nsnull;
  mIMETextOffset = 0;
  mIMEBufferLength = 0;
  
  
  selCon->SetCaretReadOnly(PR_FALSE);
  selCon->SetDisplaySelection(nsISelectionController::SELECTION_ON);

  selCon->SetSelectionFlags(nsISelectionDisplay::DISPLAY_ALL);

  NS_POSTCONDITION(mDocWeak, "bad state");

  
  mDidPreDestroy = PR_FALSE;
  
  mDidPostCreate = PR_FALSE;

  return NS_OK;
}


NS_IMETHODIMP
nsEditor::PostCreate()
{
  
  
  
  
  mFlags = ~mFlags;
  nsresult rv = SetFlags(~mFlags);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (!mDidPostCreate) {
    mDidPostCreate = PR_TRUE;

    
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
  }

  
  nsCOMPtr<nsIContent> focusedContent = GetFocusedContent();
  if (focusedContent) {
    nsCOMPtr<nsIPresShell> ps = GetPresShell();
    NS_ASSERTION(ps, "no pres shell even though we have focus");
    NS_ENSURE_TRUE(ps, NS_ERROR_UNEXPECTED);
    nsPresContext* pc = ps->GetPresContext(); 

    nsIMEStateManager::OnTextStateBlur(pc, nsnull);
    nsIMEStateManager::OnTextStateFocus(pc, focusedContent);

    nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(focusedContent);
    if (target) {
      InitializeSelection(target);
    }
  }
  return NS_OK;
}

nsresult
nsEditor::CreateEventListeners()
{
  
  if (mEventListener)
    return NS_OK;
  mEventListener = do_QueryInterface(
    static_cast<nsIDOMKeyListener*>(new nsEditorEventListener()));
  NS_ENSURE_TRUE(mEventListener, NS_ERROR_OUT_OF_MEMORY);
  return NS_OK;
}

nsresult
nsEditor::InstallEventListeners()
{
  NS_ENSURE_TRUE(mDocWeak && mEventListener,
                 NS_ERROR_NOT_INITIALIZED);

  
  nsCOMPtr<nsIContent> rootContent = do_QueryInterface(GetRoot());
  NS_ENSURE_TRUE(rootContent, NS_ERROR_NOT_AVAILABLE);
  mEventTarget = do_QueryInterface(rootContent->GetParent());
  NS_ENSURE_TRUE(mEventTarget, NS_ERROR_NOT_AVAILABLE);

  nsEditorEventListener* listener =
    reinterpret_cast<nsEditorEventListener*>(mEventListener.get());
  return listener->Connect(this);
}

void
nsEditor::RemoveEventListeners()
{
  if (!mDocWeak || !mEventListener) {
    return;
  }
  reinterpret_cast<nsEditorEventListener*>(mEventListener.get())->Disconnect();
  mEventTarget = nsnull;
}

PRBool
nsEditor::GetDesiredSpellCheckState()
{
  
  if (mSpellcheckCheckboxState != eTriUnset) {
    return (mSpellcheckCheckboxState == eTriTrue);
  }

  
  PRInt32 spellcheckLevel = Preferences::GetInt("layout.spellcheckDefault", 1);

  if (spellcheckLevel == 0) {
    return PR_FALSE;                    
  }

  if (!CanEnableSpellCheck()) {
    return PR_FALSE;
  }

  nsCOMPtr<nsIPresShell> presShell = GetPresShell();
  if (presShell) {
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

  
  RemoveEventListeners();
  mActionListeners.Clear();
  mEditorObservers.Clear();
  mDocStateListeners.Clear();
  mInlineSpellChecker = nsnull;
  mSpellcheckCheckboxState = eTriUnset;
  mRootElement = nsnull;

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
  if (mFlags == aFlags) {
    return NS_OK;
  }

  PRBool spellcheckerWasEnabled = CanEnableSpellCheck();
  mFlags = aFlags;

  if (!mDocWeak) {
    
    
    
    return NS_OK;
  }

  
  if (CanEnableSpellCheck() != spellcheckerWasEnabled) {
    nsresult rv = SyncRealTimeSpell();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  nsCOMPtr<nsIContent> focusedContent = GetFocusedContent();
  if (focusedContent) {
    
    
    PRUint32 newState = nsIContent::IME_STATUS_ENABLE;
    nsresult rv = GetPreferredIMEState(&newState);
    if (NS_SUCCEEDED(rv)) {
      
      
      nsIMEStateManager::UpdateIMEState(newState, focusedContent);
    }
  }

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
  NS_ENSURE_TRUE(aDoc, NS_ERROR_NULL_POINTER);
  *aDoc = nsnull; 
  NS_PRECONDITION(mDocWeak, "bad state, mDocWeak weak pointer not initialized");
  nsCOMPtr<nsIDOMDocument> doc = do_QueryReferent(mDocWeak);
  NS_ENSURE_TRUE(doc, NS_ERROR_NOT_INITIALIZED);
  NS_ADDREF(*aDoc = doc);
  return NS_OK;
}

already_AddRefed<nsIPresShell>
nsEditor::GetPresShell()
{
  NS_PRECONDITION(mDocWeak, "bad state, null mDocWeak");
  nsCOMPtr<nsIDocument> doc = do_QueryReferent(mDocWeak);
  NS_ENSURE_TRUE(doc, NULL);
  nsCOMPtr<nsIPresShell> ps = doc->GetShell();
  return ps.forget();
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
  NS_ENSURE_TRUE(aSel, NS_ERROR_NULL_POINTER);
  *aSel = nsnull; 
  nsCOMPtr<nsISelectionController> selCon;
  if (mSelConWeak) {
    selCon = do_QueryReferent(mSelConWeak);
  } else {
    nsCOMPtr<nsIPresShell> presShell = GetPresShell();
    selCon = do_QueryInterface(presShell);
  }
  NS_ENSURE_TRUE(selCon, NS_ERROR_NOT_INITIALIZED);
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
  NS_ENSURE_TRUE(aSelection, NS_ERROR_NULL_POINTER);
  *aSelection = nsnull;
  nsCOMPtr<nsISelectionController> selcon;
  GetSelectionController(getter_AddRefs(selcon));
  NS_ENSURE_TRUE(selcon, NS_ERROR_NOT_INITIALIZED);
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
      NS_ENSURE_SUCCESS(result, result);
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
    NS_ENSURE_SUCCESS(result, result);
    NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);
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
 
  NS_ENSURE_SUCCESS(result, result);

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
  NS_ENSURE_TRUE(mTxnMgr, NS_ERROR_FAILURE);

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
  NS_ENSURE_TRUE(hasTransaction, result);

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
  NS_ENSURE_TRUE(aIsEnabled && aCanUndo, NS_ERROR_NULL_POINTER);
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
  NS_ENSURE_TRUE(hasTransaction, result);

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
  NS_ENSURE_TRUE(aIsEnabled && aCanRedo, NS_ERROR_NULL_POINTER);

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
    if (NS_SUCCEEDED(res)) {
      mSelState = new nsSelectionState();
      if (mSelState) {
        mSelState->SaveSelection(selection);
      }
    }
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

    {
      
      
      nsRefPtr<nsCaret> caret;
      nsCOMPtr<nsIPresShell> presShell = GetPresShell();

      if (presShell)
        caret = presShell->GetCaret();

      StCaretHider caretHider(caret);

      
      EndUpdateViewBatch();
      

      
      
      ScrollSelectionIntoView(PR_FALSE);
    }

    
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
  NS_ENSURE_TRUE(aResult, NS_ERROR_NULL_POINTER);
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
  NS_ENSURE_TRUE(rootElement, NS_ERROR_NULL_POINTER); 

  PRBool hasChildNodes;
  nsresult res = rootElement->HasChildNodes(&hasChildNodes);

  *aDocumentIsEmpty = !hasChildNodes;

  return res;
}



NS_IMETHODIMP nsEditor::SelectAll()
{
  if (!mDocWeak) { return NS_ERROR_NOT_INITIALIZED; }
  ForceCompositionEnd();

  nsCOMPtr<nsISelectionController> selCon;
  GetSelectionController(getter_AddRefs(selCon));
  NS_ENSURE_TRUE(selCon, NS_ERROR_NOT_INITIALIZED);
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
  if (!mDocWeak) { return NS_ERROR_NOT_INITIALIZED; }

  
  nsCOMPtr<nsISelection> selection;
  nsresult result = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(result, result);
  NS_ENSURE_TRUE(selection, NS_ERROR_NOT_INITIALIZED);
    
  
  nsIDOMElement *rootElement = GetRoot(); 
  NS_ENSURE_TRUE(rootElement, NS_ERROR_NULL_POINTER); 
  
  
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
      NS_ENSURE_SUCCESS(result, result);
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
  if (!mDocWeak) { return NS_ERROR_NOT_INITIALIZED; } 
  nsresult res; 

  
  nsCOMPtr<nsISelection> selection; 
  res = GetSelection(getter_AddRefs(selection)); 
  NS_ENSURE_SUCCESS(res, res); 
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER); 
  
  
  nsIDOMElement *rootElement = GetRoot(); 
  NS_ENSURE_TRUE(rootElement, NS_ERROR_NULL_POINTER); 

  nsCOMPtr<nsIDOMNode> node = do_QueryInterface(rootElement);
  nsCOMPtr<nsIDOMNode> child;
  NS_ASSERTION(node, "Invalid root element");

  do {
    node->GetLastChild(getter_AddRefs(child));

    if (child) {
      if (IsContainer(child)) {
        node = child;
      } else {
        break;
      }
    }
  } while (child);

  PRUint32 length = 0;
  res = GetLengthOfDOMNode(node, length);
  NS_ENSURE_SUCCESS(res, res);

  return selection->Collapse(node, (PRInt32)length);
} 
  
NS_IMETHODIMP
nsEditor::GetDocumentModified(PRBool *outDocModified)
{
  NS_ENSURE_TRUE(outDocModified, NS_ERROR_NULL_POINTER);

  PRInt32  modCount = 0;
  GetModificationCount(&modCount);

  *outDocModified = (modCount != 0);
  return NS_OK;
}

NS_IMETHODIMP
nsEditor::GetDocumentCharacterSet(nsACString &characterSet)
{
  nsCOMPtr<nsIDocument> doc = do_QueryReferent(mDocWeak);
  NS_ENSURE_TRUE(doc, NS_ERROR_UNEXPECTED);

  characterSet = doc->GetDocumentCharacterSet();
  return NS_OK;
}

NS_IMETHODIMP
nsEditor::SetDocumentCharacterSet(const nsACString& characterSet)
{
  nsCOMPtr<nsIDocument> doc = do_QueryReferent(mDocWeak);
  NS_ENSURE_TRUE(doc, NS_ERROR_UNEXPECTED);

  doc->SetDocumentCharacterSet(characterSet);
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
nsEditor::PasteTransferable(nsITransferable *aTransferable)
{
  return NS_ERROR_NOT_IMPLEMENTED; 
}

NS_IMETHODIMP
nsEditor::CanPaste(PRInt32 aSelectionType, PRBool *aCanPaste)
{
  return NS_ERROR_NOT_IMPLEMENTED; 
}

NS_IMETHODIMP
nsEditor::CanPasteTransferable(nsITransferable *aTransferable, PRBool *aCanPaste)
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
  NS_ENSURE_TRUE(aResultIsSet, NS_ERROR_NULL_POINTER);
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
  
  nsCOMPtr<nsIContent> element (do_QueryInterface(aNode));
  if (element)
    element->SetAttr(kNameSpaceID_None, nsEditProperty::mozdirty,
                     EmptyString(), PR_FALSE);
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
  NS_TIME_FUNCTION;

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
  NS_ENSURE_SUCCESS(result, result);
  
  PRUint32 oldLeftNodeLen;
  result = GetLengthOfDOMNode(aLeftNode, oldLeftNodeLen);
  NS_ENSURE_SUCCESS(result, result);

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
  NS_ENSURE_SUCCESS(result, result);

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
  NS_ENSURE_TRUE(inNode && outNode, NS_ERROR_NULL_POINTER);
  nsCOMPtr<nsIDOMNode> parent;
  PRInt32 offset;
  nsresult res = GetNodeLocation(inNode, address_of(parent), &offset);
  NS_ENSURE_SUCCESS(res, res);

  
  nsCOMPtr<nsIContent> newContent;

  
  res = CreateHTMLContent(aNodeType, getter_AddRefs(newContent));
  nsCOMPtr<nsIDOMElement> elem = do_QueryInterface(newContent);
  NS_ENSURE_SUCCESS(res, res);
    *outNode = do_QueryInterface(elem);
  
  
  if (aAttribute && aValue && !aAttribute->IsEmpty())
  {
    res = elem->SetAttribute(*aAttribute, *aValue);
    NS_ENSURE_SUCCESS(res, res);
  }
  if (aCloneAttributes)
  {
    nsCOMPtr<nsIDOMNode>newNode = do_QueryInterface(elem);
    res = CloneAttributes(newNode, inNode);
    NS_ENSURE_SUCCESS(res, res);
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
      NS_ENSURE_SUCCESS(res, res);

      res = InsertNode(child, *outNode, -1);
      NS_ENSURE_SUCCESS(res, res);
      inNode->HasChildNodes(&bHasMoreChildren);
    }
  }
  
  res = InsertNode( *outNode, parent, offset);
  NS_ENSURE_SUCCESS(res, res);
  
  
  return DeleteNode(inNode);
}





nsresult
nsEditor::RemoveContainer(nsIDOMNode *inNode)
{
  NS_ENSURE_TRUE(inNode, NS_ERROR_NULL_POINTER);
  nsCOMPtr<nsIDOMNode> parent;
  PRInt32 offset;
  
  nsresult res = GetNodeLocation(inNode, address_of(parent), &offset);
  NS_ENSURE_SUCCESS(res, res);
  
  
  
  PRBool bHasMoreChildren;
  inNode->HasChildNodes(&bHasMoreChildren);
  nsCOMPtr<nsIDOMNodeList> nodeList;
  res = inNode->GetChildNodes(getter_AddRefs(nodeList));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(nodeList, NS_ERROR_NULL_POINTER);
  PRUint32 nodeOrigLen;
  nodeList->GetLength(&nodeOrigLen);

  
  nsAutoRemoveContainerSelNotify selNotify(mRangeUpdater, inNode, parent, offset, nodeOrigLen);
                                   
  nsCOMPtr<nsIDOMNode> child;
  while (bHasMoreChildren)
  {
    inNode->GetLastChild(getter_AddRefs(child));
    res = DeleteNode(child);
    NS_ENSURE_SUCCESS(res, res);
    res = InsertNode(child, parent, offset);
    NS_ENSURE_SUCCESS(res, res);
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
  NS_ENSURE_TRUE(inNode && outNode, NS_ERROR_NULL_POINTER);
  nsCOMPtr<nsIDOMNode> parent;
  PRInt32 offset;
  nsresult res = GetNodeLocation(inNode, address_of(parent), &offset);
  NS_ENSURE_SUCCESS(res, res);

  
  nsCOMPtr<nsIContent> newContent;

  
  res = CreateHTMLContent(aNodeType, getter_AddRefs(newContent));
  nsCOMPtr<nsIDOMElement> elem = do_QueryInterface(newContent);
  NS_ENSURE_SUCCESS(res, res);
  *outNode = do_QueryInterface(elem);
  
  
  if (aAttribute && aValue && !aAttribute->IsEmpty())
  {
    res = elem->SetAttribute(*aAttribute, *aValue);
    NS_ENSURE_SUCCESS(res, res);
  }
  
  
  nsAutoInsertContainerSelNotify selNotify(mRangeUpdater);
  
  
  res = DeleteNode(inNode);
  NS_ENSURE_SUCCESS(res, res);

  {
    nsAutoTxnsConserveSelection conserveSelection(this);
    res = InsertNode(inNode, *outNode, 0);
    NS_ENSURE_SUCCESS(res, res);
  }

  
  return InsertNode(*outNode, parent, offset);
}



nsresult
nsEditor::MoveNode(nsIDOMNode *aNode, nsIDOMNode *aParent, PRInt32 aOffset)
{
  NS_ENSURE_TRUE(aNode && aParent, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsIDOMNode> oldParent;
  PRInt32 oldOffset;
  nsresult res = GetNodeLocation(aNode, address_of(oldParent), &oldOffset);
  
  if (aOffset == -1)
  {
    PRUint32 unsignedOffset;
    
    res = GetLengthOfDOMNode(aParent, unsignedOffset);
    NS_ENSURE_SUCCESS(res, res);
    aOffset = (PRInt32)unsignedOffset;
  }
  
  
  if ((aParent == oldParent.get()) && (oldOffset == aOffset)) return NS_OK;
  
  
  nsAutoMoveNodeSelNotify selNotify(mRangeUpdater, oldParent, oldOffset, aParent, aOffset);
  
  
  if ((aParent == oldParent.get()) && (oldOffset < aOffset)) 
  {
    aOffset--;  
  }

  
  res = DeleteNode(aNode);
  NS_ENSURE_SUCCESS(res, res);
  return InsertNode(aNode, aParent, aOffset);
}


NS_IMETHODIMP
nsEditor::AddEditorObserver(nsIEditorObserver *aObserver)
{
  
  
  
  NS_ENSURE_TRUE(aObserver, NS_ERROR_NULL_POINTER);

  
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
  NS_ENSURE_TRUE(aObserver, NS_ERROR_FAILURE);

  if (!mEditorObservers.RemoveObject(aObserver))
    return NS_ERROR_FAILURE;

  return NS_OK;
}

void nsEditor::NotifyEditorObservers(void)
{
  for (PRInt32 i = 0; i < mEditorObservers.Count(); i++)
    mEditorObservers[i]->EditAction();
}


NS_IMETHODIMP
nsEditor::AddEditActionListener(nsIEditActionListener *aListener)
{
  NS_ENSURE_TRUE(aListener, NS_ERROR_NULL_POINTER);

  
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
  NS_ENSURE_TRUE(aListener, NS_ERROR_FAILURE);

  if (!mActionListeners.RemoveObject(aListener))
    return NS_ERROR_FAILURE;

  return NS_OK;
}


NS_IMETHODIMP
nsEditor::AddDocumentStateListener(nsIDocumentStateListener *aListener)
{
  NS_ENSURE_TRUE(aListener, NS_ERROR_NULL_POINTER);

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
  NS_ENSURE_TRUE(aListener, NS_ERROR_NULL_POINTER);

  if (!mDocStateListeners.RemoveObject(aListener))
    return NS_ERROR_FAILURE;

  return NS_OK;
}


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
  NS_ENSURE_TRUE(doc, NS_ERROR_NOT_INITIALIZED);

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


nsresult
nsEditor::BeginIMEComposition()
{
  mInIMEMode = PR_TRUE;
  if (mPhonetic) {
    mPhonetic->Truncate(0);
  }
  return NS_OK;
}

nsresult
nsEditor::EndIMEComposition()
{
  NS_ENSURE_TRUE(mInIMEMode, NS_OK); 

  nsresult rv = NS_OK;

  
  
  if (mTxnMgr) {
    nsCOMPtr<nsITransaction> txn;
    rv = mTxnMgr->PeekUndoStack(getter_AddRefs(txn));
    NS_ASSERTION(NS_SUCCEEDED(rv), "PeekUndoStack() failed");
    nsCOMPtr<nsIAbsorbingTransaction> plcTxn = do_QueryInterface(txn);
    if (plcTxn) {
      rv = plcTxn->Commit();
      NS_ASSERTION(NS_SUCCEEDED(rv),
                   "nsIAbsorbingTransaction::Commit() failed");
    }
  }

  
  mIMETextNode = nsnull;
  mIMETextOffset = 0;
  mIMEBufferLength = 0;
  mInIMEMode = PR_FALSE;
  mIsIMEComposing = PR_FALSE;

  
  NotifyEditorObservers();

  return rv;
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
GetEditorContentWindow(nsIDOMElement *aRoot, nsIWidget **aResult)
{
  NS_ENSURE_TRUE(aRoot && aResult, NS_ERROR_NULL_POINTER);

  *aResult = 0;

  nsCOMPtr<nsIContent> content = do_QueryInterface(aRoot);

  NS_ENSURE_TRUE(content, NS_ERROR_FAILURE);

  
  nsIFrame *frame = content->GetPrimaryFrame();

  NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);

  *aResult = frame->GetNearestWidget();
  NS_ENSURE_TRUE(*aResult, NS_ERROR_FAILURE);

  NS_ADDREF(*aResult);
  return NS_OK;
}

nsresult
nsEditor::GetWidget(nsIWidget **aWidget)
{
  NS_ENSURE_TRUE(aWidget, NS_ERROR_NULL_POINTER);
  *aWidget = nsnull;

  nsCOMPtr<nsIWidget> widget;
  nsresult res = GetEditorContentWindow(GetRoot(), getter_AddRefs(widget));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(widget, NS_ERROR_NOT_AVAILABLE);

  NS_ADDREF(*aWidget = widget);

  return NS_OK;
}

NS_IMETHODIMP
nsEditor::ForceCompositionEnd()
{






#if defined(XP_MACOSX) || defined(XP_WIN) || defined(XP_OS2)
  
  
  
  
  
  
  
  if(! mInIMEMode)
    return NS_OK;
#endif

  nsCOMPtr<nsIWidget> widget;
  nsresult res = GetWidget(getter_AddRefs(widget));
  NS_ENSURE_SUCCESS(res, res);

  if (widget) {
    res = widget->ResetInputState();
    NS_ENSURE_SUCCESS(res, res);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsEditor::GetPreferredIMEState(PRUint32 *aState)
{
  NS_ENSURE_ARG_POINTER(aState);
  *aState = nsIContent::IME_STATUS_ENABLE;

  if (IsReadonly() || IsDisabled()) {
    *aState = nsIContent::IME_STATUS_DISABLE;
    return NS_OK;
  }

  nsCOMPtr<nsIContent> content = do_QueryInterface(GetRoot());
  NS_ENSURE_TRUE(content, NS_ERROR_FAILURE);

  nsIFrame* frame = content->GetPrimaryFrame();
  NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);

  switch (frame->GetStyleUIReset()->mIMEMode) {
    case NS_STYLE_IME_MODE_AUTO:
      if (IsPasswordEditor())
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




NS_IMETHODIMP
nsEditor::GetRootElement(nsIDOMElement **aRootElement)
{
  NS_ENSURE_ARG_POINTER(aRootElement);
  NS_ENSURE_TRUE(mRootElement, NS_ERROR_NOT_AVAILABLE);
  *aRootElement = mRootElement;
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
  NS_ENSURE_TRUE(aDestNode && aSourceNode, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsIDOMElement> destElement = do_QueryInterface(aDestNode);
  nsCOMPtr<nsIDOMElement> sourceElement = do_QueryInterface(aSourceNode);
  NS_ENSURE_TRUE(destElement && sourceElement, NS_ERROR_NO_INTERFACE);

  nsAutoString attrValue;
  PRBool isAttrSet;
  nsresult rv = GetAttributeValue(sourceElement,
                                  aAttribute,
                                  attrValue,
                                  &isAttrSet);
  NS_ENSURE_SUCCESS(rv, rv);
  if (isAttrSet)
    rv = SetAttribute(destElement, aAttribute, attrValue);
  else
    rv = RemoveAttribute(destElement, aAttribute);

  return rv;
}


NS_IMETHODIMP
nsEditor::CloneAttributes(nsIDOMNode *aDestNode, nsIDOMNode *aSourceNode)
{
  NS_ENSURE_TRUE(aDestNode && aSourceNode, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsIDOMElement> destElement = do_QueryInterface(aDestNode);
  nsCOMPtr<nsIDOMElement> sourceElement = do_QueryInterface(aSourceNode);
  NS_ENSURE_TRUE(destElement && sourceElement, NS_ERROR_NO_INTERFACE);

  nsCOMPtr<nsIDOMNamedNodeMap> sourceAttributes;
  sourceElement->GetAttributes(getter_AddRefs(sourceAttributes));
  nsCOMPtr<nsIDOMNamedNodeMap> destAttributes;
  destElement->GetAttributes(getter_AddRefs(destAttributes));
  NS_ENSURE_TRUE(sourceAttributes && destAttributes, NS_ERROR_FAILURE);

  nsAutoEditBatch beginBatching(this);

  
  
  nsIDOMElement *rootElement = GetRoot();
  NS_ENSURE_TRUE(rootElement, NS_ERROR_NULL_POINTER);

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


NS_IMETHODIMP nsEditor::ScrollSelectionIntoView(PRBool aScrollToAnchor)
{
  nsCOMPtr<nsISelectionController> selCon;
  if (NS_SUCCEEDED(GetSelectionController(getter_AddRefs(selCon))) && selCon)
  {
    PRInt16 region = nsISelectionController::SELECTION_FOCUS_REGION;

    if (aScrollToAnchor)
      region = nsISelectionController::SELECTION_ANCHOR_REGION;

    selCon->ScrollSelectionIntoView(nsISelectionController::SELECTION_NORMAL,
                                    region, 0);
  }

  return NS_OK;
}

NS_IMETHODIMP nsEditor::InsertTextImpl(const nsAString& aStringToInsert, 
                                          nsCOMPtr<nsIDOMNode> *aInOutNode, 
                                          PRInt32 *aInOutOffset,
                                          nsIDOMDocument *aDoc)
{
  
  
  
  
  nsresult res;
  NS_ENSURE_TRUE(aInOutNode && *aInOutNode && aInOutOffset && aDoc, NS_ERROR_NULL_POINTER);
  if (!mInIMEMode && aStringToInsert.IsEmpty()) return NS_OK;
  nsCOMPtr<nsIDOMText> nodeAsText = do_QueryInterface(*aInOutNode);
  if (!nodeAsText && IsPlaintextEditor()) {
    
    
    
    
    if (*aInOutNode == GetRoot() && *aInOutOffset == 0) {
      nsCOMPtr<nsIDOMNode> possibleTextNode;
      res = (*aInOutNode)->GetFirstChild(getter_AddRefs(possibleTextNode));
      if (NS_SUCCEEDED(res)) {
        nodeAsText = do_QueryInterface(possibleTextNode);
        if (nodeAsText) {
          *aInOutNode = possibleTextNode;
        }
      }
    }
    
    
    
    if (!nodeAsText && *aInOutNode == GetRoot() && *aInOutOffset > 0) {
      nsCOMPtr<nsIDOMNodeList> children;
      res = (*aInOutNode)->GetChildNodes(getter_AddRefs(children));
      if (NS_SUCCEEDED(res)) {
        nsCOMPtr<nsIDOMNode> possibleMozBRNode;
        children->Item(*aInOutOffset, getter_AddRefs(possibleMozBRNode));
        if (possibleMozBRNode && nsTextEditUtils::IsMozBR(possibleMozBRNode)) {
          nsCOMPtr<nsIDOMNode> possibleTextNode;
          res = children->Item(*aInOutOffset - 1, getter_AddRefs(possibleTextNode));
          if (NS_SUCCEEDED(res)) {
            nodeAsText = do_QueryInterface(possibleTextNode);
            if (nodeAsText) {
              PRUint32 length;
              res = nodeAsText->GetLength(&length);
              if (NS_SUCCEEDED(res)) {
                *aInOutOffset = PRInt32(length);
                *aInOutNode = possibleTextNode;
              }
            }
          }
        } else {
          
          
          nsCOMPtr<nsIDOMNode> possibleTextNode;
          res = children->Item(*aInOutOffset - 1, getter_AddRefs(possibleTextNode));
          nodeAsText = do_QueryInterface(possibleTextNode);
          if (nodeAsText) {
            PRUint32 length;
            res = nodeAsText->GetLength(&length);
            if (NS_SUCCEEDED(res)) {
              *aInOutOffset = PRInt32(length);
              *aInOutNode = possibleTextNode;
            }
          }
        }
      }
    }
    
    
    
    if (nsTextEditUtils::IsMozBR(*aInOutNode) && *aInOutOffset == 0) {
      nsCOMPtr<nsIDOMNode> previous;
      (*aInOutNode)->GetPreviousSibling(getter_AddRefs(previous));
      nodeAsText = do_QueryInterface(previous);
      if (nodeAsText) {
        PRUint32 length;
        res = nodeAsText->GetLength(&length);
        if (NS_SUCCEEDED(res)) {
          *aInOutOffset = PRInt32(length);
          *aInOutNode = previous;
        }
      } else {
        nsCOMPtr<nsIDOMNode> parent;
        (*aInOutNode)->GetParentNode(getter_AddRefs(parent));
        if (parent == GetRoot()) {
          *aInOutNode = parent;
        }
      }
    }
  }
  PRInt32 offset = *aInOutOffset;
  if (mInIMEMode)
  {
    if (!nodeAsText)
    {
      
      res = aDoc->CreateTextNode(EmptyString(), getter_AddRefs(nodeAsText));
      NS_ENSURE_SUCCESS(res, res);
      NS_ENSURE_TRUE(nodeAsText, NS_ERROR_NULL_POINTER);
      nsCOMPtr<nsIDOMNode> newNode = do_QueryInterface(nodeAsText);
      
      res = InsertNode(newNode, *aInOutNode, offset);
      NS_ENSURE_SUCCESS(res, res);
      offset = 0;
    }
    res = InsertTextIntoTextNodeImpl(aStringToInsert, nodeAsText, offset);
    NS_ENSURE_SUCCESS(res, res);
  }
  else
  {
    if (nodeAsText)
    {
      
      res = InsertTextIntoTextNodeImpl(aStringToInsert, nodeAsText, offset);
      NS_ENSURE_SUCCESS(res, res);
      *aInOutOffset += aStringToInsert.Length();
    }
    else
    {
      
      
      res = aDoc->CreateTextNode(aStringToInsert, getter_AddRefs(nodeAsText));
      NS_ENSURE_SUCCESS(res, res);
      NS_ENSURE_TRUE(nodeAsText, NS_ERROR_NULL_POINTER);
      nsCOMPtr<nsIDOMNode> newNode = do_QueryInterface(nodeAsText);
      
      res = InsertNode(newNode, *aInOutNode, offset);
      NS_ENSURE_SUCCESS(res, res);
      *aInOutNode = newNode;
      *aInOutOffset = aStringToInsert.Length();
    }
  }
  return res;
}


nsresult nsEditor::InsertTextIntoTextNodeImpl(const nsAString& aStringToInsert, 
                                              nsIDOMCharacterData *aTextNode, 
                                              PRInt32 aOffset,
                                              PRBool aSuppressIME)
{
  nsRefPtr<EditTxn> txn;
  nsresult result = NS_OK;
  PRBool isIMETransaction = PR_FALSE;
  
  
  if (mIMETextRangeList && mInIMEMode && !aSuppressIME)
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
    isIMETransaction = PR_TRUE;
  }
  else
  {
    nsRefPtr<InsertTextTxn> insertTxn;
    result = CreateTxnForInsertText(aStringToInsert, aTextNode, aOffset,
                                    getter_AddRefs(insertTxn));
    txn = insertTxn;
  }
  NS_ENSURE_SUCCESS(result, result);

  
  PRInt32 i;
  for (i = 0; i < mActionListeners.Count(); i++)
    mActionListeners[i]->WillInsertText(aTextNode, aOffset, aStringToInsert);
  
  
  BeginUpdateViewBatch();
  result = DoTransaction(txn);
  EndUpdateViewBatch();

  mRangeUpdater.SelAdjInsertText(aTextNode, aOffset, aStringToInsert);
  
  
  for (i = 0; i < mActionListeners.Count(); i++)
    mActionListeners[i]->DidInsertText(aTextNode, aOffset, aStringToInsert, result);

  
  
  
  
  
  
  
  
  
  
  if (isIMETransaction && mIMETextNode)
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
  NS_ENSURE_TRUE(aRoot && outFirstNode, NS_ERROR_NULL_POINTER);
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
  NS_ENSURE_TRUE(aRoot && outLastNode, NS_ERROR_NULL_POINTER);
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
        NS_ENSURE_SUCCESS(rv, rv);
        
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
  NS_ENSURE_TRUE(aTextNode && aTxn, NS_ERROR_NULL_POINTER);
  nsresult rv;

  nsRefPtr<InsertTextTxn> txn = new InsertTextTxn();
  rv = txn->Init(aTextNode, aOffset, aStringToInsert, this);
  if (NS_SUCCEEDED(rv))
  {
    txn.forget(aTxn);
  }

  return rv;
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
  NS_ENSURE_TRUE(aElement, NS_ERROR_NULL_POINTER);

  nsRefPtr<DeleteTextTxn> txn = new DeleteTextTxn();

  nsresult rv = txn->Init(this, aElement, aOffset, aLength, &mRangeUpdater);
  if (NS_SUCCEEDED(rv))
  {
    txn.forget(aTxn);
  }

  return rv;
}




NS_IMETHODIMP nsEditor::CreateTxnForSplitNode(nsIDOMNode *aNode,
                                         PRUint32    aOffset,
                                         SplitElementTxn **aTxn)
{
  NS_ENSURE_TRUE(aNode, NS_ERROR_NULL_POINTER);

  nsRefPtr<SplitElementTxn> txn = new SplitElementTxn();

  nsresult rv = txn->Init(this, aNode, aOffset);
  if (NS_SUCCEEDED(rv))
  {
    txn.forget(aTxn);
  }

  return rv;
}

NS_IMETHODIMP nsEditor::CreateTxnForJoinNode(nsIDOMNode  *aLeftNode,
                                             nsIDOMNode  *aRightNode,
                                             JoinElementTxn **aTxn)
{
  NS_ENSURE_TRUE(aLeftNode && aRightNode, NS_ERROR_NULL_POINTER);

  nsRefPtr<JoinElementTxn> txn = new JoinElementTxn();

  nsresult rv = txn->Init(this, aLeftNode, aRightNode);
  if (NS_SUCCEEDED(rv))
  {
    txn.forget(aTxn);
  }

  return rv;
}







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
    NS_ENSURE_SUCCESS(result, result);
    NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);

    
    nsCOMPtr<nsIDOMNode> selStartNode, selEndNode;
    PRInt32 selStartOffset, selEndOffset;
    result = GetStartNodeAndOffset(selection, getter_AddRefs(selStartNode), &selStartOffset);
    if (NS_FAILED(result)) selStartNode = nsnull;  
    result = GetEndNodeAndOffset(selection, getter_AddRefs(selEndNode), &selEndOffset);
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
        
        nsCOMPtr<nsIPresShell> ps = GetPresShell();
        if (ps)
          ps->FlushPendingNotifications(Flush_Frames);

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
  nsresult result = NS_OK;
  if (aNodeToKeep && aNodeToJoin && aParent)
  {
    
    nsCOMPtr<nsISelection> selection;
    GetSelection(getter_AddRefs(selection));
    NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);

    
    nsCOMPtr<nsIDOMNode> selStartNode, selEndNode;
    PRInt32 selStartOffset, selEndOffset, joinOffset, keepOffset;
    result = GetStartNodeAndOffset(selection, getter_AddRefs(selStartNode), &selStartOffset);
    if (NS_FAILED(result)) selStartNode = nsnull;
    result = GetEndNodeAndOffset(selection, getter_AddRefs(selEndNode), &selEndOffset);
    
    if (NS_FAILED(result)) selStartNode = nsnull;

    nsCOMPtr<nsIDOMNode> leftNode;
    if (aNodeToKeepIsFirst)
      leftNode = aNodeToKeep;
    else
      leftNode = aNodeToJoin;

    PRUint32 firstNodeLength;
    result = GetLengthOfDOMNode(leftNode, firstNodeLength);
    NS_ENSURE_SUCCESS(result, result);
    nsCOMPtr<nsIDOMNode> parent;
    result = GetNodeLocation(aNodeToJoin, address_of(parent), &joinOffset);
    NS_ENSURE_SUCCESS(result, result);
    result = GetNodeLocation(aNodeToKeep, address_of(parent), &keepOffset);
    NS_ENSURE_SUCCESS(result, result);
    
    
    
    
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
  NS_ENSURE_TRUE(cChild && content, NS_ERROR_NULL_POINTER);

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
  NS_ENSURE_SUCCESS(result, result);
  
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
  NS_ENSURE_SUCCESS(result, result);
  
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
  NS_ENSURE_TRUE(aCurrentNode, nsnull);
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
  NS_ENSURE_TRUE(aCurrentNode, nsnull);
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
  NS_ENSURE_TRUE(parentElement, PR_FALSE);
  
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
    NS_ENSURE_TRUE(childElement, PR_FALSE);
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
  NS_ENSURE_TRUE(inNode, PR_FALSE);

  nsIDOMElement *rootElement = GetRoot();

  nsCOMPtr<nsIDOMNode> rootNode = do_QueryInterface(rootElement);

  return inNode == rootNode;
}

PRBool 
nsEditor::IsDescendantOfBody(nsIDOMNode *inNode) 
{
  NS_ENSURE_TRUE(inNode, PR_FALSE);
  nsIDOMElement *rootElement = GetRoot();
  NS_ENSURE_TRUE(rootElement, PR_FALSE);
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
  NS_ENSURE_TRUE(aNode, PR_FALSE);

  if (IsMozEditorBogusNode(aNode) || !IsModifiableNode(aNode)) return PR_FALSE;

  
  
  nsCOMPtr<nsIContent> content = do_QueryInterface(aNode);
  if (content)
  {
    nsIFrame *resultFrame = content->GetPrimaryFrame();
    if (!resultFrame)   
      return PR_FALSE;
    NS_ASSERTION(content->IsNodeOfType(nsINode::eTEXT) ||
                 content->IsElement(),
                 "frame for non element-or-text?");
    if (!content->IsNodeOfType(nsINode::eTEXT))
      return PR_TRUE;  

    
    while (resultFrame) {
      if (resultFrame->GetStateBits() & NS_FRAME_IS_DIRTY) 
      {
        
        
        
        
        
        return IsTextInDirtyFrameVisible(aNode);
      }
      if (resultFrame->HasAnyNoncollapsedCharacters()) {
        return PR_TRUE;
      }
      resultFrame = resultFrame->GetNextContinuation();
    }
  }
  return PR_FALSE;  
}

PRBool
nsEditor::IsMozEditorBogusNode(nsIDOMNode *aNode)
{
  nsCOMPtr<nsIContent> element = do_QueryInterface(aNode);
  return element &&
         element->AttrValueIs(kNameSpaceID_None, kMOZEditorBogusNodeAttrAtom,
                              kMOZEditorBogusNodeValue, eCaseMatters);
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
  nsCOMPtr<nsINode> parentNode = do_QueryInterface(parent);
  NS_PRECONDITION(parentNode, "null parentNode in nsEditor::GetIndexOf");
  NS_PRECONDITION(parentNode->IsNodeOfType(nsINode::eCONTENT) ||
                    parentNode->IsNodeOfType(nsINode::eDOCUMENT),
                  "The parent node must be an element node or a document node");

  nsCOMPtr<nsIContent> cChild = do_QueryInterface(child);
  NS_PRECONDITION(cChild, "null content in nsEditor::GetIndexOf");

  return parentNode->IndexOf(cChild);
}
  




nsCOMPtr<nsIDOMNode> 
nsEditor::GetChildAt(nsIDOMNode *aParent, PRInt32 aOffset)
{
  nsCOMPtr<nsIDOMNode> resultNode;
  
  nsCOMPtr<nsIContent> parent = do_QueryInterface(aParent);

  NS_ENSURE_TRUE(parent, resultNode);

  resultNode = do_QueryInterface(parent->GetChildAt(aOffset));

  return resultNode;
}
  





nsresult 
nsEditor::GetStartNodeAndOffset(nsISelection *aSelection,
                                       nsIDOMNode **outStartNode,
                                       PRInt32 *outStartOffset)
{
  NS_ENSURE_TRUE(outStartNode && outStartOffset && aSelection, NS_ERROR_NULL_POINTER);

  *outStartNode = nsnull;
  *outStartOffset = 0;

  nsCOMPtr<nsISelectionPrivate>selPrivate(do_QueryInterface(aSelection));
  nsCOMPtr<nsIEnumerator> enumerator;
  nsresult result = selPrivate->GetEnumerator(getter_AddRefs(enumerator));
  NS_ENSURE_SUCCESS(result, result);
  NS_ENSURE_TRUE(enumerator, NS_ERROR_FAILURE);

  enumerator->First(); 
  nsCOMPtr<nsISupports> currentItem;
  result = enumerator->CurrentItem(getter_AddRefs(currentItem));
  NS_ENSURE_SUCCESS(result, result);

  nsCOMPtr<nsIDOMRange> range( do_QueryInterface(currentItem) );
  NS_ENSURE_TRUE(range, NS_ERROR_FAILURE);

  result = range->GetStartContainer(outStartNode);
  NS_ENSURE_SUCCESS(result, result);

  result = range->GetStartOffset(outStartOffset);
  NS_ENSURE_SUCCESS(result, result);

  return NS_OK;
}





nsresult 
nsEditor::GetEndNodeAndOffset(nsISelection *aSelection,
                                       nsIDOMNode **outEndNode,
                                       PRInt32 *outEndOffset)
{
  NS_ENSURE_TRUE(outEndNode && outEndOffset, NS_ERROR_NULL_POINTER);

  *outEndNode = nsnull;
    
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
  NS_ENSURE_TRUE(range, NS_ERROR_FAILURE);
    
  if (NS_FAILED(range->GetEndContainer(outEndNode)))
    return NS_ERROR_FAILURE;
    
  if (NS_FAILED(range->GetEndOffset(outEndOffset)))
    return NS_ERROR_FAILURE;
    
  return NS_OK;
}





nsresult 
nsEditor::IsPreformatted(nsIDOMNode *aNode, PRBool *aResult)
{
  nsCOMPtr<nsIContent> content = do_QueryInterface(aNode);
  
  NS_ENSURE_TRUE(aResult && content, NS_ERROR_NULL_POINTER);
  
  nsCOMPtr<nsIPresShell> ps = GetPresShell();
  NS_ENSURE_TRUE(ps, NS_ERROR_NOT_INITIALIZED);

  
  nsRefPtr<nsStyleContext> elementStyle;
  if (!content->IsElement()) {
    content = content->GetParent();
  }
  if (content && content->IsElement()) {
    elementStyle = nsComputedDOMStyle::GetStyleContextForElement(content->AsElement(),
                                                                 nsnull,
                                                                 ps);
  }

  if (!elementStyle)
  {
    
    
    
    *aResult = PR_FALSE;
    return NS_OK;
  }

  const nsStyleText* styleText = elementStyle->GetStyleText();

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
  NS_ENSURE_TRUE(aNode && aSplitPointParent && outOffset, NS_ERROR_NULL_POINTER);
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
    NS_ENSURE_SUCCESS(res, res);
    
    if (!(aNoEmptyContainers || nodeAsText) || (offset && (offset != (PRInt32)len)))
    {
      bDoSplit = PR_TRUE;
      res = SplitNode(nodeToSplit, offset, getter_AddRefs(tempNode));
      NS_ENSURE_SUCCESS(res, res);
      if (outRightNode) *outRightNode = nodeToSplit;
      if (outLeftNode)  *outLeftNode  = tempNode;
    }

    res = nodeToSplit->GetParentNode(getter_AddRefs(parentNode));
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_TRUE(parentNode, NS_ERROR_FAILURE);

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
  NS_ENSURE_TRUE(aLeftNode && aRightNode && aOutJoinNode && outOffset, NS_ERROR_NULL_POINTER);

  
  
  
  
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
      NS_ENSURE_SUCCESS(res, res);
    }
    
    *aOutJoinNode = rightNodeToJoin;
    *outOffset = length;
    
    
    res = JoinNodes(leftNodeToJoin, rightNodeToJoin, parentNode);
    NS_ENSURE_SUCCESS(res, res);
    
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

    
    nsCOMPtr<nsIPresShell> ps = GetPresShell();
    if (ps) {
      nsCOMPtr<nsIViewManager> viewManager = ps->GetViewManager();
      if (viewManager) {
        mBatch.BeginUpdateViewBatch(viewManager);
      }
    }
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
    nsCOMPtr<nsIPresShell> presShell = GetPresShell();

    if (presShell)
      caret = presShell->GetCaret();

    StCaretHider caretHider(caret);

    PRUint32 flags = 0;

    GetFlags(&flags);

    
    PRUint32 updateFlag = NS_VMREFRESH_IMMEDIATE;

    
    
    if (flags & nsIPlaintextEditor::eEditorUseAsyncUpdatesMask) {
      updateFlag = NS_VMREFRESH_DEFERRED;
    }
    mBatch.EndUpdateViewBatch(updateFlag);

    

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


NS_IMETHODIMP 
nsEditor::DeleteSelectionImpl(nsIEditor::EDirection aAction)
{
  nsCOMPtr<nsISelection>selection;
  nsresult res = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);
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
  NS_ENSURE_SUCCESS(result, result);

  nsCOMPtr<nsIDOMNode> newNode;
  result = CreateNode(aTag, parentSelectedNode, offsetOfNewNode,
                      getter_AddRefs(newNode));
  
  *aNewNode = newNode;
  NS_IF_ADDREF(*aNewNode);

  
  nsCOMPtr<nsISelection> selection;
  result = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(result, result);
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);
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
  NS_ENSURE_SUCCESS(result, result);
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);

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

    nsCOMPtr<nsIDOMNode> selectedNode;
    selection->GetAnchorNode(getter_AddRefs(selectedNode));
    
    
    if (selectedNode)
    {
      PRBool testCollapsed = PR_FALSE;
      selection->GetIsCollapsed(&testCollapsed);
      if (!testCollapsed) {
        result = selection->CollapseToEnd();
        NS_ENSURE_SUCCESS(result, result);
      }
    }
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
  NS_ENSURE_SUCCESS(rv, rv);
  
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
  NS_ENSURE_TRUE(aElement, NS_ERROR_NULL_POINTER);

  nsRefPtr<ChangeAttributeTxn> txn = new ChangeAttributeTxn();

  nsresult rv = txn->Init(this, aElement, aAttribute, aValue, PR_FALSE);
  if (NS_SUCCEEDED(rv))
  {
    txn.forget(aTxn);
  }

  return rv;
}


NS_IMETHODIMP 
nsEditor::CreateTxnForRemoveAttribute(nsIDOMElement *aElement, 
                                      const nsAString& aAttribute,
                                      ChangeAttributeTxn ** aTxn)
{
  NS_ENSURE_TRUE(aElement, NS_ERROR_NULL_POINTER);

  nsRefPtr<ChangeAttributeTxn> txn = new ChangeAttributeTxn();

  nsresult rv = txn->Init(this, aElement, aAttribute, EmptyString(), PR_TRUE);
  if (NS_SUCCEEDED(rv))
  {
    txn.forget(aTxn);
  }

  return rv;
}


NS_IMETHODIMP nsEditor::CreateTxnForCreateElement(const nsAString& aTag,
                                                  nsIDOMNode     *aParent,
                                                  PRInt32         aPosition,
                                                  CreateElementTxn ** aTxn)
{
  NS_ENSURE_TRUE(aParent, NS_ERROR_NULL_POINTER);

  nsRefPtr<CreateElementTxn> txn = new CreateElementTxn();

  nsresult rv = txn->Init(this, aTag, aParent, aPosition);
  if (NS_SUCCEEDED(rv))
  {
    txn.forget(aTxn);
  }

  return rv;
}


NS_IMETHODIMP nsEditor::CreateTxnForInsertElement(nsIDOMNode * aNode,
                                                  nsIDOMNode * aParent,
                                                  PRInt32      aPosition,
                                                  InsertElementTxn ** aTxn)
{
  NS_ENSURE_TRUE(aNode && aParent, NS_ERROR_NULL_POINTER);

  nsRefPtr<InsertElementTxn> txn = new InsertElementTxn();

  nsresult rv = txn->Init(aNode, aParent, aPosition, this);
  if (NS_SUCCEEDED(rv))
  {
    txn.forget(aTxn);
  }

  return rv;
}

NS_IMETHODIMP nsEditor::CreateTxnForDeleteElement(nsIDOMNode * aElement,
                                             DeleteElementTxn ** aTxn)
{
  NS_ENSURE_TRUE(aElement, NS_ERROR_NULL_POINTER);

  nsRefPtr<DeleteElementTxn> txn = new DeleteElementTxn();

  nsresult rv = txn->Init(this, aElement, &mRangeUpdater);
  if (NS_SUCCEEDED(rv))
  {
    txn.forget(aTxn);
  }

  return rv;
}

NS_IMETHODIMP 
nsEditor::CreateTxnForIMEText(const nsAString& aStringToInsert,
                              IMETextTxn ** aTxn)
{
  NS_ASSERTION(aTxn, "illegal value- null ptr- aTxn");
     
  nsRefPtr<IMETextTxn> txn = new IMETextTxn();

  nsresult rv = txn->Init(mIMETextNode, mIMETextOffset, mIMEBufferLength,
                          mIMETextRangeList, aStringToInsert, this);
  if (NS_SUCCEEDED(rv))
  {
    txn.forget(aTxn);
  }

  return rv;
}


NS_IMETHODIMP 
nsEditor::CreateTxnForAddStyleSheet(nsCSSStyleSheet* aSheet, AddStyleSheetTxn* *aTxn)
{
  nsRefPtr<AddStyleSheetTxn> txn = new AddStyleSheetTxn();

  nsresult rv = txn->Init(this, aSheet);
  if (NS_SUCCEEDED(rv))
  {
    txn.forget(aTxn);
  }

  return rv;
}



NS_IMETHODIMP 
nsEditor::CreateTxnForRemoveStyleSheet(nsCSSStyleSheet* aSheet, RemoveStyleSheetTxn* *aTxn)
{
  nsRefPtr<RemoveStyleSheetTxn> txn = new RemoveStyleSheetTxn();

  nsresult rv = txn->Init(this, aSheet);
  if (NS_SUCCEEDED(rv))
  {
    txn.forget(aTxn);
  }

  return rv;
}


NS_IMETHODIMP
nsEditor::CreateTxnForDeleteSelection(nsIEditor::EDirection aAction,
                                      EditAggregateTxn ** aTxn,
                                      nsIDOMNode ** aNode,
                                      PRInt32 *aOffset,
                                      PRInt32 *aLength)
{
  NS_ENSURE_TRUE(aTxn, NS_ERROR_NULL_POINTER);
  *aTxn = nsnull;

  nsRefPtr<EditAggregateTxn> aggTxn;
  nsCOMPtr<nsISelectionController> selCon;
  GetSelectionController(getter_AddRefs(selCon));
  NS_ENSURE_TRUE(selCon, NS_ERROR_NOT_INITIALIZED);
  nsCOMPtr<nsISelection> selection;
  nsresult result = selCon->GetSelection(nsISelectionController::SELECTION_NORMAL,
                                         getter_AddRefs(selection));
  if ((NS_SUCCEEDED(result)) && selection)
  {
    
    PRBool isCollapsed;
    result = (selection->GetIsCollapsed(&isCollapsed));
    if (NS_SUCCEEDED(result) && isCollapsed && aAction == eNone)
      return NS_OK;

    
    aggTxn = new EditAggregateTxn();

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
              aggTxn->AppendChild(txn);
            }
            else
              result = NS_ERROR_OUT_OF_MEMORY;
          }
          
          
          else if (aAction != eNone)
          { 
            result = CreateTxnForDeleteInsertionPoint(range, aAction, aggTxn, aNode, aOffset, aLength);
          }
        }
      }
    }
  }

  
  
  if (NS_SUCCEEDED(result))
  {
    aggTxn.forget(aTxn);
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
  PRUint32 segOffset = aOffset, segLength = 1;
  if (aDirection == eNext) {
    if (segOffset + 1 < data.Length() &&
        NS_IS_HIGH_SURROGATE(data[segOffset]) &&
        NS_IS_LOW_SURROGATE(data[segOffset+1])) {
      
      ++segLength;
    }
  } else if (aOffset > 0) {
    --segOffset;
    if (segOffset > 0 &&
      NS_IS_LOW_SURROGATE(data[segOffset]) &&
      NS_IS_HIGH_SURROGATE(data[segOffset-1])) {
      ++segLength;
      --segOffset;
    }
  } else {
    return NS_ERROR_FAILURE;
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
  NS_ASSERTION(aAction != eNone, "invalid action");

  
  nsCOMPtr<nsIDOMNode> node;
  nsresult result = aRange->GetStartContainer(getter_AddRefs(node));
  NS_ENSURE_SUCCESS(result, result);

  PRInt32 offset;
  result = aRange->GetStartOffset(&offset);
  NS_ENSURE_SUCCESS(result, result);

  
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
          nsRefPtr<DeleteTextTxn> txn;
          result = CreateTxnForDeleteCharacter(priorNodeAsText, length,
                                               ePrevious, getter_AddRefs(txn));
          if (NS_SUCCEEDED(result)) {
            aTxn->AppendChild(txn);
            NS_ADDREF(*aNode = priorNode);
            *aOffset = txn->GetOffset();
            *aLength = txn->GetNumCharsToDelete();
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
        nsRefPtr<DeleteElementTxn> txn;
        result = CreateTxnForDeleteElement(priorNode, getter_AddRefs(txn));
        if (NS_SUCCEEDED(result)) {
          aTxn->AppendChild(txn);
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
          nsRefPtr<DeleteTextTxn> txn;
          result = CreateTxnForDeleteCharacter(nextNodeAsText, 0, eNext,
                                               getter_AddRefs(txn));
          if (NS_SUCCEEDED(result)) {
            aTxn->AppendChild(txn);
            NS_ADDREF(*aNode = nextNode);
            *aOffset = txn->GetOffset();
            *aLength = txn->GetNumCharsToDelete();
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
        nsRefPtr<DeleteElementTxn> txn;
        result = CreateTxnForDeleteElement(nextNode, getter_AddRefs(txn));
        if (NS_SUCCEEDED(result)) {
          aTxn->AppendChild(txn);
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
  NS_ENSURE_SUCCESS(result, result);

  NS_ENSURE_TRUE(*aRange, NS_ERROR_NULL_POINTER);

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
  NS_ENSURE_TRUE(aNode, NS_ERROR_NULL_POINTER);
  nsCOMPtr<nsISelection> selection;
  nsresult res = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);
  if(!selection) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMNode> parentNode;
  res = aNode->GetParentNode(getter_AddRefs(parentNode));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(parentNode, NS_ERROR_NULL_POINTER);
  
  PRInt32 offset;
  res = GetChildOffset(aNode, parentNode, offset);
  NS_ENSURE_SUCCESS(res, res);
  
  nsCOMPtr<nsIDOMRange> range;
  res = CreateRange(parentNode, offset, parentNode, offset+1, getter_AddRefs(range));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(range, NS_ERROR_NULL_POINTER);

  return selection->AddRange(range);
}

nsresult nsEditor::ClearSelection()
{
  nsCOMPtr<nsISelection> selection;
  nsresult res = nsEditor::GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(selection, NS_ERROR_FAILURE);
  return selection->RemoveAllRanges();  
}

nsresult
nsEditor::CreateHTMLContent(const nsAString& aTag, nsIContent** aContent)
{
  nsCOMPtr<nsIDOMDocument> tempDoc;
  GetDocument(getter_AddRefs(tempDoc));

  nsCOMPtr<nsIDocument> doc = do_QueryInterface(tempDoc);
  NS_ENSURE_TRUE(doc, NS_ERROR_FAILURE);

  
  
  if (aTag.IsEmpty()) {
    NS_ERROR("Don't pass an empty tag to nsEditor::CreateHTMLContent, "
             "check caller.");
    return NS_ERROR_FAILURE;
  }

  return doc->CreateElem(aTag, nsnull, kNameSpaceID_XHTML, PR_FALSE, aContent);
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
nsEditor::HandleKeyPressEvent(nsIDOMKeyEvent* aKeyEvent)
{
  
  
  
  
  
  

  nsKeyEvent* nativeKeyEvent = GetNativeKeyEvent(aKeyEvent);
  NS_ENSURE_TRUE(nativeKeyEvent, NS_ERROR_UNEXPECTED);
  NS_ASSERTION(nativeKeyEvent->message == NS_KEY_PRESS,
               "HandleKeyPressEvent gets non-keypress event");

  
  if (IsReadonly() || IsDisabled()) {
    
    
    if (nativeKeyEvent->keyCode == nsIDOMKeyEvent::DOM_VK_BACK_SPACE) {
      aKeyEvent->PreventDefault();
    }
    return NS_OK;
  }

  switch (nativeKeyEvent->keyCode) {
    case nsIDOMKeyEvent::DOM_VK_META:
    case nsIDOMKeyEvent::DOM_VK_SHIFT:
    case nsIDOMKeyEvent::DOM_VK_CONTROL:
    case nsIDOMKeyEvent::DOM_VK_ALT:
      aKeyEvent->PreventDefault(); 
      return NS_OK;
    case nsIDOMKeyEvent::DOM_VK_BACK_SPACE:
      if (nativeKeyEvent->isControl || nativeKeyEvent->isAlt ||
          nativeKeyEvent->isMeta) {
        return NS_OK;
      }
      DeleteSelection(nsIEditor::ePrevious);
      aKeyEvent->PreventDefault(); 
      return NS_OK;
    case nsIDOMKeyEvent::DOM_VK_DELETE:
      
      
      
      if (nativeKeyEvent->isShift || nativeKeyEvent->isControl ||
          nativeKeyEvent->isAlt || nativeKeyEvent->isMeta) {
        return NS_OK;
      }
      DeleteSelection(nsIEditor::eNext);
      aKeyEvent->PreventDefault(); 
      return NS_OK; 
  }
  return NS_OK;
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

already_AddRefed<nsIContent>
nsEditor::FindSelectionRoot(nsINode *aNode)
{
  nsCOMPtr<nsIContent> rootContent = do_QueryInterface(GetRoot());
  return rootContent.forget();
}

nsresult
nsEditor::InitializeSelection(nsIDOMEventTarget* aFocusEventTarget)
{
  nsCOMPtr<nsINode> targetNode = do_QueryInterface(aFocusEventTarget);
  NS_ENSURE_TRUE(targetNode, NS_ERROR_INVALID_ARG);
  nsCOMPtr<nsIContent> selectionRootContent = FindSelectionRoot(targetNode);
  if (!selectionRootContent) {
    return NS_OK;
  }

  nsCOMPtr<nsIDocument> targetDoc = do_QueryInterface(aFocusEventTarget);
  PRBool isTargetDoc =
    targetNode->IsNodeOfType(nsINode::eDOCUMENT) &&
    targetNode->HasFlag(NODE_IS_EDITABLE);

  nsCOMPtr<nsISelection> selection;
  nsresult rv = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIPresShell> presShell = GetPresShell();
  NS_ENSURE_TRUE(presShell, NS_ERROR_NOT_INITIALIZED);

  nsCOMPtr<nsISelectionController> selCon;
  rv = GetSelectionController(getter_AddRefs(selCon));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISelectionPrivate> selectionPrivate =
    do_QueryInterface(selection);
  NS_ENSURE_TRUE(selectionPrivate, NS_ERROR_UNEXPECTED);

  
  nsRefPtr<nsCaret> caret = presShell->GetCaret();
  NS_ENSURE_TRUE(caret, NS_ERROR_UNEXPECTED);
  caret->SetIgnoreUserModify(PR_FALSE);
  caret->SetCaretDOMSelection(selection);
  selCon->SetCaretReadOnly(IsReadonly());
  selCon->SetCaretEnabled(PR_TRUE);

  
  selCon->SetDisplaySelection(nsISelectionController::SELECTION_ON);
  selCon->RepaintSelection(nsISelectionController::SELECTION_NORMAL);
  
  
  
  
  
  if (selectionRootContent->GetParent()) {
    selectionPrivate->SetAncestorLimiter(selectionRootContent);
  } else {
    selectionPrivate->SetAncestorLimiter(nsnull);
  }

  
  if (isTargetDoc) {
    PRInt32 rangeCount;
    selection->GetRangeCount(&rangeCount);
    if (rangeCount == 0) {
      BeginningOfDocument();
    }
  }

  return NS_OK;
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

  
  
  if (!(mFlags & (nsIPlaintextEditor::eEditorLeftToRight |
                  nsIPlaintextEditor::eEditorRightToLeft))) {
    nsCOMPtr<nsIContent> content = do_QueryInterface(rootElement, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsIFrame* frame = content->GetPrimaryFrame();
    NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);

    
    
    if (frame->GetStyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL) {
      mFlags |= nsIPlaintextEditor::eEditorRightToLeft;
    } else {
      mFlags |= nsIPlaintextEditor::eEditorLeftToRight;
    }
  }

  
  if (mFlags & nsIPlaintextEditor::eEditorRightToLeft) {
    NS_ASSERTION(!(mFlags & nsIPlaintextEditor::eEditorLeftToRight),
                 "Unexpected mutually exclusive flag");
    mFlags &= ~nsIPlaintextEditor::eEditorRightToLeft;
    mFlags |= nsIPlaintextEditor::eEditorLeftToRight;
    rv = rootElement->SetAttribute(NS_LITERAL_STRING("dir"), NS_LITERAL_STRING("ltr"));
  } else if (mFlags & nsIPlaintextEditor::eEditorLeftToRight) {
    NS_ASSERTION(!(mFlags & nsIPlaintextEditor::eEditorRightToLeft),
                 "Unexpected mutually exclusive flag");
    mFlags |= nsIPlaintextEditor::eEditorRightToLeft;
    mFlags &= ~nsIPlaintextEditor::eEditorLeftToRight;
    rv = rootElement->SetAttribute(NS_LITERAL_STRING("dir"), NS_LITERAL_STRING("rtl"));
  }

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
    NS_ENSURE_TRUE(childList, NS_ERROR_NULL_POINTER);
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

nsKeyEvent*
nsEditor::GetNativeKeyEvent(nsIDOMKeyEvent* aDOMKeyEvent)
{
  nsCOMPtr<nsIPrivateDOMEvent> privDOMEvent = do_QueryInterface(aDOMKeyEvent);
  NS_ENSURE_TRUE(privDOMEvent, nsnull);
  nsEvent* nativeEvent = privDOMEvent->GetInternalNSEvent();
  NS_ENSURE_TRUE(nativeEvent, nsnull);
  NS_ENSURE_TRUE(nativeEvent->eventStructType == NS_KEY_EVENT, nsnull);
  return static_cast<nsKeyEvent*>(nativeEvent);
}

already_AddRefed<nsIContent>
nsEditor::GetFocusedContent()
{
  nsCOMPtr<nsIDOMEventTarget> piTarget = GetPIDOMEventTarget();
  if (!piTarget) {
    return nsnull;
  }

  nsFocusManager* fm = nsFocusManager::GetFocusManager();
  NS_ENSURE_TRUE(fm, nsnull);

  nsCOMPtr<nsIContent> content = fm->GetFocusedContent();
  return SameCOMIdentity(content, piTarget) ? content.forget() : nsnull;
}

PRBool
nsEditor::IsActiveInDOMWindow()
{
  nsCOMPtr<nsIDOMEventTarget> piTarget = GetPIDOMEventTarget();
  if (!piTarget) {
    return PR_FALSE;
  }

  nsFocusManager* fm = nsFocusManager::GetFocusManager();
  NS_ENSURE_TRUE(fm, PR_FALSE);

  nsCOMPtr<nsIDocument> doc = do_QueryReferent(mDocWeak);
  nsPIDOMWindow* ourWindow = doc->GetWindow();
  nsCOMPtr<nsPIDOMWindow> win;
  nsIContent* content =
    nsFocusManager::GetFocusedDescendant(ourWindow, PR_FALSE,
                                         getter_AddRefs(win));
  return SameCOMIdentity(content, piTarget);
}

PRBool
nsEditor::IsAcceptableInputEvent(nsIDOMEvent* aEvent)
{
  
  nsCOMPtr<nsIDOMNSEvent> NSEvent = do_QueryInterface(aEvent);
  NS_ENSURE_TRUE(NSEvent, PR_FALSE);

  PRBool isTrusted;
  nsresult rv = NSEvent->GetIsTrusted(&isTrusted);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);
  if (isTrusted) {
    return PR_TRUE;
  }
  
  
  return IsActiveInDOMWindow();
}

NS_IMETHODIMP
nsEditor::GetLastKeypressEventTrusted(PRBool *aWasTrusted)
{
  NS_ENSURE_ARG_POINTER(aWasTrusted);

  if (mLastKeypressEventWasTrusted == eTriUnset) {
    return NS_ERROR_UNEXPECTED;
  }

  *aWasTrusted = (mLastKeypressEventWasTrusted == eTriTrue);
  return NS_OK;
}

void
nsEditor::BeginKeypressHandling(nsIDOMNSEvent* aEvent)
{
  NS_ASSERTION(mLastKeypressEventWasTrusted == eTriUnset, "How come our status is not clear?");

  if (aEvent) {
    PRBool isTrusted = PR_FALSE;
    aEvent->GetIsTrusted(&isTrusted);
    mLastKeypressEventWasTrusted = isTrusted ? eTriTrue : eTriFalse;
  }
}
