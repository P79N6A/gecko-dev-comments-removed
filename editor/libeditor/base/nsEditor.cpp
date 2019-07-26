




#include "mozilla/DebugOnly.h"          

#include <stdio.h>                      
#include <string.h>                     

#include "ChangeAttributeTxn.h"         
#include "CreateElementTxn.h"           
#include "DeleteNodeTxn.h"              
#include "DeleteRangeTxn.h"             
#include "DeleteTextTxn.h"              
#include "EditAggregateTxn.h"           
#include "EditTxn.h"                    
#include "IMETextTxn.h"                 
#include "InsertElementTxn.h"           
#include "InsertTextTxn.h"              
#include "JoinElementTxn.h"             
#include "PlaceholderTxn.h"             
#include "SplitElementTxn.h"            
#include "mozFlushType.h"               
#include "mozISpellCheckingEngine.h"
#include "mozInlineSpellChecker.h"      
#include "mozilla/Preferences.h"        
#include "mozilla/Selection.h"          
#include "mozilla/Services.h"           
#include "mozilla/dom/Element.h"        
#include "mozilla/mozalloc.h"           
#include "nsAString.h"                  
#include "nsCCUncollectableMarker.h"    
#include "nsCaret.h"                    
#include "nsCaseTreatment.h"
#include "nsCharTraits.h"               
#include "nsComponentManagerUtils.h"    
#include "nsComputedDOMStyle.h"         
#include "nsContentUtils.h"             
#include "nsDOMString.h"                
#include "nsDebug.h"                    
#include "nsEditProperty.h"             
#include "nsEditor.h"
#include "nsEditorEventListener.h"      
#include "nsEditorUtils.h"              
#include "nsError.h"                    
#include "nsEvent.h"                    
#include "nsFocusManager.h"             
#include "nsFrameSelection.h"           
#include "nsGUIEvent.h"                 
#include "nsGkAtoms.h"                  
#include "nsIAbsorbingTransaction.h"    
#include "nsIAtom.h"                    
#include "nsIContent.h"                 
#include "nsIDOMAttr.h"                 
#include "nsIDOMCharacterData.h"        
#include "nsIDOMDocument.h"             
#include "nsIDOMElement.h"              
#include "nsIDOMEvent.h"                
#include "nsIDOMEventListener.h"        
#include "nsIDOMEventTarget.h"          
#include "nsIDOMHTMLElement.h"          
#include "nsIDOMKeyEvent.h"             
#include "nsIDOMMouseEvent.h"           
#include "nsIDOMNamedNodeMap.h"         
#include "nsIDOMNode.h"                 
#include "nsIDOMNodeList.h"             
#include "nsIDOMRange.h"                
#include "nsIDOMText.h"                 
#include "nsIDocument.h"                
#include "nsIDocumentStateListener.h"   
#include "nsIEditActionListener.h"      
#include "nsIEditorObserver.h"          
#include "nsIEditorSpellCheck.h"        
#include "nsIEnumerator.h"              
#include "nsIFrame.h"                   
#include "nsIInlineSpellChecker.h"      
#include "nsIMEStateManager.h"          
#include "nsINameSpaceManager.h"        
#include "nsINode.h"                    
#include "nsIObserverService.h"         
#include "nsIPlaintextEditor.h"         
#include "nsIPresShell.h"               
#include "nsIPrivateTextRange.h"        
#include "nsISelection.h"               
#include "nsISelectionController.h"     
#include "nsISelectionDisplay.h"        
#include "nsISelectionPrivate.h"        
#include "nsISupportsBase.h"            
#include "nsISupportsUtils.h"           
#include "nsITransaction.h"             
#include "nsITransactionManager.h"
#include "nsIWeakReference.h"           
#include "nsIWidget.h"                  
#include "nsPIDOMWindow.h"              
#include "nsPresContext.h"              
#include "nsRange.h"                    
#include "nsReadableUtils.h"            
#include "nsString.h"                   
#include "nsStringFwd.h"                
#include "nsStyleConsts.h"              
#include "nsStyleContext.h"             
#include "nsStyleSheetTxns.h"           
#include "nsStyleStruct.h"              
#include "nsStyleStructFwd.h"           
#include "nsTextEditUtils.h"            
#include "nsThreadUtils.h"              
#include "nsTransactionManager.h"       
#include "prtime.h"                     

class nsIOutputStream;
class nsIParserService;
class nsITransferable;

#ifdef NS_DEBUG_EDITOR
static bool gNoisy = false;
#endif

#ifdef DEBUG
#include "nsIDOMHTMLDocument.h"         
#endif

using namespace mozilla;
using namespace mozilla::widget;


extern nsIParserService *sParserService;







nsEditor::nsEditor()
:  mPlaceHolderName(nullptr)
,  mSelState(nullptr)
,  mPhonetic(nullptr)
,  mModCount(0)
,  mFlags(0)
,  mUpdateCount(0)
,  mPlaceHolderBatch(0)
,  mAction(EditAction::none)
,  mHandlingActionCount(0)
,  mIMETextOffset(0)
,  mIMEBufferLength(0)
,  mDirection(eNone)
,  mDocDirtyState(-1)
,  mSpellcheckCheckboxState(eTriUnset)
,  mInIMEMode(false)
,  mIsIMEComposing(false)
,  mShouldTxnSetSelection(true)
,  mDidPreDestroy(false)
,  mDidPostCreate(false)
,  mHandlingTrustedAction(false)
,  mDispatchInputEvent(true)
{
}

nsEditor::~nsEditor()
{
  NS_ASSERTION(!mDocWeak || mDidPreDestroy, "Why PreDestroy hasn't been called?");

  mTxnMgr = nullptr;

  delete mPhonetic;
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsEditor)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsEditor)
 NS_IMPL_CYCLE_COLLECTION_UNLINK(mRootElement)
 NS_IMPL_CYCLE_COLLECTION_UNLINK(mInlineSpellChecker)
 NS_IMPL_CYCLE_COLLECTION_UNLINK(mTxnMgr)
 NS_IMPL_CYCLE_COLLECTION_UNLINK(mIMETextRangeList)
 NS_IMPL_CYCLE_COLLECTION_UNLINK(mIMETextNode)
 NS_IMPL_CYCLE_COLLECTION_UNLINK(mActionListeners)
 NS_IMPL_CYCLE_COLLECTION_UNLINK(mEditorObservers)
 NS_IMPL_CYCLE_COLLECTION_UNLINK(mDocStateListeners)
 NS_IMPL_CYCLE_COLLECTION_UNLINK(mEventTarget)
 NS_IMPL_CYCLE_COLLECTION_UNLINK(mEventListener)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsEditor)
 nsIDocument* currentDoc =
   tmp->mRootElement ? tmp->mRootElement->GetCurrentDoc() : nullptr;
 if (currentDoc &&
     nsCCUncollectableMarker::InGeneration(cb, currentDoc->GetMarkedCCGeneration())) {
   return NS_SUCCESS_INTERRUPTED_TRAVERSE;
 }
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mRootElement)
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mInlineSpellChecker)
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mTxnMgr)
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mIMETextRangeList)
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mIMETextNode)
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mActionListeners)
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mEditorObservers)
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mDocStateListeners)
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mEventTarget)
 NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mEventListener)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsEditor)
 NS_INTERFACE_MAP_ENTRY(nsIPhonetic)
 NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
 NS_INTERFACE_MAP_ENTRY(nsIEditorIMESupport)
 NS_INTERFACE_MAP_ENTRY(nsIEditor)
 NS_INTERFACE_MAP_ENTRY(nsIObserver)
 NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIEditor)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsEditor)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsEditor)


NS_IMETHODIMP
nsEditor::Init(nsIDOMDocument *aDoc, nsIContent *aRoot, nsISelectionController *aSelCon, uint32_t aFlags)
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

  
  mIMETextNode = nullptr;
  mIMETextOffset = 0;
  mIMEBufferLength = 0;
  
  
  selCon->SetCaretReadOnly(false);
  selCon->SetDisplaySelection(nsISelectionController::SELECTION_ON);

  selCon->SetSelectionFlags(nsISelectionDisplay::DISPLAY_ALL);

  NS_POSTCONDITION(mDocWeak, "bad state");

  
  mDidPreDestroy = false;
  
  mDidPostCreate = false;

  return NS_OK;
}


NS_IMETHODIMP
nsEditor::PostCreate()
{
  
  
  
  
  mFlags = ~mFlags;
  nsresult rv = SetFlags(~mFlags);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (!mDidPostCreate) {
    mDidPostCreate = true;

    
    CreateEventListeners();
    rv = InstallEventListeners();
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    ResetModificationCount();

    
    NotifyDocumentListeners(eDocumentCreated);
    NotifyDocumentListeners(eDocumentStateChanged);

    nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
    if (obs) {
      obs->AddObserver(this,
                       SPELLCHECK_DICTIONARY_UPDATE_NOTIFICATION,
                       false);
    }
  }

  
  nsCOMPtr<nsIContent> focusedContent = GetFocusedContent();
  if (focusedContent) {
    nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(focusedContent);
    if (target) {
      InitializeSelection(target);
    }

    
    
    
    nsEditorEventListener* listener =
      reinterpret_cast<nsEditorEventListener*> (mEventListener.get());
    listener->SpellCheckIfNeeded();

    IMEState newState;
    rv = GetPreferredIMEState(&newState);
    NS_ENSURE_SUCCESS(rv, NS_OK);
    nsCOMPtr<nsIContent> content = GetFocusedContentForIME();
    nsIMEStateManager::UpdateIMEState(newState, content);
  }
  return NS_OK;
}


void
nsEditor::CreateEventListeners()
{
  
  if (!mEventListener) {
    mEventListener = new nsEditorEventListener();
  }
}

nsresult
nsEditor::InstallEventListeners()
{
  NS_ENSURE_TRUE(mDocWeak && mEventListener,
                 NS_ERROR_NOT_INITIALIZED);

  
  nsCOMPtr<nsIContent> rootContent = GetRoot();
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
  mEventTarget = nullptr;
}

bool
nsEditor::GetDesiredSpellCheckState()
{
  
  if (mSpellcheckCheckboxState != eTriUnset) {
    return (mSpellcheckCheckboxState == eTriTrue);
  }

  
  int32_t spellcheckLevel = Preferences::GetInt("layout.spellcheckDefault", 1);

  if (spellcheckLevel == 0) {
    return false;                    
  }

  if (!CanEnableSpellCheck()) {
    return false;
  }

  nsCOMPtr<nsIPresShell> presShell = GetPresShell();
  if (presShell) {
    nsPresContext* context = presShell->GetPresContext();
    if (context && !context->IsDynamic()) {
      return false;
    }
  }

  
  nsCOMPtr<nsIContent> content = GetRoot();
  if (!content) {
    return false;
  }

  if (content->IsRootOfNativeAnonymousSubtree()) {
    content = content->GetParent();
  }

  nsCOMPtr<nsIDOMHTMLElement> element = do_QueryInterface(content);
  if (!element) {
    return false;
  }

  bool enable;
  element->GetSpellcheck(&enable);

  return enable;
}

NS_IMETHODIMP
nsEditor::PreDestroy(bool aDestroyingFrames)
{
  if (mDidPreDestroy)
    return NS_OK;

  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (obs) {
    obs->RemoveObserver(this,
                        SPELLCHECK_DICTIONARY_UPDATE_NOTIFICATION);
  }

  
  
  
  
  
  
  if (mInlineSpellChecker)
    mInlineSpellChecker->Cleanup(aDestroyingFrames);

  
  NotifyDocumentListeners(eDocumentToBeDestroyed);

  
  RemoveEventListeners();
  mActionListeners.Clear();
  mEditorObservers.Clear();
  mDocStateListeners.Clear();
  mInlineSpellChecker = nullptr;
  mSpellcheckCheckboxState = eTriUnset;
  mRootElement = nullptr;

  mDidPreDestroy = true;
  return NS_OK;
}

NS_IMETHODIMP
nsEditor::GetFlags(uint32_t *aFlags)
{
  *aFlags = mFlags;
  return NS_OK;
}

