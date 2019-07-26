




#ifndef __mozinlinespellchecker_h__
#define __mozinlinespellchecker_h__

#include "nsAutoPtr.h"
#include "nsRange.h"
#include "nsIEditorSpellCheck.h"
#include "nsIEditActionListener.h"
#include "nsIInlineSpellChecker.h"
#include "nsITextServicesDocument.h"
#include "nsIDOMTreeWalker.h"
#include "nsWeakReference.h"
#include "nsEditor.h"
#include "nsIDOMEventListener.h"
#include "nsWeakReference.h"
#include "mozISpellI18NUtil.h"
#include "nsCycleCollectionParticipant.h"


#ifdef KeyPress
#undef KeyPress
#endif

class nsIDOMMouseEventListener;
class mozInlineSpellWordUtil;
class mozInlineSpellChecker;
class mozInlineSpellResume;

class mozInlineSpellStatus
{
public:
  mozInlineSpellStatus(mozInlineSpellChecker* aSpellChecker);

  nsresult InitForEditorChange(EditAction aAction,
                               nsIDOMNode* aAnchorNode, int32_t aAnchorOffset,
                               nsIDOMNode* aPreviousNode, int32_t aPreviousOffset,
                               nsIDOMNode* aStartNode, int32_t aStartOffset,
                               nsIDOMNode* aEndNode, int32_t aEndOffset);
  nsresult InitForNavigation(bool aForceCheck, int32_t aNewPositionOffset,
                             nsIDOMNode* aOldAnchorNode, int32_t aOldAnchorOffset,
                             nsIDOMNode* aNewAnchorNode, int32_t aNewAnchorOffset,
                             bool* aContinue);
  nsresult InitForSelection();
  nsresult InitForRange(nsRange* aRange);

  nsresult FinishInitOnEvent(mozInlineSpellWordUtil& aWordUtil);

  
  bool IsFullSpellCheck() const {
    return mOp == eOpChange && !mRange;
  }

  nsRefPtr<mozInlineSpellChecker> mSpellChecker;

  
  
  
  int32_t mWordCount;

  
  enum Operation { eOpChange,       
                   eOpChangeDelete, 
                   eOpNavigation,   
                   eOpSelection,    
                   eOpResume };     
  Operation mOp;

  
  
  nsRefPtr<nsRange> mRange;

  
  
  nsCOMPtr<nsIDOMRange> mCreatedRange;

  
  nsRefPtr<nsRange> mNoCheckRange;

  
  
  
  
  
  nsCOMPtr<nsIDOMRange> mAnchorRange;

  
  
  
  

  
  nsCOMPtr<nsIDOMRange> mOldNavigationAnchorRange;

  
  
  
  bool mForceNavigationWordCheck;

  
  int32_t mNewNavigationPositionOffset;

protected:
  nsresult FinishNavigationEvent(mozInlineSpellWordUtil& aWordUtil);

  nsresult FillNoCheckRangeFromAnchor(mozInlineSpellWordUtil& aWordUtil);

  nsresult GetDocument(nsIDOMDocument** aDocument);
  nsresult PositionToCollapsedRange(nsIDOMDocument* aDocument,
                                    nsIDOMNode* aNode, int32_t aOffset,
                                    nsIDOMRange** aRange);
};

class mozInlineSpellChecker : public nsIInlineSpellChecker,
                              public nsIEditActionListener,
                              public nsIDOMEventListener,
                              public nsSupportsWeakReference
{
private:
  friend class mozInlineSpellStatus;

  
  enum SpellCheckingState { SpellCheck_Uninitialized = -1,
                            SpellCheck_NotAvailable = 0,
                            SpellCheck_Available = 1};
  static SpellCheckingState gCanEnableSpellChecking;

  nsWeakPtr mEditor; 
  nsCOMPtr<nsIEditorSpellCheck> mSpellCheck;
  nsCOMPtr<nsITextServicesDocument> mTextServicesDocument;
  nsCOMPtr<nsIDOMTreeWalker> mTreeWalker;
  nsCOMPtr<mozISpellI18NUtil> mConverter;

  int32_t mNumWordsInSpellSelection;
  int32_t mMaxNumWordsInSpellSelection;

  
  
  
  
  
  int32_t mMaxMisspellingsPerCheck;

  
  
  nsCOMPtr<nsIDOMNode> mCurrentSelectionAnchorNode;
  int32_t              mCurrentSelectionOffset;

  
  
  bool mNeedsCheckAfterNavigation;

  
  
  bool mFullSpellCheckScheduled;

public:

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIEDITACTIONLISTENER
  NS_DECL_NSIINLINESPELLCHECKER
  NS_DECL_NSIDOMEVENTLISTENER
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(mozInlineSpellChecker, nsIDOMEventListener)

  
  static bool CanEnableInlineSpellChecking();
  
  static void UpdateCanEnableInlineSpellChecking();

  nsresult Blur(nsIDOMEvent* aEvent);
  nsresult MouseClick(nsIDOMEvent* aMouseEvent);
  nsresult KeyPress(nsIDOMEvent* aKeyEvent);

  mozInlineSpellChecker();
  virtual ~mozInlineSpellChecker();

  
  nsresult SpellCheckBetweenNodes(nsIDOMNode *aStartNode,
                                  int32_t aStartOffset,
                                  nsIDOMNode *aEndNode,
                                  int32_t aEndOffset);

  
  
  
  nsresult SkipSpellCheckForNode(nsIEditor* aEditor,
                                 nsIDOMNode *aNode, bool * aCheckSpelling);

  nsresult SpellCheckAfterChange(nsIDOMNode* aCursorNode, int32_t aCursorOffset,
                                 nsIDOMNode* aPreviousNode, int32_t aPreviousOffset,
                                 nsISelection* aSpellCheckSelection);

  
  
  nsresult ScheduleSpellCheck(const mozInlineSpellStatus& aStatus);

  nsresult DoSpellCheckSelection(mozInlineSpellWordUtil& aWordUtil,
                                 nsISelection* aSpellCheckSelection,
                                 mozInlineSpellStatus* aStatus);
  nsresult DoSpellCheck(mozInlineSpellWordUtil& aWordUtil,
                        nsISelection *aSpellCheckSelection,
                        mozInlineSpellStatus* aStatus,
                        bool* aDoneChecking);

  
  nsresult IsPointInSelection(nsISelection *aSelection,
                              nsIDOMNode *aNode,
                              int32_t aOffset,
                              nsIDOMRange **aRange);

  nsresult CleanupRangesInSelection(nsISelection *aSelection);

  nsresult RemoveRange(nsISelection *aSpellCheckSelection, nsIDOMRange * aRange);
  nsresult AddRange(nsISelection *aSpellCheckSelection, nsIDOMRange * aRange);
  bool     SpellCheckSelectionIsFull() { return mNumWordsInSpellSelection >= mMaxNumWordsInSpellSelection; }

  nsresult MakeSpellCheckRange(nsIDOMNode* aStartNode, int32_t aStartOffset,
                               nsIDOMNode* aEndNode, int32_t aEndOffset,
                               nsRange** aRange);

  
  nsresult RegisterEventListeners();
  nsresult UnregisterEventListeners();
  nsresult HandleNavigationEvent(bool aForceWordSpellCheck, int32_t aNewPositionOffset = 0);

  nsresult GetSpellCheckSelection(nsISelection ** aSpellCheckSelection);
  nsresult SaveCurrentSelectionPosition();

  nsresult ResumeCheck(mozInlineSpellStatus* aStatus);
};

#endif 
