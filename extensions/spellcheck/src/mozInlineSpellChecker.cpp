

































































#include "mozInlineSpellChecker.h"
#include "mozInlineSpellWordUtil.h"
#include "mozISpellI18NManager.h"
#include "nsCOMPtr.h"
#include "nsCRT.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMDocumentRange.h"
#include "nsIDOMElement.h"
#include "nsIDOMEventTarget.h"
#include "nsPIDOMEventTarget.h"
#include "nsIDOMMouseEvent.h"
#include "nsIDOMKeyEvent.h"
#include "nsIDOMNode.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMNSRange.h"
#include "nsIDOMRange.h"
#include "nsIDOMText.h"
#include "nsIPlaintextEditor.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsIRunnable.h"
#include "nsISelection.h"
#include "nsISelection2.h"
#include "nsISelectionController.h"
#include "nsIServiceManager.h"
#include "nsITextServicesFilter.h"
#include "nsString.h"
#include "nsThreadUtils.h"
#include "nsUnicharUtils.h"
#include "nsIContent.h"
#include "nsIEventStateManager.h"
#include "nsIEventListenerManager.h"
#include "nsGUIEvent.h"





#define INLINESPELL_CHECK_TIMEOUT 50





#define INLINESPELL_TIMEOUT_CHECK_FREQUENCY 50






#define MISSPELLED_WORD_COUNT_PENALTY 4


static PRBool ContentIsDescendantOf(nsINode* aPossibleDescendant,
                                    nsINode* aPossibleAncestor);

static const char kMaxSpellCheckSelectionSize[] = "extensions.spellcheck.inline.max-misspellings";

mozInlineSpellStatus::mozInlineSpellStatus(mozInlineSpellChecker* aSpellChecker)
    : mSpellChecker(aSpellChecker), mWordCount(0)
{
}








