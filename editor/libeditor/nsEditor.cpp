




#include "nsEditor.h"

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
#include "InsertNodeTxn.h"              
#include "InsertTextTxn.h"              
#include "JoinNodeTxn.h"                
#include "PlaceholderTxn.h"             
#include "SplitNodeTxn.h"               
#include "mozFlushType.h"               
#include "mozISpellCheckingEngine.h"
#include "mozInlineSpellChecker.h"      
#include "mozilla/IMEStateManager.h"    
#include "mozilla/Preferences.h"        
#include "mozilla/dom/Selection.h"      
#include "mozilla/Services.h"           
#include "mozilla/TextComposition.h"    
#include "mozilla/TextEvents.h"
#include "mozilla/dom/Element.h"        
#include "mozilla/dom/Text.h"
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
#include "nsEditorEventListener.h"      
#include "nsEditorUtils.h"              
#include "nsError.h"                    
#include "nsFocusManager.h"             
#include "nsFrameSelection.h"           
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
#include "nsIDOMMozNamedAttrMap.h"      
#include "nsIDOMMouseEvent.h"           
#include "nsIDOMNode.h"                 
#include "nsIDOMNodeList.h"             
#include "nsIDOMText.h"                 
#include "nsIDocument.h"                
#include "nsIDocumentStateListener.h"   
#include "nsIEditActionListener.h"      
#include "nsIEditorObserver.h"          
#include "nsIEditorSpellCheck.h"        
#include "nsIFrame.h"                   
#include "nsIHTMLDocument.h"            
#include "nsIInlineSpellChecker.h"      
#include "nsNameSpaceManager.h"        
#include "nsINode.h"                    
#include "nsIObserverService.h"         
#include "nsIPlaintextEditor.h"         
#include "nsIPresShell.h"               
#include "nsISelectionController.h"     
#include "nsISelectionDisplay.h"        
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
#include "nsTextNode.h"                 
#include "nsThreadUtils.h"              
#include "nsTransactionManager.h"       
#include "prtime.h"                     

class nsIOutputStream;
class nsIParserService;
class nsITransferable;

#ifdef DEBUG
#include "nsIDOMHTMLDocument.h"         
#endif

using namespace mozilla;
using namespace mozilla::dom;
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
,  mIMETextOffset(0)
,  mDirection(eNone)
,  mDocDirtyState(-1)
,  mSpellcheckCheckboxState(eTriUnset)
,  mShouldTxnSetSelection(true)
,  mDidPreDestroy(false)
,  mDidPostCreate(false)
,  mDispatchInputEvent(true)
,  mIsInEditAction(false)
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
nsEditor::Init(nsIDOMDocument *aDoc, nsIContent *aRoot,
               nsISelectionController *aSelCon, uint32_t aFlags,
               const nsAString& aValue)
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
    IMEStateManager::UpdateIMEState(newState, content, this);
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
  if (mComposition) {
    mComposition->EndHandlingComposition(this);
    mComposition = nullptr;
  }
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

  
  nsCOMPtr<nsIContent> content = GetExposedRoot();
  if (!content) {
    return false;
  }

  nsCOMPtr<nsIDOMHTMLElement> element = do_QueryInterface(content);
  if (!element) {
    return false;
  }

  if (!IsPlaintextEditor()) {
    
    
    
    nsCOMPtr<nsIHTMLDocument> doc = do_QueryInterface(content->GetCurrentDoc());
    return doc && doc->IsEditingOn();
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
      IMEStateManager::UpdateIMEState(newState, content, this);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsEditor::GetIsSelectionEditable(bool *aIsSelectionEditable)
{
  NS_ENSURE_ARG_POINTER(aIsSelectionEditable);

  
  nsRefPtr<Selection> selection = GetSelection();
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);

  
  
  nsCOMPtr<nsINode> anchorNode = selection->GetAnchorNode();
  *aIsSelectionEditable = anchorNode && IsEditable(anchorNode);

  return NS_OK;
}

NS_IMETHODIMP
nsEditor::GetIsDocumentEditable(bool *aIsDocumentEditable)
{
  NS_ENSURE_ARG_POINTER(aIsDocumentEditable);
  nsCOMPtr<nsIDocument> doc = GetDocument();
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
  *aDoc = GetDOMDocument().take();
  return *aDoc ? NS_OK : NS_ERROR_NOT_INITIALIZED;
}

already_AddRefed<nsIPresShell>
nsEditor::GetPresShell()
{
  NS_PRECONDITION(mDocWeak, "bad state, null mDocWeak");
  nsCOMPtr<nsIDocument> doc = do_QueryReferent(mDocWeak);
  NS_ENSURE_TRUE(doc, nullptr);
  nsCOMPtr<nsIPresShell> ps = doc->GetShell();
  return ps.forget();
}

already_AddRefed<nsIWidget>
nsEditor::GetWidget()
{
  nsCOMPtr<nsIPresShell> ps = GetPresShell();
  NS_ENSURE_TRUE(ps, nullptr);
  nsPresContext* pc = ps->GetPresContext();
  NS_ENSURE_TRUE(pc, nullptr);
  nsCOMPtr<nsIWidget> widget = pc->GetRootWidget();
  NS_ENSURE_TRUE(widget.get(), nullptr);
  return widget.forget();
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
nsEditor::GetSelection(nsISelection** aSelection)
{
  return GetSelection(nsISelectionController::SELECTION_NORMAL, aSelection);
}

nsresult
nsEditor::GetSelection(int16_t aSelectionType, nsISelection** aSelection)
{
  NS_ENSURE_TRUE(aSelection, NS_ERROR_NULL_POINTER);
  *aSelection = nullptr;
  nsCOMPtr<nsISelectionController> selcon;
  GetSelectionController(getter_AddRefs(selcon));
  NS_ENSURE_TRUE(selcon, NS_ERROR_NOT_INITIALIZED);
  return selcon->GetSelection(aSelectionType, aSelection);  
}

Selection*
nsEditor::GetSelection(int16_t aSelectionType)
{
  nsCOMPtr<nsISelection> sel;
  nsresult res = GetSelection(aSelectionType, getter_AddRefs(sel));
  NS_ENSURE_SUCCESS(res, nullptr);

  return static_cast<Selection*>(sel.get());
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
  
  *aTxnManager = nullptr;
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
    mTxnMgr->BeginBatch(nullptr);
  }

  return NS_OK;
}

