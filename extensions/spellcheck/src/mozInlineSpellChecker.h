





































#ifndef __mozinlinespellchecker_h__
#define __mozinlinespellchecker_h__

#include "nsAutoPtr.h"
#include "nsIDOMRange.h"
#include "nsIEditorSpellCheck.h"
#include "nsIEditActionListener.h"
#include "nsIInlineSpellChecker.h"
#include "nsITextServicesDocument.h"
#include "nsIDOMTreeWalker.h"
#include "nsWeakReference.h"
#include "nsIEditor.h"
#include "nsIDOMMouseListener.h"
#include "nsIDOMKeyListener.h"
#include "nsWeakReference.h"
#include "mozISpellI18NUtil.h"

class nsIDOMDocumentRange;
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
  nsresult InitForNavigation(PRBool aForceCheck, PRInt32 aNewPositionOffset,
                             nsIDOMNode* aOldAnchorNode, PRInt32 aOldAnchorOffset,
                             nsIDOMNode* aNewAnchorNode, PRInt32 aNewAnchorOffset,
                             PRBool* aContinue);
  nsresult InitForSelection();
  nsresult InitForRange(nsIDOMRange* aRange);

  nsresult FinishInitOnEvent(mozInlineSpellWordUtil& aWordUtil);

  nsRefPtr<mozInlineSpellChecker> mSpellChecker;

  
  
  
  PRInt32 mWordCount;

  
  enum Operation { eOpChange,       
                   eOpChangeDelete, 
                   eOpNavigation,   
                   eOpSelection,    
                   eOpResume };     
  Operation mOp;

  
  
  nsCOMPtr<nsIDOMRange> mRange;

  
  
  nsCOMPtr<nsIDOMRange> mCreatedRange;

  
  nsCOMPtr<nsIDOMRange> mNoCheckRange;

  
  
  
  
  
  nsCOMPtr<nsIDOMRange> mAnchorRange;

  
  
  
  

  
  nsCOMPtr<nsIDOMRange> mOldNavigationAnchorRange;

  
  
  
  PRBool mForceNavigationWordCheck;

  
  PRInt32 mNewNavigationPositionOffset;

protected:
  nsresult FinishNavigationEvent(mozInlineSpellWordUtil& aWordUtil);

  nsresult FillNoCheckRangeFromAnchor(mozInlineSpellWordUtil& aWordUtil);

  nsresult GetDocumentRange(nsIDOMDocumentRange** aDocRange);
  nsresult PositionToCollapsedRange(nsIDOMDocumentRange* aDocRange,
                                    nsIDOMNode* aNode, PRInt32 aOffset,
                                    nsIDOMRange** aRange);
};

class mozInlineSpellChecker : public nsIInlineSpellChecker, nsIEditActionListener, nsIDOMMouseListener, nsIDOMKeyListener,
                                     nsSupportsWeakReference
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

  
  
  PRBool mNeedsCheckAfterNavigation;

  
  
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

  NS_DECL_ISUPPORTS
  NS_DECL_NSIEDITACTIONLISTENER
  NS_DECL_NSIINLINESPELLCHECKER

  
  static PRBool CanEnableInlineSpellChecking();

  
  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent);
  NS_IMETHOD MouseDown(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseUp(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseClick(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseDblClick(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseOver(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseOut(nsIDOMEvent* aMouseEvent);
  

  
  NS_IMETHOD KeyDown(nsIDOMEvent* aKeyEvent);
  NS_IMETHOD KeyUp(nsIDOMEvent* aKeyEvent);
  NS_IMETHOD KeyPress(nsIDOMEvent* aKeyEvent);
   

  mozInlineSpellChecker();
  virtual ~mozInlineSpellChecker();

  
  nsresult SpellCheckBetweenNodes(nsIDOMNode *aStartNode,
                                  PRInt32 aStartOffset,
                                  nsIDOMNode *aEndNode,
                                  PRInt32 aEndOffset);

  
  
  
  nsresult SkipSpellCheckForNode(nsIEditor* aEditor,
                                 nsIDOMNode *aNode, PRBool * aCheckSpelling);

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
                        PRBool* aDoneChecking);

  
  nsresult IsPointInSelection(nsISelection *aSelection,
                              nsIDOMNode *aNode,
                              PRInt32 aOffset,
                              nsIDOMRange **aRange);

  nsresult CleanupRangesInSelection(nsISelection *aSelection);

  nsresult RemoveRange(nsISelection *aSpellCheckSelection, nsIDOMRange * aRange);
  nsresult AddRange(nsISelection *aSpellCheckSelection, nsIDOMRange * aRange);
  PRBool   SpellCheckSelectionIsFull() { return mNumWordsInSpellSelection >= mMaxNumWordsInSpellSelection; }

  nsresult MakeSpellCheckRange(nsIDOMNode* aStartNode, PRInt32 aStartOffset,
                               nsIDOMNode* aEndNode, PRInt32 aEndOffset,
                               nsIDOMRange** aRange);

  
  nsresult RegisterEventListeners();
  nsresult UnregisterEventListeners();
  nsresult HandleNavigationEvent(nsIDOMEvent * aEvent, PRBool aForceWordSpellCheck, PRInt32 aNewPositionOffset = 0);

  nsresult GetSpellCheckSelection(nsISelection ** aSpellCheckSelection);
  nsresult SaveCurrentSelectionPosition();

  nsresult ResumeCheck(mozInlineSpellStatus* aStatus);
};

#endif 
