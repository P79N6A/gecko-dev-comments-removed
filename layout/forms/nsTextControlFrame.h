




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

class nsTextControlFrame MOZ_FINAL : public nsContainerFrame,
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

  virtual nsIScrollableFrame* GetScrollTargetFrame() MOZ_OVERRIDE {
    return do_QueryFrame(GetFirstPrincipalChild());
  }

  virtual nscoord GetMinISize(nsRenderingContext* aRenderingContext) MOZ_OVERRIDE;
  virtual nscoord GetPrefISize(nsRenderingContext* aRenderingContext) MOZ_OVERRIDE;

  virtual mozilla::LogicalSize
  ComputeAutoSize(nsRenderingContext *aRenderingContext,
                  mozilla::WritingMode aWritingMode,
                  const mozilla::LogicalSize& aCBSize,
                  nscoord aAvailableISize,
                  const mozilla::LogicalSize& aMargin,
                  const mozilla::LogicalSize& aBorder,
                  const mozilla::LogicalSize& aPadding,
                  bool aShrinkWrap) MOZ_OVERRIDE;

  virtual void Reflow(nsPresContext*           aPresContext,
                      nsHTMLReflowMetrics&     aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&          aStatus) MOZ_OVERRIDE;

  virtual nsSize GetMinSize(nsBoxLayoutState& aBoxLayoutState) MOZ_OVERRIDE;
  virtual bool IsCollapsed() MOZ_OVERRIDE;

  virtual bool IsLeaf() const MOZ_OVERRIDE;
  
#ifdef ACCESSIBILITY
  virtual mozilla::a11y::AccType AccessibleType() MOZ_OVERRIDE;
#endif

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE
  {
    aResult.AssignLiteral("nsTextControlFrame");
    return NS_OK;
  }
#endif

  virtual bool IsFrameOfType(uint32_t aFlags) const MOZ_OVERRIDE
  {
    
    
    return nsContainerFrame::IsFrameOfType(aFlags &
      ~(nsIFrame::eReplaced | nsIFrame::eReplacedContainsBlock));
  }

  
  virtual nsresult CreateAnonymousContent(nsTArray<ContentInfo>& aElements) MOZ_OVERRIDE;
  virtual void AppendAnonymousContentTo(nsTArray<nsIContent*>& aElements,
                                        uint32_t aFilter) MOZ_OVERRIDE;

  virtual void SetInitialChildList(ChildListID     aListID,
                                   nsFrameList&    aChildList) MOZ_OVERRIDE;

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  virtual mozilla::dom::Element* GetPseudoElement(nsCSSPseudoElements::Type aType) MOZ_OVERRIDE;


  virtual void SetFocus(bool aOn , bool aRepaint) MOZ_OVERRIDE; 
  virtual nsresult SetFormProperty(nsIAtom* aName, const nsAString& aValue) MOZ_OVERRIDE;





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




  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

  
  virtual nsresult AttributeChanged(int32_t         aNameSpaceID,
                                    nsIAtom*        aAttribute,
                                    int32_t         aModType) MOZ_OVERRIDE;

  nsresult GetText(nsString& aText);

  virtual nsresult PeekOffset(nsPeekOffsetStruct *aPos) MOZ_OVERRIDE;

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

    NS_IMETHOD Run() MOZ_OVERRIDE;

    
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


