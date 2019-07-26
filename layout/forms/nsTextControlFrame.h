




#ifndef nsTextControlFrame_h___
#define nsTextControlFrame_h___

#include "mozilla/Attributes.h"
#include "nsContainerFrame.h"
#include "nsBlockFrame.h"
#include "nsIFormControlFrame.h"
#include "nsIAnonymousContentCreator.h"
#include "nsITextControlFrame.h"
#include "nsDisplayList.h"
#include "nsIScrollableFrame.h"
#include "nsStubMutationObserver.h"
#include "nsITextControlElement.h"
#include "nsIStatefulFrame.h"
#include "nsIEditor.h"

class nsISelectionController;
class nsIDOMCharacterData;
class EditorInitializerEntryTracker;
class nsTextEditorState;
namespace mozilla {
namespace dom {
class Element;
}
}

class nsTextControlFrame : public nsContainerFrame,
                           public nsIAnonymousContentCreator,
                           public nsITextControlFrame,
                           public nsIStatefulFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  NS_DECLARE_FRAME_PROPERTY(ContentScrollPos, DestroyPoint)

  nsTextControlFrame(nsIPresShell* aShell, nsStyleContext* aContext);
  virtual ~nsTextControlFrame();

  virtual void DestroyFrom(nsIFrame* aDestructRoot) MOZ_OVERRIDE;

  virtual nsIScrollableFrame* GetScrollTargetFrame() {
    if (!IsScrollable())
      return nullptr;
    return do_QueryFrame(GetFirstPrincipalChild());
  }

  virtual nscoord GetMinWidth(nsRenderingContext* aRenderingContext);
  virtual nscoord GetPrefWidth(nsRenderingContext* aRenderingContext);

  virtual nsSize ComputeAutoSize(nsRenderingContext *aRenderingContext,
                                 nsSize aCBSize, nscoord aAvailableWidth,
                                 nsSize aMargin, nsSize aBorder,
                                 nsSize aPadding, bool aShrinkWrap) MOZ_OVERRIDE;

  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  virtual nsSize GetMinSize(nsBoxLayoutState& aBoxLayoutState);
  virtual bool IsCollapsed();

  virtual bool IsLeaf() const;
  
#ifdef ACCESSIBILITY
  virtual mozilla::a11y::AccType AccessibleType() MOZ_OVERRIDE;
#endif

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    aResult.AssignLiteral("nsTextControlFrame");
    return NS_OK;
  }
#endif

  virtual bool IsFrameOfType(uint32_t aFlags) const
  {
    
    
    return nsContainerFrame::IsFrameOfType(aFlags &
      ~(nsIFrame::eReplaced | nsIFrame::eReplacedContainsBlock));
  }

  
  virtual nsresult CreateAnonymousContent(nsTArray<ContentInfo>& aElements) MOZ_OVERRIDE;
  virtual void AppendAnonymousContentTo(nsBaseContentList& aElements,
                                        uint32_t aFilter) MOZ_OVERRIDE;

  

  NS_IMETHOD SetInitialChildList(ChildListID     aListID,
                                 nsFrameList&    aChildList) MOZ_OVERRIDE;

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);


  virtual void SetFocus(bool aOn , bool aRepaint); 
  virtual nsresult SetFormProperty(nsIAtom* aName, const nsAString& aValue);
  virtual nsresult GetFormProperty(nsIAtom* aName, nsAString& aValue) const; 






  NS_IMETHOD    GetEditor(nsIEditor **aEditor) MOZ_OVERRIDE;
  NS_IMETHOD    SetSelectionStart(int32_t aSelectionStart) MOZ_OVERRIDE;
  NS_IMETHOD    SetSelectionEnd(int32_t aSelectionEnd) MOZ_OVERRIDE;
  NS_IMETHOD    SetSelectionRange(int32_t aSelectionStart,
                                  int32_t aSelectionEnd,
                                  SelectionDirection aDirection = eNone) MOZ_OVERRIDE;
  NS_IMETHOD    GetSelectionRange(int32_t* aSelectionStart,
                                  int32_t* aSelectionEnd,
                                  SelectionDirection* aDirection = nullptr) MOZ_OVERRIDE;
  NS_IMETHOD    GetOwnedSelectionController(nsISelectionController** aSelCon) MOZ_OVERRIDE;
  virtual nsFrameSelection* GetOwnedFrameSelection() MOZ_OVERRIDE;

  nsresult GetPhonetic(nsAString& aPhonetic) MOZ_OVERRIDE;

  




  virtual nsresult EnsureEditorInitialized() MOZ_OVERRIDE;





  NS_IMETHOD SaveState(nsPresState** aState) MOZ_OVERRIDE;
  NS_IMETHOD RestoreState(nsPresState* aState) MOZ_OVERRIDE;




  virtual nsIAtom* GetType() const;

  
  NS_IMETHOD AttributeChanged(int32_t         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              int32_t         aModType);

  nsresult GetText(nsString& aText);

  NS_IMETHOD PeekOffset(nsPeekOffsetStruct *aPos);

  NS_DECL_QUERYFRAME

  
  
  NS_DECLARE_FRAME_PROPERTY(TextControlInitializer, nullptr)