nsresult
mozInlineSpellStatus::InitForEditorChange(
    PRInt32 aAction,
    nsIDOMNode* aAnchorNode, PRInt32 aAnchorOffset,
    nsIDOMNode* aPreviousNode, PRInt32 aPreviousOffset,
    nsIDOMNode* aStartNode, PRInt32 aStartOffset,
    nsIDOMNode* aEndNode, PRInt32 aEndOffset)
{
  nsresult rv;

  nsCOMPtr<nsIDOMDocumentRange> docRange;
  rv = GetDocumentRange(getter_AddRefs(docRange));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = PositionToCollapsedRange(docRange, aAnchorNode, aAnchorOffset,
                                getter_AddRefs(mAnchorRange));
  NS_ENSURE_SUCCESS(rv, rv);

  if (aAction == mozInlineSpellChecker::kOpDeleteSelection) {
    
    
    
    mOp = eOpChangeDelete;
    mRange = nsnull;
    return NS_OK;
  }

  mOp = eOpChange;

  
  rv = docRange->CreateRange(getter_AddRefs(mRange));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIDOMNSRange> nsrange = do_QueryInterface(mAnchorRange, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  PRInt16 cmpResult;
  rv = nsrange->ComparePoint(aPreviousNode, aPreviousOffset, &cmpResult);
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

  
  
  if (aAction == mozInlineSpellChecker::kOpInsertText)
    mCreatedRange = mRange;

  
  if (aStartNode && aEndNode) {
    nsrange = do_QueryInterface(mRange, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = nsrange->ComparePoint(aStartNode, aStartOffset, &cmpResult);
    NS_ENSURE_SUCCESS(rv, rv);
    if (cmpResult < 0) { 
      rv = mRange->SetStart(aStartNode, aStartOffset);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    rv = nsrange->ComparePoint(aEndNode, aEndOffset, &cmpResult);
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
    PRBool aForceCheck, PRInt32 aNewPositionOffset,
    nsIDOMNode* aOldAnchorNode, PRInt32 aOldAnchorOffset,
    nsIDOMNode* aNewAnchorNode, PRInt32 aNewAnchorOffset,
    PRBool* aContinue)
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
    *aContinue = PR_FALSE;
    return NS_OK;
  }

  nsCOMPtr<nsIDOMDocumentRange> docRange;
  rv = GetDocumentRange(getter_AddRefs(docRange));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = PositionToCollapsedRange(docRange, aOldAnchorNode, aOldAnchorOffset,
                                getter_AddRefs(mOldNavigationAnchorRange));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = PositionToCollapsedRange(docRange, aNewAnchorNode, aNewAnchorOffset,
                                getter_AddRefs(mAnchorRange));
  NS_ENSURE_SUCCESS(rv, rv);

  *aContinue = PR_TRUE;
  return NS_OK;
}






nsresult
mozInlineSpellStatus::InitForSelection()
{
  mOp = eOpSelection;
  return NS_OK;
}






nsresult
mozInlineSpellStatus::InitForRange(nsIDOMRange* aRange)
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
    rv = mSpellChecker->MakeSpellCheckRange(nsnull, 0, nsnull, 0,
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
  PRInt32 newAnchorOffset, oldAnchorOffset;

  
  nsresult rv = mOldNavigationAnchorRange->GetStartContainer(
      getter_AddRefs(oldAnchorNode));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mOldNavigationAnchorRange->GetStartOffset(&oldAnchorOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  nsCOMPtr<nsIDOMRange> oldWord;
  rv = aWordUtil.GetRangeForWord(oldAnchorNode, oldAnchorOffset,
                                 getter_AddRefs(oldWord));
  NS_ENSURE_SUCCESS(rv, rv);

  
  editor = do_QueryReferent(mSpellChecker->mEditor);
  if (! editor)
    return NS_ERROR_FAILURE; 

  nsCOMPtr<nsIDOMNSRange> oldWordNS = do_QueryInterface(oldWord, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mAnchorRange->GetStartContainer(getter_AddRefs(newAnchorNode));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mAnchorRange->GetStartOffset(&newAnchorOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRBool isInRange = PR_FALSE;
  if (! mForceNavigationWordCheck) {
    rv = oldWordNS->IsPointInRange(newAnchorNode,
                                   newAnchorOffset + mNewNavigationPositionOffset,
                                   &isInRange);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (isInRange) {
    
    mRange = nsnull;
  } else {
    
    mRange = oldWord;

    
    
    mSpellChecker->mNeedsCheckAfterNavigation = PR_FALSE;
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

  PRInt32 anchorOffset;
  rv = mAnchorRange->GetStartOffset(&anchorOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  return aWordUtil.GetRangeForWord(anchorNode, anchorOffset,
                                   getter_AddRefs(mNoCheckRange));
}






nsresult
mozInlineSpellStatus::GetDocumentRange(nsIDOMDocumentRange** aDocRange)
{
  nsresult rv;
  *aDocRange = nsnull;
  if (! mSpellChecker->mEditor)
    return NS_ERROR_UNEXPECTED;

  nsCOMPtr<nsIEditor> editor = do_QueryReferent(mSpellChecker->mEditor, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMDocument> domDoc;
  rv = editor->GetDocument(getter_AddRefs(domDoc));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMDocumentRange> docRange = do_QueryInterface(domDoc, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  docRange.swap(*aDocRange);
  return NS_OK;
}







nsresult
mozInlineSpellStatus::PositionToCollapsedRange(nsIDOMDocumentRange* aDocRange,
    nsIDOMNode* aNode, PRInt32 aOffset, nsIDOMRange** aRange)
{
  *aRange = nsnull;
  nsCOMPtr<nsIDOMRange> range;
  nsresult rv = aDocRange->CreateRange(getter_AddRefs(range));
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
  mozInlineSpellResume(const mozInlineSpellStatus& aStatus) : mStatus(aStatus) {}
  mozInlineSpellStatus mStatus;
  nsresult Post()
  {
    return NS_DispatchToMainThread(this);
  }

  NS_IMETHOD Run()
  {
    mStatus.mSpellChecker->ResumeCheck(&mStatus);
    return NS_OK;
  }
};


NS_INTERFACE_MAP_BEGIN(mozInlineSpellChecker)
NS_INTERFACE_MAP_ENTRY(nsIInlineSpellChecker)
NS_INTERFACE_MAP_ENTRY(nsIEditActionListener)
NS_INTERFACE_MAP_ENTRY(nsIDOMFocusListener)
NS_INTERFACE_MAP_ENTRY(nsIDOMMouseListener)
NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMKeyListener)
NS_INTERFACE_MAP_ENTRY(nsIDOMKeyListener)
NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsIDOMEventListener, nsIDOMKeyListener)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(mozInlineSpellChecker)
NS_IMPL_RELEASE(mozInlineSpellChecker)

mozInlineSpellChecker::SpellCheckingState
  mozInlineSpellChecker::gCanEnableSpellChecking =
  mozInlineSpellChecker::SpellCheck_Uninitialized;

mozInlineSpellChecker::mozInlineSpellChecker() :
    mNumWordsInSpellSelection(0),
    mMaxNumWordsInSpellSelection(250),
    mNeedsCheckAfterNavigation(PR_FALSE)
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











nsresult mozInlineSpellChecker::Cleanup(PRBool aDestroyingFrames)
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
  mEditor = nsnull;

  return rv;
}















PRBool 
mozInlineSpellChecker::CanEnableInlineSpellChecking()
{
  nsresult rv;
  if (gCanEnableSpellChecking == SpellCheck_Uninitialized) {
    gCanEnableSpellChecking = SpellCheck_NotAvailable;

    nsCOMPtr<nsIEditorSpellCheck> spellchecker =
      do_CreateInstance("@mozilla.org/editor/editorspellchecker;1", &rv);
    NS_ENSURE_SUCCESS(rv, PR_FALSE);

    PRBool canSpellCheck = PR_FALSE;
    rv = spellchecker->CanSpellCheck(&canSpellCheck);
    NS_ENSURE_SUCCESS(rv, PR_FALSE);

    if (canSpellCheck)
      gCanEnableSpellChecking = SpellCheck_Available;
  }
  return (gCanEnableSpellChecking == SpellCheck_Available);
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

  nsCOMPtr<nsPIDOMEventTarget> piTarget = do_QueryInterface(doc, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsIEventListenerManager* elmP = piTarget->GetListenerManager(PR_TRUE);
  if (elmP) {
    
    elmP->AddEventListenerByIID(static_cast<nsIDOMFocusListener *>(this),
                                NS_GET_IID(nsIDOMFocusListener),
                                NS_EVENT_FLAG_CAPTURE);
  }

  piTarget->AddEventListenerByIID(static_cast<nsIDOMMouseListener*>(this),
                                  NS_GET_IID(nsIDOMMouseListener));
  piTarget->AddEventListenerByIID(static_cast<nsIDOMKeyListener*>(this),
                                  NS_GET_IID(nsIDOMKeyListener));

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
  
  nsCOMPtr<nsPIDOMEventTarget> piTarget = do_QueryInterface(doc);
  NS_ENSURE_TRUE(piTarget, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsIEventListenerManager> elmP =
    piTarget->GetListenerManager(PR_TRUE);
  if (elmP) {
    elmP->RemoveEventListenerByIID(static_cast<nsIDOMFocusListener *>(this),
                                   NS_GET_IID(nsIDOMFocusListener),
                                   NS_EVENT_FLAG_CAPTURE);
  }

  piTarget->RemoveEventListenerByIID(static_cast<nsIDOMMouseListener*>(this),
                                     NS_GET_IID(nsIDOMMouseListener));
  piTarget->RemoveEventListenerByIID(static_cast<nsIDOMKeyListener*>(this),
                                     NS_GET_IID(nsIDOMKeyListener));
  
  return NS_OK;
}



NS_IMETHODIMP
mozInlineSpellChecker::GetEnableRealTimeSpell(PRBool* aEnabled)
{
  NS_ENSURE_ARG_POINTER(aEnabled);
  *aEnabled = mSpellCheck != nsnull;
  return NS_OK;
}



NS_IMETHODIMP
mozInlineSpellChecker::SetEnableRealTimeSpell(PRBool aEnabled)
{
  if (!aEnabled) {
    mSpellCheck = nsnull;
    return Cleanup(PR_FALSE);
  }

  if (!mSpellCheck) {
    nsresult res = NS_OK;
    nsCOMPtr<nsIEditorSpellCheck> spellchecker = do_CreateInstance("@mozilla.org/editor/editorspellchecker;1", &res);
    if (NS_SUCCEEDED(res) && spellchecker)
    {
      nsCOMPtr<nsITextServicesFilter> filter = do_CreateInstance("@mozilla.org/editor/txtsrvfiltermail;1", &res);
      spellchecker->SetFilter(filter);
      nsCOMPtr<nsIEditor> editor (do_QueryReferent(mEditor));
      res = spellchecker->InitSpellChecker(editor, PR_FALSE);
      NS_ENSURE_SUCCESS(res, res);

      nsCOMPtr<nsITextServicesDocument> tsDoc = do_CreateInstance("@mozilla.org/textservices/textservicesdocument;1", &res);
      NS_ENSURE_SUCCESS(res, res);

      res = tsDoc->SetFilter(filter);
      NS_ENSURE_SUCCESS(res, res);

      res = tsDoc->InitWithEditor(editor);
      NS_ENSURE_SUCCESS(res, res);

      mTextServicesDocument = tsDoc;
      mSpellCheck = spellchecker;

      
      RegisterEventListeners();
    }
  }

  
  
  
  
  return SpellCheckRange(nsnull);
}










NS_IMETHODIMP
mozInlineSpellChecker::SpellCheckAfterEditorChange(
    PRInt32 aAction, nsISelection *aSelection,
    nsIDOMNode *aPreviousSelectedNode, PRInt32 aPreviousSelectedOffset,
    nsIDOMNode *aStartNode, PRInt32 aStartOffset,
    nsIDOMNode *aEndNode, PRInt32 aEndOffset)
{
  nsresult rv;
  NS_ENSURE_ARG_POINTER(aSelection);
  if (!mSpellCheck)
    return NS_OK; 

  
  
  mNeedsCheckAfterNavigation = PR_TRUE;

  
  nsCOMPtr<nsIDOMNode> anchorNode;
  rv = aSelection->GetAnchorNode(getter_AddRefs(anchorNode));
  NS_ENSURE_SUCCESS(rv, rv);
  PRInt32 anchorOffset;
  rv = aSelection->GetAnchorOffset(&anchorOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  mozInlineSpellStatus status(this);
  rv = status.InitForEditorChange(aAction,
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
  NS_ENSURE_TRUE(mSpellCheck, NS_ERROR_NOT_INITIALIZED);

  mozInlineSpellStatus status(this);
  nsresult rv = status.InitForRange(aRange);
  NS_ENSURE_SUCCESS(rv, rv);
  return ScheduleSpellCheck(status);
}



NS_IMETHODIMP
mozInlineSpellChecker::GetMisspelledWord(nsIDOMNode *aNode, PRInt32 aOffset,
                                         nsIDOMRange **newword)
{
  NS_ENSURE_ARG_POINTER(aNode);
  nsCOMPtr<nsISelection> spellCheckSelection;
  nsresult res = GetSpellCheckSelection(getter_AddRefs(spellCheckSelection));
  NS_ENSURE_SUCCESS(res, res); 

  return IsPointInSelection(spellCheckSelection, aNode, aOffset, newword);
}



NS_IMETHODIMP
mozInlineSpellChecker::ReplaceWord(nsIDOMNode *aNode, PRInt32 aOffset,
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
    editor->DeleteSelection(nsIEditor::eNone);

    nsCOMPtr<nsIPlaintextEditor> textEditor(do_QueryReferent(mEditor));
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
mozInlineSpellChecker::IgnoreWords(const PRUnichar **aWordsToIgnore,
                                   PRUint32 aCount)
{
  
  for (PRUint32 index = 0; index < aCount; index++)
    mSpellCheck->IgnoreWordAllOccurrences(aWordsToIgnore[index]);

  mozInlineSpellStatus status(this);
  nsresult rv = status.InitForSelection();
  NS_ENSURE_SUCCESS(rv, rv);
  return ScheduleSpellCheck(status);
}

NS_IMETHODIMP mozInlineSpellChecker::WillCreateNode(const nsAString & aTag, nsIDOMNode *aParent, PRInt32 aPosition)
{
  return NS_OK;
}

NS_IMETHODIMP mozInlineSpellChecker::DidCreateNode(const nsAString & aTag, nsIDOMNode *aNode, nsIDOMNode *aParent,
                                                   PRInt32 aPosition, nsresult aResult)
{
  return NS_OK;
}

NS_IMETHODIMP mozInlineSpellChecker::WillInsertNode(nsIDOMNode *aNode, nsIDOMNode *aParent,
                                                    PRInt32 aPosition)
{
  return NS_OK;
}

NS_IMETHODIMP mozInlineSpellChecker::DidInsertNode(nsIDOMNode *aNode, nsIDOMNode *aParent,
                                                   PRInt32 aPosition, nsresult aResult)
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

NS_IMETHODIMP mozInlineSpellChecker::WillSplitNode(nsIDOMNode *aExistingRightNode, PRInt32 aOffset)
{
  return NS_OK;
}

NS_IMETHODIMP
mozInlineSpellChecker::DidSplitNode(nsIDOMNode *aExistingRightNode,
                                    PRInt32 aOffset,
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

NS_IMETHODIMP mozInlineSpellChecker::WillInsertText(nsIDOMCharacterData *aTextNode, PRInt32 aOffset, const nsAString & aString)
{
  return NS_OK;
}

NS_IMETHODIMP mozInlineSpellChecker::DidInsertText(nsIDOMCharacterData *aTextNode, PRInt32 aOffset,
                                                   const nsAString & aString, nsresult aResult)
{
  return NS_OK;
}

NS_IMETHODIMP mozInlineSpellChecker::WillDeleteText(nsIDOMCharacterData *aTextNode, PRInt32 aOffset, PRInt32 aLength)
{
  return NS_OK;
}

NS_IMETHODIMP mozInlineSpellChecker::DidDeleteText(nsIDOMCharacterData *aTextNode, PRInt32 aOffset, PRInt32 aLength, nsresult aResult)
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
    nsIDOMNode* aStartNode, PRInt32 aStartOffset,
    nsIDOMNode* aEndNode, PRInt32 aEndOffset,
    nsIDOMRange** aRange)
{
  nsresult rv;
  *aRange = nsnull;

  nsCOMPtr<nsIEditor> editor (do_QueryReferent(mEditor));
  NS_ENSURE_TRUE(editor, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsIDOMDocument> doc;
  rv = editor->GetDocument(getter_AddRefs(doc));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMDocumentRange> docrange = do_QueryInterface(doc);
  NS_ENSURE_TRUE(docrange, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDOMRange> range;
  rv = docrange->CreateRange(getter_AddRefs(range));
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

    PRUint32 childCount;
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

  range.swap(*aRange);
  return NS_OK;
}

nsresult
mozInlineSpellChecker::SpellCheckBetweenNodes(nsIDOMNode *aStartNode,
                                              PRInt32 aStartOffset,
                                              nsIDOMNode *aEndNode,
                                              PRInt32 aEndOffset)
{
  nsCOMPtr<nsIDOMRange> range;
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







nsresult
mozInlineSpellChecker::SkipSpellCheckForNode(nsIEditor* aEditor,
                                             nsIDOMNode *aNode,
                                             PRBool *checkSpelling)
{
  *checkSpelling = PR_TRUE;
  NS_ENSURE_ARG_POINTER(aNode);

  PRUint32 flags;
  aEditor->GetFlags(&flags);
  if (flags & nsIPlaintextEditor::eEditorMailMask)
  {
    nsCOMPtr<nsIDOMNode> parent;
    aNode->GetParentNode(getter_AddRefs(parent));

    while (parent)
    {
      nsCOMPtr<nsIDOMElement> parentElement = do_QueryInterface(parent);
      if (!parentElement)
        break;

      nsAutoString parentTagName;
      parentElement->GetTagName(parentTagName);

      if (parentTagName.Equals(NS_LITERAL_STRING("blockquote"), nsCaseInsensitiveStringComparator()))
      {
        nsAutoString quotetype;
        parentElement->GetAttribute(NS_LITERAL_STRING("type"), quotetype);
        if (quotetype.Equals(NS_LITERAL_STRING("cite"), nsCaseInsensitiveStringComparator()))
        {
          *checkSpelling = PR_FALSE;
          break;
        }
      }
      else if (parentTagName.Equals(NS_LITERAL_STRING("pre"), nsCaseInsensitiveStringComparator()))
      {
        nsAutoString classname;
        parentElement->GetAttribute(NS_LITERAL_STRING("class"),classname);
        if (classname.Equals(NS_LITERAL_STRING("moz-signature")))
          *checkSpelling = PR_FALSE;
      }

      nsCOMPtr<nsIDOMNode> nextParent;
      parent->GetParentNode(getter_AddRefs(nextParent));
      parent = nextParent;
    }
  }
  else {
    
    nsCOMPtr<nsIContent> content = do_QueryInterface(aNode);
    *checkSpelling = !!(content->IntrinsicState() & NS_EVENT_STATE_MOZ_READWRITE);
  }

  return NS_OK;
}






nsresult
mozInlineSpellChecker::ScheduleSpellCheck(const mozInlineSpellStatus& aStatus)
{
  mozInlineSpellResume* resume = new mozInlineSpellResume(aStatus);
  NS_ENSURE_TRUE(resume, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = resume->Post();
  if (NS_FAILED(rv))
    delete resume;
  return rv;
}












nsresult
mozInlineSpellChecker::DoSpellCheckSelection(mozInlineSpellWordUtil& aWordUtil,
                                             nsISelection* aSpellCheckSelection,
                                             mozInlineSpellStatus* aStatus)
{
  nsresult rv;

  
  mNumWordsInSpellSelection = 0;

  
  
  
  nsCOMArray<nsIDOMRange> ranges;

  PRInt32 count;
  aSpellCheckSelection->GetRangeCount(&count);

  PRInt32 idx;
  nsCOMPtr<nsIDOMRange> checkRange;
  for (idx = 0; idx < count; idx ++) {
    aSpellCheckSelection->GetRangeAt(idx, getter_AddRefs(checkRange));
    if (checkRange) {
      if (! ranges.AppendObject(checkRange))
        return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  
  
  
  
  
  aSpellCheckSelection->RemoveAllRanges();

  
  
  
  mozInlineSpellStatus status(this);
  rv = status.InitForRange(nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool doneChecking;
  for (idx = 0; idx < count; idx ++) {
    checkRange = ranges[idx];
    if (checkRange) {
      
      
      
      
      status.mRange = checkRange;
      rv = DoSpellCheck(aWordUtil, aSpellCheckSelection, &status,
                        &doneChecking);
      NS_ENSURE_SUCCESS(rv, rv);
      NS_ASSERTION(doneChecking, "We gave the spellchecker one word, but it didn't finish checking?!?!");

      status.mWordCount = 0;
    }
  }

  return NS_OK;
}
































nsresult mozInlineSpellChecker::DoSpellCheck(mozInlineSpellWordUtil& aWordUtil,
                                             nsISelection *aSpellCheckSelection,
                                             mozInlineSpellStatus* aStatus,
                                             PRBool* aDoneChecking)
{
  nsCOMPtr<nsIDOMNode> beginNode, endNode;
  PRInt32 beginOffset, endOffset;
  *aDoneChecking = PR_TRUE;

  
  
  nsCOMPtr<nsIEditor> editor (do_QueryReferent(mEditor));
  if (! editor)
    return NS_ERROR_FAILURE;

  PRBool iscollapsed;
  nsresult rv = aStatus->mRange->GetCollapsed(&iscollapsed);
  NS_ENSURE_SUCCESS(rv, rv);
  if (iscollapsed)
    return NS_OK;

  nsCOMPtr<nsISelection2> sel2 = do_QueryInterface(aSpellCheckSelection, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  PRInt32 originalRangeCount;
  rv = aSpellCheckSelection->GetRangeCount(&originalRangeCount);
  NS_ENSURE_SUCCESS(rv, rv);

  
  NS_ENSURE_SUCCESS(rv, rv);
  aStatus->mRange->GetStartContainer(getter_AddRefs(beginNode));
  aStatus->mRange->GetStartOffset(&beginOffset);
  aStatus->mRange->GetEndContainer(getter_AddRefs(endNode));
  aStatus->mRange->GetEndOffset(&endOffset);
  aWordUtil.SetEnd(endNode, endOffset);
  aWordUtil.SetPosition(beginNode, beginOffset);

  
  editor = do_QueryReferent(mEditor);
  if (! editor)
    return NS_ERROR_FAILURE;
  
  
  nsCOMPtr<nsIDOMNSRange> noCheckRange, createdRange;
  if (aStatus->mNoCheckRange)
    noCheckRange = do_QueryInterface(aStatus->mNoCheckRange);
  if (aStatus->mCreatedRange)
    createdRange = do_QueryInterface(aStatus->mCreatedRange);

  PRInt32 wordsSinceTimeCheck = 0;
  PRTime beginTime = PR_Now();

  nsAutoString wordText;
  nsCOMPtr<nsIDOMRange> wordRange;
  PRBool dontCheckWord;
  while (NS_SUCCEEDED(aWordUtil.GetNextWord(wordText,
                                            getter_AddRefs(wordRange),
                                            &dontCheckWord)) &&
         wordRange) {
    wordsSinceTimeCheck ++;

    
    wordRange->GetStartContainer(getter_AddRefs(beginNode));
    wordRange->GetEndContainer(getter_AddRefs(endNode));
    wordRange->GetStartOffset(&beginOffset);
    wordRange->GetEndOffset(&endOffset);

#ifdef DEBUG_INLINESPELL
    printf("->Got word \"%s\"", NS_ConvertUTF16toUTF8(wordText).get());
    if (dontCheckWord)
      printf(" (not checking)");
    printf("\n");
#endif

    
    
    
    if (originalRangeCount > 0) {
      
      PRBool inCreatedRange = PR_FALSE;
      if (createdRange)
        createdRange->IsPointInRange(beginNode, beginOffset, &inCreatedRange);
      if (! inCreatedRange) {
        nsCOMArray<nsIDOMRange> ranges;
        rv = sel2->GetRangesForIntervalCOMArray(beginNode, beginOffset,
                                                endNode, endOffset,
                                                PR_TRUE, &ranges);
        NS_ENSURE_SUCCESS(rv, rv);
        for (PRInt32 i = 0; i < ranges.Count(); i ++)
          RemoveRange(aSpellCheckSelection, ranges[i]);
      }
    }

    
    if (dontCheckWord)
      continue;

    
    PRBool checkSpelling;
    rv = SkipSpellCheckForNode(editor, beginNode, &checkSpelling);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!checkSpelling)
      continue;

    
    
    
    
    
    
    
    if (noCheckRange) {
      PRBool inExclusion = PR_FALSE;
      noCheckRange->IsPointInRange(beginNode, beginOffset, &inExclusion);
      if (inExclusion)
        continue;
    }

    
    PRBool isMisspelled;
    aWordUtil.NormalizeWord(wordText);
    rv = mSpellCheck->CheckCurrentWordNoSuggest(wordText.get(), &isMisspelled);
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
        *aDoneChecking = PR_FALSE;
        return NS_OK;
      }
    }
  }

  return NS_OK;
}






nsresult
mozInlineSpellChecker::ResumeCheck(mozInlineSpellStatus* aStatus)
{
  if (! mSpellCheck)
    return NS_OK; 

  nsCOMPtr<nsIEditor> editor = do_QueryReferent(mEditor);
  if (! editor)
    return NS_OK; 

  mozInlineSpellWordUtil wordUtil;
  nsresult rv = wordUtil.Init(mEditor);
  if (NS_FAILED(rv))
    return NS_OK; 

  nsCOMPtr<nsISelection> spellCheckSelection;
  rv = GetSpellCheckSelection(getter_AddRefs(spellCheckSelection));
  NS_ENSURE_SUCCESS(rv, rv);
  CleanupRangesInSelection(spellCheckSelection);

  rv = aStatus->FinishInitOnEvent(wordUtil);
  NS_ENSURE_SUCCESS(rv, rv);
  if (! aStatus->mRange)
    return NS_OK; 

  PRBool doneChecking = PR_TRUE;
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
                                          PRInt32 aOffset,
                                          nsIDOMRange **aRange)
{
  *aRange = nsnull;

  nsresult rv;
  nsCOMPtr<nsISelection2> sel2 = do_QueryInterface(aSelection, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMArray<nsIDOMRange> ranges;
  rv = sel2->GetRangesForIntervalCOMArray(aNode, aOffset, aNode, aOffset,
                                          PR_TRUE, &ranges);
  NS_ENSURE_SUCCESS(rv, rv);

  if (ranges.Count() == 0)
    return NS_OK; 

  
  
  NS_ADDREF(*aRange = ranges[0]);
  return NS_OK;
}

nsresult
mozInlineSpellChecker::CleanupRangesInSelection(nsISelection *aSelection)
{
  
  
  NS_ENSURE_ARG_POINTER(aSelection);

  PRInt32 count;
  aSelection->GetRangeCount(&count);

  for (PRInt32 index = 0; index < count; index++)
  {
    nsCOMPtr<nsIDOMRange> checkRange;
    aSelection->GetRangeAt(index, getter_AddRefs(checkRange));

    if (checkRange)
    {
      PRBool collapsed;
      checkRange->GetCollapsed(&collapsed);
      if (collapsed)
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
mozInlineSpellChecker::RemoveRange(nsISelection* aSpellCheckSelection,
                                   nsIDOMRange* aRange)
{
  NS_ENSURE_ARG_POINTER(aSpellCheckSelection);
  NS_ENSURE_ARG_POINTER(aRange);

  nsresult rv = aSpellCheckSelection->RemoveRange(aRange);
  if (NS_SUCCEEDED(rv) && mNumWordsInSpellSelection)
    mNumWordsInSpellSelection--;

  return rv;
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



PRBool 
ContentIsDescendantOf(nsINode* aPossibleDescendant,
                      nsINode* aPossibleAncestor)
{
  NS_PRECONDITION(aPossibleDescendant, "The possible descendant is null!");
  NS_PRECONDITION(aPossibleAncestor, "The possible ancestor is null!");

  do {
    if (aPossibleDescendant == aPossibleAncestor)
      return PR_TRUE;
    aPossibleDescendant = aPossibleDescendant->GetNodeParent();
  } while (aPossibleDescendant);

  return PR_FALSE;
}














nsresult
mozInlineSpellChecker::HandleNavigationEvent(nsIDOMEvent* aEvent,
                                             PRBool aForceWordSpellCheck,
                                             PRInt32 aNewPositionOffset)
{
  nsresult rv;

  
  
  
  
  if (! mNeedsCheckAfterNavigation)
    return NS_OK;

  nsCOMPtr<nsIDOMNode> currentAnchorNode = mCurrentSelectionAnchorNode;
  PRInt32 currentAnchorOffset = mCurrentSelectionOffset;

  
  rv = SaveCurrentSelectionPosition();
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool shouldPost;
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
  return NS_OK;
}

NS_IMETHODIMP mozInlineSpellChecker::Focus(nsIDOMEvent* aEvent)
{
  return NS_OK;
}

NS_IMETHODIMP mozInlineSpellChecker::Blur(nsIDOMEvent* aEvent)
{
  
  HandleNavigationEvent(aEvent, PR_TRUE);
  return NS_OK;
}

NS_IMETHODIMP mozInlineSpellChecker::MouseClick(nsIDOMEvent *aMouseEvent)
{
  nsCOMPtr<nsIDOMMouseEvent>mouseEvent = do_QueryInterface(aMouseEvent);
  NS_ENSURE_TRUE(mouseEvent, NS_OK);

  
  
  PRUint16 button;
  mouseEvent->GetButton(&button);
  if (button == 0)
    HandleNavigationEvent(mouseEvent, PR_FALSE);
  else
    HandleNavigationEvent(mouseEvent, PR_TRUE);
  return NS_OK;
}

NS_IMETHODIMP mozInlineSpellChecker::MouseDown(nsIDOMEvent* aMouseEvent)
{
  return NS_OK;
}

NS_IMETHODIMP mozInlineSpellChecker::MouseUp(nsIDOMEvent* aMouseEvent)
{
  return NS_OK;
}

NS_IMETHODIMP mozInlineSpellChecker::MouseDblClick(nsIDOMEvent* aMouseEvent)
{
  return NS_OK;
}

NS_IMETHODIMP mozInlineSpellChecker::MouseOver(nsIDOMEvent* aMouseEvent)
{
  return NS_OK;
}

NS_IMETHODIMP mozInlineSpellChecker::MouseOut(nsIDOMEvent* aMouseEvent)
{
  return NS_OK;
}

NS_IMETHODIMP mozInlineSpellChecker::KeyDown(nsIDOMEvent* aKeyEvent)
{
  return NS_OK;
}


NS_IMETHODIMP mozInlineSpellChecker::KeyUp(nsIDOMEvent* aKeyEvent)
{
  return NS_OK;
}


NS_IMETHODIMP mozInlineSpellChecker::KeyPress(nsIDOMEvent* aKeyEvent)
{
  nsCOMPtr<nsIDOMKeyEvent>keyEvent = do_QueryInterface(aKeyEvent);
  NS_ENSURE_TRUE(keyEvent, NS_OK);

  PRUint32 keyCode;
  keyEvent->GetKeyCode(&keyCode);

  
  switch (keyCode)
  {
    case nsIDOMKeyEvent::DOM_VK_RIGHT:
    case nsIDOMKeyEvent::DOM_VK_LEFT:
      HandleNavigationEvent(aKeyEvent, PR_FALSE, keyCode == nsIDOMKeyEvent::DOM_VK_RIGHT ? 1 : -1);
      break;
    case nsIDOMKeyEvent::DOM_VK_UP:
    case nsIDOMKeyEvent::DOM_VK_DOWN:
    case nsIDOMKeyEvent::DOM_VK_HOME:
    case nsIDOMKeyEvent::DOM_VK_END:
    case nsIDOMKeyEvent::DOM_VK_PAGE_UP:
    case nsIDOMKeyEvent::DOM_VK_PAGE_DOWN:
      HandleNavigationEvent(aKeyEvent, PR_TRUE );
      break;
  }

  return NS_OK;
}