NS_IMETHODIMP
nsEditor::SetFlags(uint32_t aFlags)
{
  if (mFlags == aFlags) {
    return NS_OK;
  }

  bool spellcheckerWasEnabled = CanEnableSpellCheck();
  mFlags = aFlags;

  if (!mDocWeak) {
    
    
    
    return NS_OK;
  }

  
  if (CanEnableSpellCheck() != spellcheckerWasEnabled) {
    nsresult rv = SyncRealTimeSpell();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  if (!mDidPostCreate) {
    return NS_OK;
  }

  
  
  nsCOMPtr<nsIContent> focusedContent = GetFocusedContent();
  if (focusedContent) {
    IMEState newState;
    nsresult rv = GetPreferredIMEState(&newState);
    if (NS_SUCCEEDED(rv)) {
      
      
      nsCOMPtr<nsIContent> content = GetFocusedContentForIME();
      nsIMEStateManager::UpdateIMEState(newState, content);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsEditor::GetIsSelectionEditable(bool *aIsSelectionEditable)
{
  NS_ENSURE_ARG_POINTER(aIsSelectionEditable);

  
  nsCOMPtr<nsISelection> selection;
  nsresult res = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);

  
  
  nsCOMPtr<nsIDOMNode> anchorNode;
  selection->GetAnchorNode(getter_AddRefs(anchorNode));
  *aIsSelectionEditable = anchorNode && IsEditable(anchorNode);

  return NS_OK;
}

NS_IMETHODIMP
nsEditor::GetIsDocumentEditable(bool *aIsDocumentEditable)
{
  NS_ENSURE_ARG_POINTER(aIsDocumentEditable);
  nsCOMPtr<nsIDOMDocument> doc = GetDOMDocument();
  *aIsDocumentEditable = !!doc;

  return NS_OK;
}

already_AddRefed<nsIDocument>
nsEditor::GetDocument()
{
  NS_PRECONDITION(mDocWeak, "bad state, mDocWeak weak pointer not initialized");
  nsCOMPtr<nsIDocument> doc = do_QueryReferent(mDocWeak);
  return doc.forget();
}

already_AddRefed<nsIDOMDocument>
nsEditor::GetDOMDocument()
{
  NS_PRECONDITION(mDocWeak, "bad state, mDocWeak weak pointer not initialized");
  nsCOMPtr<nsIDOMDocument> doc = do_QueryReferent(mDocWeak);
  return doc.forget();
}

NS_IMETHODIMP 
nsEditor::GetDocument(nsIDOMDocument **aDoc)
{
  nsCOMPtr<nsIDOMDocument> doc = GetDOMDocument();
  doc.forget(aDoc);
  return *aDoc ? NS_OK : NS_ERROR_NOT_INITIALIZED;
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
  *aSel = nullptr; 
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
nsEditor::DeleteSelection(EDirection aAction, EStripWrappers aStripWrappers)
{
  MOZ_ASSERT(aStripWrappers == eStrip || aStripWrappers == eNoStrip);
  return DeleteSelectionImpl(aAction, aStripWrappers);
}



NS_IMETHODIMP
nsEditor::GetSelection(nsISelection **aSelection)
{
  NS_ENSURE_TRUE(aSelection, NS_ERROR_NULL_POINTER);
  *aSelection = nullptr;
  nsCOMPtr<nsISelectionController> selcon;
  GetSelectionController(getter_AddRefs(selcon));
  NS_ENSURE_TRUE(selcon, NS_ERROR_NOT_INITIALIZED);
  return selcon->GetSelection(nsISelectionController::SELECTION_NORMAL, aSelection);  
}

Selection*
nsEditor::GetSelection()
{
  nsCOMPtr<nsISelection> sel;
  nsresult res = GetSelection(getter_AddRefs(sel));
  NS_ENSURE_SUCCESS(res, nullptr);

  nsCOMPtr<nsISelectionPrivate> selPrivate = do_QueryInterface(sel);
  NS_ENSURE_TRUE(selPrivate, nullptr);

  nsRefPtr<nsFrameSelection> frameSel;
  res = selPrivate->GetFrameSelection(getter_AddRefs(frameSel));
  NS_ENSURE_SUCCESS(res, nullptr);

  return frameSel->GetSelection(nsISelectionController::SELECTION_NORMAL);
}

NS_IMETHODIMP
nsEditor::DoTransaction(nsITransaction* aTxn)
{
  if (mPlaceHolderBatch && !mPlaceHolderTxn) {
    nsCOMPtr<nsIAbsorbingTransaction> plcTxn = new PlaceholderTxn();

    
    mPlaceHolderTxn = do_GetWeakReference(plcTxn);
    plcTxn->Init(mPlaceHolderName, mSelState, this);
    
    mSelState = nullptr;

    
    nsCOMPtr<nsITransaction> theTxn = do_QueryInterface(plcTxn);
    
    DoTransaction(theTxn);

    if (mTxnMgr) {
      nsCOMPtr<nsITransaction> topTxn = mTxnMgr->PeekUndoStack();
      if (topTxn) {
        plcTxn = do_QueryInterface(topTxn);
        if (plcTxn) {
          
          
          
          
          mPlaceHolderTxn = do_GetWeakReference(plcTxn);
        }
      }
    }
  }

  if (aTxn) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    nsRefPtr<Selection> selection = GetSelection();
    NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);

    selection->StartBatchChanges();

    nsresult res;
    if (mTxnMgr) {
      res = mTxnMgr->DoTransaction(aTxn);
    } else {
      res = aTxn->DoTransaction();
    }
    if (NS_SUCCEEDED(res)) {
      DoAfterDoTransaction(aTxn);
    }

    
    selection->EndBatchChanges();

    NS_ENSURE_SUCCESS(res, res);
  }

  return NS_OK;
}


NS_IMETHODIMP
nsEditor::EnableUndo(bool aEnable)
{
  if (aEnable) {
    if (!mTxnMgr) {
      mTxnMgr = new nsTransactionManager();
    }
    mTxnMgr->SetMaxTransactionCount(-1);
  } else if (mTxnMgr) {
    
    mTxnMgr->Clear();
    mTxnMgr->SetMaxTransactionCount(0);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsEditor::GetNumberOfUndoItems(int32_t* aNumItems)
{
  *aNumItems = 0;
  return mTxnMgr ? mTxnMgr->GetNumberOfUndoItems(aNumItems) : NS_OK;
}

NS_IMETHODIMP
nsEditor::GetNumberOfRedoItems(int32_t* aNumItems)
{
  *aNumItems = 0;
  return mTxnMgr ? mTxnMgr->GetNumberOfRedoItems(aNumItems) : NS_OK;
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

  
  mTxnMgr = static_cast<nsTransactionManager*>(aTxnManager);
  return NS_OK;
}

NS_IMETHODIMP 
nsEditor::Undo(uint32_t aCount)
{
#ifdef NS_DEBUG_EDITOR
  if (gNoisy) { printf("Editor::Undo ----------\n"); }
#endif

  ForceCompositionEnd();

  bool hasTxnMgr, hasTransaction = false;
  CanUndo(&hasTxnMgr, &hasTransaction);
  NS_ENSURE_TRUE(hasTransaction, NS_OK);

  nsAutoRules beginRulesSniffing(this, EditAction::undo, nsIEditor::eNone);

  if (!mTxnMgr) {
    return NS_OK;
  }

  for (uint32_t i = 0; i < aCount; ++i) {
    nsresult rv = mTxnMgr->UndoTransaction();
    NS_ENSURE_SUCCESS(rv, rv);

    DoAfterUndoTransaction();
  }

  return NS_OK;
}


NS_IMETHODIMP nsEditor::CanUndo(bool *aIsEnabled, bool *aCanUndo)
{
  NS_ENSURE_TRUE(aIsEnabled && aCanUndo, NS_ERROR_NULL_POINTER);
  *aIsEnabled = !!mTxnMgr;
  if (*aIsEnabled) {
    int32_t numTxns = 0;
    mTxnMgr->GetNumberOfUndoItems(&numTxns);
    *aCanUndo = !!numTxns;
  } else {
    *aCanUndo = false;
  }
  return NS_OK;
}


NS_IMETHODIMP 
nsEditor::Redo(uint32_t aCount)
{
#ifdef NS_DEBUG_EDITOR
  if (gNoisy) { printf("Editor::Redo ----------\n"); }
#endif

  bool hasTxnMgr, hasTransaction = false;
  CanRedo(&hasTxnMgr, &hasTransaction);
  NS_ENSURE_TRUE(hasTransaction, NS_OK);

  nsAutoRules beginRulesSniffing(this, EditAction::redo, nsIEditor::eNone);

  if (!mTxnMgr) {
    return NS_OK;
  }

  for (uint32_t i = 0; i < aCount; ++i) {
    nsresult rv = mTxnMgr->RedoTransaction();
    NS_ENSURE_SUCCESS(rv, rv);

    DoAfterRedoTransaction();
  }

  return NS_OK;
}


NS_IMETHODIMP nsEditor::CanRedo(bool *aIsEnabled, bool *aCanRedo)
{
  NS_ENSURE_TRUE(aIsEnabled && aCanRedo, NS_ERROR_NULL_POINTER);

  *aIsEnabled = !!mTxnMgr;
  if (*aIsEnabled) {
    int32_t numTxns = 0;
    mTxnMgr->GetNumberOfRedoItems(&numTxns);
    *aCanRedo = !!numTxns;
  } else {
    *aCanRedo = false;
  }
  return NS_OK;
}


NS_IMETHODIMP 
nsEditor::BeginTransaction()
{
  BeginUpdateViewBatch();

  if (mTxnMgr) {
    mTxnMgr->BeginBatch();
  }

  return NS_OK;
}

NS_IMETHODIMP 
nsEditor::EndTransaction()
{
  if (mTxnMgr) {
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
    mPlaceHolderTxn = nullptr;
    mPlaceHolderName = aName;
    nsRefPtr<Selection> selection = GetSelection();
    if (selection) {
      mSelState = new nsSelectionState();
      mSelState->SaveSelection(selection);
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
      selPrivate->SetCanCacheFrameOffset(true);
    }

    {
      
      
      nsRefPtr<nsCaret> caret;
      nsCOMPtr<nsIPresShell> presShell = GetPresShell();

      if (presShell)
        caret = presShell->GetCaret();

      
      EndUpdateViewBatch();
      

      
      
      ScrollSelectionIntoView(false);
    }

    
    if (selPrivate) {
      selPrivate->SetCanCacheFrameOffset(false);
    }

    if (mSelState)
    {
      
      
      delete mSelState;
      mSelState = nullptr;
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
nsEditor::ShouldTxnSetSelection(bool *aResult)
{
  NS_ENSURE_TRUE(aResult, NS_ERROR_NULL_POINTER);
  *aResult = mShouldTxnSetSelection;
  return NS_OK;
}

NS_IMETHODIMP  
nsEditor::SetShouldTxnSetSelection(bool aShould)
{
  mShouldTxnSetSelection = aShould;
  return NS_OK;
}

NS_IMETHODIMP
nsEditor::GetDocumentIsEmpty(bool *aDocumentIsEmpty)
{
  *aDocumentIsEmpty = true;

  dom::Element* root = GetRoot();
  NS_ENSURE_TRUE(root, NS_ERROR_NULL_POINTER); 

  *aDocumentIsEmpty = !root->HasChildren();
  return NS_OK;
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
    
  
  dom::Element* rootElement = GetRoot();
  NS_ENSURE_TRUE(rootElement, NS_ERROR_NULL_POINTER); 
  
  
  nsCOMPtr<nsINode> firstNode = GetFirstEditableNode(rootElement);
  if (!firstNode) {
    
    return selection->CollapseNative(rootElement, 0);
  }

  if (firstNode->NodeType() == nsIDOMNode::TEXT_NODE) {
    
    return selection->CollapseNative(firstNode, 0);
  }

  
  nsCOMPtr<nsIContent> parent = firstNode->GetParent();
  if (!parent) {
    return NS_ERROR_NULL_POINTER;
  }

  int32_t offsetInParent = parent->IndexOf(firstNode);
  return selection->CollapseNative(parent, offsetInParent);
}

NS_IMETHODIMP
nsEditor::EndOfDocument()
{ 
  NS_ENSURE_TRUE(mDocWeak, NS_ERROR_NOT_INITIALIZED);

  
  nsCOMPtr<nsISelection> selection; 
  nsresult rv = GetSelection(getter_AddRefs(selection)); 
  NS_ENSURE_SUCCESS(rv, rv); 
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER); 
  
  
  nsINode* node = GetRoot();
  NS_ENSURE_TRUE(node, NS_ERROR_NULL_POINTER); 
  nsINode* child = node->GetLastChild();

  while (child && IsContainer(child->AsDOMNode())) {
    node = child;
    child = node->GetLastChild();
  }

  uint32_t length = node->Length();
  return selection->CollapseNative(node, int32_t(length));
} 
  
NS_IMETHODIMP
nsEditor::GetDocumentModified(bool *outDocModified)
{
  NS_ENSURE_TRUE(outDocModified, NS_ERROR_NULL_POINTER);

  int32_t  modCount = 0;
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
nsEditor::CanCut(bool *aCanCut)
{
  return NS_ERROR_NOT_IMPLEMENTED; 
}

NS_IMETHODIMP
nsEditor::Copy()
{
  return NS_ERROR_NOT_IMPLEMENTED; 
}

NS_IMETHODIMP
nsEditor::CanCopy(bool *aCanCut)
{
  return NS_ERROR_NOT_IMPLEMENTED; 
}

NS_IMETHODIMP
nsEditor::Paste(int32_t aSelectionType)
{
  return NS_ERROR_NOT_IMPLEMENTED; 
}

NS_IMETHODIMP
nsEditor::PasteTransferable(nsITransferable *aTransferable)
{
  return NS_ERROR_NOT_IMPLEMENTED; 
}

NS_IMETHODIMP
nsEditor::CanPaste(int32_t aSelectionType, bool *aCanPaste)
{
  return NS_ERROR_NOT_IMPLEMENTED; 
}

NS_IMETHODIMP
nsEditor::CanPasteTransferable(nsITransferable *aTransferable, bool *aCanPaste)
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
                            bool *aResultIsSet)
{
  NS_ENSURE_TRUE(aResultIsSet, NS_ERROR_NULL_POINTER);
  *aResultIsSet = false;
  if (!aElement) {
    return NS_OK;
  }
  nsAutoString value;
  nsresult rv = aElement->GetAttribute(aAttribute, value);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!DOMStringIsNull(value)) {
    *aResultIsSet = true;
    aResultValue = value;
  }
  return rv;
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


bool
nsEditor::OutputsMozDirty()
{
  
  
  return !(mFlags & nsIPlaintextEditor::eEditorAllowInteraction) ||
          (mFlags & nsIPlaintextEditor::eEditorMailMask);
}


NS_IMETHODIMP
nsEditor::MarkNodeDirty(nsIDOMNode* aNode)
{  
  
  if (!OutputsMozDirty()) {
    return NS_OK;
  }
  nsCOMPtr<dom::Element> element = do_QueryInterface(aNode);
  if (element) {
    element->SetAttr(kNameSpaceID_None, nsEditProperty::mozdirty,
                     EmptyString(), false);
  }
  return NS_OK;
}

NS_IMETHODIMP nsEditor::GetInlineSpellChecker(bool autoCreate,
                                              nsIInlineSpellChecker ** aInlineSpellChecker)
{
  NS_ENSURE_ARG_POINTER(aInlineSpellChecker);

  if (mDidPreDestroy) {
    
    
    *aInlineSpellChecker = nullptr;
    return autoCreate ? NS_ERROR_NOT_AVAILABLE : NS_OK;
  }

  
  bool canSpell = mozInlineSpellChecker::CanEnableInlineSpellChecking();
  if (!canSpell) {
    *aInlineSpellChecker = nullptr;
    return NS_ERROR_FAILURE;
  }

  nsresult rv;
  if (!mInlineSpellChecker && autoCreate) {
    mInlineSpellChecker = do_CreateInstance(MOZ_INLINESPELLCHECKER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (mInlineSpellChecker) {
    rv = mInlineSpellChecker->Init(this);
    if (NS_FAILED(rv))
      mInlineSpellChecker = nullptr;
    NS_ENSURE_SUCCESS(rv, rv);
  }

  NS_IF_ADDREF(*aInlineSpellChecker = mInlineSpellChecker);

  return NS_OK;
}

NS_IMETHODIMP nsEditor::Observe(nsISupports* aSubj, const char *aTopic,
                                const PRUnichar *aData)
{
  NS_ASSERTION(!strcmp(aTopic,
                       SPELLCHECK_DICTIONARY_UPDATE_NOTIFICATION),
               "Unexpected observer topic");

  
  SyncRealTimeSpell();

  
  if (mInlineSpellChecker) {
    
    nsCOMPtr<nsIEditorSpellCheck> editorSpellCheck;
    mInlineSpellChecker->GetSpellChecker(getter_AddRefs(editorSpellCheck));
    if (editorSpellCheck) {
      
      
      editorSpellCheck->CheckCurrentDictionary();
    }

    
    mInlineSpellChecker->SpellCheckRange(nullptr); 
  }

  return NS_OK;
}

NS_IMETHODIMP nsEditor::SyncRealTimeSpell()
{
  bool enable = GetDesiredSpellCheckState();

  
  nsCOMPtr<nsIInlineSpellChecker> spellChecker;
  GetInlineSpellChecker(enable, getter_AddRefs(spellChecker));

  if (mInlineSpellChecker) {
    
    
    
    mInlineSpellChecker->SetEnableRealTimeSpell(enable && spellChecker);
  }

  return NS_OK;
}

NS_IMETHODIMP nsEditor::SetSpellcheckUserOverride(bool enable)
{
  mSpellcheckCheckboxState = enable ? eTriTrue : eTriFalse;

  return SyncRealTimeSpell();
}

NS_IMETHODIMP nsEditor::CreateNode(const nsAString& aTag,
                                   nsIDOMNode *    aParent,
                                   int32_t         aPosition,
                                   nsIDOMNode **   aNewNode)
{
  int32_t i;

  nsAutoRules beginRulesSniffing(this, EditAction::createNode, nsIEditor::eNext);
  
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
                                   int32_t      aPosition)
{
  int32_t i;
  nsAutoRules beginRulesSniffing(this, EditAction::insertNode, nsIEditor::eNext);

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
                    int32_t      aOffset,
                    nsIDOMNode **aNewLeftNode)
{
  int32_t i;
  nsAutoRules beginRulesSniffing(this, EditAction::splitNode, nsIEditor::eNext);

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


nsresult
nsEditor::JoinNodes(nsINode* aNodeToKeep, nsIContent* aNodeToMove)
{
  
  
  
  MOZ_ASSERT(aNodeToKeep && aNodeToMove && aNodeToMove->GetParentNode());
  nsresult res = JoinNodes(aNodeToKeep->AsDOMNode(), aNodeToMove->AsDOMNode(),
                           aNodeToMove->GetParentNode()->AsDOMNode());
  NS_ASSERTION(NS_SUCCEEDED(res), "JoinNodes failed");
  NS_ENSURE_SUCCESS(res, res);
  return NS_OK;
}

NS_IMETHODIMP
nsEditor::JoinNodes(nsIDOMNode * aLeftNode,
                    nsIDOMNode * aRightNode,
                    nsIDOMNode * aParent)
{
  int32_t i;
  nsAutoRules beginRulesSniffing(this, EditAction::joinNode, nsIEditor::ePrevious);

  
  
  int32_t offset = GetChildOffset(aRightNode, aParent);
  
  uint32_t oldLeftNodeLen;
  nsresult result = GetLengthOfDOMNode(aLeftNode, oldLeftNodeLen);
  NS_ENSURE_SUCCESS(result, result);

  for (i = 0; i < mActionListeners.Count(); i++)
    mActionListeners[i]->WillJoinNodes(aLeftNode, aRightNode, aParent);

  nsRefPtr<JoinElementTxn> txn;
  result = CreateTxnForJoinNode(aLeftNode, aRightNode, getter_AddRefs(txn));
  if (NS_SUCCEEDED(result))  {
    result = DoTransaction(txn);  
  }

  mRangeUpdater.SelAdjJoinNodes(aLeftNode, aRightNode, aParent, offset, (int32_t)oldLeftNodeLen);
  
  for (i = 0; i < mActionListeners.Count(); i++)
    mActionListeners[i]->DidJoinNodes(aLeftNode, aRightNode, aParent, result);

  return result;
}


NS_IMETHODIMP
nsEditor::DeleteNode(nsIDOMNode* aNode)
{
  nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
  NS_ENSURE_STATE(node);
  return DeleteNode(node);
}

nsresult
nsEditor::DeleteNode(nsINode* aNode)
{
  nsAutoRules beginRulesSniffing(this, EditAction::createNode, nsIEditor::ePrevious);

  
  for (int32_t i = 0; i < mActionListeners.Count(); i++) {
    mActionListeners[i]->WillDeleteNode(aNode->AsDOMNode());
  }

  nsRefPtr<DeleteNodeTxn> txn;
  nsresult res = CreateTxnForDeleteNode(aNode, getter_AddRefs(txn));
  if (NS_SUCCEEDED(res))  {
    res = DoTransaction(txn);
  }

  for (int32_t i = 0; i < mActionListeners.Count(); i++) {
    mActionListeners[i]->DidDeleteNode(aNode->AsDOMNode(), res);
  }

  NS_ENSURE_SUCCESS(res, res);
  return NS_OK;
}






nsresult
nsEditor::ReplaceContainer(nsIDOMNode *inNode, 
                           nsCOMPtr<nsIDOMNode> *outNode, 
                           const nsAString &aNodeType,
                           const nsAString *aAttribute,
                           const nsAString *aValue,
                           bool aCloneAttributes)
{
  NS_ENSURE_TRUE(inNode && outNode, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsINode> node = do_QueryInterface(inNode);
  NS_ENSURE_STATE(node);

  nsCOMPtr<dom::Element> element;
  nsresult rv = ReplaceContainer(node, getter_AddRefs(element), aNodeType,
                                 aAttribute, aValue, aCloneAttributes);
  *outNode = element ? element->AsDOMNode() : nullptr;
  return rv;
}

nsresult
nsEditor::ReplaceContainer(nsINode* aNode,
                           dom::Element** outNode,
                           const nsAString& aNodeType,
                           const nsAString* aAttribute,
                           const nsAString* aValue,
                           bool aCloneAttributes)
{
  MOZ_ASSERT(aNode);
  MOZ_ASSERT(outNode);

  *outNode = nullptr;

  nsCOMPtr<nsIContent> parent = aNode->GetParent();
  NS_ENSURE_STATE(parent);

  int32_t offset = parent->IndexOf(aNode);

  
  
  nsresult res = CreateHTMLContent(aNodeType, outNode);
  NS_ENSURE_SUCCESS(res, res);

  nsCOMPtr<nsIDOMElement> elem = do_QueryInterface(*outNode);
  
  nsIDOMNode* inNode = aNode->AsDOMNode();

  
  if (aAttribute && aValue && !aAttribute->IsEmpty()) {
    res = elem->SetAttribute(*aAttribute, *aValue);
    NS_ENSURE_SUCCESS(res, res);
  }
  if (aCloneAttributes) {
    res = CloneAttributes(elem, inNode);
    NS_ENSURE_SUCCESS(res, res);
  }
  
  
  
  
  nsAutoReplaceContainerSelNotify selStateNotify(mRangeUpdater, inNode, elem);
  {
    nsAutoTxnsConserveSelection conserveSelection(this);
    while (aNode->HasChildren()) {
      nsCOMPtr<nsIDOMNode> child = aNode->GetFirstChild()->AsDOMNode();

      res = DeleteNode(child);
      NS_ENSURE_SUCCESS(res, res);

      res = InsertNode(child, elem, -1);
      NS_ENSURE_SUCCESS(res, res);
    }
  }

  
  res = InsertNode(elem, parent->AsDOMNode(), offset);
  NS_ENSURE_SUCCESS(res, res);
  
  
  return DeleteNode(inNode);
}





nsresult
nsEditor::RemoveContainer(nsIDOMNode* aNode)
{
  nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
  return RemoveContainer(node);
}

nsresult
nsEditor::RemoveContainer(nsINode* aNode)
{
  NS_ENSURE_TRUE(aNode, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsINode> parent = aNode->GetParentNode();
  NS_ENSURE_STATE(parent);

  int32_t offset = parent->IndexOf(aNode);
  
  
  
  uint32_t nodeOrigLen = aNode->GetChildCount();

  
  nsAutoRemoveContainerSelNotify selNotify(mRangeUpdater, aNode, parent, offset, nodeOrigLen);
                                   
  while (aNode->HasChildren()) {
    nsCOMPtr<nsIContent> child = aNode->GetLastChild();
    nsresult rv = DeleteNode(child->AsDOMNode());
    NS_ENSURE_SUCCESS(rv, rv);

    rv = InsertNode(child->AsDOMNode(), parent->AsDOMNode(), offset);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return DeleteNode(aNode->AsDOMNode());
}








nsresult
nsEditor::InsertContainerAbove( nsIDOMNode *inNode, 
                                nsCOMPtr<nsIDOMNode> *outNode, 
                                const nsAString &aNodeType,
                                const nsAString *aAttribute,
                                const nsAString *aValue)
{
  NS_ENSURE_TRUE(inNode && outNode, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsIContent> node = do_QueryInterface(inNode);
  NS_ENSURE_STATE(node);

  nsCOMPtr<dom::Element> element;
  nsresult rv = InsertContainerAbove(node, getter_AddRefs(element), aNodeType,
                                     aAttribute, aValue);
  *outNode = element ? element->AsDOMNode() : nullptr;
  return rv;
}

nsresult
nsEditor::InsertContainerAbove(nsIContent* aNode,
                               dom::Element** aOutNode,
                               const nsAString& aNodeType,
                               const nsAString* aAttribute,
                               const nsAString* aValue)
{
  MOZ_ASSERT(aNode);

  nsCOMPtr<nsIContent> parent = aNode->GetParent();
  NS_ENSURE_STATE(parent);
  int32_t offset = parent->IndexOf(aNode);

  
  nsCOMPtr<dom::Element> newContent;

  
  nsresult res = CreateHTMLContent(aNodeType, getter_AddRefs(newContent));
  NS_ENSURE_SUCCESS(res, res);

  
  if (aAttribute && aValue && !aAttribute->IsEmpty()) {
    nsIDOMNode* elem = newContent->AsDOMNode();
    res = static_cast<nsIDOMElement*>(elem)->SetAttribute(*aAttribute, *aValue);
    NS_ENSURE_SUCCESS(res, res);
  }
  
  
  nsAutoInsertContainerSelNotify selNotify(mRangeUpdater);
  
  
  res = DeleteNode(aNode->AsDOMNode());
  NS_ENSURE_SUCCESS(res, res);

  {
    nsAutoTxnsConserveSelection conserveSelection(this);
    res = InsertNode(aNode->AsDOMNode(), newContent->AsDOMNode(), 0);
    NS_ENSURE_SUCCESS(res, res);
  }

  
  res = InsertNode(newContent->AsDOMNode(), parent->AsDOMNode(), offset);
  newContent.forget(aOutNode);
  return res;  
}



nsresult
nsEditor::MoveNode(nsIContent* aNode, nsINode* aParent, int32_t aOffset)
{
  MOZ_ASSERT(aNode && aParent);
  MOZ_ASSERT(aOffset == -1 || (0 <= aOffset &&
                               aOffset <= (int32_t)aParent->Length()));
  nsresult res = MoveNode(aNode->AsDOMNode(), aParent->AsDOMNode(), aOffset);
  NS_ASSERTION(NS_SUCCEEDED(res), "MoveNode failed");
  NS_ENSURE_SUCCESS(res, res);
  return NS_OK;
}

nsresult
nsEditor::MoveNode(nsIDOMNode *aNode, nsIDOMNode *aParent, int32_t aOffset)
{
  NS_ENSURE_TRUE(aNode && aParent, NS_ERROR_NULL_POINTER);
  nsresult res;

  int32_t oldOffset;
  nsCOMPtr<nsIDOMNode> oldParent = GetNodeLocation(aNode, &oldOffset);
  
  if (aOffset == -1)
  {
    uint32_t unsignedOffset;
    
    res = GetLengthOfDOMNode(aParent, unsignedOffset);
    NS_ENSURE_SUCCESS(res, res);
    aOffset = (int32_t)unsignedOffset;
  }
  
  
  if ((aParent == oldParent.get()) && (oldOffset == aOffset)) return NS_OK;
  
  
  nsAutoMoveNodeSelNotify selNotify(mRangeUpdater, oldParent, oldOffset, aParent, aOffset);
  
  
  if ((aParent == oldParent.get()) && (oldOffset < aOffset)) 
  {
    aOffset--;  
  }

  
  nsCOMPtr<nsIDOMNode> node = aNode;
  res = DeleteNode(node);
  NS_ENSURE_SUCCESS(res, res);
  return InsertNode(node, aParent, aOffset);
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

class EditorInputEventDispatcher : public nsRunnable
{
public:
  EditorInputEventDispatcher(nsEditor* aEditor,
                             bool aIsTrusted,
                             nsIContent* aTarget) :
    mEditor(aEditor), mTarget(aTarget), mIsTrusted(aIsTrusted)
  {
  }

  NS_IMETHOD Run()
  {
    
    

    if (!mTarget->IsInDoc()) {
      return NS_OK;
    }

    nsCOMPtr<nsIPresShell> ps = mEditor->GetPresShell();
    if (!ps) {
      return NS_OK;
    }

    nsEvent inputEvent(mIsTrusted, NS_FORM_INPUT);
    inputEvent.mFlags.mCancelable = false;
    inputEvent.time = static_cast<uint64_t>(PR_Now() / 1000);
    nsEventStatus status = nsEventStatus_eIgnore;
    nsresult rv =
      ps->HandleEventWithTarget(&inputEvent, nullptr, mTarget, &status);
    NS_ENSURE_SUCCESS(rv, NS_OK); 
    return NS_OK;
  }

private:
  nsRefPtr<nsEditor> mEditor;
  nsCOMPtr<nsIContent> mTarget;
  bool mIsTrusted;
};

void nsEditor::NotifyEditorObservers(void)
{
  for (int32_t i = 0; i < mEditorObservers.Count(); i++) {
    mEditorObservers[i]->EditAction();
  }

  if (!mDispatchInputEvent) {
    return;
  }

  
  
  
  

  nsCOMPtr<nsIContent> target = GetInputEventTargetContent();
  NS_ENSURE_TRUE_VOID(target);

  nsContentUtils::AddScriptRunner(
    new EditorInputEventDispatcher(this, mHandlingTrustedAction, target));
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
                                       uint32_t aFlags,
                                       nsAString& aOutputString)
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsEditor::OutputToStream(nsIOutputStream* aOutputStream,
                         const nsAString& aFormatType,
                         const nsACString& aCharsetOverride,
                         uint32_t aFlags)
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsEditor::DumpContentTree()
{
#ifdef DEBUG
  if (mRootElement) {
    mRootElement->List(stdout);
  }
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
nsEditor::DebugUnitTests(int32_t *outNumTests, int32_t *outNumTestsFailed)
{
#ifdef DEBUG
  NS_NOTREACHED("This should never get called. Overridden by subclasses");
#endif
  return NS_OK;
}


bool     
nsEditor::ArePreservingSelection()
{
  return !(mSavedSel.IsEmpty());
}

void
nsEditor::PreserveSelectionAcrossActions(Selection* aSel)
{
  mSavedSel.SaveSelection(aSel);
  mRangeUpdater.RegisterSelectionState(mSavedSel);
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
  mInIMEMode = true;
  if (mPhonetic) {
    mPhonetic->Truncate(0);
  }
  return NS_OK;
}

void
nsEditor::EndIMEComposition()
{
  NS_ENSURE_TRUE_VOID(mInIMEMode); 

  
  
  if (mTxnMgr) {
    nsCOMPtr<nsITransaction> txn = mTxnMgr->PeekUndoStack();
    nsCOMPtr<nsIAbsorbingTransaction> plcTxn = do_QueryInterface(txn);
    if (plcTxn) {
      DebugOnly<nsresult> rv = plcTxn->Commit();
      NS_ASSERTION(NS_SUCCEEDED(rv),
                   "nsIAbsorbingTransaction::Commit() failed");
    }
  }

  
  mIMETextNode = nullptr;
  mIMETextOffset = 0;
  mIMEBufferLength = 0;
  mInIMEMode = false;
  mIsIMEComposing = false;

  
  NotifyEditorObservers();
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

NS_IMETHODIMP
nsEditor::ForceCompositionEnd()
{
  nsCOMPtr<nsIPresShell> ps = GetPresShell();
  if (!ps) {
    return NS_ERROR_NOT_AVAILABLE;
  }
  nsPresContext* pc = ps->GetPresContext();
  if (!pc) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  if (!mInIMEMode) {
    
    
    
    
    
    
    
    
    return nsIMEStateManager::NotifyIME(NOTIFY_IME_OF_CURSOR_POS_CHANGED, pc);
  }

  return nsIMEStateManager::NotifyIME(REQUEST_TO_COMMIT_COMPOSITION, pc);
}

NS_IMETHODIMP
nsEditor::GetPreferredIMEState(IMEState *aState)
{
  NS_ENSURE_ARG_POINTER(aState);
  aState->mEnabled = IMEState::ENABLED;
  aState->mOpen = IMEState::DONT_CHANGE_OPEN_STATE;

  if (IsReadonly() || IsDisabled()) {
    aState->mEnabled = IMEState::DISABLED;
    return NS_OK;
  }

  nsCOMPtr<nsIContent> content = GetRoot();
  NS_ENSURE_TRUE(content, NS_ERROR_FAILURE);

  nsIFrame* frame = content->GetPrimaryFrame();
  NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);

  switch (frame->GetStyleUIReset()->mIMEMode) {
    case NS_STYLE_IME_MODE_AUTO:
      if (IsPasswordEditor())
        aState->mEnabled = IMEState::PASSWORD;
      break;
    case NS_STYLE_IME_MODE_DISABLED:
      
      aState->mEnabled = IMEState::PASSWORD;
      break;
    case NS_STYLE_IME_MODE_ACTIVE:
      aState->mOpen = IMEState::OPEN;
      break;
    case NS_STYLE_IME_MODE_INACTIVE:
      aState->mOpen = IMEState::CLOSED;
      break;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsEditor::GetComposing(bool* aResult)
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
  nsCOMPtr<nsIDOMElement> rootElement = do_QueryInterface(mRootElement);
  rootElement.forget(aRootElement);
  return NS_OK;
}




NS_IMETHODIMP
nsEditor::StartOperation(EditAction opID, nsIEditor::EDirection aDirection)
{
  mAction = opID;
  mDirection = aDirection;
  return NS_OK;
}




NS_IMETHODIMP
nsEditor::EndOperation()
{
  mAction = EditAction::none;
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
  bool isAttrSet;
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

  
  
  nsCOMPtr<nsIDOMNode> p = aDestNode;
  nsCOMPtr<nsIDOMNode> rootNode = do_QueryInterface(GetRoot());
  NS_ENSURE_TRUE(rootNode, NS_ERROR_NULL_POINTER);
  bool destInBody = true;
  while (p && p != rootNode)
  {
    nsCOMPtr<nsIDOMNode> tmp;
    if (NS_FAILED(p->GetParentNode(getter_AddRefs(tmp))) || !tmp)
    {
      destInBody = false;
      break;
    }
    p = tmp;
  }

  uint32_t sourceCount;
  sourceAttributes->GetLength(&sourceCount);
  uint32_t i, destCount;
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
              result = SetAttributeOrEquivalent(destElement, sourceAttrName, sourceAttrValue, false);
            }
            else {
              
              
              result = SetAttributeOrEquivalent(destElement, sourceAttrName, sourceAttrValue, true);
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


NS_IMETHODIMP nsEditor::ScrollSelectionIntoView(bool aScrollToAnchor)
{
  nsCOMPtr<nsISelectionController> selCon;
  if (NS_SUCCEEDED(GetSelectionController(getter_AddRefs(selCon))) && selCon)
  {
    int16_t region = nsISelectionController::SELECTION_FOCUS_REGION;

    if (aScrollToAnchor)
      region = nsISelectionController::SELECTION_ANCHOR_REGION;

    selCon->ScrollSelectionIntoView(nsISelectionController::SELECTION_NORMAL,
      region, nsISelectionController::SCROLL_OVERFLOW_HIDDEN);
  }

  return NS_OK;
}

NS_IMETHODIMP nsEditor::InsertTextImpl(const nsAString& aStringToInsert, 
                                          nsCOMPtr<nsIDOMNode> *aInOutNode, 
                                          int32_t *aInOutOffset,
                                          nsIDOMDocument *aDoc)
{
  
  
  
  
  nsresult res;
  NS_ENSURE_TRUE(aInOutNode && *aInOutNode && aInOutOffset && aDoc, NS_ERROR_NULL_POINTER);
  if (!mInIMEMode && aStringToInsert.IsEmpty()) return NS_OK;
  nsCOMPtr<nsIDOMText> nodeAsText = do_QueryInterface(*aInOutNode);
  if (!nodeAsText && IsPlaintextEditor()) {
    nsCOMPtr<nsIDOMNode> rootNode = do_QueryInterface(GetRoot());
    
    
    
    
    if (*aInOutNode == rootNode && *aInOutOffset == 0) {
      nsCOMPtr<nsIDOMNode> possibleTextNode;
      res = (*aInOutNode)->GetFirstChild(getter_AddRefs(possibleTextNode));
      if (NS_SUCCEEDED(res)) {
        nodeAsText = do_QueryInterface(possibleTextNode);
        if (nodeAsText) {
          *aInOutNode = possibleTextNode;
        }
      }
    }
    
    
    
    if (!nodeAsText && *aInOutNode == rootNode && *aInOutOffset > 0) {
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
              uint32_t length;
              res = nodeAsText->GetLength(&length);
              if (NS_SUCCEEDED(res)) {
                *aInOutOffset = int32_t(length);
                *aInOutNode = possibleTextNode;
              }
            }
          }
        } else {
          
          
          nsCOMPtr<nsIDOMNode> possibleTextNode;
          res = children->Item(*aInOutOffset - 1, getter_AddRefs(possibleTextNode));
          nodeAsText = do_QueryInterface(possibleTextNode);
          if (nodeAsText) {
            uint32_t length;
            res = nodeAsText->GetLength(&length);
            if (NS_SUCCEEDED(res)) {
              *aInOutOffset = int32_t(length);
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
        uint32_t length;
        res = nodeAsText->GetLength(&length);
        if (NS_SUCCEEDED(res)) {
          *aInOutOffset = int32_t(length);
          *aInOutNode = previous;
        }
      } else {
        nsCOMPtr<nsIDOMNode> parent;
        (*aInOutNode)->GetParentNode(getter_AddRefs(parent));
        if (parent == rootNode) {
          *aInOutNode = parent;
        }
      }
    }
  }
  int32_t offset = *aInOutOffset;
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
                                              int32_t aOffset,
                                              bool aSuppressIME)
{
  nsRefPtr<EditTxn> txn;
  nsresult result = NS_OK;
  bool isIMETransaction = false;
  
  
  if (mIMETextRangeList && mInIMEMode && !aSuppressIME)
  {
    if (!mIMETextNode)
    {
      mIMETextNode = aTextNode;
      mIMETextOffset = aOffset;
    }
    uint16_t len ;
    len = mIMETextRangeList->GetLength();
    if (len > 0)
    {
      nsCOMPtr<nsIPrivateTextRange> range;
      for (uint16_t i = 0; i < len; i++) 
      {
        range = mIMETextRangeList->Item(i);
        if (range)
        {
          uint16_t type;
          result = range->GetRangeType(&type);
          if (NS_SUCCEEDED(result)) 
          {
            if (type == nsIPrivateTextRange::TEXTRANGE_RAWINPUT) 
            {
              uint16_t start, end;
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
    isIMETransaction = true;
  }
  else
  {
    nsRefPtr<InsertTextTxn> insertTxn;
    result = CreateTxnForInsertText(aStringToInsert, aTextNode, aOffset,
                                    getter_AddRefs(insertTxn));
    txn = insertTxn;
  }
  NS_ENSURE_SUCCESS(result, result);

  
  int32_t i;
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
    uint32_t len;
    mIMETextNode->GetLength(&len);
    if (!len)
    {
      DeleteNode(mIMETextNode);
      mIMETextNode = nullptr;
      static_cast<IMETextTxn*>(txn.get())->MarkFixed();  
    }
  }
  
  return result;
}


NS_IMETHODIMP nsEditor::SelectEntireDocument(nsISelection *aSelection)
{
  if (!aSelection) { return NS_ERROR_NULL_POINTER; }

  nsCOMPtr<nsIDOMElement> rootElement = do_QueryInterface(GetRoot());
  if (!rootElement) { return NS_ERROR_NOT_INITIALIZED; }

  return aSelection->SelectAllChildren(rootElement);
}


nsINode*
nsEditor::GetFirstEditableNode(nsINode* aRoot)
{
  MOZ_ASSERT(aRoot);

  nsIContent* node = GetLeftmostChild(aRoot);
  if (node && !IsEditable(node)) {
    node = GetNextNode(node,  true);
  }

  return (node != aRoot) ? node : nullptr;
}


NS_IMETHODIMP
nsEditor::NotifyDocumentListeners(TDocumentListenerNotification aNotificationType)
{
  int32_t numListeners = mDocStateListeners.Count();
  if (!numListeners)    
    return NS_OK;
 
  nsCOMArray<nsIDocumentStateListener> listeners(mDocStateListeners);
  nsresult rv = NS_OK;
  int32_t i;

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
        bool docIsDirty;
        rv = GetDocumentModified(&docIsDirty);
        NS_ENSURE_SUCCESS(rv, rv);
        
        if (docIsDirty == mDocDirtyState)
          return NS_OK;
        
        mDocDirtyState = (int8_t)docIsDirty;
        
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
                                               int32_t aOffset,
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
                              uint32_t             aOffset,
                              uint32_t             aLength)
{
  nsRefPtr<DeleteTextTxn> txn;
  nsresult result = CreateTxnForDeleteText(aElement, aOffset, aLength,
                                           getter_AddRefs(txn));
  nsAutoRules beginRulesSniffing(this, EditAction::deleteText, nsIEditor::ePrevious);
  if (NS_SUCCEEDED(result))  
  {
    
    int32_t i;
    for (i = 0; i < mActionListeners.Count(); i++)
      mActionListeners[i]->WillDeleteText(aElement, aOffset, aLength);
    
    result = DoTransaction(txn); 
    
    
    for (i = 0; i < mActionListeners.Count(); i++)
      mActionListeners[i]->DidDeleteText(aElement, aOffset, aLength, result);
  }
  return result;
}


nsresult
nsEditor::CreateTxnForDeleteText(nsIDOMCharacterData* aElement,
                                 uint32_t             aOffset,
                                 uint32_t             aLength,
                                 DeleteTextTxn**      aTxn)
{
  NS_ENSURE_TRUE(aElement, NS_ERROR_NULL_POINTER);

  nsRefPtr<DeleteTextTxn> txn = new DeleteTextTxn();

  nsresult res = txn->Init(this, aElement, aOffset, aLength, &mRangeUpdater);
  NS_ENSURE_SUCCESS(res, res);

  txn.forget(aTxn);
  return NS_OK;
}




NS_IMETHODIMP nsEditor::CreateTxnForSplitNode(nsIDOMNode *aNode,
                                         uint32_t    aOffset,
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
                        int32_t      aOffset,
                        nsIDOMNode*  aNewLeftNode,
                        nsIDOMNode*  aParent)
{
#ifdef NS_DEBUG_EDITOR
  if (gNoisy) { printf("SplitNodeImpl: left=%p, right=%p, offset=%d\n", (void*)aNewLeftNode, (void*)aExistingRightNode, aOffset); }
#endif

  NS_ASSERTION(((nullptr!=aExistingRightNode) &&
                (nullptr!=aNewLeftNode) &&
                (nullptr!=aParent)),
                "null arg");
  nsresult result;
  if ((nullptr!=aExistingRightNode) &&
      (nullptr!=aNewLeftNode) &&
      (nullptr!=aParent))
  {
    
    nsCOMPtr<nsISelection> selection;
    result = GetSelection(getter_AddRefs(selection));
    NS_ENSURE_SUCCESS(result, result);
    NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);

    
    nsCOMPtr<nsIDOMNode> selStartNode, selEndNode;
    int32_t selStartOffset, selEndOffset;
    result = GetStartNodeAndOffset(selection, getter_AddRefs(selStartNode), &selStartOffset);
    if (NS_FAILED(result)) selStartNode = nullptr;  
    result = GetEndNodeAndOffset(selection, getter_AddRefs(selEndNode), &selEndOffset);
    if (NS_FAILED(result)) selStartNode = nullptr;  

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
            int32_t i=aOffset-1;
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
                        bool         aNodeToKeepIsFirst)
{
  NS_ASSERTION(aNodeToKeep && aNodeToJoin && aParent, "null arg");
  nsresult result = NS_OK;
  if (aNodeToKeep && aNodeToJoin && aParent)
  {
    
    nsCOMPtr<nsISelection> selection;
    GetSelection(getter_AddRefs(selection));
    NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);

    
    nsCOMPtr<nsIDOMNode> selStartNode, selEndNode;
    int32_t selStartOffset, selEndOffset, joinOffset, keepOffset;
    result = GetStartNodeAndOffset(selection, getter_AddRefs(selStartNode), &selStartOffset);
    if (NS_FAILED(result)) selStartNode = nullptr;
    result = GetEndNodeAndOffset(selection, getter_AddRefs(selEndNode), &selEndOffset);
    
    if (NS_FAILED(result)) selStartNode = nullptr;

    nsCOMPtr<nsIDOMNode> leftNode;
    if (aNodeToKeepIsFirst)
      leftNode = aNodeToKeep;
    else
      leftNode = aNodeToJoin;

    uint32_t firstNodeLength;
    result = GetLengthOfDOMNode(leftNode, firstNodeLength);
    NS_ENSURE_SUCCESS(result, result);
    nsCOMPtr<nsIDOMNode> parent = GetNodeLocation(aNodeToJoin, &joinOffset);
    parent = GetNodeLocation(aNodeToKeep, &keepOffset);
    
    
    
    
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
        int32_t i;  
        uint32_t childCount=0;
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
        
        
        bool bNeedToAdjust = false;
        
        
        if (selStartNode.get() == aNodeToJoin)
        {
          bNeedToAdjust = true;
          selStartNode = aNodeToKeep;
          if (aNodeToKeepIsFirst)
          {
            selStartOffset += firstNodeLength;
          }
        }
        else if ((selStartNode.get() == aNodeToKeep) && !aNodeToKeepIsFirst)
        {
          bNeedToAdjust = true;
          selStartOffset += firstNodeLength;
        }
                
        
        if (selEndNode.get() == aNodeToJoin)
        {
          bNeedToAdjust = true;
          selEndNode = aNodeToKeep;
          if (aNodeToKeepIsFirst)
          {
            selEndOffset += firstNodeLength;
          }
        }
        else if ((selEndNode.get() == aNodeToKeep) && !aNodeToKeepIsFirst)
        {
          bNeedToAdjust = true;
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


int32_t
nsEditor::GetChildOffset(nsIDOMNode* aChild, nsIDOMNode* aParent)
{
  MOZ_ASSERT(aChild && aParent);

  nsCOMPtr<nsINode> parent = do_QueryInterface(aParent);
  nsCOMPtr<nsINode> child = do_QueryInterface(aChild);
  MOZ_ASSERT(parent && child);

  int32_t idx = parent->IndexOf(child);
  MOZ_ASSERT(idx != -1);
  return idx;
}


already_AddRefed<nsIDOMNode>
nsEditor::GetNodeLocation(nsIDOMNode* aChild, int32_t* outOffset)
{
  MOZ_ASSERT(aChild && outOffset);
  NS_ENSURE_TRUE(aChild && outOffset, nullptr);
  *outOffset = -1;

  nsCOMPtr<nsIDOMNode> parent;

  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
    aChild->GetParentNode(getter_AddRefs(parent))));
  if (parent) {
    *outOffset = GetChildOffset(aChild, parent);
  }

  return parent.forget();
}



nsresult
nsEditor::GetLengthOfDOMNode(nsIDOMNode *aNode, uint32_t &aCount) 
{
  aCount = 0;
  nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
  NS_ENSURE_TRUE(node, NS_ERROR_NULL_POINTER);
  aCount = node->Length();
  return NS_OK;
}


nsresult 
nsEditor::GetPriorNode(nsIDOMNode  *aParentNode, 
                       int32_t      aOffset, 
                       bool         aEditableNode, 
                       nsCOMPtr<nsIDOMNode> *aResultNode,
                       bool         bNoBlockCrossing)
{
  NS_ENSURE_TRUE(aResultNode, NS_ERROR_NULL_POINTER);
  *aResultNode = nullptr;

  nsCOMPtr<nsINode> parentNode = do_QueryInterface(aParentNode);
  NS_ENSURE_TRUE(parentNode, NS_ERROR_NULL_POINTER);

  *aResultNode = do_QueryInterface(GetPriorNode(parentNode, aOffset,
                                                aEditableNode,
                                                bNoBlockCrossing));
  return NS_OK;
}

nsIContent*
nsEditor::GetPriorNode(nsINode* aParentNode,
                       int32_t aOffset,
                       bool aEditableNode,
                       bool aNoBlockCrossing)
{
  MOZ_ASSERT(aParentNode);

  
  
  if (!aOffset || aParentNode->NodeType() == nsIDOMNode::TEXT_NODE) {
    if (aNoBlockCrossing && IsBlockNode(aParentNode)) {
      
      return nullptr;
    }
    return GetPriorNode(aParentNode, aEditableNode, aNoBlockCrossing);
  }

  
  if (nsIContent* child = aParentNode->GetChildAt(aOffset)) {
    return GetPriorNode(child, aEditableNode, aNoBlockCrossing);
  }

  
  
  nsIContent* resultNode = GetRightmostChild(aParentNode, aNoBlockCrossing);
  if (!resultNode || !aEditableNode || IsEditable(resultNode)) {
    return resultNode;
  }

  
  return GetPriorNode(resultNode, aEditableNode, aNoBlockCrossing);
}


nsresult 
nsEditor::GetNextNode(nsIDOMNode   *aParentNode, 
                      int32_t      aOffset, 
                      bool         aEditableNode, 
                      nsCOMPtr<nsIDOMNode> *aResultNode,
                      bool         bNoBlockCrossing)
{
  NS_ENSURE_TRUE(aResultNode, NS_ERROR_NULL_POINTER);
  *aResultNode = nullptr;

  nsCOMPtr<nsINode> parentNode = do_QueryInterface(aParentNode);
  NS_ENSURE_TRUE(parentNode, NS_ERROR_NULL_POINTER);

  *aResultNode = do_QueryInterface(GetNextNode(parentNode, aOffset,
                                               aEditableNode,
                                               bNoBlockCrossing));
  return NS_OK;
}

nsIContent*
nsEditor::GetNextNode(nsINode* aParentNode,
                      int32_t aOffset,
                      bool aEditableNode,
                      bool aNoBlockCrossing)
{
  MOZ_ASSERT(aParentNode);

  
  if (aParentNode->NodeType() == nsIDOMNode::TEXT_NODE) {
    nsINode* parent = aParentNode->GetParentNode();
    NS_ENSURE_TRUE(parent, nullptr);
    aOffset = parent->IndexOf(aParentNode) + 1; 
    aParentNode = parent;
  }

  
  nsIContent* child = aParentNode->GetChildAt(aOffset);
  if (child) {
    if (aNoBlockCrossing && IsBlockNode(child)) {
      return child;
    }

    nsIContent* resultNode = GetLeftmostChild(child, aNoBlockCrossing);
    if (!resultNode) {
      return child;
    }

    if (!IsDescendantOfEditorRoot(resultNode)) {
      return nullptr;
    }

    if (!aEditableNode || IsEditable(resultNode)) {
      return resultNode;
    }

    
    return GetNextNode(resultNode, aEditableNode, aNoBlockCrossing);
  }
    
  
  
  if (aNoBlockCrossing && IsBlockNode(aParentNode)) {
    
    return nullptr;
  }

  return GetNextNode(aParentNode, aEditableNode, aNoBlockCrossing);
}


nsresult 
nsEditor::GetPriorNode(nsIDOMNode  *aCurrentNode, 
                       bool         aEditableNode, 
                       nsCOMPtr<nsIDOMNode> *aResultNode,
                       bool         bNoBlockCrossing)
{
  NS_ENSURE_TRUE(aResultNode, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsINode> currentNode = do_QueryInterface(aCurrentNode);
  NS_ENSURE_TRUE(currentNode, NS_ERROR_NULL_POINTER);

  *aResultNode = do_QueryInterface(GetPriorNode(currentNode, aEditableNode,
                                                bNoBlockCrossing));
  return NS_OK;
}

nsIContent*
nsEditor::GetPriorNode(nsINode* aCurrentNode, bool aEditableNode,
                       bool aNoBlockCrossing )
{
  MOZ_ASSERT(aCurrentNode);

  if (!IsDescendantOfEditorRoot(aCurrentNode)) {
    return nullptr;
  }

  return FindNode(aCurrentNode, false, aEditableNode, aNoBlockCrossing);
}

nsIContent*
nsEditor::FindNextLeafNode(nsINode  *aCurrentNode, 
                           bool      aGoForward,
                           bool      bNoBlockCrossing)
{
  
  NS_PRECONDITION(IsDescendantOfEditorRoot(aCurrentNode) &&
                  !IsEditorRoot(aCurrentNode),
                  "Bogus arguments");

  nsINode* cur = aCurrentNode;
  for (;;) {
    
    
    nsIContent* sibling =
      aGoForward ? cur->GetNextSibling() : cur->GetPreviousSibling();
    if (sibling) {
      if (bNoBlockCrossing && IsBlockNode(sibling)) {
        
        return sibling;
      }
      nsIContent *leaf =
        aGoForward ? GetLeftmostChild(sibling, bNoBlockCrossing) :
                     GetRightmostChild(sibling, bNoBlockCrossing);
      if (!leaf) { 
        return sibling;
      }

      return leaf;
    }

    nsINode *parent = cur->GetParentNode();
    if (!parent) {
      return nullptr;
    }

    NS_ASSERTION(IsDescendantOfEditorRoot(parent),
                 "We started with a proper descendant of root, and should stop "
                 "if we ever hit the root, so we better have a descendant of "
                 "root now!");
    if (IsEditorRoot(parent) ||
        (bNoBlockCrossing && IsBlockNode(parent))) {
      return nullptr;
    }

    cur = parent;
  }

  NS_NOTREACHED("What part of for(;;) do you not understand?");
  return nullptr;
}

nsresult
nsEditor::GetNextNode(nsIDOMNode* aCurrentNode,
                      bool aEditableNode,
                      nsCOMPtr<nsIDOMNode> *aResultNode,
                      bool bNoBlockCrossing)
{
  nsCOMPtr<nsINode> currentNode = do_QueryInterface(aCurrentNode);
  if (!currentNode || !aResultNode) {
    return NS_ERROR_NULL_POINTER;
  }

  *aResultNode = do_QueryInterface(GetNextNode(currentNode, aEditableNode,
                                               bNoBlockCrossing));
  return NS_OK;
}

nsIContent*
nsEditor::GetNextNode(nsINode* aCurrentNode,
                      bool aEditableNode,
                      bool bNoBlockCrossing)
{
  MOZ_ASSERT(aCurrentNode);

  if (!IsDescendantOfEditorRoot(aCurrentNode)) {
    return nullptr;
  }

  return FindNode(aCurrentNode, true, aEditableNode, bNoBlockCrossing);
}

nsIContent*
nsEditor::FindNode(nsINode *aCurrentNode,
                   bool     aGoForward,
                   bool     aEditableNode,
                   bool     bNoBlockCrossing)
{
  if (IsEditorRoot(aCurrentNode)) {
    
    
    

    return nullptr;
  }

  nsCOMPtr<nsIContent> candidate =
    FindNextLeafNode(aCurrentNode, aGoForward, bNoBlockCrossing);
  
  if (!candidate) {
    return nullptr;
  }

  if (!aEditableNode || IsEditable(candidate)) {
    return candidate;
  }

  return FindNode(candidate, aGoForward, aEditableNode, bNoBlockCrossing);
}

nsIDOMNode*
nsEditor::GetRightmostChild(nsIDOMNode* aCurrentNode,
                            bool bNoBlockCrossing)
{
  nsCOMPtr<nsINode> currentNode = do_QueryInterface(aCurrentNode);
  nsIContent* result = GetRightmostChild(currentNode, bNoBlockCrossing);
  return result ? result->AsDOMNode() : nullptr;
}

nsIContent*
nsEditor::GetRightmostChild(nsINode *aCurrentNode,
                            bool     bNoBlockCrossing)
{
  NS_ENSURE_TRUE(aCurrentNode, nullptr);
  nsIContent *cur = aCurrentNode->GetLastChild();
  if (!cur) {
    return nullptr;
  }
  for (;;) {
    if (bNoBlockCrossing && IsBlockNode(cur)) {
      return cur;
    }
    nsIContent* next = cur->GetLastChild();
    if (!next) {
      return cur;
    }
    cur = next;
  }

  NS_NOTREACHED("What part of for(;;) do you not understand?");
  return nullptr;
}

nsIContent*
nsEditor::GetLeftmostChild(nsINode *aCurrentNode,
                           bool     bNoBlockCrossing)
{
  NS_ENSURE_TRUE(aCurrentNode, nullptr);
  nsIContent *cur = aCurrentNode->GetFirstChild();
  if (!cur) {
    return nullptr;
  }
  for (;;) {
    if (bNoBlockCrossing && IsBlockNode(cur)) {
      return cur;
    }
    nsIContent *next = cur->GetFirstChild();
    if (!next) {
      return cur;
    }
    cur = next;
  }

  NS_NOTREACHED("What part of for(;;) do you not understand?");
  return nullptr;
}

nsIDOMNode*
nsEditor::GetLeftmostChild(nsIDOMNode* aCurrentNode,
                           bool bNoBlockCrossing)
{
  nsCOMPtr<nsINode> currentNode = do_QueryInterface(aCurrentNode);
  nsIContent* result = GetLeftmostChild(currentNode, bNoBlockCrossing);
  return result ? result->AsDOMNode() : nullptr;
}

bool
nsEditor::IsBlockNode(nsIDOMNode* aNode)
{
  nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
  return IsBlockNode(node);
}

bool
nsEditor::IsBlockNode(nsINode* aNode)
{
  
  
  
  
  return false;
}

bool
nsEditor::CanContain(nsIDOMNode* aParent, nsIDOMNode* aChild)
{
  nsCOMPtr<nsIContent> parent = do_QueryInterface(aParent);
  NS_ENSURE_TRUE(parent, false);

  switch (parent->NodeType()) {
  case nsIDOMNode::ELEMENT_NODE:
  case nsIDOMNode::DOCUMENT_FRAGMENT_NODE:
    return TagCanContain(parent->Tag(), aChild);
  }
  return false;
}

bool
nsEditor::CanContainTag(nsIDOMNode* aParent, nsIAtom* aChildTag)
{
  nsCOMPtr<nsIContent> parent = do_QueryInterface(aParent);
  NS_ENSURE_TRUE(parent, false);

  switch (parent->NodeType()) {
  case nsIDOMNode::ELEMENT_NODE:
  case nsIDOMNode::DOCUMENT_FRAGMENT_NODE:
    return TagCanContainTag(parent->Tag(), aChildTag);
  }
  return false;
}

bool 
nsEditor::TagCanContain(nsIAtom* aParentTag, nsIDOMNode* aChild)
{
  nsCOMPtr<nsIContent> child = do_QueryInterface(aChild);
  NS_ENSURE_TRUE(child, false);

  switch (child->NodeType()) {
  case nsIDOMNode::TEXT_NODE:
  case nsIDOMNode::ELEMENT_NODE:
  case nsIDOMNode::DOCUMENT_FRAGMENT_NODE:
    return TagCanContainTag(aParentTag, child->Tag());
  }
  return false;
}

bool 
nsEditor::TagCanContainTag(nsIAtom* aParentTag, nsIAtom* aChildTag)
{
  return true;
}

bool
nsEditor::IsRoot(nsIDOMNode* inNode)
{
  NS_ENSURE_TRUE(inNode, false);

  nsCOMPtr<nsIDOMNode> rootNode = do_QueryInterface(GetRoot());

  return inNode == rootNode;
}

bool 
nsEditor::IsRoot(nsINode* inNode)
{
  NS_ENSURE_TRUE(inNode, false);

  nsCOMPtr<nsINode> rootNode = GetRoot();

  return inNode == rootNode;
}

bool
nsEditor::IsEditorRoot(nsINode* aNode)
{
  NS_ENSURE_TRUE(aNode, false);
  nsCOMPtr<nsINode> rootNode = GetEditorRoot();
  return aNode == rootNode;
}

bool 
nsEditor::IsDescendantOfRoot(nsIDOMNode* inNode)
{
  nsCOMPtr<nsINode> node = do_QueryInterface(inNode);
  return IsDescendantOfRoot(node);
}

bool
nsEditor::IsDescendantOfRoot(nsINode* inNode)
{
  NS_ENSURE_TRUE(inNode, false);
  nsCOMPtr<nsIContent> root = GetRoot();
  NS_ENSURE_TRUE(root, false);

  return nsContentUtils::ContentIsDescendantOf(inNode, root);
}

bool
nsEditor::IsDescendantOfEditorRoot(nsIDOMNode* aNode)
{
  nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
  return IsDescendantOfEditorRoot(node);
}

bool
nsEditor::IsDescendantOfEditorRoot(nsINode* aNode)
{
  NS_ENSURE_TRUE(aNode, false);
  nsCOMPtr<nsIContent> root = GetEditorRoot();
  NS_ENSURE_TRUE(root, false);

  return nsContentUtils::ContentIsDescendantOf(aNode, root);
}

bool 
nsEditor::IsContainer(nsIDOMNode *aNode)
{
  return aNode ? true : false;
}

static inline bool
IsElementVisible(dom::Element* aElement)
{
  if (aElement->GetPrimaryFrame()) {
    
    return true;
  }

  nsIContent *cur = aElement;
  for (; ;) {
    
    
    
    bool haveLazyBitOnChild = cur->HasFlag(NODE_NEEDS_FRAME);
    cur = cur->GetFlattenedTreeParent();
    if (!cur) {
      if (!haveLazyBitOnChild) {
        
        
        return false;
      }

      
      
      break;
    }

    if (cur->GetPrimaryFrame()) {
      if (!haveLazyBitOnChild) {
        
        
        return false;
      }

      if (cur->GetPrimaryFrame()->IsLeaf()) {
        
        return false;
      }

      
      
      break;
    }
  }

  
  
  
  
  nsRefPtr<nsStyleContext> styleContext =
    nsComputedDOMStyle::GetStyleContextForElementNoFlush(aElement,
                                                         nullptr, nullptr);
  if (styleContext) {
    return styleContext->GetStyleDisplay()->mDisplay != NS_STYLE_DISPLAY_NONE;
  }
  return false;
}

bool 
nsEditor::IsEditable(nsIDOMNode *aNode)
{
  nsCOMPtr<nsIContent> content = do_QueryInterface(aNode);
  return IsEditable(content);
}

bool
nsEditor::IsEditable(nsIContent *aNode)
{
  NS_ENSURE_TRUE(aNode, false);

  if (IsMozEditorBogusNode(aNode) || !IsModifiableNode(aNode)) return false;

  
  
  if (aNode->IsElement() && !IsElementVisible(aNode->AsElement())) {
    
    
    
    return false;
  }
  switch (aNode->NodeType()) {
    case nsIDOMNode::ELEMENT_NODE:
    case nsIDOMNode::TEXT_NODE:
      return true; 
    default:
      return false;
  }
}

bool
nsEditor::IsMozEditorBogusNode(nsIContent *element)
{
  return element &&
         element->AttrValueIs(kNameSpaceID_None, kMOZEditorBogusNodeAttrAtom,
                              kMOZEditorBogusNodeValue, eCaseMatters);
}

uint32_t
nsEditor::CountEditableChildren(nsINode* aNode)
{
  MOZ_ASSERT(aNode);
  uint32_t count = 0;
  for (nsIContent* child = aNode->GetFirstChild();
       child;
       child = child->GetNextSibling()) {
    if (IsEditable(child)) {
      ++count;
    }
  }
  return count;
}




NS_IMETHODIMP nsEditor::IncrementModificationCount(int32_t inNumMods)
{
  uint32_t oldModCount = mModCount;

  mModCount += inNumMods;

  if ((oldModCount == 0 && mModCount != 0)
   || (oldModCount != 0 && mModCount == 0))
    NotifyDocumentListeners(eDocumentStateChanged);
  return NS_OK;
}


NS_IMETHODIMP nsEditor::GetModificationCount(int32_t *outModCount)
{
  NS_ENSURE_ARG_POINTER(outModCount);
  *outModCount = mModCount;
  return NS_OK;
}


NS_IMETHODIMP nsEditor::ResetModificationCount()
{
  bool doNotify = (mModCount != 0);

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

    return nullptr;
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





bool 
nsEditor::NodesSameType(nsIDOMNode *aNode1, nsIDOMNode *aNode2)
{
  if (!aNode1 || !aNode2) {
    NS_NOTREACHED("null node passed to nsEditor::NodesSameType()");
    return false;
  }

  nsCOMPtr<nsIContent> content1 = do_QueryInterface(aNode1);
  NS_ENSURE_TRUE(content1, false);

  nsCOMPtr<nsIContent> content2 = do_QueryInterface(aNode2);
  NS_ENSURE_TRUE(content2, false);

  return AreNodesSameType(content1, content2);
}


bool
nsEditor::AreNodesSameType(nsIContent* aNode1, nsIContent* aNode2)
{
  MOZ_ASSERT(aNode1);
  MOZ_ASSERT(aNode2);
  return aNode1->Tag() == aNode2->Tag();
}





bool
nsEditor::IsTextNode(nsIDOMNode *aNode)
{
  if (!aNode)
  {
    NS_NOTREACHED("null node passed to IsTextNode()");
    return false;
  }
  
  uint16_t nodeType;
  aNode->GetNodeType(&nodeType);
  return (nodeType == nsIDOMNode::TEXT_NODE);
}

bool
nsEditor::IsTextNode(nsINode *aNode)
{
  return aNode->NodeType() == nsIDOMNode::TEXT_NODE;
}




nsCOMPtr<nsIDOMNode> 
nsEditor::GetChildAt(nsIDOMNode *aParent, int32_t aOffset)
{
  nsCOMPtr<nsIDOMNode> resultNode;
  
  nsCOMPtr<nsIContent> parent = do_QueryInterface(aParent);

  NS_ENSURE_TRUE(parent, resultNode);

  resultNode = do_QueryInterface(parent->GetChildAt(aOffset));

  return resultNode;
}






nsCOMPtr<nsIDOMNode>
nsEditor::GetNodeAtRangeOffsetPoint(nsIDOMNode* aParentOrNode, int32_t aOffset)
{
  if (IsTextNode(aParentOrNode)) {
    return aParentOrNode;
  }
  return GetChildAt(aParentOrNode, aOffset);
}





nsresult 
nsEditor::GetStartNodeAndOffset(nsISelection *aSelection,
                                       nsIDOMNode **outStartNode,
                                       int32_t *outStartOffset)
{
  NS_ENSURE_TRUE(outStartNode && outStartOffset && aSelection, NS_ERROR_NULL_POINTER);

  *outStartNode = nullptr;
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
                                       int32_t *outEndOffset)
{
  NS_ENSURE_TRUE(outEndNode && outEndOffset, NS_ERROR_NULL_POINTER);

  *outEndNode = nullptr;
    
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
nsEditor::IsPreformatted(nsIDOMNode *aNode, bool *aResult)
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
                                                                 nullptr,
                                                                 ps);
  }

  if (!elementStyle)
  {
    
    
    
    *aResult = false;
    return NS_OK;
  }

  const nsStyleText* styleText = elementStyle->GetStyleText();

  *aResult = styleText->WhiteSpaceIsSignificant();
  return NS_OK;
}












nsresult
nsEditor::SplitNodeDeep(nsIDOMNode *aNode, 
                        nsIDOMNode *aSplitPointParent, 
                        int32_t aSplitPointOffset,
                        int32_t *outOffset,
                        bool    aNoEmptyContainers,
                        nsCOMPtr<nsIDOMNode> *outLeftNode,
                        nsCOMPtr<nsIDOMNode> *outRightNode)
{
  nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
  NS_ENSURE_TRUE(node && aSplitPointParent && outOffset, NS_ERROR_NULL_POINTER);
  int32_t offset = aSplitPointOffset;

  if (outLeftNode)  *outLeftNode  = nullptr;
  if (outRightNode) *outRightNode = nullptr;

  nsCOMPtr<nsINode> nodeToSplit = do_QueryInterface(aSplitPointParent);
  while (nodeToSplit) {
    
    
    
    
    
    
    nsCOMPtr<nsIDOMCharacterData> nodeAsText = do_QueryInterface(nodeToSplit);
    uint32_t len = nodeToSplit->Length();
    bool bDoSplit = false;
    
    if (!(aNoEmptyContainers || nodeAsText) || (offset && (offset != (int32_t)len)))
    {
      bDoSplit = true;
      nsCOMPtr<nsIDOMNode> tempNode;
      nsresult rv = SplitNode(nodeToSplit->AsDOMNode(), offset,
                              getter_AddRefs(tempNode));
      NS_ENSURE_SUCCESS(rv, rv);

      if (outRightNode) {
        *outRightNode = nodeToSplit->AsDOMNode();
      }
      if (outLeftNode) {
        *outLeftNode = tempNode;
      }
    }

    nsINode* parentNode = nodeToSplit->GetParentNode();
    NS_ENSURE_TRUE(parentNode, NS_ERROR_FAILURE);

    if (!bDoSplit && offset) {
      
      offset = parentNode->IndexOf(nodeToSplit) + 1;
      if (outLeftNode) {
        *outLeftNode = nodeToSplit->AsDOMNode();
      }
    } else {
      offset = parentNode->IndexOf(nodeToSplit);
      if (outRightNode) {
        *outRightNode = nodeToSplit->AsDOMNode();
      }
    }

    if (nodeToSplit == node) {
      
      break;
    }

    nodeToSplit = parentNode;
  }

  if (!nodeToSplit) {
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
                       int32_t *outOffset)
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
    
    uint32_t length;
    res = GetLengthOfDOMNode(leftNodeToJoin, length);
    NS_ENSURE_SUCCESS(res, res);
    
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

void
nsEditor::BeginUpdateViewBatch()
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
  }

  mUpdateCount++;
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
    

    nsCOMPtr<nsISelection>selection;
    GetSelection(getter_AddRefs(selection));

    if (selection) {
      nsCOMPtr<nsISelectionPrivate>selPrivate(do_QueryInterface(selection));
      selPrivate->EndBatchChanges();
    }
  }

  return NS_OK;
}

bool 
nsEditor::GetShouldTxnSetSelection()
{
  return mShouldTxnSetSelection;
}


NS_IMETHODIMP 
nsEditor::DeleteSelectionImpl(EDirection aAction,
                              EStripWrappers aStripWrappers)
{
  MOZ_ASSERT(aStripWrappers == eStrip || aStripWrappers == eNoStrip);

  nsCOMPtr<nsISelection>selection;
  nsresult res = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);
  nsRefPtr<EditAggregateTxn> txn;
  nsCOMPtr<nsINode> deleteNode;
  int32_t deleteCharOffset = 0, deleteCharLength = 0;
  res = CreateTxnForDeleteSelection(aAction, getter_AddRefs(txn),
                                    getter_AddRefs(deleteNode),
                                    &deleteCharOffset, &deleteCharLength);
  nsCOMPtr<nsIDOMCharacterData> deleteCharData(do_QueryInterface(deleteNode));

  if (NS_SUCCEEDED(res))  
  {
    nsAutoRules beginRulesSniffing(this, EditAction::deleteSelection, aAction);
    int32_t i;
    
    if (!deleteNode)
      for (i = 0; i < mActionListeners.Count(); i++)
        mActionListeners[i]->WillDeleteSelection(selection);
    else if (deleteCharData)
      for (i = 0; i < mActionListeners.Count(); i++)
        mActionListeners[i]->WillDeleteText(deleteCharData, deleteCharOffset, 1);
    else
      for (i = 0; i < mActionListeners.Count(); i++)
        mActionListeners[i]->WillDeleteNode(deleteNode->AsDOMNode());

    
    res = DoTransaction(txn);  

    
    if (!deleteNode)
      for (i = 0; i < mActionListeners.Count(); i++)
        mActionListeners[i]->DidDeleteSelection(selection);
    else if (deleteCharData)
      for (i = 0; i < mActionListeners.Count(); i++)
        mActionListeners[i]->DidDeleteText(deleteCharData, deleteCharOffset, 1, res);
    else
      for (i = 0; i < mActionListeners.Count(); i++)
        mActionListeners[i]->DidDeleteNode(deleteNode->AsDOMNode(), res);
  }

  return res;
}


NS_IMETHODIMP
nsEditor::DeleteSelectionAndCreateNode(const nsAString& aTag,
                                           nsIDOMNode ** aNewNode)
{
  nsresult result = DeleteSelectionAndPrepareToCreateNode();
  NS_ENSURE_SUCCESS(result, result);

  nsRefPtr<Selection> selection = GetSelection();
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsINode> node = selection->GetAnchorNode();
  int32_t offset = selection->GetAnchorOffset();

  nsCOMPtr<nsIDOMNode> newNode;
  result = CreateNode(aTag, node->AsDOMNode(), offset,
                      getter_AddRefs(newNode));
  
  
  *aNewNode = newNode;
  NS_IF_ADDREF(*aNewNode);

  
  return selection->Collapse(node, offset + 1);
}




int32_t
nsEditor::GetIMEBufferLength()
{
  return mIMEBufferLength;
}

void
nsEditor::SetIsIMEComposing(){  
  
  nsCOMPtr<nsIPrivateTextRange> rangePtr;
  uint16_t listlen, type;

  mIsIMEComposing = false;
  listlen = mIMETextRangeList->GetLength();

  for (uint16_t i = 0; i < listlen; i++)
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
        mIsIMEComposing = true;
#ifdef DEBUG_IME
        printf("nsEditor::mIsIMEComposing = true\n");
#endif
        break;
      }
  }
  return;
}

bool
nsEditor::IsIMEComposing() {
  return mIsIMEComposing;
}

nsresult
nsEditor::DeleteSelectionAndPrepareToCreateNode()
{
  nsresult res;
  nsRefPtr<Selection> selection = GetSelection();
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);
  MOZ_ASSERT(selection->GetAnchorFocusRange());

  if (!selection->GetAnchorFocusRange()->Collapsed()) {
    res = DeleteSelection(nsIEditor::eNone, nsIEditor::eStrip);
    NS_ENSURE_SUCCESS(res, res);

    MOZ_ASSERT(selection->GetAnchorFocusRange() &&
               selection->GetAnchorFocusRange()->Collapsed(),
               "Selection not collapsed after delete");
  }

  
  
  nsCOMPtr<nsINode> node = selection->GetAnchorNode();
  MOZ_ASSERT(node, "Selection has no ranges in it");

  if (node && node->IsNodeOfType(nsINode::eDATA_NODE)) {
    NS_ASSERTION(node->GetParentNode(),
                 "It's impossible to insert into chardata with no parent -- "
                 "fix the caller");
    NS_ENSURE_STATE(node->GetParentNode());

    int32_t offset = selection->GetAnchorOffset();

    if (offset == 0) {
      res = selection->Collapse(node->GetParentNode(),
                                node->GetParentNode()->IndexOf(node));
      MOZ_ASSERT(NS_SUCCEEDED(res));
      NS_ENSURE_SUCCESS(res, res);
    } else if (offset == (int32_t)node->Length()) {
      res = selection->Collapse(node->GetParentNode(),
                                node->GetParentNode()->IndexOf(node) + 1);
      MOZ_ASSERT(NS_SUCCEEDED(res));
      NS_ENSURE_SUCCESS(res, res);
    } else {
      nsCOMPtr<nsIDOMNode> tmp;
      res = SplitNode(node->AsDOMNode(), offset, getter_AddRefs(tmp));
      NS_ENSURE_SUCCESS(res, res);
      res = selection->Collapse(node->GetParentNode(),
                                node->GetParentNode()->IndexOf(node));
      MOZ_ASSERT(NS_SUCCEEDED(res));
      NS_ENSURE_SUCCESS(res, res);
    }
  }
  return NS_OK;
}



void
nsEditor::DoAfterDoTransaction(nsITransaction *aTxn)
{
  bool isTransientTransaction;
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
    aTxn->GetIsTransient(&isTransientTransaction)));
  
  if (!isTransientTransaction)
  {
    
    
    
    
    int32_t modCount;
    GetModificationCount(&modCount);
    if (modCount < 0)
      modCount = -modCount;
        
    
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
      IncrementModificationCount(1)));
  }
}


void
nsEditor::DoAfterUndoTransaction()
{
  
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
    IncrementModificationCount(-1)));
}

void
nsEditor::DoAfterRedoTransaction()
{
  
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
    IncrementModificationCount(1)));
}

NS_IMETHODIMP 
nsEditor::CreateTxnForSetAttribute(nsIDOMElement *aElement, 
                                   const nsAString& aAttribute, 
                                   const nsAString& aValue,
                                   ChangeAttributeTxn ** aTxn)
{
  NS_ENSURE_TRUE(aElement, NS_ERROR_NULL_POINTER);

  nsRefPtr<ChangeAttributeTxn> txn = new ChangeAttributeTxn();

  nsresult rv = txn->Init(this, aElement, aAttribute, aValue, false);
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

  nsresult rv = txn->Init(this, aElement, aAttribute, EmptyString(), true);
  if (NS_SUCCEEDED(rv))
  {
    txn.forget(aTxn);
  }

  return rv;
}


NS_IMETHODIMP nsEditor::CreateTxnForCreateElement(const nsAString& aTag,
                                                  nsIDOMNode     *aParent,
                                                  int32_t         aPosition,
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
                                                  int32_t      aPosition,
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

nsresult
nsEditor::CreateTxnForDeleteNode(nsINode* aNode, DeleteNodeTxn** aTxn)
{
  NS_ENSURE_TRUE(aNode, NS_ERROR_NULL_POINTER);

  nsRefPtr<DeleteNodeTxn> txn = new DeleteNodeTxn();

  nsresult res = txn->Init(this, aNode, &mRangeUpdater);
  NS_ENSURE_SUCCESS(res, res);

  txn.forget(aTxn);
  return NS_OK;
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


nsresult
nsEditor::CreateTxnForDeleteSelection(EDirection aAction,
                                      EditAggregateTxn** aTxn,
                                      nsINode** aNode,
                                      int32_t* aOffset,
                                      int32_t* aLength)
{
  MOZ_ASSERT(aTxn);
  *aTxn = nullptr;

  nsRefPtr<Selection> selection = GetSelection();
  NS_ENSURE_STATE(selection);

  
  if (selection->Collapsed() && aAction == eNone) {
    return NS_OK;
  }

  
  nsRefPtr<EditAggregateTxn> aggTxn = new EditAggregateTxn();

  nsSelectionIterator iter(selection);
  for (iter.First(); iter.IsDone() != NS_OK; iter.Next()) {
    nsRefPtr<nsRange> range = iter.CurrentItem();
    NS_ENSURE_STATE(range);

    
    
    if (!range->Collapsed()) {
      nsRefPtr<DeleteRangeTxn> txn = new DeleteRangeTxn();
      txn->Init(this, range, &mRangeUpdater);
      aggTxn->AppendChild(txn);
    } else if (aAction != eNone) {
      
      
      nsresult res = CreateTxnForDeleteInsertionPoint(range, aAction, aggTxn,
                                                      aNode, aOffset, aLength);
      NS_ENSURE_SUCCESS(res, res);
    }
  }

  aggTxn.forget(aTxn);

  return NS_OK;
}

nsresult
nsEditor::CreateTxnForDeleteCharacter(nsIDOMCharacterData* aData,
                                      uint32_t             aOffset,
                                      EDirection           aDirection,
                                      DeleteTextTxn**      aTxn)
{
  NS_ASSERTION(aDirection == eNext || aDirection == ePrevious,
               "invalid direction");
  nsAutoString data;
  aData->GetData(data);
  NS_ASSERTION(data.Length(), "Trying to delete from a zero-length node");
  NS_ENSURE_STATE(data.Length());

  uint32_t segOffset = aOffset, segLength = 1;
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



nsresult
nsEditor::CreateTxnForDeleteInsertionPoint(nsRange*          aRange,
                                           EDirection        aAction,
                                           EditAggregateTxn* aTxn,
                                           nsINode**         aNode,
                                           int32_t*          aOffset,
                                           int32_t*          aLength)
{
  MOZ_ASSERT(aAction != eNone);

  nsresult res;

  
  nsCOMPtr<nsINode> node = aRange->GetStartParent();
  NS_ENSURE_STATE(node);

  int32_t offset = aRange->StartOffset();

  
  
  nsCOMPtr<nsIDOMCharacterData> nodeAsCharData = do_QueryInterface(node);

  uint32_t count = node->Length();

  bool isFirst = (0 == offset);
  bool isLast  = (count == (uint32_t)offset);

  
  

  
  
  if (aAction == ePrevious && isFirst) {
    
    
    nsCOMPtr<nsIContent> priorNode = GetPriorNode(node, true);
    NS_ENSURE_STATE(priorNode);

    
    
    nsCOMPtr<nsIDOMCharacterData> priorNodeAsCharData =
      do_QueryInterface(priorNode);
    if (priorNodeAsCharData) {
      uint32_t length = priorNode->Length();
      
      NS_ENSURE_STATE(length);
      nsRefPtr<DeleteTextTxn> txn;
      res = CreateTxnForDeleteCharacter(priorNodeAsCharData, length,
                                        ePrevious, getter_AddRefs(txn));
      NS_ENSURE_SUCCESS(res, res);

      *aOffset = txn->GetOffset();
      *aLength = txn->GetNumCharsToDelete();
      aTxn->AppendChild(txn);
    } else {
      
      nsRefPtr<DeleteNodeTxn> txn;
      res = CreateTxnForDeleteNode(priorNode, getter_AddRefs(txn));
      NS_ENSURE_SUCCESS(res, res);

      aTxn->AppendChild(txn);
    }

    NS_ADDREF(*aNode = priorNode);

    return NS_OK;
  }

  if (aAction == eNext && isLast) {
    
    
    nsCOMPtr<nsIContent> nextNode = GetNextNode(node, true);
    NS_ENSURE_STATE(nextNode);

    
    
    nsCOMPtr<nsIDOMCharacterData> nextNodeAsCharData =
      do_QueryInterface(nextNode);
    if (nextNodeAsCharData) {
      uint32_t length = nextNode->Length();
      
      NS_ENSURE_STATE(length);
      nsRefPtr<DeleteTextTxn> txn;
      res = CreateTxnForDeleteCharacter(nextNodeAsCharData, 0, eNext,
                                        getter_AddRefs(txn));
      NS_ENSURE_SUCCESS(res, res);

      *aOffset = txn->GetOffset();
      *aLength = txn->GetNumCharsToDelete();
      aTxn->AppendChild(txn);
    } else {
      
      nsRefPtr<DeleteNodeTxn> txn;
      res = CreateTxnForDeleteNode(nextNode, getter_AddRefs(txn));
      NS_ENSURE_SUCCESS(res, res);
      aTxn->AppendChild(txn);
    }

    NS_ADDREF(*aNode = nextNode);

    return NS_OK;
  }

  if (nodeAsCharData) {
    
    nsRefPtr<DeleteTextTxn> txn;
    res = CreateTxnForDeleteCharacter(nodeAsCharData, offset, aAction,
                                      getter_AddRefs(txn));
    NS_ENSURE_SUCCESS(res, res);

    aTxn->AppendChild(txn);
    NS_ADDREF(*aNode = node);
    *aOffset = txn->GetOffset();
    *aLength = txn->GetNumCharsToDelete();
  } else {
    
    
    nsCOMPtr<nsINode> selectedNode;
    if (aAction == ePrevious) {
      selectedNode = GetPriorNode(node, offset, true);
    } else if (aAction == eNext) {
      selectedNode = GetNextNode(node, offset, true);
    }

    while (selectedNode &&
           selectedNode->IsNodeOfType(nsINode::eDATA_NODE) &&
           !selectedNode->Length()) {
      
      if (aAction == ePrevious) {
        selectedNode = GetPriorNode(selectedNode, true);
      } else if (aAction == eNext) {
        selectedNode = GetNextNode(selectedNode, true);
      }
    }
    NS_ENSURE_STATE(selectedNode);

    nsCOMPtr<nsIDOMCharacterData> selectedNodeAsCharData =
      do_QueryInterface(selectedNode);
    if (selectedNodeAsCharData) {
      
      uint32_t position = 0;
      if (aAction == ePrevious) {
        position = selectedNode->Length();
      }
      nsRefPtr<DeleteTextTxn> delTextTxn;
      res = CreateTxnForDeleteCharacter(selectedNodeAsCharData, position,
                                        aAction, getter_AddRefs(delTextTxn));
      NS_ENSURE_SUCCESS(res, res);
      NS_ENSURE_TRUE(delTextTxn, NS_ERROR_NULL_POINTER);

      aTxn->AppendChild(delTextTxn);
      *aOffset = delTextTxn->GetOffset();
      *aLength = delTextTxn->GetNumCharsToDelete();
    } else {
      nsRefPtr<DeleteNodeTxn> delElementTxn;
      res = CreateTxnForDeleteNode(selectedNode, getter_AddRefs(delElementTxn));
      NS_ENSURE_SUCCESS(res, res);
      NS_ENSURE_TRUE(delElementTxn, NS_ERROR_NULL_POINTER);

      aTxn->AppendChild(delElementTxn);
    }

    NS_ADDREF(*aNode = selectedNode);
  }

  return NS_OK;
}

nsresult 
nsEditor::CreateRange(nsIDOMNode *aStartParent, int32_t aStartOffset,
                      nsIDOMNode *aEndParent, int32_t aEndOffset,
                      nsIDOMRange **aRange)
{
  return nsRange::CreateRange(aStartParent, aStartOffset, aEndParent,
                              aEndOffset, aRange);
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
  
  int32_t offset = GetChildOffset(aNode, parentNode);
  
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
nsEditor::CreateHTMLContent(const nsAString& aTag, dom::Element** aContent)
{
  nsCOMPtr<nsIDocument> doc = GetDocument();
  NS_ENSURE_TRUE(doc, NS_ERROR_FAILURE);

  
  
  if (aTag.IsEmpty()) {
    NS_ERROR("Don't pass an empty tag to nsEditor::CreateHTMLContent, "
             "check caller.");
    return NS_ERROR_FAILURE;
  }

  return doc->CreateElem(aTag, nullptr, kNameSpaceID_XHTML,
                         reinterpret_cast<nsIContent**>(aContent));
}

nsresult
nsEditor::SetAttributeOrEquivalent(nsIDOMElement * aElement,
                                   const nsAString & aAttribute,
                                   const nsAString & aValue,
                                   bool aSuppressTransaction)
{
  return SetAttribute(aElement, aAttribute, aValue);
}

nsresult
nsEditor::RemoveAttributeOrEquivalent(nsIDOMElement * aElement,
                                      const nsAString & aAttribute,
                                      bool aSuppressTransaction)
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
    case nsIDOMKeyEvent::DOM_VK_WIN:
    case nsIDOMKeyEvent::DOM_VK_SHIFT:
    case nsIDOMKeyEvent::DOM_VK_CONTROL:
    case nsIDOMKeyEvent::DOM_VK_ALT:
      aKeyEvent->PreventDefault(); 
      return NS_OK;
    case nsIDOMKeyEvent::DOM_VK_BACK_SPACE:
      if (nativeKeyEvent->IsControl() || nativeKeyEvent->IsAlt() ||
          nativeKeyEvent->IsMeta() || nativeKeyEvent->IsOS()) {
        return NS_OK;
      }
      DeleteSelection(nsIEditor::ePrevious, nsIEditor::eStrip);
      aKeyEvent->PreventDefault(); 
      return NS_OK;
    case nsIDOMKeyEvent::DOM_VK_DELETE:
      
      
      
      if (nativeKeyEvent->IsShift() || nativeKeyEvent->IsControl() ||
          nativeKeyEvent->IsAlt() || nativeKeyEvent->IsMeta() ||
          nativeKeyEvent->IsOS()) {
        return NS_OK;
      }
      DeleteSelection(nsIEditor::eNext, nsIEditor::eStrip);
      aKeyEvent->PreventDefault(); 
      return NS_OK; 
  }
  return NS_OK;
}

nsresult
nsEditor::HandleInlineSpellCheck(EditAction action,
                                   nsISelection *aSelection,
                                   nsIDOMNode *previousSelectedNode,
                                   int32_t previousSelectedOffset,
                                   nsIDOMNode *aStartNode,
                                   int32_t aStartOffset,
                                   nsIDOMNode *aEndNode,
                                   int32_t aEndOffset)
{
  
  return mInlineSpellChecker ? mInlineSpellChecker->SpellCheckAfterEditorChange(
                                 (int32_t)action, aSelection,
                                 previousSelectedNode, previousSelectedOffset,
                                 aStartNode, aStartOffset, aEndNode,
                                 aEndOffset)
                             : NS_OK;
}

already_AddRefed<nsIContent>
nsEditor::FindSelectionRoot(nsINode *aNode)
{
  nsCOMPtr<nsIContent> rootContent = GetRoot();
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

  bool isTargetDoc =
    targetNode->NodeType() == nsIDOMNode::DOCUMENT_NODE &&
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
  caret->SetIgnoreUserModify(false);
  caret->SetCaretDOMSelection(selection);
  selCon->SetCaretReadOnly(IsReadonly());
  selCon->SetCaretEnabled(true);

  
  selCon->SetDisplaySelection(nsISelectionController::SELECTION_ON);
  selCon->SetSelectionFlags(nsISelectionDisplay::DISPLAY_ALL);
  selCon->RepaintSelection(nsISelectionController::SELECTION_NORMAL);
  
  
  
  
  
  if (selectionRootContent->GetParent()) {
    selectionPrivate->SetAncestorLimiter(selectionRootContent);
  } else {
    selectionPrivate->SetAncestorLimiter(nullptr);
  }

  
  if (isTargetDoc) {
    int32_t rangeCount;
    selection->GetRangeCount(&rangeCount);
    if (rangeCount == 0) {
      BeginningOfDocument();
    }
  }

  return NS_OK;
}

dom::Element *
nsEditor::GetRoot()
{
  if (!mRootElement)
  {
    nsCOMPtr<nsIDOMElement> root;

    
    GetRootElement(getter_AddRefs(root));
  }

  return mRootElement;
}

dom::Element*
nsEditor::GetEditorRoot()
{
  return GetRoot();
}

nsresult
nsEditor::DetermineCurrentDirection()
{
  
  dom::Element *rootElement = GetRoot();

  
  
  if (!(mFlags & (nsIPlaintextEditor::eEditorLeftToRight |
                  nsIPlaintextEditor::eEditorRightToLeft))) {

    nsIFrame* frame = rootElement->GetPrimaryFrame();
    NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);

    
    
    if (frame->GetStyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL) {
      mFlags |= nsIPlaintextEditor::eEditorRightToLeft;
    } else {
      mFlags |= nsIPlaintextEditor::eEditorLeftToRight;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsEditor::SwitchTextDirection()
{
  
  dom::Element *rootElement = GetRoot();
  nsresult rv = DetermineCurrentDirection();
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (mFlags & nsIPlaintextEditor::eEditorRightToLeft) {
    NS_ASSERTION(!(mFlags & nsIPlaintextEditor::eEditorLeftToRight),
                 "Unexpected mutually exclusive flag");
    mFlags &= ~nsIPlaintextEditor::eEditorRightToLeft;
    mFlags |= nsIPlaintextEditor::eEditorLeftToRight;
    rv = rootElement->SetAttr(kNameSpaceID_None, nsGkAtoms::dir, NS_LITERAL_STRING("ltr"), true);
  } else if (mFlags & nsIPlaintextEditor::eEditorLeftToRight) {
    NS_ASSERTION(!(mFlags & nsIPlaintextEditor::eEditorRightToLeft),
                 "Unexpected mutually exclusive flag");
    mFlags |= nsIPlaintextEditor::eEditorRightToLeft;
    mFlags &= ~nsIPlaintextEditor::eEditorLeftToRight;
    rv = rootElement->SetAttr(kNameSpaceID_None, nsGkAtoms::dir, NS_LITERAL_STRING("rtl"), true);
  }

  return rv;
}

void
nsEditor::SwitchTextDirectionTo(uint32_t aDirection)
{
  
  dom::Element *rootElement = GetRoot();
  nsresult rv = DetermineCurrentDirection();
  NS_ENSURE_SUCCESS_VOID(rv);

  
  if (aDirection == nsIPlaintextEditor::eEditorLeftToRight &&
      (mFlags & nsIPlaintextEditor::eEditorRightToLeft)) {
    NS_ASSERTION(!(mFlags & nsIPlaintextEditor::eEditorLeftToRight),
                 "Unexpected mutually exclusive flag");
    mFlags &= ~nsIPlaintextEditor::eEditorRightToLeft;
    mFlags |= nsIPlaintextEditor::eEditorLeftToRight;
    rootElement->SetAttr(kNameSpaceID_None, nsGkAtoms::dir, NS_LITERAL_STRING("ltr"), true);
  } else if (aDirection == nsIPlaintextEditor::eEditorRightToLeft &&
             (mFlags & nsIPlaintextEditor::eEditorLeftToRight)) {
    NS_ASSERTION(!(mFlags & nsIPlaintextEditor::eEditorRightToLeft),
                 "Unexpected mutually exclusive flag");
    mFlags |= nsIPlaintextEditor::eEditorRightToLeft;
    mFlags &= ~nsIPlaintextEditor::eEditorLeftToRight;
    rootElement->SetAttr(kNameSpaceID_None, nsGkAtoms::dir, NS_LITERAL_STRING("rtl"), true);
  }
}

#if DEBUG_JOE
void
nsEditor::DumpNode(nsIDOMNode *aNode, int32_t indent)
{
  int32_t i;
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
    uint32_t numChildren;
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
    nsAutoCString cstr;
    LossyCopyUTF16toASCII(str, cstr);
    cstr.ReplaceChar('\n', ' ');
    printf("<textnode> %s\n", cstr.get());
  }
}
#endif

bool
nsEditor::IsModifiableNode(nsIDOMNode *aNode)
{
  return true;
}

bool
nsEditor::IsModifiableNode(nsINode *aNode)
{
  return true;
}

nsKeyEvent*
nsEditor::GetNativeKeyEvent(nsIDOMKeyEvent* aDOMKeyEvent)
{
  NS_ENSURE_TRUE(aDOMKeyEvent, nullptr);
  nsEvent* nativeEvent = aDOMKeyEvent->GetInternalNSEvent();
  NS_ENSURE_TRUE(nativeEvent, nullptr);
  NS_ENSURE_TRUE(nativeEvent->eventStructType == NS_KEY_EVENT, nullptr);
  return static_cast<nsKeyEvent*>(nativeEvent);
}

already_AddRefed<nsIContent>
nsEditor::GetFocusedContent()
{
  nsCOMPtr<nsIDOMEventTarget> piTarget = GetDOMEventTarget();
  if (!piTarget) {
    return nullptr;
  }

  nsFocusManager* fm = nsFocusManager::GetFocusManager();
  NS_ENSURE_TRUE(fm, nullptr);

  nsCOMPtr<nsIContent> content = fm->GetFocusedContent();
  return SameCOMIdentity(content, piTarget) ? content.forget() : nullptr;
}

already_AddRefed<nsIContent>
nsEditor::GetFocusedContentForIME()
{
  return GetFocusedContent();
}

bool
nsEditor::IsActiveInDOMWindow()
{
  nsCOMPtr<nsIDOMEventTarget> piTarget = GetDOMEventTarget();
  if (!piTarget) {
    return false;
  }

  nsFocusManager* fm = nsFocusManager::GetFocusManager();
  NS_ENSURE_TRUE(fm, false);

  nsCOMPtr<nsIDocument> doc = do_QueryReferent(mDocWeak);
  nsPIDOMWindow* ourWindow = doc->GetWindow();
  nsCOMPtr<nsPIDOMWindow> win;
  nsIContent* content =
    nsFocusManager::GetFocusedDescendant(ourWindow, false,
                                         getter_AddRefs(win));
  return SameCOMIdentity(content, piTarget);
}

bool
nsEditor::IsAcceptableInputEvent(nsIDOMEvent* aEvent)
{
  
  NS_ENSURE_TRUE(aEvent, false);

  
  
  nsCOMPtr<nsIDOMMouseEvent> mouseEvent = do_QueryInterface(aEvent);
  if (mouseEvent) {
    nsCOMPtr<nsIContent> focusedContent = GetFocusedContent();
    if (!focusedContent) {
      return false;
    }
  }

  bool isTrusted;
  nsresult rv = aEvent->GetIsTrusted(&isTrusted);
  NS_ENSURE_SUCCESS(rv, false);
  if (isTrusted) {
    return true;
  }

  
  
  if (mouseEvent) {
    return false;
  }

  
  
  return IsActiveInDOMWindow();
}

void
nsEditor::OnFocus(nsIDOMEventTarget* aFocusEventTarget)
{
  InitializeSelection(aFocusEventTarget);
  if (mInlineSpellChecker) {
    mInlineSpellChecker->UpdateCurrentDictionary();
  }
}

NS_IMETHODIMP
nsEditor::GetSuppressDispatchingInputEvent(bool *aSuppressed)
{
  NS_ENSURE_ARG_POINTER(aSuppressed);
  *aSuppressed = !mDispatchInputEvent;
  return NS_OK;
}

NS_IMETHODIMP
nsEditor::SetSuppressDispatchingInputEvent(bool aSuppress)
{
  mDispatchInputEvent = !aSuppress;
  return NS_OK;
}

nsEditor::HandlingTrustedAction::HandlingTrustedAction(nsEditor* aSelf,
                                                       nsIDOMEvent* aEvent)
{
  MOZ_ASSERT(aEvent);

  bool isTrusted = false;
  aEvent->GetIsTrusted(&isTrusted);
  Init(aSelf, isTrusted);
}