NS_IMETHODIMP 
nsEditor::EndTransaction()
{
  if (mTxnMgr) {
    mTxnMgr->EndBatch(false);
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
    NotifyEditorObservers(eNotifyEditorObserversOfBefore);
    
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
    nsRefPtr<Selection> selection = GetSelection();

    
    
    
    
    
    
    if (selection) {
      selection->SetCanCacheFrameOffset(true);
    }

    {
      
      
      nsRefPtr<nsCaret> caret;
      nsCOMPtr<nsIPresShell> presShell = GetPresShell();

      if (presShell)
        caret = presShell->GetCaret();

      
      EndUpdateViewBatch();
      

      
      
      ScrollSelectionIntoView(false);
    }

    
    if (selection) {
      selection->SetCanCacheFrameOffset(false);
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
      
      
      if (!mComposition) {
        NotifyEditorObservers(eNotifyEditorObserversOfEnd);
      }
    } else {
      NotifyEditorObservers(eNotifyEditorObserversOfCancel);
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

  nsRefPtr<Selection> selection = GetSelection();
  NS_ENSURE_TRUE(selection, NS_ERROR_NOT_INITIALIZED);
  return SelectEntireDocument(selection);
}

NS_IMETHODIMP nsEditor::BeginningOfDocument()
{
  if (!mDocWeak) { return NS_ERROR_NOT_INITIALIZED; }

  
  nsRefPtr<Selection> selection = GetSelection();
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

  
  nsRefPtr<Selection> selection = GetSelection(); 
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
nsEditor::CanDelete(bool *aCanDelete)
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
nsEditor::SetAttribute(nsIDOMElement* aElement, const nsAString& aAttribute,
                       const nsAString& aValue)
{
  nsCOMPtr<Element> element = do_QueryInterface(aElement);
  NS_ENSURE_TRUE(element, NS_ERROR_NULL_POINTER);
  nsCOMPtr<nsIAtom> attribute = do_GetAtom(aAttribute);

  nsRefPtr<ChangeAttributeTxn> txn =
    CreateTxnForSetAttribute(*element, *attribute, aValue);
  return DoTransaction(txn);
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
nsEditor::RemoveAttribute(nsIDOMElement* aElement, const nsAString& aAttribute)
{
  nsCOMPtr<Element> element = do_QueryInterface(aElement);
  NS_ENSURE_TRUE(element, NS_ERROR_NULL_POINTER);
  nsCOMPtr<nsIAtom> attribute = do_GetAtom(aAttribute);

  nsRefPtr<ChangeAttributeTxn> txn =
    CreateTxnForRemoveAttribute(*element, *attribute);
  return DoTransaction(txn);
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
    element->SetAttr(kNameSpaceID_None, nsGkAtoms::mozdirty,
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
                                const char16_t *aData)
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

NS_IMETHODIMP
nsEditor::CreateNode(const nsAString& aTag,
                     nsIDOMNode* aParent,
                     int32_t aPosition,
                     nsIDOMNode** aNewNode)
{
  nsCOMPtr<nsIAtom> tag = do_GetAtom(aTag);
  nsCOMPtr<nsINode> parent = do_QueryInterface(aParent);
  NS_ENSURE_STATE(parent);
  *aNewNode = GetAsDOMNode(CreateNode(tag, parent, aPosition).take());
  NS_ENSURE_STATE(*aNewNode);
  return NS_OK;
}

already_AddRefed<Element>
nsEditor::CreateNode(nsIAtom* aTag,
                     nsINode* aParent,
                     int32_t aPosition)
{
  MOZ_ASSERT(aTag && aParent);

  nsAutoRules beginRulesSniffing(this, EditAction::createNode, nsIEditor::eNext);

  for (auto& listener : mActionListeners) {
    listener->WillCreateNode(nsDependentAtomString(aTag),
                             GetAsDOMNode(aParent), aPosition);
  }

  nsCOMPtr<Element> ret;

  nsRefPtr<CreateElementTxn> txn =
    CreateTxnForCreateElement(*aTag, *aParent, aPosition);
  nsresult res = DoTransaction(txn);
  if (NS_SUCCEEDED(res)) {
    ret = txn->GetNewNode();
    MOZ_ASSERT(ret);
  }

  mRangeUpdater.SelAdjCreateNode(aParent, aPosition);

  for (auto& listener : mActionListeners) {
    listener->DidCreateNode(nsDependentAtomString(aTag), GetAsDOMNode(ret),
                            GetAsDOMNode(aParent), aPosition, res);
  }

  return ret.forget();
}


NS_IMETHODIMP
nsEditor::InsertNode(nsIDOMNode* aNode, nsIDOMNode* aParent, int32_t aPosition)
{
  nsCOMPtr<nsIContent> node = do_QueryInterface(aNode);
  nsCOMPtr<nsINode> parent = do_QueryInterface(aParent);
  NS_ENSURE_TRUE(node && parent, NS_ERROR_NULL_POINTER);

  return InsertNode(*node, *parent, aPosition);
}

nsresult
nsEditor::InsertNode(nsIContent& aNode, nsINode& aParent, int32_t aPosition)
{
  nsAutoRules beginRulesSniffing(this, EditAction::insertNode, nsIEditor::eNext);

  for (auto& listener : mActionListeners) {
    listener->WillInsertNode(aNode.AsDOMNode(), aParent.AsDOMNode(),
                             aPosition);
  }

  nsRefPtr<InsertNodeTxn> txn = CreateTxnForInsertNode(aNode, aParent,
                                                       aPosition);
  nsresult res = DoTransaction(txn);

  mRangeUpdater.SelAdjInsertNode(aParent.AsDOMNode(), aPosition);

  for (auto& listener : mActionListeners) {
    listener->DidInsertNode(aNode.AsDOMNode(), aParent.AsDOMNode(), aPosition,
                            res);
  }

  return res;
}


NS_IMETHODIMP
nsEditor::SplitNode(nsIDOMNode* aNode,
                    int32_t aOffset,
                    nsIDOMNode** aNewLeftNode)
{
  nsCOMPtr<nsIContent> node = do_QueryInterface(aNode);
  NS_ENSURE_STATE(node);
  ErrorResult rv;
  nsCOMPtr<nsIContent> newNode = SplitNode(*node, aOffset, rv);
  *aNewLeftNode = GetAsDOMNode(newNode.forget().take());
  return rv.ErrorCode();
}

nsIContent*
nsEditor::SplitNode(nsIContent& aNode, int32_t aOffset, ErrorResult& aResult)
{
  nsAutoRules beginRulesSniffing(this, EditAction::splitNode,
                                 nsIEditor::eNext);

  for (auto& listener : mActionListeners) {
    listener->WillSplitNode(aNode.AsDOMNode(), aOffset);
  }

  nsRefPtr<SplitNodeTxn> txn = CreateTxnForSplitNode(aNode, aOffset);
  aResult = DoTransaction(txn);

  nsCOMPtr<nsIContent> newNode = aResult.Failed() ? nullptr
                                                  : txn->GetNewNode();

  mRangeUpdater.SelAdjSplitNode(aNode, aOffset, newNode);

  for (auto& listener : mActionListeners) {
    listener->DidSplitNode(aNode.AsDOMNode(), aOffset, GetAsDOMNode(newNode),
                           aResult.ErrorCode());
  }

  return newNode;
}


NS_IMETHODIMP
nsEditor::JoinNodes(nsIDOMNode* aLeftNode,
                    nsIDOMNode* aRightNode,
                    nsIDOMNode*)
{
  nsCOMPtr<nsINode> leftNode = do_QueryInterface(aLeftNode);
  nsCOMPtr<nsINode> rightNode = do_QueryInterface(aRightNode);
  NS_ENSURE_STATE(leftNode && rightNode && leftNode->GetParentNode());
  return JoinNodes(*leftNode, *rightNode);
}

nsresult
nsEditor::JoinNodes(nsINode& aLeftNode, nsINode& aRightNode)
{
  nsCOMPtr<nsINode> parent = aLeftNode.GetParentNode();
  MOZ_ASSERT(parent);

  nsAutoRules beginRulesSniffing(this, EditAction::joinNode,
                                 nsIEditor::ePrevious);

  
  
  int32_t offset = parent->IndexOf(&aRightNode);
  
  uint32_t oldLeftNodeLen = aLeftNode.Length();

  for (auto& listener : mActionListeners) {
    listener->WillJoinNodes(aLeftNode.AsDOMNode(), aRightNode.AsDOMNode(),
                            parent->AsDOMNode());
  }

  nsresult result;
  nsRefPtr<JoinNodeTxn> txn = CreateTxnForJoinNode(aLeftNode, aRightNode);
  if (txn)  {
    result = DoTransaction(txn);
  }

  mRangeUpdater.SelAdjJoinNodes(aLeftNode, aRightNode, *parent, offset,
                                (int32_t)oldLeftNodeLen);

  for (auto& listener : mActionListeners) {
    listener->DidJoinNodes(aLeftNode.AsDOMNode(), aRightNode.AsDOMNode(),
                           parent->AsDOMNode(), result);
  }

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

  
  for (auto& listener : mActionListeners) {
    listener->WillDeleteNode(aNode->AsDOMNode());
  }

  nsRefPtr<DeleteNodeTxn> txn;
  nsresult res = CreateTxnForDeleteNode(aNode, getter_AddRefs(txn));
  if (NS_SUCCEEDED(res))  {
    res = DoTransaction(txn);
  }

  for (auto& listener : mActionListeners) {
    listener->DidDeleteNode(aNode->AsDOMNode(), res);
  }

  NS_ENSURE_SUCCESS(res, res);
  return NS_OK;
}






already_AddRefed<Element>
nsEditor::ReplaceContainer(Element* aOldContainer,
                           nsIAtom* aNodeType,
                           nsIAtom* aAttribute,
                           const nsAString* aValue,
                           ECloneAttributes aCloneAttributes)
{
  MOZ_ASSERT(aOldContainer && aNodeType);

  nsCOMPtr<nsIContent> parent = aOldContainer->GetParent();
  NS_ENSURE_TRUE(parent, nullptr);

  int32_t offset = parent->IndexOf(aOldContainer);

  
  nsCOMPtr<Element> ret = CreateHTMLContent(aNodeType);
  NS_ENSURE_TRUE(ret, nullptr);

  
  nsresult res;
  if (aAttribute && aValue && aAttribute != nsGkAtoms::_empty) {
    res = ret->SetAttr(kNameSpaceID_None, aAttribute, *aValue, true);
    NS_ENSURE_SUCCESS(res, nullptr);
  }
  if (aCloneAttributes == eCloneAttributes) {
    CloneAttributes(ret, aOldContainer);
  }
  
  
  
  
  AutoReplaceContainerSelNotify selStateNotify(mRangeUpdater, aOldContainer,
                                               ret);
  {
    nsAutoTxnsConserveSelection conserveSelection(this);
    while (aOldContainer->HasChildren()) {
      nsCOMPtr<nsIContent> child = aOldContainer->GetFirstChild();

      res = DeleteNode(child);
      NS_ENSURE_SUCCESS(res, nullptr);

      res = InsertNode(*child, *ret, -1);
      NS_ENSURE_SUCCESS(res, nullptr);
    }
  }

  
  res = InsertNode(*ret, *parent, offset);
  NS_ENSURE_SUCCESS(res, nullptr);
  
  
  res = DeleteNode(aOldContainer);
  NS_ENSURE_SUCCESS(res, nullptr);

  return ret.forget();
}





nsresult
nsEditor::RemoveContainer(nsIContent* aNode)
{
  MOZ_ASSERT(aNode);

  nsCOMPtr<nsINode> parent = aNode->GetParentNode();
  NS_ENSURE_STATE(parent);

  int32_t offset = parent->IndexOf(aNode);

  
  uint32_t nodeOrigLen = aNode->GetChildCount();

  
  nsAutoRemoveContainerSelNotify selNotify(mRangeUpdater, aNode, parent,
                                           offset, nodeOrigLen);

  while (aNode->HasChildren()) {
    nsCOMPtr<nsIContent> child = aNode->GetLastChild();
    nsresult rv = DeleteNode(child);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = InsertNode(*child, *parent, offset);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return DeleteNode(aNode);
}








already_AddRefed<Element>
nsEditor::InsertContainerAbove(nsIContent* aNode,
                               nsIAtom* aNodeType,
                               nsIAtom* aAttribute,
                               const nsAString* aValue)
{
  MOZ_ASSERT(aNode && aNodeType);

  nsCOMPtr<nsIContent> parent = aNode->GetParent();
  NS_ENSURE_TRUE(parent, nullptr);
  int32_t offset = parent->IndexOf(aNode);

  
  nsCOMPtr<Element> newContent = CreateHTMLContent(aNodeType);
  NS_ENSURE_TRUE(newContent, nullptr);

  
  nsresult res;
  if (aAttribute && aValue && aAttribute != nsGkAtoms::_empty) {
    res = newContent->SetAttr(kNameSpaceID_None, aAttribute, *aValue, true);
    NS_ENSURE_SUCCESS(res, nullptr);
  }

  
  nsAutoInsertContainerSelNotify selNotify(mRangeUpdater);

  
  res = DeleteNode(aNode);
  NS_ENSURE_SUCCESS(res, nullptr);

  {
    nsAutoTxnsConserveSelection conserveSelection(this);
    res = InsertNode(*aNode, *newContent, 0);
    NS_ENSURE_SUCCESS(res, nullptr);
  }

  
  res = InsertNode(*newContent, *parent, offset);
  NS_ENSURE_SUCCESS(res, nullptr);

  return newContent.forget();
}



nsresult
nsEditor::MoveNode(nsIContent* aNode, nsINode* aParent, int32_t aOffset)
{
  MOZ_ASSERT(aNode);
  MOZ_ASSERT(aParent);
  MOZ_ASSERT(aOffset == -1 ||
             (0 <= aOffset &&
              AssertedCast<uint32_t>(aOffset) <= aParent->Length()));

  nsCOMPtr<nsINode> oldParent = aNode->GetParentNode();
  int32_t oldOffset = oldParent ? oldParent->IndexOf(aNode) : -1;

  if (aOffset == -1) {
    
    aOffset = AssertedCast<int32_t>(aParent->Length());
  }

  
  if (aParent == oldParent && aOffset == oldOffset) {
    return NS_OK;
  }

  
  nsAutoMoveNodeSelNotify selNotify(mRangeUpdater, oldParent, oldOffset,
                                    aParent, aOffset);

  
  if (aParent == oldParent && oldOffset < aOffset) {
    
    aOffset--;
  }

  
  nsCOMPtr<nsINode> kungFuDeathGrip = aNode;

  nsresult rv = DeleteNode(aNode);
  NS_ENSURE_SUCCESS(rv, rv);

  return InsertNode(*aNode, *aParent, aOffset);
}


NS_IMETHODIMP
nsEditor::AddEditorObserver(nsIEditorObserver *aObserver)
{
  
  
  
  NS_ENSURE_TRUE(aObserver, NS_ERROR_NULL_POINTER);

  
  if (!mEditorObservers.Contains(aObserver)) {
    mEditorObservers.AppendElement(*aObserver);
  }

  return NS_OK;
}


NS_IMETHODIMP
nsEditor::RemoveEditorObserver(nsIEditorObserver *aObserver)
{
  NS_ENSURE_TRUE(aObserver, NS_ERROR_FAILURE);

  mEditorObservers.RemoveElement(aObserver);

  return NS_OK;
}

class EditorInputEventDispatcher : public nsRunnable
{
public:
  EditorInputEventDispatcher(nsEditor* aEditor,
                             nsIContent* aTarget,
                             bool aIsComposing)
    : mEditor(aEditor)
    , mTarget(aTarget)
    , mIsComposing(aIsComposing)
  {
  }

  NS_IMETHOD Run()
  {
    
    

    if (!mTarget->IsInComposedDoc()) {
      return NS_OK;
    }

    nsCOMPtr<nsIPresShell> ps = mEditor->GetPresShell();
    if (!ps) {
      return NS_OK;
    }

    nsCOMPtr<nsIWidget> widget = mEditor->GetWidget();
    if (!widget) {
      return NS_OK;
    }

    
    
    InternalEditorInputEvent inputEvent(true, NS_EDITOR_INPUT, widget);
    inputEvent.time = static_cast<uint64_t>(PR_Now() / 1000);
    inputEvent.mIsComposing = mIsComposing;
    nsEventStatus status = nsEventStatus_eIgnore;
    nsresult rv =
      ps->HandleEventWithTarget(&inputEvent, nullptr, mTarget, &status);
    NS_ENSURE_SUCCESS(rv, NS_OK); 
    return NS_OK;
  }

private:
  nsRefPtr<nsEditor> mEditor;
  nsCOMPtr<nsIContent> mTarget;
  bool mIsComposing;
};

void
nsEditor::NotifyEditorObservers(NotificationForEditorObservers aNotification)
{
  switch (aNotification) {
    case eNotifyEditorObserversOfEnd:
      mIsInEditAction = false;
      for (auto& observer : mEditorObservers) {
        observer->EditAction();
      }

      if (!mDispatchInputEvent) {
        return;
      }

      FireInputEvent();
      break;
    case eNotifyEditorObserversOfBefore:
      mIsInEditAction = true;
      for (auto& observer : mEditorObservers) {
        observer->BeforeEditAction();
      }
      break;
    case eNotifyEditorObserversOfCancel:
      mIsInEditAction = false;
      for (auto& observer : mEditorObservers) {
        observer->CancelEditAction();
      }
      break;
    default:
      MOZ_CRASH("Handle all notifications here");
      break;
  }
}

void
nsEditor::FireInputEvent()
{
  
  
  
  

  nsCOMPtr<nsIContent> target = GetInputEventTargetContent();
  NS_ENSURE_TRUE_VOID(target);

  
  
  
  nsContentUtils::AddScriptRunner(
    new EditorInputEventDispatcher(this, target, !!GetComposition()));
}

NS_IMETHODIMP
nsEditor::AddEditActionListener(nsIEditActionListener *aListener)
{
  NS_ENSURE_TRUE(aListener, NS_ERROR_NULL_POINTER);

  
  if (!mActionListeners.Contains(aListener)) {
    mActionListeners.AppendElement(*aListener);
  }

  return NS_OK;
}


NS_IMETHODIMP
nsEditor::RemoveEditActionListener(nsIEditActionListener *aListener)
{
  NS_ENSURE_TRUE(aListener, NS_ERROR_FAILURE);

  mActionListeners.RemoveElement(aListener);

  return NS_OK;
}


NS_IMETHODIMP
nsEditor::AddDocumentStateListener(nsIDocumentStateListener *aListener)
{
  NS_ENSURE_TRUE(aListener, NS_ERROR_NULL_POINTER);

  if (!mDocStateListeners.Contains(aListener)) {
    mDocStateListeners.AppendElement(*aListener);
  }

  return NS_OK;
}


NS_IMETHODIMP
nsEditor::RemoveDocumentStateListener(nsIDocumentStateListener *aListener)
{
  NS_ENSURE_TRUE(aListener, NS_ERROR_NULL_POINTER);

  mDocStateListeners.RemoveElement(aListener);

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
nsEditor::RestorePreservedSelection(Selection* aSel)
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

void
nsEditor::EnsureComposition(mozilla::WidgetGUIEvent* aEvent)
{
  if (mComposition) {
    return;
  }
  
  
  mComposition = IMEStateManager::GetTextCompositionFor(aEvent);
  if (!mComposition) {
    MOZ_CRASH("IMEStateManager doesn't return proper composition");
  }
  mComposition->StartHandlingComposition(this);
}

nsresult
nsEditor::BeginIMEComposition(WidgetCompositionEvent* aCompositionEvent)
{
  MOZ_ASSERT(!mComposition, "There is composition already");
  EnsureComposition(aCompositionEvent);
  if (mPhonetic) {
    mPhonetic->Truncate(0);
  }
  return NS_OK;
}

void
nsEditor::EndIMEComposition()
{
  NS_ENSURE_TRUE_VOID(mComposition); 

  
  
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
  mComposition->EndHandlingComposition(this);
  mComposition = nullptr;

  
  NotifyEditorObservers(eNotifyEditorObserversOfEnd);
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

  return mComposition ?
    IMEStateManager::NotifyIME(REQUEST_TO_COMMIT_COMPOSITION, pc) : NS_OK;
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

  switch (frame->StyleUIReset()->mIMEMode) {
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
nsEditor::CloneAttributes(nsIDOMNode* aDest, nsIDOMNode* aSource)
{
  NS_ENSURE_TRUE(aDest && aSource, NS_ERROR_NULL_POINTER);

  nsCOMPtr<Element> dest = do_QueryInterface(aDest);
  nsCOMPtr<Element> source = do_QueryInterface(aSource);
  NS_ENSURE_TRUE(dest && source, NS_ERROR_NO_INTERFACE);

  CloneAttributes(dest, source);

  return NS_OK;
}

void
nsEditor::CloneAttributes(Element* aDest, Element* aSource)
{
  MOZ_ASSERT(aDest && aSource);

  nsAutoEditBatch beginBatching(this);

  
  
  NS_ENSURE_TRUE(GetRoot(), );
  bool destInBody = GetRoot()->Contains(aDest);

  
  nsRefPtr<nsDOMAttributeMap> destAttributes = aDest->Attributes();
  while (nsRefPtr<Attr> attr = destAttributes->Item(0)) {
    if (destInBody) {
      RemoveAttribute(static_cast<nsIDOMElement*>(GetAsDOMNode(aDest)),
                      attr->NodeName());
    } else {
      ErrorResult ignored;
      aDest->RemoveAttribute(attr->NodeName(), ignored);
    }
  }

  
  nsRefPtr<nsDOMAttributeMap> sourceAttributes = aSource->Attributes();
  uint32_t sourceCount = sourceAttributes->Length();
  for (uint32_t i = 0; i < sourceCount; i++) {
    nsRefPtr<Attr> attr = sourceAttributes->Item(i);
    nsAutoString value;
    attr->GetValue(value);
    if (destInBody) {
      SetAttributeOrEquivalent(static_cast<nsIDOMElement*>(GetAsDOMNode(aDest)),
                               attr->NodeName(), value, false);
    } else {
      
      
      SetAttributeOrEquivalent(static_cast<nsIDOMElement*>(GetAsDOMNode(aDest)),
                               attr->NodeName(), value, true);
    }
  }
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

nsresult
nsEditor::InsertTextImpl(const nsAString& aStringToInsert,
                         nsCOMPtr<nsINode>* aInOutNode,
                         int32_t* aInOutOffset,
                         nsIDocument* aDoc)
{
  
  
  

  NS_ENSURE_TRUE(aInOutNode && *aInOutNode && aInOutOffset && aDoc,
                 NS_ERROR_NULL_POINTER);
  if (!mComposition && aStringToInsert.IsEmpty()) {
    return NS_OK;
  }

  nsCOMPtr<nsINode> node = *aInOutNode;
  uint32_t offset = static_cast<uint32_t>(*aInOutOffset);

  if (!node->IsNodeOfType(nsINode::eTEXT) && IsPlaintextEditor()) {
    nsCOMPtr<nsINode> root = GetRoot();
    
    
    
    if (node == root && offset == 0 && node->HasChildren() &&
        node->GetFirstChild()->IsNodeOfType(nsINode::eTEXT)) {
      node = node->GetFirstChild();
    }
    
    
    
    if (node == root && offset > 0 && node->GetChildAt(offset - 1) &&
        node->GetChildAt(offset - 1)->IsNodeOfType(nsINode::eTEXT)) {
      node = node->GetChildAt(offset - 1);
      offset = node->Length();
    }
    
    
    
    if (nsTextEditUtils::IsMozBR(node) && offset == 0) {
      if (node->GetPreviousSibling() &&
          node->GetPreviousSibling()->IsNodeOfType(nsINode::eTEXT)) {
        node = node->GetPreviousSibling();
        offset = node->Length();
      } else if (node->GetParentNode() && node->GetParentNode() == root) {
        node = node->GetParentNode();
      }
    }
  }

  nsresult res;
  if (mComposition) {
    if (!node->IsNodeOfType(nsINode::eTEXT)) {
      
      nsRefPtr<nsTextNode> newNode = aDoc->CreateTextNode(EmptyString());
      
      res = InsertNode(*newNode, *node, offset);
      NS_ENSURE_SUCCESS(res, res);
      node = newNode;
      offset = 0;
    }
    res = InsertTextIntoTextNodeImpl(aStringToInsert, *node->GetAsText(),
                                     offset);
    NS_ENSURE_SUCCESS(res, res);
    offset += aStringToInsert.Length();
  } else {
    if (node->IsNodeOfType(nsINode::eTEXT)) {
      
      res = InsertTextIntoTextNodeImpl(aStringToInsert, *node->GetAsText(),
                                       offset);
      NS_ENSURE_SUCCESS(res, res);
      offset += aStringToInsert.Length();
    } else {
      
      
      nsRefPtr<nsTextNode> newNode = aDoc->CreateTextNode(aStringToInsert);
      
      res = InsertNode(*newNode, *node, offset);
      NS_ENSURE_SUCCESS(res, res);
      node = newNode;
      offset = aStringToInsert.Length();
    }
  }

  *aInOutNode = node;
  *aInOutOffset = static_cast<int32_t>(offset);
  return NS_OK;
}


nsresult
nsEditor::InsertTextIntoTextNodeImpl(const nsAString& aStringToInsert,
                                     Text& aTextNode,
                                     int32_t aOffset, bool aSuppressIME)
{
  nsRefPtr<EditTxn> txn;
  bool isIMETransaction = false;
  
  
  
  if (mComposition && !aSuppressIME) {
    if (!mIMETextNode) {
      mIMETextNode = &aTextNode;
      mIMETextOffset = aOffset;
    }
    
    const TextRangeArray* ranges = mComposition->GetRanges();
    for (uint32_t i = 0; i < (ranges ? ranges->Length() : 0); ++i) {
      const TextRange& textRange = ranges->ElementAt(i);
      if (!textRange.Length() ||
          textRange.mRangeType != NS_TEXTRANGE_RAWINPUT) {
        continue;
      }
      if (!mPhonetic) {
        mPhonetic = new nsString();
      }
      nsAutoString stringToInsert(aStringToInsert);
      stringToInsert.Mid(*mPhonetic,
                         textRange.mStartOffset, textRange.Length());
    }

    txn = CreateTxnForIMEText(aStringToInsert);
    isIMETransaction = true;
  } else {
    txn = CreateTxnForInsertText(aStringToInsert, aTextNode, aOffset);
  }

  
  for (auto& listener : mActionListeners) {
    listener->WillInsertText(
      static_cast<nsIDOMCharacterData*>(aTextNode.AsDOMNode()), aOffset,
      aStringToInsert);
  }

  
  
  BeginUpdateViewBatch();
  nsresult res = DoTransaction(txn);
  EndUpdateViewBatch();

  mRangeUpdater.SelAdjInsertText(aTextNode, aOffset, aStringToInsert);

  
  for (auto& listener : mActionListeners) {
    listener->DidInsertText(
      static_cast<nsIDOMCharacterData*>(aTextNode.AsDOMNode()),
      aOffset, aStringToInsert, res);
  }

  
  
  
  
  
  
  
  

  
  if (isIMETransaction && mIMETextNode) {
    uint32_t len = mIMETextNode->Length();
    if (!len) {
      DeleteNode(mIMETextNode);
      mIMETextNode = nullptr;
      static_cast<IMETextTxn*>(txn.get())->MarkFixed();
    }
  }

  return res;
}


nsresult
nsEditor::SelectEntireDocument(Selection* aSelection)
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
  if (!mDocStateListeners.Length()) {
    
    return NS_OK;
  }
 
  nsTArray<OwningNonNull<nsIDocumentStateListener>>
    listeners(mDocStateListeners);
  nsresult rv = NS_OK;

  switch (aNotificationType)
  {
    case eDocumentCreated:
      for (auto& listener : listeners) {
        rv = listener->NotifyDocumentCreated();
        if (NS_FAILED(rv))
          break;
      }
      break;
      
    case eDocumentToBeDestroyed:
      for (auto& listener : listeners) {
        rv = listener->NotifyDocumentWillBeDestroyed();
        if (NS_FAILED(rv))
          break;
      }
      break;

    case eDocumentStateChanged:
      {
        bool docIsDirty;
        rv = GetDocumentModified(&docIsDirty);
        NS_ENSURE_SUCCESS(rv, rv);

        if (static_cast<int8_t>(docIsDirty) == mDocDirtyState)
          return NS_OK;

        mDocDirtyState = docIsDirty;

        for (auto& listener : listeners) {
          rv = listener->NotifyDocumentStateChanged(mDocDirtyState);
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


already_AddRefed<InsertTextTxn>
nsEditor::CreateTxnForInsertText(const nsAString& aStringToInsert,
                                 Text& aTextNode, int32_t aOffset)
{
  nsRefPtr<InsertTextTxn> txn = new InsertTextTxn(aTextNode, aOffset,
                                                  aStringToInsert, *this);
  return txn.forget();
}


nsresult
nsEditor::DeleteText(nsGenericDOMDataNode& aCharData, uint32_t aOffset,
                     uint32_t aLength)
{
  nsRefPtr<DeleteTextTxn> txn =
    CreateTxnForDeleteText(aCharData, aOffset, aLength);
  NS_ENSURE_STATE(txn);

  nsAutoRules beginRulesSniffing(this, EditAction::deleteText, nsIEditor::ePrevious);

  
  for (auto& listener : mActionListeners) {
    listener->WillDeleteText(
        static_cast<nsIDOMCharacterData*>(GetAsDOMNode(&aCharData)), aOffset,
        aLength);
  }

  nsresult res = DoTransaction(txn);

  
  for (auto& listener : mActionListeners) {
    listener->DidDeleteText(
        static_cast<nsIDOMCharacterData*>(GetAsDOMNode(&aCharData)), aOffset,
        aLength, res);
  }

  return res;
}


already_AddRefed<DeleteTextTxn>
nsEditor::CreateTxnForDeleteText(nsGenericDOMDataNode& aCharData,
                                 uint32_t aOffset, uint32_t aLength)
{
  nsRefPtr<DeleteTextTxn> txn =
    new DeleteTextTxn(*this, aCharData, aOffset, aLength, &mRangeUpdater);

  nsresult res = txn->Init();
  NS_ENSURE_SUCCESS(res, nullptr);

  return txn.forget();
}

already_AddRefed<SplitNodeTxn>
nsEditor::CreateTxnForSplitNode(nsIContent& aNode, uint32_t aOffset)
{
  nsRefPtr<SplitNodeTxn> txn = new SplitNodeTxn(*this, aNode, aOffset);
  return txn.forget();
}

already_AddRefed<JoinNodeTxn>
nsEditor::CreateTxnForJoinNode(nsINode& aLeftNode, nsINode& aRightNode)
{
  nsRefPtr<JoinNodeTxn> txn = new JoinNodeTxn(*this, aLeftNode, aRightNode);

  NS_ENSURE_SUCCESS(txn->CheckValidity(), nullptr);

  return txn.forget();
}







struct SavedRange {
  nsRefPtr<Selection> mSelection;
  nsCOMPtr<nsINode> mStartNode;
  nsCOMPtr<nsINode> mEndNode;
  int32_t mStartOffset;
  int32_t mEndOffset;
};

nsresult
nsEditor::SplitNodeImpl(nsIContent& aExistingRightNode,
                        int32_t aOffset,
                        nsIContent& aNewLeftNode)
{
  
  nsAutoTArray<SavedRange, 10> savedRanges;
  for (size_t i = 0; i < nsISelectionController::NUM_SELECTIONTYPES - 1; ++i) {
    SelectionType type(1 << i);
    SavedRange range;
    range.mSelection = GetSelection(type);
    if (type == nsISelectionController::SELECTION_NORMAL) {
      NS_ENSURE_TRUE(range.mSelection, NS_ERROR_NULL_POINTER);
    } else if (!range.mSelection) {
      
      continue;
    }

    for (uint32_t j = 0; j < range.mSelection->RangeCount(); ++j) {
      nsRefPtr<nsRange> r = range.mSelection->GetRangeAt(j);
      MOZ_ASSERT(r->IsPositioned());
      range.mStartNode = r->GetStartParent();
      range.mStartOffset = r->StartOffset();
      range.mEndNode = r->GetEndParent();
      range.mEndOffset = r->EndOffset();

      savedRanges.AppendElement(range);
    }
  }

  nsCOMPtr<nsINode> parent = aExistingRightNode.GetParentNode();
  NS_ENSURE_TRUE(parent, NS_ERROR_NULL_POINTER);

  ErrorResult rv;
  parent->InsertBefore(aNewLeftNode, &aExistingRightNode, rv);
  NS_ENSURE_SUCCESS(rv.ErrorCode(), rv.ErrorCode());

  
  
  
  if (aOffset < 0) {
    
    return NS_OK;
  }

  
  if (aExistingRightNode.GetAsText() && aNewLeftNode.GetAsText()) {
    
    nsAutoString leftText;
    aExistingRightNode.GetAsText()->SubstringData(0, aOffset, leftText);
    aExistingRightNode.GetAsText()->DeleteData(0, aOffset);
    
    aNewLeftNode.GetAsText()->SetData(leftText);
  } else {
    
    
    nsCOMPtr<nsINodeList> childNodes = aExistingRightNode.ChildNodes();
    for (int32_t i = aOffset - 1; i >= 0; i--) {
      nsCOMPtr<nsIContent> childNode = childNodes->Item(i);
      if (childNode) {
        aExistingRightNode.RemoveChild(*childNode, rv);
        if (!rv.Failed()) {
          nsCOMPtr<nsIContent> firstChild = aNewLeftNode.GetFirstChild();
          aNewLeftNode.InsertBefore(*childNode, firstChild, rv);
        }
      }
      if (rv.Failed()) {
        break;
      }
    }
  }

  
  nsCOMPtr<nsIPresShell> ps = GetPresShell();
  if (ps) {
    ps->FlushPendingNotifications(Flush_Frames);
  }

  bool shouldSetSelection = GetShouldTxnSetSelection();

  nsRefPtr<Selection> previousSelection;
  for (size_t i = 0; i < savedRanges.Length(); ++i) {
    
    SavedRange& range = savedRanges[i];

    
    if (range.mSelection != previousSelection) {
      nsresult rv = range.mSelection->RemoveAllRanges();
      NS_ENSURE_SUCCESS(rv, rv);
      previousSelection = range.mSelection;
    }

    if (shouldSetSelection &&
        range.mSelection->Type() ==
          nsISelectionController::SELECTION_NORMAL) {
      
      
      continue;
    }

    
    if (range.mStartNode == &aExistingRightNode) {
      if (range.mStartOffset < aOffset) {
        range.mStartNode = &aNewLeftNode;
      } else {
        range.mStartOffset -= aOffset;
      }
    }

    if (range.mEndNode == &aExistingRightNode) {
      if (range.mEndOffset < aOffset) {
        range.mEndNode = &aNewLeftNode;
      } else {
        range.mEndOffset -= aOffset;
      }
    }

    nsRefPtr<nsRange> newRange;
    nsresult rv = nsRange::CreateRange(range.mStartNode, range.mStartOffset,
                                       range.mEndNode, range.mEndOffset,
                                       getter_AddRefs(newRange));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = range.mSelection->AddRange(newRange);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (shouldSetSelection) {
    
    nsRefPtr<Selection> selection = GetSelection();
    NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);
    selection->Collapse(&aNewLeftNode, aOffset);
  }

  return NS_OK;
}

nsresult
nsEditor::JoinNodesImpl(nsINode* aNodeToKeep,
                        nsINode* aNodeToJoin,
                        nsINode* aParent)
{
  MOZ_ASSERT(aNodeToKeep);
  MOZ_ASSERT(aNodeToJoin);
  MOZ_ASSERT(aParent);

  uint32_t firstNodeLength = aNodeToJoin->Length();

  int32_t joinOffset;
  GetNodeLocation(aNodeToJoin, &joinOffset);
  int32_t keepOffset;
  nsINode* parent = GetNodeLocation(aNodeToKeep, &keepOffset);

  
  nsAutoTArray<SavedRange, 10> savedRanges;
  for (size_t i = 0; i < nsISelectionController::NUM_SELECTIONTYPES - 1; ++i) {
    SelectionType type(1 << i);
    SavedRange range;
    range.mSelection = GetSelection(type);
    if (type == nsISelectionController::SELECTION_NORMAL) {
      NS_ENSURE_TRUE(range.mSelection, NS_ERROR_NULL_POINTER);
    } else if (!range.mSelection) {
      
      continue;
    }

    for (uint32_t j = 0; j < range.mSelection->RangeCount(); ++j) {
      nsRefPtr<nsRange> r = range.mSelection->GetRangeAt(j);
      MOZ_ASSERT(r->IsPositioned());
      range.mStartNode = r->GetStartParent();
      range.mStartOffset = r->StartOffset();
      range.mEndNode = r->GetEndParent();
      range.mEndOffset = r->EndOffset();

      
      
      
      if (range.mStartNode) {
        if (range.mStartNode == parent &&
            joinOffset < range.mStartOffset &&
            range.mStartOffset <= keepOffset) {
          range.mStartNode = aNodeToJoin;
          range.mStartOffset = firstNodeLength;
        }
        if (range.mEndNode == parent &&
            joinOffset < range.mEndOffset &&
            range.mEndOffset <= keepOffset) {
          range.mEndNode = aNodeToJoin;
          range.mEndOffset = firstNodeLength;
        }
      }

      savedRanges.AppendElement(range);
    }
  }

  
  
  nsCOMPtr<nsIDOMCharacterData> keepNodeAsText( do_QueryInterface(aNodeToKeep) );
  nsCOMPtr<nsIDOMCharacterData> joinNodeAsText( do_QueryInterface(aNodeToJoin) );
  if (keepNodeAsText && joinNodeAsText) {
    nsAutoString rightText;
    nsAutoString leftText;
    keepNodeAsText->GetData(rightText);
    joinNodeAsText->GetData(leftText);
    leftText += rightText;
    keepNodeAsText->SetData(leftText);
  } else {
    
    nsCOMPtr<nsINodeList> childNodes = aNodeToJoin->ChildNodes();
    MOZ_ASSERT(childNodes);

    
    
    nsCOMPtr<nsIContent> firstNode = aNodeToKeep->GetFirstChild();

    
    for (uint32_t i = childNodes->Length(); i > 0; --i) {
      nsCOMPtr<nsIContent> childNode = childNodes->Item(i - 1);
      if (childNode) {
        
        ErrorResult err;
        aNodeToKeep->InsertBefore(*childNode, firstNode, err);
        NS_ENSURE_SUCCESS(err.ErrorCode(), err.ErrorCode());
        firstNode = childNode.forget();
      }
    }
  }

  
  ErrorResult err;
  aParent->RemoveChild(*aNodeToJoin, err);

  bool shouldSetSelection = GetShouldTxnSetSelection();

  nsRefPtr<Selection> previousSelection;
  for (size_t i = 0; i < savedRanges.Length(); ++i) {
    
    SavedRange& range = savedRanges[i];

    
    if (range.mSelection != previousSelection) {
      nsresult rv = range.mSelection->RemoveAllRanges();
      NS_ENSURE_SUCCESS(rv, rv);
      previousSelection = range.mSelection;
    }

    if (shouldSetSelection &&
        range.mSelection->Type() ==
          nsISelectionController::SELECTION_NORMAL) {
      
      
      continue;
    }

    
    if (range.mStartNode == aNodeToJoin) {
      range.mStartNode = aNodeToKeep;
    } else if (range.mStartNode == aNodeToKeep) {
      range.mStartOffset += firstNodeLength;
    }

    
    if (range.mEndNode == aNodeToJoin) {
      range.mEndNode = aNodeToKeep;
    } else if (range.mEndNode == aNodeToKeep) {
      range.mEndOffset += firstNodeLength;
    }

    nsRefPtr<nsRange> newRange;
    nsresult rv = nsRange::CreateRange(range.mStartNode, range.mStartOffset,
                                       range.mEndNode, range.mEndOffset,
                                       getter_AddRefs(newRange));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = range.mSelection->AddRange(newRange);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (shouldSetSelection) {
    
    nsRefPtr<Selection> selection = GetSelection();
    NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);
    selection->Collapse(aNodeToKeep, AssertedCast<int32_t>(firstNodeLength));
  }

  return err.ErrorCode();
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

nsINode*
nsEditor::GetNodeLocation(nsINode* aChild, int32_t* aOffset)
{
  MOZ_ASSERT(aChild);
  MOZ_ASSERT(aOffset);

  nsINode* parent = aChild->GetParentNode();
  if (parent) {
    *aOffset = parent->IndexOf(aChild);
    MOZ_ASSERT(*aOffset != -1);
  } else {
    *aOffset = -1;
  }
  return parent;
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
nsEditor::CanContain(nsINode& aParent, nsIContent& aChild)
{
  switch (aParent.NodeType()) {
  case nsIDOMNode::ELEMENT_NODE:
  case nsIDOMNode::DOCUMENT_FRAGMENT_NODE:
    return TagCanContain(*aParent.NodeInfo()->NameAtom(), aChild);
  }
  return false;
}

bool
nsEditor::CanContainTag(nsINode& aParent, nsIAtom& aChildTag)
{
  switch (aParent.NodeType()) {
  case nsIDOMNode::ELEMENT_NODE:
  case nsIDOMNode::DOCUMENT_FRAGMENT_NODE:
    return TagCanContainTag(*aParent.NodeInfo()->NameAtom(), aChildTag);
  }
  return false;
}

bool 
nsEditor::TagCanContain(nsIAtom& aParentTag, nsIContent& aChild)
{
  switch (aChild.NodeType()) {
  case nsIDOMNode::TEXT_NODE:
  case nsIDOMNode::ELEMENT_NODE:
  case nsIDOMNode::DOCUMENT_FRAGMENT_NODE:
    return TagCanContainTag(aParentTag, *aChild.NodeInfo()->NameAtom());
  }
  return false;
}

bool 
nsEditor::TagCanContainTag(nsIAtom& aParentTag, nsIAtom& aChildTag)
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
nsEditor::IsContainer(nsINode* aNode)
{
  return aNode ? true : false;
}

bool
nsEditor::IsContainer(nsIDOMNode* aNode)
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
    return styleContext->StyleDisplay()->mDisplay != NS_STYLE_DISPLAY_NONE;
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
nsEditor::IsEditable(nsINode* aNode)
{
  NS_ENSURE_TRUE(aNode, false);

  if (!aNode->IsNodeOfType(nsINode::eCONTENT) || IsMozEditorBogusNode(aNode) ||
      !IsModifiableNode(aNode)) {
    return false;
  }

  
  
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
nsEditor::IsMozEditorBogusNode(nsINode* element)
{
  return element && element->IsElement() &&
         element->AsElement()->AttrValueIs(kNameSpaceID_None,
             kMOZEditorBogusNodeAttrAtom, kMOZEditorBogusNodeValue,
             eCaseMatters);
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
    NS_ASSERTION(aNode, "null node passed to nsEditor::GetTag()");

    return nullptr;
  }
  
  return content->NodeInfo()->NameAtom();
}





nsresult 
nsEditor::GetTagString(nsIDOMNode *aNode, nsAString& outString)
{
  if (!aNode) 
  {
    NS_NOTREACHED("null node passed to nsEditor::GetTagString()");
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
  return aNode1->NodeInfo()->NameAtom() == aNode2->NodeInfo()->NameAtom();
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
nsEditor::GetStartNodeAndOffset(Selection* aSelection,
                                       nsIDOMNode **outStartNode,
                                       int32_t *outStartOffset)
{
  NS_ENSURE_TRUE(outStartNode && outStartOffset && aSelection, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsINode> startNode;
  nsresult rv = GetStartNodeAndOffset(aSelection, getter_AddRefs(startNode),
                                      outStartOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  if (startNode) {
    NS_ADDREF(*outStartNode = startNode->AsDOMNode());
  } else {
    *outStartNode = nullptr;
  }
  return NS_OK;
}

nsresult
nsEditor::GetStartNodeAndOffset(Selection* aSelection, nsINode** aStartNode,
                                int32_t* aStartOffset)
{
  MOZ_ASSERT(aSelection);
  MOZ_ASSERT(aStartNode);
  MOZ_ASSERT(aStartOffset);

  *aStartNode = nullptr;
  *aStartOffset = 0;

  NS_ENSURE_TRUE(aSelection->RangeCount(), NS_ERROR_FAILURE);

  const nsRange* range = aSelection->GetRangeAt(0);
  NS_ENSURE_TRUE(range, NS_ERROR_FAILURE);

  NS_ENSURE_TRUE(range->IsPositioned(), NS_ERROR_FAILURE);

  NS_IF_ADDREF(*aStartNode = range->GetStartParent());
  *aStartOffset = range->StartOffset();
  return NS_OK;
}





nsresult 
nsEditor::GetEndNodeAndOffset(Selection* aSelection,
                                       nsIDOMNode **outEndNode,
                                       int32_t *outEndOffset)
{
  NS_ENSURE_TRUE(outEndNode && outEndOffset && aSelection, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsINode> endNode;
  nsresult rv = GetEndNodeAndOffset(aSelection, getter_AddRefs(endNode),
                                    outEndOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  if (endNode) {
    NS_ADDREF(*outEndNode = endNode->AsDOMNode());
  } else {
    *outEndNode = nullptr;
  }
  return NS_OK;
}

nsresult
nsEditor::GetEndNodeAndOffset(Selection* aSelection, nsINode** aEndNode,
                              int32_t* aEndOffset)
{
  MOZ_ASSERT(aSelection);
  MOZ_ASSERT(aEndNode);
  MOZ_ASSERT(aEndOffset);

  *aEndNode = nullptr;
  *aEndOffset = 0;

  NS_ENSURE_TRUE(aSelection->RangeCount(), NS_ERROR_FAILURE);

  const nsRange* range = aSelection->GetRangeAt(0);
  NS_ENSURE_TRUE(range, NS_ERROR_FAILURE);

  NS_ENSURE_TRUE(range->IsPositioned(), NS_ERROR_FAILURE);

  NS_IF_ADDREF(*aEndNode = range->GetEndParent());
  *aEndOffset = range->EndOffset();
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
    elementStyle = nsComputedDOMStyle::GetStyleContextForElementNoFlush(content->AsElement(),
                                                                        nullptr,
                                                                        ps);
  }

  if (!elementStyle)
  {
    
    
    
    *aResult = false;
    return NS_OK;
  }

  const nsStyleText* styleText = elementStyle->StyleText();

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






::DOMPoint
nsEditor::JoinNodeDeep(nsIContent& aLeftNode, nsIContent& aRightNode)
{
  
  
  

  nsCOMPtr<nsIContent> leftNodeToJoin = &aLeftNode;
  nsCOMPtr<nsIContent> rightNodeToJoin = &aRightNode;
  nsCOMPtr<nsINode> parentNode = aRightNode.GetParentNode();

  ::DOMPoint ret;

  while (leftNodeToJoin && rightNodeToJoin && parentNode &&
         AreNodesSameType(leftNodeToJoin, rightNodeToJoin)) {
    uint32_t length = leftNodeToJoin->Length();

    ret.node = rightNodeToJoin;
    ret.offset = length;

    
    nsresult res = JoinNodes(*leftNodeToJoin, *rightNodeToJoin);
    NS_ENSURE_SUCCESS(res, ::DOMPoint());

    if (parentNode->GetAsText()) {
      
      return ret;
    }

    
    parentNode = rightNodeToJoin;
    leftNodeToJoin = parentNode->GetChildAt(length - 1);
    rightNodeToJoin = parentNode->GetChildAt(length);

    
    while (leftNodeToJoin && !IsEditable(leftNodeToJoin)) {
      leftNodeToJoin = leftNodeToJoin->GetPreviousSibling();
    }
    if (!leftNodeToJoin) {
      return ret;
    }

    while (rightNodeToJoin && !IsEditable(rightNodeToJoin)) {
      rightNodeToJoin = rightNodeToJoin->GetNextSibling();
    }
    if (!rightNodeToJoin) {
      return ret;
    }
  }

  return ret;
}

void
nsEditor::BeginUpdateViewBatch()
{
  NS_PRECONDITION(mUpdateCount >= 0, "bad state");

  if (0 == mUpdateCount)
  {
    

    nsRefPtr<Selection> selection = GetSelection();

    if (selection) {
      selection->StartBatchChanges();
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
    

    nsRefPtr<Selection> selection = GetSelection();
    if (selection) {
      selection->EndBatchChanges();
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

  nsRefPtr<Selection> selection = GetSelection();
  NS_ENSURE_STATE(selection);
  nsRefPtr<EditAggregateTxn> txn;
  nsCOMPtr<nsINode> deleteNode;
  int32_t deleteCharOffset = 0, deleteCharLength = 0;
  nsresult res = CreateTxnForDeleteSelection(aAction, getter_AddRefs(txn),
                                             getter_AddRefs(deleteNode),
                                             &deleteCharOffset,
                                             &deleteCharLength);
  nsCOMPtr<nsIDOMCharacterData> deleteCharData(do_QueryInterface(deleteNode));

  if (NS_SUCCEEDED(res))  
  {
    nsAutoRules beginRulesSniffing(this, EditAction::deleteSelection, aAction);
    
    if (!deleteNode) {
      for (auto& listener : mActionListeners) {
        listener->WillDeleteSelection(selection);
      }
    } else if (deleteCharData) {
      for (auto& listener : mActionListeners) {
        listener->WillDeleteText(deleteCharData, deleteCharOffset, 1);
      }
    } else {
      for (auto& listener : mActionListeners) {
        listener->WillDeleteNode(deleteNode->AsDOMNode());
      }
    }

    
    res = DoTransaction(txn);  

    
    if (!deleteNode) {
      for (auto& listener : mActionListeners) {
        listener->DidDeleteSelection(selection);
      }
    } else if (deleteCharData) {
      for (auto& listener : mActionListeners) {
        listener->DidDeleteText(deleteCharData, deleteCharOffset, 1, res);
      }
    } else {
      for (auto& listener : mActionListeners) {
        listener->DidDeleteNode(deleteNode->AsDOMNode(), res);
      }
    }
  }

  return res;
}

already_AddRefed<Element>
nsEditor::DeleteSelectionAndCreateElement(nsIAtom& aTag)
{
  nsresult res = DeleteSelectionAndPrepareToCreateNode();
  NS_ENSURE_SUCCESS(res, nullptr);

  nsRefPtr<Selection> selection = GetSelection();
  NS_ENSURE_TRUE(selection, nullptr);

  nsCOMPtr<nsINode> node = selection->GetAnchorNode();
  uint32_t offset = selection->AnchorOffset();

  nsCOMPtr<Element> newElement = CreateNode(&aTag, node, offset);

  
  res = selection->Collapse(node, offset + 1);
  NS_ENSURE_SUCCESS(res, nullptr);

  return newElement.forget();
}




TextComposition*
nsEditor::GetComposition() const
{
  return mComposition;
}

bool
nsEditor::IsIMEComposing() const
{
  return mComposition && mComposition->IsComposing();
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

    uint32_t offset = selection->AnchorOffset();

    if (offset == 0) {
      res = selection->Collapse(node->GetParentNode(),
                                node->GetParentNode()->IndexOf(node));
      MOZ_ASSERT(NS_SUCCEEDED(res));
      NS_ENSURE_SUCCESS(res, res);
    } else if (offset == node->Length()) {
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

already_AddRefed<ChangeAttributeTxn>
nsEditor::CreateTxnForSetAttribute(Element& aElement, nsIAtom& aAttribute,
                                   const nsAString& aValue)
{
  nsRefPtr<ChangeAttributeTxn> txn =
    new ChangeAttributeTxn(aElement, aAttribute, &aValue);

  return txn.forget();
}


already_AddRefed<ChangeAttributeTxn>
nsEditor::CreateTxnForRemoveAttribute(Element& aElement, nsIAtom& aAttribute)
{
  nsRefPtr<ChangeAttributeTxn> txn =
    new ChangeAttributeTxn(aElement, aAttribute, nullptr);

  return txn.forget();
}


already_AddRefed<CreateElementTxn>
nsEditor::CreateTxnForCreateElement(nsIAtom& aTag,
                                    nsINode& aParent,
                                    int32_t aPosition)
{
  nsRefPtr<CreateElementTxn> txn =
    new CreateElementTxn(*this, aTag, aParent, aPosition);

  return txn.forget();
}


already_AddRefed<InsertNodeTxn>
nsEditor::CreateTxnForInsertNode(nsIContent& aNode,
                                 nsINode& aParent,
                                 int32_t aPosition)
{
  nsRefPtr<InsertNodeTxn> txn = new InsertNodeTxn(aNode, aParent, aPosition,
                                                  *this);
  return txn.forget();
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

already_AddRefed<IMETextTxn>
nsEditor::CreateTxnForIMEText(const nsAString& aStringToInsert)
{
  
  
  nsRefPtr<IMETextTxn> txn = new IMETextTxn(*mIMETextNode, mIMETextOffset,
                                            mComposition->String().Length(),
                                            mComposition->GetRanges(),
                                            aStringToInsert, *this);
  return txn.forget();
}


NS_IMETHODIMP 
nsEditor::CreateTxnForAddStyleSheet(CSSStyleSheet* aSheet, AddStyleSheetTxn* *aTxn)
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
nsEditor::CreateTxnForRemoveStyleSheet(CSSStyleSheet* aSheet, RemoveStyleSheetTxn* *aTxn)
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

  for (uint32_t rangeIdx = 0; rangeIdx < selection->RangeCount(); ++rangeIdx) {
    nsRefPtr<nsRange> range = selection->GetRangeAt(rangeIdx);
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

already_AddRefed<DeleteTextTxn>
nsEditor::CreateTxnForDeleteCharacter(nsGenericDOMDataNode& aData,
                                      uint32_t aOffset, EDirection aDirection)
{
  NS_ASSERTION(aDirection == eNext || aDirection == ePrevious,
               "Invalid direction");
  nsAutoString data;
  aData.GetData(data);
  NS_ASSERTION(data.Length(), "Trying to delete from a zero-length node");
  NS_ENSURE_TRUE(data.Length(), nullptr);

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
    return nullptr;
  }
  return CreateTxnForDeleteText(aData, segOffset, segLength);
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

  
  

  uint32_t count = node->Length();

  bool isFirst = (0 == offset);
  bool isLast  = (count == (uint32_t)offset);

  
  

  
  
  if (aAction == ePrevious && isFirst) {
    
    
    nsCOMPtr<nsIContent> priorNode = GetPriorNode(node, true);
    NS_ENSURE_STATE(priorNode);

    
    
    if (priorNode->IsNodeOfType(nsINode::eDATA_NODE)) {
      nsRefPtr<nsGenericDOMDataNode> priorNodeAsCharData =
        static_cast<nsGenericDOMDataNode*>(priorNode.get());
      uint32_t length = priorNode->Length();
      
      NS_ENSURE_STATE(length);
      nsRefPtr<DeleteTextTxn> txn =
        CreateTxnForDeleteCharacter(*priorNodeAsCharData, length, ePrevious);
      NS_ENSURE_STATE(txn);

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

    
    
    if (nextNode->IsNodeOfType(nsINode::eDATA_NODE)) {
      nsRefPtr<nsGenericDOMDataNode> nextNodeAsCharData =
        static_cast<nsGenericDOMDataNode*>(nextNode.get());
      uint32_t length = nextNode->Length();
      
      NS_ENSURE_STATE(length);
      nsRefPtr<DeleteTextTxn> txn =
        CreateTxnForDeleteCharacter(*nextNodeAsCharData, 0, eNext);
      NS_ENSURE_STATE(txn);

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

  if (node->IsNodeOfType(nsINode::eDATA_NODE)) {
    nsRefPtr<nsGenericDOMDataNode> nodeAsCharData =
      static_cast<nsGenericDOMDataNode*>(node.get());
    
    nsRefPtr<DeleteTextTxn> txn = CreateTxnForDeleteCharacter(*nodeAsCharData,
                                                              offset, aAction);
    NS_ENSURE_STATE(txn);

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

    if (selectedNode->IsNodeOfType(nsINode::eDATA_NODE)) {
      nsRefPtr<nsGenericDOMDataNode> selectedNodeAsCharData =
        static_cast<nsGenericDOMDataNode*>(selectedNode.get());
      
      uint32_t position = 0;
      if (aAction == ePrevious) {
        position = selectedNode->Length();
      }
      nsRefPtr<DeleteTextTxn> delTextTxn =
        CreateTxnForDeleteCharacter(*selectedNodeAsCharData, position,
                                    aAction);
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
                      nsRange** aRange)
{
  return nsRange::CreateRange(aStartParent, aStartOffset, aEndParent,
                              aEndOffset, aRange);
}

nsresult 
nsEditor::AppendNodeToSelectionAsRange(nsIDOMNode *aNode)
{
  NS_ENSURE_TRUE(aNode, NS_ERROR_NULL_POINTER);
  nsRefPtr<Selection> selection = GetSelection();
  NS_ENSURE_TRUE(selection, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDOMNode> parentNode;
  nsresult res = aNode->GetParentNode(getter_AddRefs(parentNode));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(parentNode, NS_ERROR_NULL_POINTER);
  
  int32_t offset = GetChildOffset(aNode, parentNode);
  
  nsRefPtr<nsRange> range;
  res = CreateRange(parentNode, offset, parentNode, offset+1, getter_AddRefs(range));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(range, NS_ERROR_NULL_POINTER);

  return selection->AddRange(range);
}

nsresult nsEditor::ClearSelection()
{
  nsRefPtr<Selection> selection = GetSelection();
  NS_ENSURE_TRUE(selection, NS_ERROR_FAILURE);
  return selection->RemoveAllRanges();  
}

already_AddRefed<Element>
nsEditor::CreateHTMLContent(nsIAtom* aTag)
{
  MOZ_ASSERT(aTag);

  nsCOMPtr<nsIDocument> doc = GetDocument();
  if (!doc) {
    return nullptr;
  }

  
  
  if (aTag == nsGkAtoms::_empty) {
    NS_ERROR("Don't pass an empty tag to nsEditor::CreateHTMLContent, "
             "check caller.");
    return nullptr;
  }

  nsCOMPtr<nsIContent> ret;
  nsresult res = doc->CreateElem(nsDependentAtomString(aTag), nullptr,
                                 kNameSpaceID_XHTML, getter_AddRefs(ret));
  NS_ENSURE_SUCCESS(res, nullptr);
  return dont_AddRef(ret.forget().take()->AsElement());
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
  
  
  
  
  
  

  WidgetKeyboardEvent* nativeKeyEvent =
    aKeyEvent->GetInternalNSEvent()->AsKeyboardEvent();
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
                                   Selection* aSelection,
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

  nsRefPtr<Selection> selection = GetSelection();
  NS_ENSURE_STATE(selection);

  nsCOMPtr<nsIPresShell> presShell = GetPresShell();
  NS_ENSURE_TRUE(presShell, NS_ERROR_NOT_INITIALIZED);

  nsCOMPtr<nsISelectionController> selCon;
  nsresult rv = GetSelectionController(getter_AddRefs(selCon));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsRefPtr<nsCaret> caret = presShell->GetCaret();
  NS_ENSURE_TRUE(caret, NS_ERROR_UNEXPECTED);
  caret->SetIgnoreUserModify(false);
  caret->SetSelection(selection);
  selCon->SetCaretReadOnly(IsReadonly());
  selCon->SetCaretEnabled(true);

  
  selCon->SetDisplaySelection(nsISelectionController::SELECTION_ON);
  selCon->SetSelectionFlags(nsISelectionDisplay::DISPLAY_ALL);
  selCon->RepaintSelection(nsISelectionController::SELECTION_NORMAL);
  
  
  
  
  
  if (selectionRootContent->GetParent()) {
    selection->SetAncestorLimiter(selectionRootContent);
  } else {
    selection->SetAncestorLimiter(nullptr);
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

NS_IMETHODIMP
nsEditor::FinalizeSelection()
{
  nsCOMPtr<nsISelectionController> selCon;
  nsresult rv = GetSelectionController(getter_AddRefs(selCon));
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<Selection> selection = GetSelection();
  NS_ENSURE_STATE(selection);

  selection->SetAncestorLimiter(nullptr);

  nsCOMPtr<nsIPresShell> presShell = GetPresShell();
  NS_ENSURE_TRUE(presShell, NS_ERROR_NOT_INITIALIZED);

  selCon->SetCaretEnabled(false);

  nsFocusManager* fm = nsFocusManager::GetFocusManager();
  NS_ENSURE_TRUE(fm, NS_ERROR_NOT_INITIALIZED);
  fm->UpdateCaretForCaretBrowsingMode();

  if (!HasIndependentSelection()) {
    
    
    
    
    nsCOMPtr<nsIDocument> doc = GetDocument();
    ErrorResult ret;
    if (!doc || !doc->HasFocus(ret)) {
      
      selCon->SetDisplaySelection(nsISelectionController::SELECTION_DISABLED);
    } else {
      
      
      
      selCon->SetDisplaySelection(nsISelectionController::SELECTION_ON);
    }
  } else if (IsFormWidget() || IsPasswordEditor() ||
             IsReadonly() || IsDisabled() || IsInputFiltered()) {
    
    
    selCon->SetDisplaySelection(nsISelectionController::SELECTION_HIDDEN);
  } else {
    
    
    selCon->SetDisplaySelection(nsISelectionController::SELECTION_DISABLED);
  }

  selCon->RepaintSelection(nsISelectionController::SELECTION_NORMAL);
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

Element*
nsEditor::GetExposedRoot()
{
  Element* rootElement = GetRoot();

  
  if (rootElement && rootElement->IsRootOfNativeAnonymousSubtree()) {
    rootElement = rootElement->GetParent()->AsElement();
  }

  return rootElement;
}

nsresult
nsEditor::DetermineCurrentDirection()
{
  
  nsIContent* rootElement = GetExposedRoot();
  NS_ENSURE_TRUE(rootElement, NS_ERROR_FAILURE);

  
  
  if (!(mFlags & (nsIPlaintextEditor::eEditorLeftToRight |
                  nsIPlaintextEditor::eEditorRightToLeft))) {

    nsIFrame* frame = rootElement->GetPrimaryFrame();
    NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);

    
    
    if (frame->StyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL) {
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
  
  nsIContent* rootElement = GetExposedRoot();

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

  if (NS_SUCCEEDED(rv)) {
    FireInputEvent();
  }

  return rv;
}

void
nsEditor::SwitchTextDirectionTo(uint32_t aDirection)
{
  
  nsIContent* rootElement = GetExposedRoot();

  nsresult rv = DetermineCurrentDirection();
  NS_ENSURE_SUCCESS_VOID(rv);

  
  if (aDirection == nsIPlaintextEditor::eEditorLeftToRight &&
      (mFlags & nsIPlaintextEditor::eEditorRightToLeft)) {
    NS_ASSERTION(!(mFlags & nsIPlaintextEditor::eEditorLeftToRight),
                 "Unexpected mutually exclusive flag");
    mFlags &= ~nsIPlaintextEditor::eEditorRightToLeft;
    mFlags |= nsIPlaintextEditor::eEditorLeftToRight;
    rv = rootElement->SetAttr(kNameSpaceID_None, nsGkAtoms::dir, NS_LITERAL_STRING("ltr"), true);
  } else if (aDirection == nsIPlaintextEditor::eEditorRightToLeft &&
             (mFlags & nsIPlaintextEditor::eEditorLeftToRight)) {
    NS_ASSERTION(!(mFlags & nsIPlaintextEditor::eEditorRightToLeft),
                 "Unexpected mutually exclusive flag");
    mFlags |= nsIPlaintextEditor::eEditorRightToLeft;
    mFlags &= ~nsIPlaintextEditor::eEditorLeftToRight;
    rv = rootElement->SetAttr(kNameSpaceID_None, nsGkAtoms::dir, NS_LITERAL_STRING("rtl"), true);
  }

  if (NS_SUCCEEDED(rv)) {
    FireInputEvent();
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

  WidgetEvent* widgetEvent = aEvent->GetInternalNSEvent();
  if (NS_WARN_IF(!widgetEvent)) {
    return false;
  }

  
  
  if (widgetEvent->IsUsingCoordinates()) {
    nsCOMPtr<nsIContent> focusedContent = GetFocusedContent();
    if (!focusedContent) {
      return false;
    }
  }

  
  
  
  
  
  bool needsWidget = false;
  WidgetGUIEvent* widgetGUIEvent = nullptr;
  switch (widgetEvent->message) {
    case NS_USER_DEFINED_EVENT:
      
      
      return false;
    case NS_COMPOSITION_START:
    case NS_COMPOSITION_END:
    case NS_COMPOSITION_UPDATE:
    case NS_COMPOSITION_CHANGE:
    case NS_COMPOSITION_COMMIT_AS_IS:
      
      
      widgetGUIEvent = aEvent->GetInternalNSEvent()->AsCompositionEvent();
      needsWidget = true;
      break;
    default:
      break;
  }
  if (needsWidget &&
      (!widgetGUIEvent || !widgetGUIEvent->widget)) {
    return false;
  }

  
  if (widgetEvent->mFlags.mIsTrusted) {
    return true;
  }

  
  
  if (widgetEvent->AsMouseEventBase()) {
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

NS_IMETHODIMP
nsEditor::GetIsInEditAction(bool* aIsInEditAction)
{
  MOZ_ASSERT(aIsInEditAction, "aIsInEditAction must not be null");
  *aIsInEditAction = mIsInEditAction;
  return NS_OK;
}
