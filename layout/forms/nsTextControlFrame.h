




































#ifndef nsTextControlFrame_h___
#define nsTextControlFrame_h___

#include "nsStackFrame.h"
#include "nsBlockFrame.h"
#include "nsIFormControlFrame.h"
#include "nsIDOMMouseListener.h"
#include "nsIAnonymousContentCreator.h"
#include "nsIEditor.h"
#include "nsITextControlFrame.h"
#include "nsIFontMetrics.h"
#include "nsWeakReference.h" 
#include "nsIScrollableViewProvider.h"
#include "nsContentUtils.h"
#include "nsDisplayList.h"

class nsIEditor;
class nsISelectionController;
class nsTextInputSelectionImpl;
class nsTextInputListener;
class nsIDOMCharacterData;
class nsIScrollableView;
#ifdef ACCESSIBILITY
class nsIAccessible;
#endif


class nsTextControlFrame : public nsStackFrame,
                           public nsIAnonymousContentCreator,
                           public nsITextControlFrame,
                           public nsIScrollableViewProvider

{
public:
  NS_DECL_FRAMEARENA_HELPERS

  nsTextControlFrame(nsIPresShell* aShell, nsStyleContext* aContext);
  virtual ~nsTextControlFrame();

  virtual void RemovedAsPrimaryFrame(); 

  virtual void Destroy();

  virtual nscoord GetMinWidth(nsIRenderingContext* aRenderingContext);
  virtual nsSize ComputeAutoSize(nsIRenderingContext *aRenderingContext,
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
  NS_IMETHOD GetAccessible(nsIAccessible** aAccessible);
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

  

  
  
  
  nsresult SetValue(const nsAString& aValue);
  NS_IMETHOD SetInitialChildList(nsIAtom*        aListName,
                                 nsFrameList&    aChildList);


  virtual void SetFocus(PRBool aOn , PRBool aRepaint); 
  virtual nsresult SetFormProperty(nsIAtom* aName, const nsAString& aValue);
  virtual nsresult GetFormProperty(nsIAtom* aName, nsAString& aValue) const; 






  NS_IMETHOD    GetEditor(nsIEditor **aEditor);
  NS_IMETHOD    OwnsValue(PRBool* aOwnsValue);
  NS_IMETHOD    GetValue(nsAString& aValue, PRBool aIgnoreWrap) const;
  NS_IMETHOD    GetTextLength(PRInt32* aTextLength);
  NS_IMETHOD    CheckFireOnChange();
  NS_IMETHOD    SetSelectionStart(PRInt32 aSelectionStart);
  NS_IMETHOD    SetSelectionEnd(PRInt32 aSelectionEnd);
  NS_IMETHOD    SetSelectionRange(PRInt32 aSelectionStart, PRInt32 aSelectionEnd);
  NS_IMETHOD    GetSelectionRange(PRInt32* aSelectionStart, PRInt32* aSelectionEnd);
  virtual nsISelectionController* GetOwnedSelectionController()
    { return mSelCon; }
  virtual nsFrameSelection* GetOwnedFrameSelection()
    { return mFrameSel; }

  nsresult GetPhonetic(nsAString& aPhonetic);



  virtual nsIAtom* GetType() const;

  
  NS_IMETHOD AttributeChanged(PRInt32         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              PRInt32         aModType);

  NS_IMETHOD GetText(nsString* aText);

  NS_DECL_QUERYFRAME

public: 
  



  PRBool IsSingleLineTextControl() const;
  



  PRBool IsTextArea() const;
  



  PRBool IsPlainTextControl() const;
  



  PRBool IsPasswordTextControl() const;
  void FireOnInput();
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

  
  static NS_HIDDEN_(void) ShutDown();

  
  nsresult MaybeBeginSecureKeyboardInput();
  void MaybeEndSecureKeyboardInput();

protected:
  class EditorInitializer;
  friend class EditorInitializer;

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
        mFrame->DelayedEditorInit();
        shell->ObserveNativeAnonMutationsForPrint(observes);
      }
      return NS_OK;
    }

  private:
    nsWeakFrame mWeakFrame;
    nsTextControlFrame* mFrame;
  };

  
  
  void DelayedEditorInit();

  nsresult DOMPointToOffset(nsIDOMNode* aNode, PRInt32 aNodeOffset, PRInt32 *aResult);
  nsresult OffsetToDOMPoint(PRInt32 aOffset, nsIDOMNode** aResult, PRInt32* aPosition);

  




  PRBool IsScrollable() const;
  




  nsresult InitEditor();
  



  void RemoveNewlines(nsString &aString);
  




  PRBool GetMaxLength(PRInt32* aMaxLength);
  




  PRBool AttributeExists(nsIAtom *aAtt) const
  { return mContent && mContent->HasAttr(kNameSpaceID_None, aAtt); }

  



  void PreDestroy();
  



  
  



  PRInt32 GetCols();
  


  PRInt32 GetWrapCols();
  



  PRInt32 GetRows();

  
  
  
  nsresult CalcIntrinsicSize(nsIRenderingContext* aRenderingContext,
                             nsSize&              aIntrinsicSize);

  
  virtual nsIScrollableView* GetScrollableView();

private:
  
  nsresult SetSelectionInternal(nsIDOMNode *aStartNode, PRInt32 aStartOffset,
                                nsIDOMNode *aEndNode, PRInt32 aEndOffset);
  nsresult SelectAllContents();
  nsresult SetSelectionEndPoints(PRInt32 aSelStart, PRInt32 aSelEnd);
  
private:
  nsCOMPtr<nsIContent> mAnonymousDiv;

  nsCOMPtr<nsIEditor> mEditor;

  
  PRPackedBool mUseEditor;
  PRPackedBool mIsProcessing;
  PRPackedBool mNotifyOnInput;
  PRPackedBool mDidPreDestroy; 
  
  
  PRPackedBool mFireChangeEventState;
  PRPackedBool mInSecureKeyboardInputMode;

  nsCOMPtr<nsISelectionController> mSelCon;
  nsCOMPtr<nsFrameSelection> mFrameSel;
  nsTextInputListener* mTextListener;
  nsString mFocusedValue;
};

#endif


