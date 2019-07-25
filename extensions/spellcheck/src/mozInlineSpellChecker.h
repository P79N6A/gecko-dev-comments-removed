




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
#include "nsIEditor.h"
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

  nsresult InitForEditorChange(PRInt32 aAction,
                               nsIDOMNode* aAnchorNode, PRInt32 aAnchorOffset,
                               nsIDOMNode* aPreviousNode, PRInt32 aPreviousOffset,
                               nsIDOMNode* aStartNode, PRInt32 aStartOffset,
                               nsIDOMNode* aEndNode, PRInt32 aEndOffset);
  nsresult InitForNavigation(bool aForceCheck, PRInt32 aNewPositionOffset,
                             nsIDOMNode* aOldAnchorNode, PRInt32 aOldAnchorOffset,
                             nsIDOMNode* aNewAnchorNode, PRInt32 aNewAnchorOffset,
                             bool* aContinue);
  nsresult InitForSelection();
  nsresult InitForRange(nsRange* aRange);

  nsresult FinishInitOnEvent(mozInlineSpellWordUtil& aWordUtil);

  
  bool IsFullSpellCheck() const {
    return mOp == eOpChange && !mRange;
  }

  nsRefPtr<mozInlineSpellChecker> mSpellChecker;

  
  
  
  PRInt32 mWordCount;

  
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

  
  PRInt32 mNewNavigationPositionOffset;

protected:
  nsresult FinishNavigationEvent(mozInlineSpellWordUtil& aWordUtil);

  nsresult FillNoCheckRangeFromAnchor(mozInlineSpellWordUtil& aWordUtil);

  nsresult GetDocument(nsIDOMDocument** aDocument);
  nsresult PositionToCollapsedRange(nsIDOMDocument* aDocument,
                                    nsIDOMNode* aNode, PRInt32 aOffset,
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

  PRInt32 mNumWordsInSpellSelection;
  PRInt32 mMaxNumWordsInSpellSelection;

  
  
  
  
  
  PRInt32 mMaxMisspellingsPerCheck;

  
  
  nsCOMPtr<nsIDOMNode> mCurrentSelectionAnchorNode;
  PRInt32              mCurrentSelectionOffset;

  
  
  bool mNeedsCheckAfterNavigation;

  
  
  bool mFullSpellCheckScheduled;

  
  
  enum OperationID
  {
    kOpIgnore = -1,
    kOpNone = 0,
    kOpUndo,
    kOpRedo,
    kOpInsertNode,
    kOpCreateNode,
    kOpDeleteNode,
    kOpSplitNode,
    kOpJoinNode,
    kOpDeleteSelection,

    kOpInsertBreak    = 1000,
    kOpInsertText     = 1001,
    kOpInsertIMEText  = 1002,
    kOpDeleteText     = 1003,

    kOpMakeList            = 3001,
    kOpIndent              = 3002,
    kOpOutdent             = 3003,
    kOpAlign               = 3004,
    kOpMakeBasicBlock      = 3005,
    kOpRemoveList          = 3006,
    kOpMakeDefListItem     = 3007,
    kOpInsertElement       = 3008,
    kOpInsertQuotation     = 3009,
    kOpSetTextProperty     = 3010,
    kOpRemoveTextProperty  = 3011,
    kOpHTMLPaste           = 3012,
    kOpLoadHTML            = 3013,
    kOpResetTextProperties = 3014,
    kOpSetAbsolutePosition = 3015,
    kOpRemoveAbsolutePosition = 3016,
    kOpDecreaseZIndex      = 3017,
    kOpIncreaseZIndex      = 3018
  };

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
                                  PRInt32 aStartOffset,
                                  nsIDOMNode *aEndNode,
                                  PRInt32 aEndOffset);

  
  
  
  nsresult SkipSpellCheckForNode(nsIEditor* aEditor,
                                 nsIDOMNode *aNode, bool * aCheckSpelling);

  nsresult SpellCheckAfterChange(nsIDOMNode* aCursorNode, PRInt32 aCursorOffset,
                                 nsIDOMNode* aPreviousNode, PRInt32 aPreviousOffset,
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
                              PRInt32 aOffset,
                              nsIDOMRange **aRange);

  nsresult CleanupRangesInSelection(nsISelection *aSelection);

  nsresult RemoveRange(nsISelection *aSpellCheckSelection, nsIDOMRange * aRange);
  nsresult AddRange(nsISelection *aSpellCheckSelection, nsIDOMRange * aRange);
  bool     SpellCheckSelectionIsFull() { return mNumWordsInSpellSelection >= mMaxNumWordsInSpellSelection; }

  nsresult MakeSpellCheckRange(nsIDOMNode* aStartNode, PRInt32 aStartOffset,
                               nsIDOMNode* aEndNode, PRInt32 aEndOffset,
                               nsRange** aRange);

  
  nsresult RegisterEventListeners();
  nsresult UnregisterEventListeners();
  nsresult HandleNavigationEvent(bool aForceWordSpellCheck, PRInt32 aNewPositionOffset = 0);

  nsresult GetSpellCheckSelection(nsISelection ** aSpellCheckSelection);
  nsresult SaveCurrentSelectionPosition();

  nsresult ResumeCheck(mozInlineSpellStatus* aStatus);
};

#endif 
