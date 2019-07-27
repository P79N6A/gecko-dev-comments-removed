

































#include "mozInlineSpellChecker.h"
#include "mozInlineSpellWordUtil.h"
#include "mozISpellI18NManager.h"
#include "nsCOMPtr.h"
#include "nsCRT.h"
#include "nsIDOMNode.h"
#include "nsIDOMDocument.h"
#include "nsIDOMElement.h"
#include "nsIDOMHTMLElement.h"
#include "nsIDOMMouseEvent.h"
#include "nsIDOMKeyEvent.h"
#include "nsIDOMNode.h"
#include "nsIDOMNodeList.h"
#include "nsGenericHTMLElement.h"
#include "nsRange.h"
#include "mozilla/dom/Selection.h"
#include "nsIPlaintextEditor.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsIRunnable.h"
#include "nsISelection.h"
#include "nsISelectionPrivate.h"
#include "nsISelectionController.h"
#include "nsIServiceManager.h"
#include "nsITextServicesFilter.h"
#include "nsString.h"
#include "nsThreadUtils.h"
#include "nsUnicharUtils.h"
#include "nsIContent.h"
#include "nsRange.h"
#include "nsContentUtils.h"
#include "nsEditor.h"
#include "mozilla/Services.h"
#include "nsIObserverService.h"
#include "nsITextControlElement.h"
#include "prtime.h"

using namespace mozilla;
using namespace mozilla::dom;





#define INLINESPELL_CHECK_TIMEOUT 50





#define INLINESPELL_TIMEOUT_CHECK_FREQUENCY 50






#define MISSPELLED_WORD_COUNT_PENALTY 4



#define INLINESPELL_STARTED_TOPIC "inlineSpellChecker-spellCheck-started"
#define INLINESPELL_ENDED_TOPIC "inlineSpellChecker-spellCheck-ended"

static bool ContentIsDescendantOf(nsINode* aPossibleDescendant,
                                    nsINode* aPossibleAncestor);

static const char kMaxSpellCheckSelectionSize[] = "extensions.spellcheck.inline.max-misspellings";

mozInlineSpellStatus::mozInlineSpellStatus(mozInlineSpellChecker* aSpellChecker)
    : mSpellChecker(aSpellChecker), mWordCount(0)
{
}