protected:
  


  void ReflowTextControlChild(nsIFrame*                aFrame,
                              nsPresContext*           aPresContext,
                              const nsHTMLReflowState& aReflowState,
                              nsReflowStatus&          aStatus,
                              nsHTMLReflowMetrics& aParentDesiredSize);

public: 
  void SetValueChanged(bool aValueChanged);
  
  
  nsresult MaybeBeginSecureKeyboardInput();
  void MaybeEndSecureKeyboardInput();

  NS_STACK_CLASS class ValueSetter {
  public:
    ValueSetter(nsIEditor* aEditor)
      : mEditor(aEditor)
      , mCanceled(false)
    {
      MOZ_ASSERT(aEditor);

      
      
      
      mEditor->GetSuppressDispatchingInputEvent(&mOuterTransaction);
    }
    void Cancel() {
      mCanceled = true;
    }
    void Init() {
      mEditor->SetSuppressDispatchingInputEvent(true);
    }
    ~ValueSetter() {
      mEditor->SetSuppressDispatchingInputEvent(mOuterTransaction);

      if (mCanceled) {
        return;
      }
    }

  private:
    nsCOMPtr<nsIEditor> mEditor;
    bool mOuterTransaction;
    bool mCanceled;
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

  DEFINE_TEXTCTRL_CONST_FORWARDER(bool, IsSingleLineTextControl)
  DEFINE_TEXTCTRL_CONST_FORWARDER(bool, IsTextArea)
  DEFINE_TEXTCTRL_CONST_FORWARDER(bool, IsPlainTextControl)
  DEFINE_TEXTCTRL_CONST_FORWARDER(bool, IsPasswordTextControl)
  DEFINE_TEXTCTRL_FORWARDER(int32_t, GetCols)
  DEFINE_TEXTCTRL_FORWARDER(int32_t, GetWrapCols)
  DEFINE_TEXTCTRL_FORWARDER(int32_t, GetRows)

#undef DEFINE_TEXTCTRL_CONST_FORWARDER
#undef DEFINE_TEXTCTRL_FORWARDER

protected:
  class EditorInitializer;
  friend class EditorInitializer;
  friend class nsTextEditorState; 

  class EditorInitializer : public nsRunnable {
  public:
    EditorInitializer(nsTextControlFrame* aFrame) :
      mFrame(aFrame) {}

    NS_IMETHOD Run();

    
    void Revoke() {
      mFrame = nullptr;
    }

  private:
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
      mFrame = nullptr;
    }

  private:
    nsTextControlFrame* mFrame;
  };

  nsresult OffsetToDOMPoint(int32_t aOffset, nsIDOMNode** aResult, int32_t* aPosition);

  




  bool IsScrollable() const;

  




  nsresult UpdateValueDisplay(bool aNotify,
                              bool aBeforeEditorInit = false,
                              const nsAString *aValue = nullptr);

  




  bool GetMaxLength(int32_t* aMaxLength);

  




  bool AttributeExists(nsIAtom *aAtt) const
  { return mContent && mContent->HasAttr(kNameSpaceID_None, aAtt); }

  



  void PreDestroy();

  
  
  
  nsresult CalcIntrinsicSize(nsRenderingContext* aRenderingContext,
                             nsSize&             aIntrinsicSize,
                             float               aFontSizeInflation);

  nsresult ScrollSelectionIntoView() MOZ_OVERRIDE;

private:
  
  nsresult SetSelectionInternal(nsIDOMNode *aStartNode, int32_t aStartOffset,
                                nsIDOMNode *aEndNode, int32_t aEndOffset,
                                SelectionDirection aDirection = eNone);
  nsresult SelectAllOrCollapseToEndOfText(bool aSelect);
  nsresult SetSelectionEndPoints(int32_t aSelStart, int32_t aSelEnd,
                                 SelectionDirection aDirection = eNone);

  


  mozilla::dom::Element* GetRootNodeAndInitializeEditor();
  nsresult GetRootNodeAndInitializeEditor(nsIDOMElement **aRootElement);

  void FinishedInitializer() {
    Properties().Delete(TextControlInitializer());
  }

private:
  
  bool mUseEditor;
  bool mIsProcessing;
  
  bool mUsePlaceholder;

#ifdef DEBUG
  bool mInEditorInitialization;
  friend class EditorInitializerEntryTracker;
#endif

  nsRevocableEventPtr<ScrollOnFocusEvent> mScrollEvent;
};

#endif


