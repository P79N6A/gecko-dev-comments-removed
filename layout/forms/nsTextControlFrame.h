




































#ifndef nsTextControlFrame_h___
#define nsTextControlFrame_h___

#include "nsStackFrame.h"
#include "nsBlockFrame.h"
#include "nsIFormControlFrame.h"
#include "nsIAnonymousContentCreator.h"
#include "nsITextControlFrame.h"
#include "nsDisplayList.h"
#include "nsIScrollableFrame.h"
#include "nsStubMutationObserver.h"
#include "nsITextControlElement.h"
#include "nsIStatefulFrame.h"

class nsIEditor;
class nsISelectionController;
class nsIDOMCharacterData;
#ifdef ACCESSIBILITY
class nsIAccessible;
#endif
class EditorInitializerEntryTracker;
class nsTextEditorState;

class nsTextControlFrame : public nsStackFrame,
                           public nsIAnonymousContentCreator,
                           public nsITextControlFrame,
                           public nsIStatefulFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  NS_DECLARE_FRAME_PROPERTY(ContentScrollPos, DestroyPoint)

  nsTextControlFrame(nsIPresShell* aShell, nsStyleContext* aContext);
  virtual ~nsTextControlFrame();

  virtual void DestroyFrom(nsIFrame* aDestructRoot);

  virtual nsIScrollableFrame* GetScrollTargetFrame() {
    if (!IsScrollable())
      return nsnull;
    return do_QueryFrame(GetFirstChild(nsnull));
  }

  virtual nscoord GetMinWidth(nsRenderingContext* aRenderingContext);
  virtual nsSize ComputeAutoSize(nsRenderingContext *aRenderingContext,
                                 nsSize aCBSize, nscoord aAvailableWidth,
                                 nsSize aMargin, nsSize aBorder,
                                 nsSize aPadding, PRBool aShrinkWrap);

  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  virtual nsSize GetPrefSize(nsBoxLayoutState& aBoxLayoutState);
  virtual nsSize GetMinSize(nsBoxLayoutState& aBoxLayoutState);
  virtual nsSize GetMaxSize(nsBoxLayoutState& aBoxLayoutState);
  virtual nscoord GetBoxAscent(nsBoxLayoutState& aBoxLayoutState);
  virtual PRBool IsCollapsed(nsBoxLayoutState& aBoxLayoutState);

  DECL_DO_GLOBAL_REFLOW_COUNT_DSP(nsTextControlFrame, nsStackFrame)

  virtual PRBool IsLeaf() const;
  
#ifdef ACCESSIBILITY
  virtual already_AddRefed<nsAccessible> CreateAccessible();
#endif

#ifdef NS_DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    aResult.AssignLiteral("nsTextControlFrame");
    return NS_OK;
  }
#endif

  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
  {
    
    
    return nsStackFrame::IsFrameOfType(aFlags &
      ~(nsIFrame::eReplaced | nsIFrame::eReplacedContainsBlock));
  }

  
  virtual nsresult CreateAnonymousContent(nsTArray<nsIContent*>& aElements);
  virtual void AppendAnonymousContentTo(nsBaseContentList& aElements,
                                        PRUint32 aFilter);

  

  NS_IMETHOD SetInitialChildList(nsIAtom*        aListName,
                                 nsFrameList&    aChildList);


  virtual void SetFocus(PRBool aOn , PRBool aRepaint); 
  virtual nsresult SetFormProperty(nsIAtom* aName, const nsAString& aValue);
  virtual nsresult GetFormProperty(nsIAtom* aName, nsAString& aValue) const; 






  NS_IMETHOD    GetEditor(nsIEditor **aEditor);
  NS_IMETHOD    GetTextLength(PRInt32* aTextLength);
  NS_IMETHOD    CheckFireOnChange();
  NS_IMETHOD    SetSelectionStart(PRInt32 aSelectionStart);
  NS_IMETHOD    SetSelectionEnd(PRInt32 aSelectionEnd);
  NS_IMETHOD    SetSelectionRange(PRInt32 aSelectionStart, PRInt32 aSelectionEnd);
  NS_IMETHOD    GetSelectionRange(PRInt32* aSelectionStart, PRInt32* aSelectionEnd);
  NS_IMETHOD    GetOwnedSelectionController(nsISelectionController** aSelCon);
  virtual nsFrameSelection* GetOwnedFrameSelection();

  nsresult GetPhonetic(nsAString& aPhonetic);

  




  virtual nsresult EnsureEditorInitialized();





  NS_IMETHOD SaveState(SpecialStateID aStateID, nsPresState** aState);
  NS_IMETHOD RestoreState(nsPresState* aState);




  virtual nsIAtom* GetType() const;

  
  NS_IMETHOD AttributeChanged(PRInt32         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              PRInt32         aModType);

  nsresult GetText(nsString& aText);

  NS_DECL_QUERYFRAME