nsresult
mozInlineSpellStatus::InitForEditorChange(
    EditAction aAction,
    nsIDOMNode* aAnchorNode, int32_t aAnchorOffset,
    nsIDOMNode* aPreviousNode, int32_t aPreviousOffset,
    nsIDOMNode* aStartNode, int32_t aStartOffset,
    nsIDOMNode* aEndNode, int32_t aEndOffset)
{
  nsresult rv;

  nsCOMPtr<nsIDOMDocument> doc;
  rv = GetDocument(getter_AddRefs(doc));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = PositionToCollapsedRange(doc, aAnchorNode, aAnchorOffset,
                                getter_AddRefs(mAnchorRange));
  NS_ENSURE_SUCCESS(rv, rv);

  if (aAction == EditAction::deleteSelection) {
    
    
    
    mOp = eOpChangeDelete;
    mRange = nullptr;
    return NS_OK;
  }

  mOp = eOpChange;

  
  nsCOMPtr<nsINode> prevNode = do_QueryInterface(aPreviousNode);
  NS_ENSURE_STATE(prevNode);

  mRange = new nsRange(prevNode);

  
  int16_t cmpResult;
  rv = mAnchorRange->ComparePoint(aPreviousNode, aPreviousOffset, &cmpResult);
  NS_ENSURE_SUCCESS(rv, rv);
  if (cmpResult < 0) {
    
    rv = mRange->SetStart(aPreviousNode, aPreviousOffset);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mRange->SetEnd(aAnchorNode, aAnchorOffset);
  } else {
    
    rv = mRange->SetStart(aAnchorNode, aAnchorOffset);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mRange->SetEnd(aPreviousNode, aPreviousOffset);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (aAction == EditAction::insertText)
    mCreatedRange = mRange;

  
  if (aStartNode && aEndNode) {
    rv = mRange->ComparePoint(aStartNode, aStartOffset, &cmpResult);
    NS_ENSURE_SUCCESS(rv, rv);
    if (cmpResult < 0) { 
      rv = mRange->SetStart(aStartNode, aStartOffset);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    rv = mRange->ComparePoint(aEndNode, aEndOffset, &cmpResult);
    NS_ENSURE_SUCCESS(rv, rv);
    if (cmpResult > 0) { 
      rv = mRange->SetEnd(aEndNode, aEndOffset);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}








nsresult
mozInlineSpellStatus::InitForNavigation(
    bool aForceCheck, int32_t aNewPositionOffset,
    nsIDOMNode* aOldAnchorNode, int32_t aOldAnchorOffset,
    nsIDOMNode* aNewAnchorNode, int32_t aNewAnchorOffset,
    bool* aContinue)
{
  nsresult rv;
  mOp = eOpNavigation;

  mForceNavigationWordCheck = aForceCheck;
  mNewNavigationPositionOffset = aNewPositionOffset;

  
  nsCOMPtr<nsIEditor> editor = do_QueryReferent(mSpellChecker->mEditor, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIDOMElement> rootElt;
  rv = editor->GetRootElement(getter_AddRefs(rootElt));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsINode> root = do_QueryInterface(rootElt, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsINode> currentAnchor = do_QueryInterface(aOldAnchorNode, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  if (root && currentAnchor && ! ContentIsDescendantOf(currentAnchor, root)) {
    *aContinue = false;
    return NS_OK;
  }

  nsCOMPtr<nsIDOMDocument> doc;
  rv = GetDocument(getter_AddRefs(doc));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = PositionToCollapsedRange(doc, aOldAnchorNode, aOldAnchorOffset,
                                getter_AddRefs(mOldNavigationAnchorRange));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = PositionToCollapsedRange(doc, aNewAnchorNode, aNewAnchorOffset,
                                getter_AddRefs(mAnchorRange));
  NS_ENSURE_SUCCESS(rv, rv);

  *aContinue = true;
  return NS_OK;
}






nsresult
mozInlineSpellStatus::InitForSelection()
{
  mOp = eOpSelection;
  return NS_OK;
}






nsresult
mozInlineSpellStatus::InitForRange(nsRange* aRange)
{
  mOp = eOpChange;
  mRange = aRange;
  return NS_OK;
}











nsresult
mozInlineSpellStatus::FinishInitOnEvent(mozInlineSpellWordUtil& aWordUtil)
{
  nsresult rv;
  if (! mRange) {
    rv = mSpellChecker->MakeSpellCheckRange(nullptr, 0, nullptr, 0,
                                            getter_AddRefs(mRange));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  switch (mOp) {
    case eOpChange:
      if (mAnchorRange)
        return FillNoCheckRangeFromAnchor(aWordUtil);
      break;
    case eOpChangeDelete:
      if (mAnchorRange) {
        rv = FillNoCheckRangeFromAnchor(aWordUtil);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      
      
      
      mRange = mNoCheckRange;
      break;
    case eOpNavigation:
      return FinishNavigationEvent(aWordUtil);
    case eOpSelection:
      
      break;
    case eOpResume:
      
      break;
    default:
      NS_NOTREACHED("Bad operation");
      return NS_ERROR_NOT_INITIALIZED;
  }
  return NS_OK;
}














nsresult
mozInlineSpellStatus::FinishNavigationEvent(mozInlineSpellWordUtil& aWordUtil)
{
  nsCOMPtr<nsIEditor> editor = do_QueryReferent(mSpellChecker->mEditor);
  if (! editor)
    return NS_ERROR_FAILURE; 

  NS_ASSERTION(mAnchorRange, "No anchor for navigation!");
  nsCOMPtr<nsIDOMNode> newAnchorNode, oldAnchorNode;
  int32_t newAnchorOffset, oldAnchorOffset;

  
  nsresult rv = mOldNavigationAnchorRange->GetStartContainer(
      getter_AddRefs(oldAnchorNode));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mOldNavigationAnchorRange->GetStartOffset(&oldAnchorOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  nsRefPtr<nsRange> oldWord;
  rv = aWordUtil.GetRangeForWord(oldAnchorNode, oldAnchorOffset,
                                 getter_AddRefs(oldWord));
  NS_ENSURE_SUCCESS(rv, rv);

  
  editor = do_QueryReferent(mSpellChecker->mEditor);
  if (! editor)
    return NS_ERROR_FAILURE; 

  
  rv = mAnchorRange->GetStartContainer(getter_AddRefs(newAnchorNode));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mAnchorRange->GetStartOffset(&newAnchorOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  
  bool isInRange = false;
  if (! mForceNavigationWordCheck) {
    rv = oldWord->IsPointInRange(newAnchorNode,
                                 newAnchorOffset + mNewNavigationPositionOffset,
                                 &isInRange);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (isInRange) {
    
    mRange = nullptr;
  } else {
    
    mRange = oldWord;

    
    
    mSpellChecker->mNeedsCheckAfterNavigation = false;
  }
  return NS_OK;
}








nsresult
mozInlineSpellStatus::FillNoCheckRangeFromAnchor(
    mozInlineSpellWordUtil& aWordUtil)
{
  nsCOMPtr<nsIDOMNode> anchorNode;
  nsresult rv = mAnchorRange->GetStartContainer(getter_AddRefs(anchorNode));
  NS_ENSURE_SUCCESS(rv, rv);

  int32_t anchorOffset;
  rv = mAnchorRange->GetStartOffset(&anchorOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  return aWordUtil.GetRangeForWord(anchorNode, anchorOffset,
                                   getter_AddRefs(mNoCheckRange));
}






nsresult
mozInlineSpellStatus::GetDocument(nsIDOMDocument** aDocument)
{
  nsresult rv;
  *aDocument = nullptr;
  if (! mSpellChecker->mEditor)
    return NS_ERROR_UNEXPECTED;

  nsCOMPtr<nsIEditor> editor = do_QueryReferent(mSpellChecker->mEditor, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMDocument> domDoc;
  rv = editor->GetDocument(getter_AddRefs(domDoc));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(domDoc, NS_ERROR_NULL_POINTER);
  domDoc.forget(aDocument);
  return NS_OK;
}







nsresult
mozInlineSpellStatus::PositionToCollapsedRange(nsIDOMDocument* aDocument,
    nsIDOMNode* aNode, int32_t aOffset, nsIDOMRange** aRange)
{
  *aRange = nullptr;
  nsCOMPtr<nsIDOMRange> range;
  nsresult rv = aDocument->CreateRange(getter_AddRefs(range));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = range->SetStart(aNode, aOffset);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = range->SetEnd(aNode, aOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  range.swap(*aRange);
  return NS_OK;
}



class mozInlineSpellResume : public nsRunnable
{
public:
  mozInlineSpellResume(const mozInlineSpellStatus& aStatus,
                       uint32_t aDisabledAsyncToken)
    : mDisabledAsyncToken(aDisabledAsyncToken), mStatus(aStatus) {}

  nsresult Post()
  {
    return NS_DispatchToMainThread(this);
  }

  NS_IMETHOD Run()
  {
    
    
    if (mDisabledAsyncToken == mStatus.mSpellChecker->mDisabledAsyncToken) {
      mStatus.mSpellChecker->ResumeCheck(&mStatus);
    }
    return NS_OK;
  }

private:
  uint32_t mDisabledAsyncToken;
  mozInlineSpellStatus mStatus;
};


class InitEditorSpellCheckCallback final : public nsIEditorSpellCheckCallback
{
  ~InitEditorSpellCheckCallback() {}
public:
  NS_DECL_ISUPPORTS

  explicit InitEditorSpellCheckCallback(mozInlineSpellChecker* aSpellChecker)
    : mSpellChecker(aSpellChecker) {}

  NS_IMETHOD EditorSpellCheckDone() override
  {
    return mSpellChecker ? mSpellChecker->EditorSpellCheckInited() : NS_OK;
  }

  void Cancel()
  {
    mSpellChecker = nullptr;
  }

private:
  nsRefPtr<mozInlineSpellChecker> mSpellChecker;
};
NS_IMPL_ISUPPORTS(InitEditorSpellCheckCallback, nsIEditorSpellCheckCallback)


NS_INTERFACE_MAP_BEGIN(mozInlineSpellChecker)
  NS_INTERFACE_MAP_ENTRY(nsIInlineSpellChecker)
  NS_INTERFACE_MAP_ENTRY(nsIEditActionListener)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsIDOMEventListener)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMEventListener)
  NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION(mozInlineSpellChecker)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(mozInlineSpellChecker)
NS_IMPL_CYCLE_COLLECTING_RELEASE(mozInlineSpellChecker)

NS_IMPL_CYCLE_COLLECTION(mozInlineSpellChecker,
                         mSpellCheck,
                         mTreeWalker,
                         mCurrentSelectionAnchorNode)

mozInlineSpellChecker::SpellCheckingState
  mozInlineSpellChecker::gCanEnableSpellChecking =
  mozInlineSpellChecker::SpellCheck_Uninitialized;

mozInlineSpellChecker::mozInlineSpellChecker() :
    mNumWordsInSpellSelection(0),
    mMaxNumWordsInSpellSelection(250),
    mNumPendingSpellChecks(0),
    mNumPendingUpdateCurrentDictionary(0),
    mDisabledAsyncToken(0),
    mNeedsCheckAfterNavigation(false),
    mFullSpellCheckScheduled(false)
{
  nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
  if (prefs)
    prefs->GetIntPref(kMaxSpellCheckSelectionSize, &mMaxNumWordsInSpellSelection); 
  mMaxMisspellingsPerCheck = mMaxNumWordsInSpellSelection * 3 / 4;
}

mozInlineSpellChecker::~mozInlineSpellChecker()
{
}

NS_IMETHODIMP
mozInlineSpellChecker::GetSpellChecker(nsIEditorSpellCheck **aSpellCheck)
{
  *aSpellCheck = mSpellCheck;
  NS_IF_ADDREF(*aSpellCheck);
  return NS_OK;
}

NS_IMETHODIMP
mozInlineSpellChecker::Init(nsIEditor *aEditor)
{
  mEditor = do_GetWeakReference(aEditor);
  return NS_OK;
}











nsresult mozInlineSpellChecker::Cleanup(bool aDestroyingFrames)
{
  mNumWordsInSpellSelection = 0;
  nsCOMPtr<nsISelection> spellCheckSelection;
  nsresult rv = GetSpellCheckSelection(getter_AddRefs(spellCheckSelection));
  if (NS_FAILED(rv)) {
    
    UnregisterEventListeners();
  } else {
    if (!aDestroyingFrames) {
      spellCheckSelection->RemoveAllRanges();
    }

    rv = UnregisterEventListeners();
  }

  
  
  
  
  
  
  

  nsCOMPtr<nsIEditor> editor = do_QueryReferent(mEditor);
  if (mPendingSpellCheck) {
    
    mPendingSpellCheck = nullptr;
    mPendingInitEditorSpellCheckCallback->Cancel();
    mPendingInitEditorSpellCheckCallback = nullptr;
    ChangeNumPendingSpellChecks(-1, editor);
  }

  
  
  mDisabledAsyncToken++;

  if (mNumPendingUpdateCurrentDictionary > 0) {
    
    ChangeNumPendingSpellChecks(-mNumPendingUpdateCurrentDictionary, editor);
    mNumPendingUpdateCurrentDictionary = 0;
  }
  if (mNumPendingSpellChecks > 0) {
    
    
    ChangeNumPendingSpellChecks(-mNumPendingSpellChecks, editor);
  }

  mEditor = nullptr;
  mFullSpellCheckScheduled = false;

  return rv;
}















bool 
mozInlineSpellChecker::CanEnableInlineSpellChecking()
{
  nsresult rv;
  if (gCanEnableSpellChecking == SpellCheck_Uninitialized) {
    gCanEnableSpellChecking = SpellCheck_NotAvailable;

    nsCOMPtr<nsIEditorSpellCheck> spellchecker =
      do_CreateInstance("@mozilla.org/editor/editorspellchecker;1", &rv);
    NS_ENSURE_SUCCESS(rv, false);

    bool canSpellCheck = false;
    rv = spellchecker->CanSpellCheck(&canSpellCheck);
    NS_ENSURE_SUCCESS(rv, false);

    if (canSpellCheck)
      gCanEnableSpellChecking = SpellCheck_Available;
  }
  return (gCanEnableSpellChecking == SpellCheck_Available);
}

void 
mozInlineSpellChecker::UpdateCanEnableInlineSpellChecking()
{
  gCanEnableSpellChecking = SpellCheck_Uninitialized;
}





nsresult
mozInlineSpellChecker::RegisterEventListeners()
{
  nsCOMPtr<nsIEditor> editor (do_QueryReferent(mEditor));
  NS_ENSURE_TRUE(editor, NS_ERROR_NULL_POINTER);

  editor->AddEditActionListener(this);

  nsCOMPtr<nsIDOMDocument> doc;
  nsresult rv = editor->GetDocument(getter_AddRefs(doc));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<EventTarget> piTarget = do_QueryInterface(doc, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  piTarget->AddEventListener(NS_LITERAL_STRING("blur"), this,
                             true, false);
  piTarget->AddEventListener(NS_LITERAL_STRING("click"), this,
                             false, false);
  piTarget->AddEventListener(NS_LITERAL_STRING("keypress"), this,
                             false, false);
  return NS_OK;
}



nsresult
mozInlineSpellChecker::UnregisterEventListeners()
{
  nsCOMPtr<nsIEditor> editor (do_QueryReferent(mEditor));
  NS_ENSURE_TRUE(editor, NS_ERROR_NULL_POINTER);

  editor->RemoveEditActionListener(this);

  nsCOMPtr<nsIDOMDocument> doc;
  editor->GetDocument(getter_AddRefs(doc));
  NS_ENSURE_TRUE(doc, NS_ERROR_NULL_POINTER);

  nsCOMPtr<EventTarget> piTarget = do_QueryInterface(doc);
  NS_ENSURE_TRUE(piTarget, NS_ERROR_NULL_POINTER);

  piTarget->RemoveEventListener(NS_LITERAL_STRING("blur"), this, true);
  piTarget->RemoveEventListener(NS_LITERAL_STRING("click"), this, false);
  piTarget->RemoveEventListener(NS_LITERAL_STRING("keypress"), this, false);
  return NS_OK;
}



NS_IMETHODIMP
mozInlineSpellChecker::GetEnableRealTimeSpell(bool* aEnabled)
{
  NS_ENSURE_ARG_POINTER(aEnabled);
  *aEnabled = mSpellCheck != nullptr || mPendingSpellCheck != nullptr;
  return NS_OK;
}



NS_IMETHODIMP
mozInlineSpellChecker::SetEnableRealTimeSpell(bool aEnabled)
{
  if (!aEnabled) {
    mSpellCheck = nullptr;
    return Cleanup(false);
  }

  if (mSpellCheck) {
    
    
    
    
    return SpellCheckRange(nullptr);
  }

  if (mPendingSpellCheck) {
    
    return NS_OK;
  }

  mPendingSpellCheck =
    do_CreateInstance("@mozilla.org/editor/editorspellchecker;1");
  NS_ENSURE_STATE(mPendingSpellCheck);

  nsCOMPtr<nsITextServicesFilter> filter =
    do_CreateInstance("@mozilla.org/editor/txtsrvfiltermail;1");
  if (!filter) {
    mPendingSpellCheck = nullptr;
    NS_ENSURE_STATE(filter);
  }
  mPendingSpellCheck->SetFilter(filter);

  mPendingInitEditorSpellCheckCallback = new InitEditorSpellCheckCallback(this);
  if (!mPendingInitEditorSpellCheckCallback) {
    mPendingSpellCheck = nullptr;
    NS_ENSURE_STATE(mPendingInitEditorSpellCheckCallback);
  }

  nsCOMPtr<nsIEditor> editor = do_QueryReferent(mEditor);
  nsresult rv = mPendingSpellCheck->InitSpellChecker(
                  editor, false, mPendingInitEditorSpellCheckCallback);
  if (NS_FAILED(rv)) {
    mPendingSpellCheck = nullptr;
    mPendingInitEditorSpellCheckCallback = nullptr;
    NS_ENSURE_SUCCESS(rv, rv);
  }

  ChangeNumPendingSpellChecks(1);

  return NS_OK;
}


nsresult
mozInlineSpellChecker::EditorSpellCheckInited()
{
  NS_ASSERTION(mPendingSpellCheck, "Spell check should be pending!");

  
  RegisterEventListeners();

  mSpellCheck = mPendingSpellCheck;
  mPendingSpellCheck = nullptr;
  mPendingInitEditorSpellCheckCallback = nullptr;
  ChangeNumPendingSpellChecks(-1);

  
  
  
  
  return SpellCheckRange(nullptr);
}




void
mozInlineSpellChecker::ChangeNumPendingSpellChecks(int32_t aDelta,
                                                   nsIEditor* aEditor)
{
  int8_t oldNumPending = mNumPendingSpellChecks;
  mNumPendingSpellChecks += aDelta;
  NS_ASSERTION(mNumPendingSpellChecks >= 0,
               "Unbalanced ChangeNumPendingSpellChecks calls!");
  if (oldNumPending == 0 && mNumPendingSpellChecks > 0) {
    NotifyObservers(INLINESPELL_STARTED_TOPIC, aEditor);
  } else if (oldNumPending > 0 && mNumPendingSpellChecks == 0) {
    NotifyObservers(INLINESPELL_ENDED_TOPIC, aEditor);
  }
}



void
mozInlineSpellChecker::NotifyObservers(const char* aTopic, nsIEditor* aEditor)
{
  nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
  if (!os)
    return;
  nsCOMPtr<nsIEditor> editor = aEditor;
  if (!editor) {
    editor = do_QueryReferent(mEditor);
  }
  os->NotifyObservers(editor, aTopic, nullptr);
}










NS_IMETHODIMP
mozInlineSpellChecker::SpellCheckAfterEditorChange(
    int32_t aAction, nsISelection *aSelection,
    nsIDOMNode *aPreviousSelectedNode, int32_t aPreviousSelectedOffset,
    nsIDOMNode *aStartNode, int32_t aStartOffset,
    nsIDOMNode *aEndNode, int32_t aEndOffset)
{
  nsresult rv;
  NS_ENSURE_ARG_POINTER(aSelection);
  if (!mSpellCheck)
    return NS_OK; 

  
  
  mNeedsCheckAfterNavigation = true;

  
  nsCOMPtr<nsIDOMNode> anchorNode;
  rv = aSelection->GetAnchorNode(getter_AddRefs(anchorNode));
  NS_ENSURE_SUCCESS(rv, rv);
  int32_t anchorOffset;
  rv = aSelection->GetAnchorOffset(&anchorOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  mozInlineSpellStatus status(this);
  rv = status.InitForEditorChange((EditAction)aAction,
                                  anchorNode, anchorOffset,
                                  aPreviousSelectedNode, aPreviousSelectedOffset,
                                  aStartNode, aStartOffset,
                                  aEndNode, aEndOffset);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = ScheduleSpellCheck(status);
  NS_ENSURE_SUCCESS(rv, rv);

  
  SaveCurrentSelectionPosition();
  return NS_OK;
}






nsresult
mozInlineSpellChecker::SpellCheckRange(nsIDOMRange* aRange)
{
  if (!mSpellCheck) {
    NS_WARN_IF_FALSE(mPendingSpellCheck,
                     "Trying to spellcheck, but checking seems to be disabled");
    return NS_ERROR_NOT_INITIALIZED;
  }

  mozInlineSpellStatus status(this);
  nsRange* range = static_cast<nsRange*>(aRange);
  nsresult rv = status.InitForRange(range);
  NS_ENSURE_SUCCESS(rv, rv);
  return ScheduleSpellCheck(status);
}



NS_IMETHODIMP
mozInlineSpellChecker::GetMisspelledWord(nsIDOMNode *aNode, int32_t aOffset,
                                         nsIDOMRange **newword)
{
  NS_ENSURE_ARG_POINTER(aNode);
  nsCOMPtr<nsISelection> spellCheckSelection;
  nsresult res = GetSpellCheckSelection(getter_AddRefs(spellCheckSelection));
  NS_ENSURE_SUCCESS(res, res); 

  return IsPointInSelection(spellCheckSelection, aNode, aOffset, newword);
}



NS_IMETHODIMP
mozInlineSpellChecker::ReplaceWord(nsIDOMNode *aNode, int32_t aOffset,
                                   const nsAString &newword)
{
  nsCOMPtr<nsIEditor> editor (do_QueryReferent(mEditor));
  NS_ENSURE_TRUE(editor, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(newword.Length() != 0, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDOMRange> range;
  nsresult res = GetMisspelledWord(aNode, aOffset, getter_AddRefs(range));
  NS_ENSURE_SUCCESS(res, res); 

  if (range)
  {
    editor->BeginTransaction();
  
    nsCOMPtr<nsISelection> selection;
    res = editor->GetSelection(getter_AddRefs(selection));
    NS_ENSURE_SUCCESS(res, res);
    selection->RemoveAllRanges();
    selection->AddRange(range);
    editor->DeleteSelection(nsIEditor::eNone, nsIEditor::eStrip);

    nsCOMPtr<nsIPlaintextEditor> textEditor(do_QueryReferent(mEditor));
    if (textEditor)
      textEditor->InsertText(newword);

    editor->EndTransaction();
  }

  return NS_OK;
}



NS_IMETHODIMP
mozInlineSpellChecker::AddWordToDictionary(const nsAString &word)
{
  NS_ENSURE_TRUE(mSpellCheck, NS_ERROR_NOT_INITIALIZED);

  nsAutoString wordstr(word);
  nsresult rv = mSpellCheck->AddWordToDictionary(wordstr.get());
  NS_ENSURE_SUCCESS(rv, rv); 

  mozInlineSpellStatus status(this);
  rv = status.InitForSelection();
  NS_ENSURE_SUCCESS(rv, rv);
  return ScheduleSpellCheck(status);
}



NS_IMETHODIMP
mozInlineSpellChecker::RemoveWordFromDictionary(const nsAString &word)
{
  NS_ENSURE_TRUE(mSpellCheck, NS_ERROR_NOT_INITIALIZED);

  nsAutoString wordstr(word);
  nsresult rv = mSpellCheck->RemoveWordFromDictionary(wordstr.get());
  NS_ENSURE_SUCCESS(rv, rv); 
  
  mozInlineSpellStatus status(this);
  rv = status.InitForRange(nullptr);
  NS_ENSURE_SUCCESS(rv, rv);
  return ScheduleSpellCheck(status);
}



NS_IMETHODIMP
mozInlineSpellChecker::IgnoreWord(const nsAString &word)
{
  NS_ENSURE_TRUE(mSpellCheck, NS_ERROR_NOT_INITIALIZED);

  nsAutoString wordstr(word);
  nsresult rv = mSpellCheck->IgnoreWordAllOccurrences(wordstr.get());
  NS_ENSURE_SUCCESS(rv, rv); 

  mozInlineSpellStatus status(this);
  rv = status.InitForSelection();
  NS_ENSURE_SUCCESS(rv, rv);
  return ScheduleSpellCheck(status);
}



NS_IMETHODIMP
mozInlineSpellChecker::IgnoreWords(const char16_t **aWordsToIgnore,
                                   uint32_t aCount)
{
  NS_ENSURE_TRUE(mSpellCheck, NS_ERROR_NOT_INITIALIZED);

  
  for (uint32_t index = 0; index < aCount; index++)
    mSpellCheck->IgnoreWordAllOccurrences(aWordsToIgnore[index]);

  mozInlineSpellStatus status(this);
  nsresult rv = status.InitForSelection();
  NS_ENSURE_SUCCESS(rv, rv);
  return ScheduleSpellCheck(status);
}

NS_IMETHODIMP mozInlineSpellChecker::WillCreateNode(const nsAString & aTag, nsIDOMNode *aParent, int32_t aPosition)
{
  return NS_OK;
}

NS_IMETHODIMP mozInlineSpellChecker::DidCreateNode(const nsAString & aTag, nsIDOMNode *aNode, nsIDOMNode *aParent,
                                                   int32_t aPosition, nsresult aResult)
{
  return NS_OK;
}

NS_IMETHODIMP mozInlineSpellChecker::WillInsertNode(nsIDOMNode *aNode, nsIDOMNode *aParent,
                                                    int32_t aPosition)
{
  return NS_OK;
}

NS_IMETHODIMP mozInlineSpellChecker::DidInsertNode(nsIDOMNode *aNode, nsIDOMNode *aParent,
                                                   int32_t aPosition, nsresult aResult)
{

  return NS_OK;
}

NS_IMETHODIMP mozInlineSpellChecker::WillDeleteNode(nsIDOMNode *aChild)
{
  return NS_OK;
}

NS_IMETHODIMP mozInlineSpellChecker::DidDeleteNode(nsIDOMNode *aChild, nsresult aResult)
{
  return NS_OK;
}

NS_IMETHODIMP mozInlineSpellChecker::WillSplitNode(nsIDOMNode *aExistingRightNode, int32_t aOffset)
{
  return NS_OK;
}

NS_IMETHODIMP
mozInlineSpellChecker::DidSplitNode(nsIDOMNode *aExistingRightNode,
                                    int32_t aOffset,
                                    nsIDOMNode *aNewLeftNode, nsresult aResult)
{
  return SpellCheckBetweenNodes(aNewLeftNode, 0, aNewLeftNode, 0);
}

NS_IMETHODIMP mozInlineSpellChecker::WillJoinNodes(nsIDOMNode *aLeftNode, nsIDOMNode *aRightNode, nsIDOMNode *aParent)
{
  return NS_OK;
}

NS_IMETHODIMP mozInlineSpellChecker::DidJoinNodes(nsIDOMNode *aLeftNode, nsIDOMNode *aRightNode, 
                                                  nsIDOMNode *aParent, nsresult aResult)
{
  return SpellCheckBetweenNodes(aRightNode, 0, aRightNode, 0);
}

NS_IMETHODIMP mozInlineSpellChecker::WillInsertText(nsIDOMCharacterData *aTextNode, int32_t aOffset, const nsAString & aString)
{
  return NS_OK;
}

NS_IMETHODIMP mozInlineSpellChecker::DidInsertText(nsIDOMCharacterData *aTextNode, int32_t aOffset,
                                                   const nsAString & aString, nsresult aResult)
{
  return NS_OK;
}

NS_IMETHODIMP mozInlineSpellChecker::WillDeleteText(nsIDOMCharacterData *aTextNode, int32_t aOffset, int32_t aLength)
{
  return NS_OK;
}

NS_IMETHODIMP mozInlineSpellChecker::DidDeleteText(nsIDOMCharacterData *aTextNode, int32_t aOffset, int32_t aLength, nsresult aResult)
{
  return NS_OK;
}

NS_IMETHODIMP mozInlineSpellChecker::WillDeleteSelection(nsISelection *aSelection)
{
  return NS_OK;
}

NS_IMETHODIMP mozInlineSpellChecker::DidDeleteSelection(nsISelection *aSelection)
{
  return NS_OK;
}











nsresult
mozInlineSpellChecker::MakeSpellCheckRange(
    nsIDOMNode* aStartNode, int32_t aStartOffset,
    nsIDOMNode* aEndNode, int32_t aEndOffset,
    nsRange** aRange)
{
  nsresult rv;
  *aRange = nullptr;

  nsCOMPtr<nsIEditor> editor (do_QueryReferent(mEditor));
  NS_ENSURE_TRUE(editor, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsIDOMDocument> doc;
  rv = editor->GetDocument(getter_AddRefs(doc));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(doc, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDOMRange> range;
  rv = doc->CreateRange(getter_AddRefs(range));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIDOMElement> rootElem;
  if (! aStartNode || ! aEndNode) {
    rv = editor->GetRootElement(getter_AddRefs(rootElem));
    NS_ENSURE_SUCCESS(rv, rv);

    aStartNode = rootElem;
    aStartOffset = 0;

    aEndNode = rootElem;
    aEndOffset = -1;
  }

  if (aEndOffset == -1) {
    nsCOMPtr<nsIDOMNodeList> childNodes;
    rv = aEndNode->GetChildNodes(getter_AddRefs(childNodes));
    NS_ENSURE_SUCCESS(rv, rv);

    uint32_t childCount;
    rv = childNodes->GetLength(&childCount);
    NS_ENSURE_SUCCESS(rv, rv);

    aEndOffset = childCount;
  }

  
  
  if (aStartNode == aEndNode && aStartOffset == aEndOffset)
    return NS_OK;

  rv = range->SetStart(aStartNode, aStartOffset);
  NS_ENSURE_SUCCESS(rv, rv);
  if (aEndOffset)
    rv = range->SetEnd(aEndNode, aEndOffset);
  else
    rv = range->SetEndAfter(aEndNode);
  NS_ENSURE_SUCCESS(rv, rv);

  *aRange = static_cast<nsRange*>(range.forget().take());
  return NS_OK;
}

nsresult
mozInlineSpellChecker::SpellCheckBetweenNodes(nsIDOMNode *aStartNode,
                                              int32_t aStartOffset,
                                              nsIDOMNode *aEndNode,
                                              int32_t aEndOffset)
{
  nsRefPtr<nsRange> range;
  nsresult rv = MakeSpellCheckRange(aStartNode, aStartOffset,
                                    aEndNode, aEndOffset,
                                    getter_AddRefs(range));
  NS_ENSURE_SUCCESS(rv, rv);

  if (! range)
    return NS_OK; 

  mozInlineSpellStatus status(this);
  rv = status.InitForRange(range);
  NS_ENSURE_SUCCESS(rv, rv);
  return ScheduleSpellCheck(status);
}







bool
mozInlineSpellChecker::ShouldSpellCheckNode(nsIEditor* aEditor,
                                            nsINode *aNode)
{
  MOZ_ASSERT(aNode);
  if (!aNode->IsContent())
    return true;

  nsIContent *content = aNode->AsContent();

  uint32_t flags;
  aEditor->GetFlags(&flags);
  if (flags & nsIPlaintextEditor::eEditorMailMask) {
    nsIContent *parent = content->GetParent();
    while (parent) {
      if (parent->IsHTMLElement(nsGkAtoms::blockquote) &&
          parent->AttrValueIs(kNameSpaceID_None,
                              nsGkAtoms::type,
                              nsGkAtoms::cite,
                              eIgnoreCase)) {
        return false;
      }
      if (parent->IsHTMLElement(nsGkAtoms::pre) &&
          parent->AttrValueIs(kNameSpaceID_None,
                              nsGkAtoms::_class,
                              nsGkAtoms::mozsignature,
                              eIgnoreCase)) {
        return false;
      }

      parent = parent->GetParent();
    }
  } else {
    
    
    if (!content->IsEditable()) {
      return false;
    }

    
    
    
    if (content->IsInAnonymousSubtree()) {
      nsIContent *node = content->GetParent();
      while (node && node->IsInNativeAnonymousSubtree()) {
        node = node->GetParent();
      }
      nsCOMPtr<nsITextControlElement> textControl = do_QueryInterface(node);
      if (textControl) {
        return true;
      }
    }

    
    
    nsIContent *parent = content;
    while (!parent->IsHTMLElement()) {
      parent = parent->GetParent();
      if (!parent) {
        return true;
      }
    }

    
    return static_cast<nsGenericHTMLElement *>(parent)->Spellcheck();
  }

  return true;
}






nsresult
mozInlineSpellChecker::ScheduleSpellCheck(const mozInlineSpellStatus& aStatus)
{
  if (mFullSpellCheckScheduled) {
    
    return NS_OK;
  }

  nsRefPtr<mozInlineSpellResume> resume =
    new mozInlineSpellResume(aStatus, mDisabledAsyncToken);
  NS_ENSURE_TRUE(resume, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = resume->Post();
  if (NS_SUCCEEDED(rv)) {
    if (aStatus.IsFullSpellCheck()) {
      
      
      mFullSpellCheckScheduled = true;
    }
    ChangeNumPendingSpellChecks(1);
  }
  return rv;
}












nsresult
mozInlineSpellChecker::DoSpellCheckSelection(mozInlineSpellWordUtil& aWordUtil,
                                             Selection* aSpellCheckSelection,
                                             mozInlineSpellStatus* aStatus)
{
  nsresult rv;

  
  mNumWordsInSpellSelection = 0;

  
  
  
  nsTArray<nsRefPtr<nsRange>> ranges;

  int32_t count = aSpellCheckSelection->RangeCount();

  for (int32_t idx = 0; idx < count; idx++) {
    nsRange *range = aSpellCheckSelection->GetRangeAt(idx);
    if (range) {
      ranges.AppendElement(range);
    }
  }

  
  
  
  
  
  aSpellCheckSelection->RemoveAllRanges();

  
  
  
  mozInlineSpellStatus status(this);
  rv = status.InitForRange(nullptr);
  NS_ENSURE_SUCCESS(rv, rv);

  bool doneChecking;
  for (int32_t idx = 0; idx < count; idx++) {
    
    
    
    
    status.mRange = ranges[idx];
    rv = DoSpellCheck(aWordUtil, aSpellCheckSelection, &status,
                      &doneChecking);
    NS_ENSURE_SUCCESS(rv, rv);
    MOZ_ASSERT(doneChecking,
               "We gave the spellchecker one word, but it didn't finish checking?!?!");

    status.mWordCount = 0;
  }

  return NS_OK;
}
































nsresult mozInlineSpellChecker::DoSpellCheck(mozInlineSpellWordUtil& aWordUtil,
                                             Selection *aSpellCheckSelection,
                                             mozInlineSpellStatus* aStatus,
                                             bool* aDoneChecking)
{
  *aDoneChecking = true;

  NS_ENSURE_TRUE(mSpellCheck, NS_ERROR_NOT_INITIALIZED);

  
  
  nsCOMPtr<nsIEditor> editor (do_QueryReferent(mEditor));
  if (! editor)
    return NS_ERROR_FAILURE;

  if (aStatus->mRange->Collapsed())
    return NS_OK;

  
  
  
  int32_t originalRangeCount = aSpellCheckSelection->RangeCount();

  
  {
    
    
    nsINode* beginNode = aStatus->mRange->GetStartParent();
    int32_t beginOffset = aStatus->mRange->StartOffset();
    nsINode* endNode = aStatus->mRange->GetEndParent();
    int32_t endOffset = aStatus->mRange->EndOffset();

    
    
    nsINode* rootNode = aWordUtil.GetRootNode();
    if (!nsContentUtils::ContentIsDescendantOf(beginNode, rootNode) ||
        !nsContentUtils::ContentIsDescendantOf(endNode, rootNode)) {
      
      return NS_OK;
    }

    aWordUtil.SetEnd(endNode, endOffset);
    aWordUtil.SetPosition(beginNode, beginOffset);
  }

  
  
  editor = do_QueryReferent(mEditor);
  if (! editor)
    return NS_ERROR_FAILURE;

  int32_t wordsSinceTimeCheck = 0;
  PRTime beginTime = PR_Now();

  nsAutoString wordText;
  nsRefPtr<nsRange> wordRange;
  bool dontCheckWord;
  while (NS_SUCCEEDED(aWordUtil.GetNextWord(wordText,
                                            getter_AddRefs(wordRange),
                                            &dontCheckWord)) &&
         wordRange) {
    wordsSinceTimeCheck++;

    
    nsINode *beginNode;
    nsINode *endNode;
    int32_t beginOffset, endOffset;

    ErrorResult erv;
    beginNode = wordRange->GetStartContainer(erv);
    endNode = wordRange->GetEndContainer(erv);
    beginOffset = wordRange->GetStartOffset(erv);
    endOffset = wordRange->GetEndOffset(erv);

#ifdef DEBUG_INLINESPELL
    printf("->Got word \"%s\"", NS_ConvertUTF16toUTF8(wordText).get());
    if (dontCheckWord)
      printf(" (not checking)");
    printf("\n");
#endif

    
    
    
    if (originalRangeCount > 0) {
      
      if (!aStatus->mCreatedRange ||
          !aStatus->mCreatedRange->IsPointInRange(*beginNode, beginOffset, erv)) {
        nsTArray<nsRefPtr<nsRange>> ranges;
        aSpellCheckSelection->GetRangesForInterval(*beginNode, beginOffset,
                                                   *endNode, endOffset,
                                                   true, ranges, erv);
        ENSURE_SUCCESS(erv, erv.ErrorCode());
        for (uint32_t i = 0; i < ranges.Length(); i++)
          RemoveRange(aSpellCheckSelection, ranges[i]);
      }
    }

    
    if (dontCheckWord)
      continue;

    
    if (!ShouldSpellCheckNode(editor, beginNode))
      continue;

    
    
    
    
    
    
    
    if (aStatus->mNoCheckRange &&
        aStatus->mNoCheckRange->IsPointInRange(*beginNode, beginOffset, erv)) {
      continue;
    }

    
    bool isMisspelled;
    aWordUtil.NormalizeWord(wordText);
    nsresult rv = mSpellCheck->CheckCurrentWordNoSuggest(wordText.get(), &isMisspelled);
    if (NS_FAILED(rv))
      continue;

    if (isMisspelled) {
      
      wordsSinceTimeCheck += MISSPELLED_WORD_COUNT_PENALTY;
      AddRange(aSpellCheckSelection, wordRange);

      aStatus->mWordCount ++;
      if (aStatus->mWordCount >= mMaxMisspellingsPerCheck ||
          SpellCheckSelectionIsFull())
        break;
    }

    
    if (wordsSinceTimeCheck >= INLINESPELL_TIMEOUT_CHECK_FREQUENCY) {
      wordsSinceTimeCheck = 0;
      if (PR_Now() > PRTime(beginTime + INLINESPELL_CHECK_TIMEOUT * PR_USEC_PER_MSEC)) {
        

        
        rv = aStatus->mRange->SetStart(endNode, endOffset);
        if (NS_FAILED(rv)) {
          
          
          
          return NS_OK;
        }
        *aDoneChecking = false;
        return NS_OK;
      }
    }
  }

  return NS_OK;
}


class AutoChangeNumPendingSpellChecks
{
public:
  AutoChangeNumPendingSpellChecks(mozInlineSpellChecker* aSpellChecker,
                                  int32_t aDelta)
    : mSpellChecker(aSpellChecker), mDelta(aDelta) {}

  ~AutoChangeNumPendingSpellChecks()
  {
    mSpellChecker->ChangeNumPendingSpellChecks(mDelta);
  }

private:
  nsRefPtr<mozInlineSpellChecker> mSpellChecker;
  int32_t mDelta;
};






nsresult
mozInlineSpellChecker::ResumeCheck(mozInlineSpellStatus* aStatus)
{
  
  
  
  
  AutoChangeNumPendingSpellChecks autoChangeNumPending(this, -1);

  if (aStatus->IsFullSpellCheck()) {
    
    
    NS_ASSERTION(mFullSpellCheckScheduled,
                 "How could this be false?  The full spell check is "
                 "calling us!!");
    mFullSpellCheckScheduled = false;
  }

  if (! mSpellCheck)
    return NS_OK; 

  nsCOMPtr<nsIEditor> editor = do_QueryReferent(mEditor);
  if (! editor)
    return NS_OK; 

  mozInlineSpellWordUtil wordUtil;
  nsresult rv = wordUtil.Init(mEditor);
  if (NS_FAILED(rv))
    return NS_OK; 

  nsCOMPtr<nsISelection> spellCheckSelectionRef;
  rv = GetSpellCheckSelection(getter_AddRefs(spellCheckSelectionRef));
  NS_ENSURE_SUCCESS(rv, rv);

  auto spellCheckSelection =
    static_cast<Selection *>(spellCheckSelectionRef.get());

  nsAutoString currentDictionary;
  rv = mSpellCheck->GetCurrentDictionary(currentDictionary);
  if (NS_FAILED(rv)) {
    
    int32_t count = spellCheckSelection->RangeCount();
    for (int32_t index = count - 1; index >= 0; index--) {
      nsRange *checkRange = spellCheckSelection->GetRangeAt(index);
      if (checkRange) {
        RemoveRange(spellCheckSelection, checkRange);
      }
    }
    return NS_OK;
  }

  CleanupRangesInSelection(spellCheckSelection);

  rv = aStatus->FinishInitOnEvent(wordUtil);
  NS_ENSURE_SUCCESS(rv, rv);
  if (! aStatus->mRange)
    return NS_OK; 

  bool doneChecking = true;
  if (aStatus->mOp == mozInlineSpellStatus::eOpSelection)
    rv = DoSpellCheckSelection(wordUtil, spellCheckSelection, aStatus);
  else
    rv = DoSpellCheck(wordUtil, spellCheckSelection, aStatus, &doneChecking);
  NS_ENSURE_SUCCESS(rv, rv);

  if (! doneChecking)
    rv = ScheduleSpellCheck(*aStatus);
  return rv;
}










nsresult
mozInlineSpellChecker::IsPointInSelection(nsISelection *aSelection,
                                          nsIDOMNode *aNode,
                                          int32_t aOffset,
                                          nsIDOMRange **aRange)
{
  *aRange = nullptr;

  nsCOMPtr<nsISelectionPrivate> privSel(do_QueryInterface(aSelection));

  nsTArray<nsRange*> ranges;
  nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
  nsresult rv = privSel->GetRangesForIntervalArray(node, aOffset, node, aOffset,
                                                   true, &ranges);
  NS_ENSURE_SUCCESS(rv, rv);

  if (ranges.Length() == 0)
    return NS_OK; 

  
  
  NS_ADDREF(*aRange = ranges[0]);
  return NS_OK;
}

nsresult
mozInlineSpellChecker::CleanupRangesInSelection(Selection *aSelection)
{
  
  
  if (!aSelection)
    return NS_ERROR_FAILURE;

  int32_t count = aSelection->RangeCount();

  for (int32_t index = 0; index < count; index++)
  {
    nsRange *checkRange = aSelection->GetRangeAt(index);
    if (checkRange)
    {
      if (checkRange->Collapsed())
      {
        RemoveRange(aSelection, checkRange);
        index--;
        count--;
      }
    }
  }

  return NS_OK;
}








nsresult
mozInlineSpellChecker::RemoveRange(Selection *aSpellCheckSelection,
                                   nsRange *aRange)
{
  NS_ENSURE_ARG_POINTER(aSpellCheckSelection);
  NS_ENSURE_ARG_POINTER(aRange);

  ErrorResult rv;
  aSpellCheckSelection->RemoveRange(*aRange, rv);
  if (!rv.Failed() && mNumWordsInSpellSelection)
    mNumWordsInSpellSelection--;

  return rv.StealNSResult();
}








nsresult
mozInlineSpellChecker::AddRange(nsISelection* aSpellCheckSelection,
                                nsIDOMRange* aRange)
{
  NS_ENSURE_ARG_POINTER(aSpellCheckSelection);
  NS_ENSURE_ARG_POINTER(aRange);

  nsresult rv = NS_OK;

  if (!SpellCheckSelectionIsFull())
  {
    rv = aSpellCheckSelection->AddRange(aRange);
    if (NS_SUCCEEDED(rv))
      mNumWordsInSpellSelection++;
  }

  return rv;
}

nsresult mozInlineSpellChecker::GetSpellCheckSelection(nsISelection ** aSpellCheckSelection)
{ 
  nsCOMPtr<nsIEditor> editor (do_QueryReferent(mEditor));
  NS_ENSURE_TRUE(editor, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsISelectionController> selcon;
  nsresult rv = editor->GetSelectionController(getter_AddRefs(selcon));
  NS_ENSURE_SUCCESS(rv, rv); 

  nsCOMPtr<nsISelection> spellCheckSelection;
  return selcon->GetSelection(nsISelectionController::SELECTION_SPELLCHECK, aSpellCheckSelection);
}

nsresult mozInlineSpellChecker::SaveCurrentSelectionPosition()
{
  nsCOMPtr<nsIEditor> editor (do_QueryReferent(mEditor));
  NS_ENSURE_TRUE(editor, NS_OK);

  
  nsCOMPtr<nsISelection> selection;
  nsresult rv = editor->GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selection->GetFocusNode(getter_AddRefs(mCurrentSelectionAnchorNode));
  NS_ENSURE_SUCCESS(rv, rv);
  
  selection->GetFocusOffset(&mCurrentSelectionOffset);

  return NS_OK;
}



bool 
ContentIsDescendantOf(nsINode* aPossibleDescendant,
                      nsINode* aPossibleAncestor)
{
  NS_PRECONDITION(aPossibleDescendant, "The possible descendant is null!");
  NS_PRECONDITION(aPossibleAncestor, "The possible ancestor is null!");

  do {
    if (aPossibleDescendant == aPossibleAncestor)
      return true;
    aPossibleDescendant = aPossibleDescendant->GetParentNode();
  } while (aPossibleDescendant);

  return false;
}














nsresult
mozInlineSpellChecker::HandleNavigationEvent(bool aForceWordSpellCheck,
                                             int32_t aNewPositionOffset)
{
  nsresult rv;

  
  
  
  
  if (! mNeedsCheckAfterNavigation)
    return NS_OK;

  nsCOMPtr<nsIDOMNode> currentAnchorNode = mCurrentSelectionAnchorNode;
  int32_t currentAnchorOffset = mCurrentSelectionOffset;

  
  rv = SaveCurrentSelectionPosition();
  NS_ENSURE_SUCCESS(rv, rv);

  bool shouldPost;
  mozInlineSpellStatus status(this);
  rv = status.InitForNavigation(aForceWordSpellCheck, aNewPositionOffset,
                                currentAnchorNode, currentAnchorOffset,
                                mCurrentSelectionAnchorNode, mCurrentSelectionOffset,
                                &shouldPost);
  NS_ENSURE_SUCCESS(rv, rv);
  if (shouldPost) {
    rv = ScheduleSpellCheck(status);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP mozInlineSpellChecker::HandleEvent(nsIDOMEvent* aEvent)
{
  nsAutoString eventType;
  aEvent->GetType(eventType);

  if (eventType.EqualsLiteral("blur")) {
    return Blur(aEvent);
  }
  if (eventType.EqualsLiteral("click")) {
    return MouseClick(aEvent);
  }
  if (eventType.EqualsLiteral("keypress")) {
    return KeyPress(aEvent);
  }

  return NS_OK;
}

nsresult mozInlineSpellChecker::Blur(nsIDOMEvent* aEvent)
{
  
  HandleNavigationEvent(true);
  return NS_OK;
}

nsresult mozInlineSpellChecker::MouseClick(nsIDOMEvent *aMouseEvent)
{
  nsCOMPtr<nsIDOMMouseEvent>mouseEvent = do_QueryInterface(aMouseEvent);
  NS_ENSURE_TRUE(mouseEvent, NS_OK);

  
  
  int16_t button;
  mouseEvent->GetButton(&button);
  HandleNavigationEvent(button != 0);
  return NS_OK;
}

nsresult mozInlineSpellChecker::KeyPress(nsIDOMEvent* aKeyEvent)
{
  nsCOMPtr<nsIDOMKeyEvent>keyEvent = do_QueryInterface(aKeyEvent);
  NS_ENSURE_TRUE(keyEvent, NS_OK);

  uint32_t keyCode;
  keyEvent->GetKeyCode(&keyCode);

  
  switch (keyCode)
  {
    case nsIDOMKeyEvent::DOM_VK_RIGHT:
    case nsIDOMKeyEvent::DOM_VK_LEFT:
      HandleNavigationEvent(false, keyCode == nsIDOMKeyEvent::DOM_VK_RIGHT ? 1 : -1);
      break;
    case nsIDOMKeyEvent::DOM_VK_UP:
    case nsIDOMKeyEvent::DOM_VK_DOWN:
    case nsIDOMKeyEvent::DOM_VK_HOME:
    case nsIDOMKeyEvent::DOM_VK_END:
    case nsIDOMKeyEvent::DOM_VK_PAGE_UP:
    case nsIDOMKeyEvent::DOM_VK_PAGE_DOWN:
      HandleNavigationEvent(true );
      break;
  }

  return NS_OK;
}


class UpdateCurrentDictionaryCallback final : public nsIEditorSpellCheckCallback
{
public:
  NS_DECL_ISUPPORTS

  explicit UpdateCurrentDictionaryCallback(mozInlineSpellChecker* aSpellChecker,
                                           uint32_t aDisabledAsyncToken)
    : mSpellChecker(aSpellChecker), mDisabledAsyncToken(aDisabledAsyncToken) {}

  NS_IMETHOD EditorSpellCheckDone() override
  {
    
    
    return mSpellChecker->mDisabledAsyncToken > mDisabledAsyncToken ?
           NS_OK :
           mSpellChecker->CurrentDictionaryUpdated();
  }

private:
  ~UpdateCurrentDictionaryCallback() {}

  nsRefPtr<mozInlineSpellChecker> mSpellChecker;
  uint32_t mDisabledAsyncToken;
};
NS_IMPL_ISUPPORTS(UpdateCurrentDictionaryCallback, nsIEditorSpellCheckCallback)

NS_IMETHODIMP mozInlineSpellChecker::UpdateCurrentDictionary()
{
  
  
  
  nsCOMPtr<nsIEditorSpellCheck> spellCheck = mSpellCheck ? mSpellCheck :
                                             mPendingSpellCheck;
  if (!spellCheck) {
    return NS_OK;
  }

  if (NS_FAILED(spellCheck->GetCurrentDictionary(mPreviousDictionary))) {
    mPreviousDictionary.Truncate();
  }

  nsRefPtr<UpdateCurrentDictionaryCallback> cb =
    new UpdateCurrentDictionaryCallback(this, mDisabledAsyncToken);
  NS_ENSURE_STATE(cb);
  nsresult rv = spellCheck->UpdateCurrentDictionary(cb);
  if (NS_FAILED(rv)) {
    cb = nullptr;
    NS_ENSURE_SUCCESS(rv, rv);
  }
  mNumPendingUpdateCurrentDictionary++;
  ChangeNumPendingSpellChecks(1);

  return NS_OK;
}


nsresult mozInlineSpellChecker::CurrentDictionaryUpdated()
{
  mNumPendingUpdateCurrentDictionary--;
  NS_ASSERTION(mNumPendingUpdateCurrentDictionary >= 0,
               "CurrentDictionaryUpdated called without corresponding "
               "UpdateCurrentDictionary call!");
  ChangeNumPendingSpellChecks(-1);

  nsAutoString currentDictionary;
  if (!mSpellCheck ||
      NS_FAILED(mSpellCheck->GetCurrentDictionary(currentDictionary))) {
    currentDictionary.Truncate();
  }

  if (!mPreviousDictionary.Equals(currentDictionary)) {
    nsresult rv = SpellCheckRange(nullptr);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP
mozInlineSpellChecker::GetSpellCheckPending(bool* aPending)
{
  *aPending = mNumPendingSpellChecks > 0;
  return NS_OK;
}
