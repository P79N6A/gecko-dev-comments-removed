




#ifndef nsTextControlFrame_h___
#define nsTextControlFrame_h___

#include "mozilla/Attributes.h"
#include "nsContainerFrame.h"
#include "nsIAnonymousContentCreator.h"
#include "nsIContent.h"
#include "nsITextControlFrame.h"
#include "nsITextControlElement.h"
#include "nsIStatefulFrame.h"

class nsISelectionController;
class EditorInitializerEntryTracker;
class nsTextEditorState;
class nsIEditor;
namespace mozilla {
namespace dom {
class Element;
} 
} 

class nsTextControlFrame final : public nsContainerFrame,
                                 public nsIAnonymousContentCreator,
                                 public nsITextControlFrame,
                                 public nsIStatefulFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  NS_DECLARE_FRAME_PROPERTY(ContentScrollPos, DeleteValue<nsPoint>)

  explicit nsTextControlFrame(nsStyleContext* aContext);
  virtual ~nsTextControlFrame();

  virtual void DestroyFrom(nsIFrame* aDestructRoot) override;

  virtual nsIScrollableFrame* GetScrollTargetFrame() override {
    return do_QueryFrame(GetFirstPrincipalChild());
  }

  virtual nscoord GetMinISize(nsRenderingContext* aRenderingContext) override;
  virtual nscoord GetPrefISize(nsRenderingContext* aRenderingContext) override;

  virtual mozilla::LogicalSize
  ComputeAutoSize(nsRenderingContext *aRenderingContext,
                  mozilla::WritingMode aWritingMode,
                  const mozilla::LogicalSize& aCBSize,
                  nscoord aAvailableISize,
                  const mozilla::LogicalSize& aMargin,
                  const mozilla::LogicalSize& aBorder,
                  const mozilla::LogicalSize& aPadding,
                  bool aShrinkWrap) override;

  virtual void Reflow(nsPresContext*           aPresContext,
                      nsHTMLReflowMetrics&     aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&          aStatus) override;

  virtual nsSize GetMinSize(nsBoxLayoutState& aBoxLayoutState) override;
  virtual bool IsCollapsed() override;

  virtual bool IsLeaf() const override;
  
#ifdef ACCESSIBILITY
  virtual mozilla::a11y::AccType AccessibleType() override;
#endif

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const override
  {
    aResult.AssignLiteral("nsTextControlFrame");
    return NS_OK;
  }
#endif

  virtual bool IsFrameOfType(uint32_t aFlags) const override
  {
    
    
    return nsContainerFrame::IsFrameOfType(aFlags &
      ~(nsIFrame::eReplaced | nsIFrame::eReplacedContainsBlock));
  }

  
  virtual nsresult CreateAnonymousContent(nsTArray<ContentInfo>& aElements) override;
  virtual void AppendAnonymousContentTo(nsTArray<nsIContent*>& aElements,
                                        uint32_t aFilter) override;

  virtual void SetInitialChildList(ChildListID     aListID,
                                   nsFrameList&    aChildList) override;

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) override;

  virtual mozilla::dom::Element* GetPseudoElement(nsCSSPseudoElements::Type aType) override;


  virtual void SetFocus(bool aOn , bool aRepaint) override; 
  virtual nsresult SetFormProperty(nsIAtom* aName, const nsAString& aValue) override;





  NS_IMETHOD    GetEditor(nsIEditor **aEditor) override;
  NS_IMETHOD    SetSelectionStart(int32_t aSelectionStart) override;
  NS_IMETHOD    SetSelectionEnd(int32_t aSelectionEnd) override;
  NS_IMETHOD    SetSelectionRange(int32_t aSelectionStart,
                                  int32_t aSelectionEnd,
                                  SelectionDirection aDirection = eNone) override;
  NS_IMETHOD    GetSelectionRange(int32_t* aSelectionStart,
                                  int32_t* aSelectionEnd,
                                  SelectionDirection* aDirection = nullptr) override;
  NS_IMETHOD    GetOwnedSelectionController(nsISelectionController** aSelCon) override;
  virtual nsFrameSelection* GetOwnedFrameSelection() override;

  nsresult GetPhonetic(nsAString& aPhonetic) override;

  




  virtual nsresult EnsureEditorInitialized() override;





  NS_IMETHOD SaveState(nsPresState** aState) override;
  NS_IMETHOD RestoreState(nsPresState* aState) override;




  virtual nsIAtom* GetType() const override;

  
  virtual nsresult AttributeChanged(int32_t         aNameSpaceID,
                                    nsIAtom*        aAttribute,
                                    int32_t         aModType) override;

  nsresult GetText(nsString& aText);

  virtual nsresult PeekOffset(nsPeekOffsetStruct *aPos) override;

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
    explicit EditorInitializer(nsTextControlFrame* aFrame) :
      mFrame(aFrame) {}

    NS_IMETHOD Run() override;

    
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
    explicit ScrollOnFocusEvent(nsTextControlFrame* aFrame) :
      mFrame(aFrame) {}

    NS_DECL_NSIRUNNABLE

    void Revoke() {
      mFrame = nullptr;
    }

  private:
    nsTextControlFrame* mFrame;
  };

  nsresult OffsetToDOMPoint(int32_t aOffset, nsIDOMNode** aResult, int32_t* aPosition);

  




  nsresult UpdateValueDisplay(bool aNotify,
                              bool aBeforeEditorInit = false,
                              const nsAString *aValue = nullptr);

  




  bool GetMaxLength(int32_t* aMaxLength);

  




  bool AttributeExists(nsIAtom *aAtt) const
  { return mContent && mContent->HasAttr(kNameSpaceID_None, aAtt); }

  



  void PreDestroy();

  
  
  
  nsresult CalcIntrinsicSize(nsRenderingContext* aRenderingContext,
                             mozilla::WritingMode aWM,
                             mozilla::LogicalSize& aIntrinsicSize,
                             float aFontSizeInflation);

  nsresult ScrollSelectionIntoView() override;

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
  
  bool mEditorHasBeenInitialized;
  bool mIsProcessing;
  
  bool mUsePlaceholder;

#ifdef DEBUG
  bool mInEditorInitialization;
  friend class EditorInitializerEntryTracker;
#endif

  nsRevocableEventPtr<ScrollOnFocusEvent> mScrollEvent;
};

#endif


