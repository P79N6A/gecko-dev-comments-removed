




#ifndef NSSUBDOCUMENTFRAME_H_
#define NSSUBDOCUMENTFRAME_H_

#include "mozilla/Attributes.h"
#include "nsLeafFrame.h"
#include "nsIReflowCallback.h"
#include "nsFrameLoader.h"




class nsSubDocumentFrame : public nsLeafFrame,
                           public nsIReflowCallback
{
public:
  NS_DECL_QUERYFRAME_TARGET(nsSubDocumentFrame)
  NS_DECL_FRAMEARENA_HELPERS

  nsSubDocumentFrame(nsStyleContext* aContext);

#ifdef DEBUG
  NS_IMETHOD List(FILE* out, int32_t aIndent, uint32_t aFlags = 0) const;
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif

  NS_DECL_QUERYFRAME

  virtual nsIAtom* GetType() const;

  virtual bool IsFrameOfType(uint32_t aFlags) const
  {
    
    return nsLeafFrame::IsFrameOfType(aFlags &
      ~(nsIFrame::eReplaced | nsIFrame::eReplacedContainsBlock));
  }

  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);

  virtual void DestroyFrom(nsIFrame* aDestructRoot);

  virtual nscoord GetMinWidth(nsRenderingContext *aRenderingContext);
  virtual nscoord GetPrefWidth(nsRenderingContext *aRenderingContext);

  virtual IntrinsicSize GetIntrinsicSize();
  virtual nsSize  GetIntrinsicRatio();

  virtual nsSize ComputeAutoSize(nsRenderingContext *aRenderingContext,
                                 nsSize aCBSize, nscoord aAvailableWidth,
                                 nsSize aMargin, nsSize aBorder,
                                 nsSize aPadding, bool aShrinkWrap);

  virtual nsSize ComputeSize(nsRenderingContext *aRenderingContext,
                             nsSize aCBSize, nscoord aAvailableWidth,
                             nsSize aMargin, nsSize aBorder, nsSize aPadding,
                             uint32_t aFlags) MOZ_OVERRIDE;

  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus) MOZ_OVERRIDE;

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  NS_IMETHOD AttributeChanged(int32_t aNameSpaceID,
                              nsIAtom* aAttribute,
                              int32_t aModType);

  
  
  
  
  virtual bool SupportsVisibilityHidden() { return false; }

#ifdef ACCESSIBILITY
  virtual mozilla::a11y::AccType AccessibleType() MOZ_OVERRIDE;
#endif

  nsresult GetDocShell(nsIDocShell **aDocShell);
  nsresult BeginSwapDocShells(nsIFrame* aOther);
  void EndSwapDocShells(nsIFrame* aOther);
  nsView* EnsureInnerView();
  nsIFrame* GetSubdocumentRootFrame();
  nsIntSize GetSubdocumentSize();

  
  virtual bool ReflowFinished() MOZ_OVERRIDE;
  virtual void ReflowCallbackCanceled() MOZ_OVERRIDE;

  bool ShouldClipSubdocument()
  {
    nsFrameLoader* frameLoader = FrameLoader();
    return !frameLoader || frameLoader->ShouldClipSubdocument();
  }

  bool ShouldClampScrollPosition()
  {
    nsFrameLoader* frameLoader = FrameLoader();
    return !frameLoader || frameLoader->ShouldClampScrollPosition();
  }

protected:
  friend class AsyncFrameInit;

  
  nsIntSize GetMarginAttributes();

  nsFrameLoader* FrameLoader();

  bool IsInline() { return mIsInline; }

  virtual nscoord GetIntrinsicWidth() MOZ_OVERRIDE;
  virtual nscoord GetIntrinsicHeight() MOZ_OVERRIDE;

  
  
  
  void ShowViewer();

  







  nsIFrame* ObtainIntrinsicSizeFrame();

  



  bool PassPointerEventsToChildren();

  nsRefPtr<nsFrameLoader> mFrameLoader;
  nsView* mInnerView;
  bool mIsInline;
  bool mPostedReflowCallback;
  bool mDidCreateDoc;
  bool mCallingShow;
};

#endif 