public: 
  void FireOnInput(PRBool aTrusted);
  void SetValueChanged(PRBool aValueChanged);
  
  nsresult InitFocusedValue();

  void SetFireChangeEventState(PRBool aNewState)
  {
    mFireChangeEventState = aNewState;
  }

  PRBool GetFireChangeEventState() const
  {
    return mFireChangeEventState;
  }    

  
  nsresult MaybeBeginSecureKeyboardInput();
  void MaybeEndSecureKeyboardInput();

  class ValueSetter {
  public:
    ValueSetter(nsTextControlFrame* aFrame,
                PRBool aHasFocusValue)
      : mFrame(aFrame)
      
      
      
      
      
      , mFocusValueInit(!mFrame->mFireChangeEventState && aHasFocusValue)
      , mOuterTransaction(false)
      , mInited(false)
    {
      NS_ASSERTION(aFrame, "Should pass a valid frame");
    }
    void Cancel() {
      mInited = PR_FALSE;
    }
    void Init() {
      
      
      

      
      
      
      mOuterTransaction = mFrame->mNotifyOnInput;
      if (mOuterTransaction)
        mFrame->mNotifyOnInput = PR_FALSE;

      mInited = PR_TRUE;
    }
    ~ValueSetter() {
      if (!mInited)
        return;

      if (mOuterTransaction)
        mFrame->mNotifyOnInput = PR_TRUE;

      if (mFocusValueInit) {
        
        mFrame->InitFocusedValue();
      }
    }

  private:
    nsTextControlFrame* mFrame;
    PRPackedBool mFocusValueInit;
    PRPackedBool mOuterTransaction;
    PRPackedBool mInited;
  };
  friend class ValueSetter;

#define DEFINE_TEXTCTRL_FORWARDER(type, name)                                  \
  type name() {                                                                \
    nsCOMPtr<nsITextControlElement> txtCtrl = do_QueryInterface(GetContent()); \
    NS_ASSERTION(txtCtrl, "Content not a text control element");               \
    return txtCtrl->name();                                                    \
  }
#define DEFINE_TEXTCTRL_CONST_FORWARDER(type, name)                            \
  type name() const {                                                          \
    nsCOMPtr<nsITextControlElement> txtCtrl = do_QueryInterface(GetContent()); \
    NS_ASSERTION(txtCtrl, "Content not a text control element");               \
    return txtCtrl->name();                                                    \
  }

  DEFINE_TEXTCTRL_CONST_FORWARDER(PRBool, IsSingleLineTextControl)
  DEFINE_TEXTCTRL_CONST_FORWARDER(PRBool, IsTextArea)
  DEFINE_TEXTCTRL_CONST_FORWARDER(PRBool, IsPlainTextControl)
  DEFINE_TEXTCTRL_CONST_FORWARDER(PRBool, IsPasswordTextControl)
  DEFINE_TEXTCTRL_FORWARDER(PRInt32, GetCols)
  DEFINE_TEXTCTRL_FORWARDER(PRInt32, GetWrapCols)
  DEFINE_TEXTCTRL_FORWARDER(PRInt32, GetRows)

#undef DEFINE_TEXTCTRL_CONST_FORWARDER
#undef DEFINE_TEXTCTRL_FORWARDER

protected:
  class EditorInitializer;
  friend class EditorInitializer;
  friend class nsTextEditorState; 

  class EditorInitializer : public nsRunnable {
  public:
    EditorInitializer(nsTextControlFrame* aFrame) :
      mWeakFrame(aFrame),
      mFrame(aFrame) {}

    NS_IMETHOD Run() {
      if (mWeakFrame) {
        nsCOMPtr<nsIPresShell> shell =
          mWeakFrame.GetFrame()->PresContext()->GetPresShell();
        PRBool observes = shell->ObservesNativeAnonMutationsForPrint();
        shell->ObserveNativeAnonMutationsForPrint(PR_TRUE);
        mFrame->EnsureEditorInitialized();
        shell->ObserveNativeAnonMutationsForPrint(observes);
      }
      return NS_OK;
    }

  private:
    nsWeakFrame mWeakFrame;
    nsTextControlFrame* mFrame;
  };

  class ScrollOnFocusEvent;
  friend class ScrollOnFocusEvent;

  class ScrollOnFocusEvent : public nsRunnable {
  public:
    ScrollOnFocusEvent(nsTextControlFrame* aFrame) :
      mFrame(aFrame) {}

    NS_DECL_NSIRUNNABLE

    void Revoke() {
      mFrame = nsnull;
    }

  private:
    nsTextControlFrame* mFrame;
  };

  nsresult DOMPointToOffset(nsIDOMNode* aNode, PRInt32 aNodeOffset, PRInt32 *aResult);
  nsresult OffsetToDOMPoint(PRInt32 aOffset, nsIDOMNode** aResult, PRInt32* aPosition);

  




  PRBool IsScrollable() const;

  




  nsresult UpdateValueDisplay(PRBool aNotify,
                              PRBool aBeforeEditorInit = PR_FALSE,
                              const nsAString *aValue = nsnull);

  




  PRBool GetMaxLength(PRInt32* aMaxLength);

  




  PRBool AttributeExists(nsIAtom *aAtt) const
  { return mContent && mContent->HasAttr(kNameSpaceID_None, aAtt); }

  



  void PreDestroy();

  
  
  
  nsresult CalcIntrinsicSize(nsRenderingContext* aRenderingContext,
                             nsSize&              aIntrinsicSize);

  nsresult ScrollSelectionIntoView();

private:
  
  nsresult SetSelectionInternal(nsIDOMNode *aStartNode, PRInt32 aStartOffset,
                                nsIDOMNode *aEndNode, PRInt32 aEndOffset);
  nsresult SelectAllOrCollapseToEndOfText(PRBool aSelect);
  nsresult SetSelectionEndPoints(PRInt32 aSelStart, PRInt32 aSelEnd);

  
  PRBool GetNotifyOnInput() const { return mNotifyOnInput; }
  void SetNotifyOnInput(PRBool val) { mNotifyOnInput = val; }

  


  nsresult GetRootNodeAndInitializeEditor(nsIDOMElement **aRootElement);

private:
  
  PRPackedBool mUseEditor;
  PRPackedBool mIsProcessing;
  PRPackedBool mNotifyOnInput;
  
  
  PRPackedBool mFireChangeEventState;
  
  PRPackedBool mUsePlaceholder;

#ifdef DEBUG
  PRPackedBool mInEditorInitialization;
  friend class EditorInitializerEntryTracker;
#endif

  nsString mFocusedValue;
  nsRevocableEventPtr<ScrollOnFocusEvent> mScrollEvent;
};

#endif


