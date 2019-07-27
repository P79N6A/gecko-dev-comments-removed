




#ifndef __NS_SVGOUTERSVGFRAME_H__
#define __NS_SVGOUTERSVGFRAME_H__

#include "mozilla/Attributes.h"
#include "nsISVGSVGFrame.h"
#include "nsSVGContainerFrame.h"
#include "nsRegion.h"

class gfxContext;
class nsSVGForeignObjectFrame;




typedef nsSVGDisplayContainerFrame nsSVGOuterSVGFrameBase;

class nsSVGOuterSVGFrame final : public nsSVGOuterSVGFrameBase,
                                 public nsISVGSVGFrame
{
  friend nsContainerFrame*
  NS_NewSVGOuterSVGFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
protected:
  explicit nsSVGOuterSVGFrame(nsStyleContext* aContext);

public:
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

#ifdef DEBUG
  ~nsSVGOuterSVGFrame() {
    NS_ASSERTION(!mForeignObjectHash || mForeignObjectHash->Count() == 0,
                 "foreignObject(s) still registered!");
  }
#endif

  
  virtual nscoord GetMinISize(nsRenderingContext *aRenderingContext) override;
  virtual nscoord GetPrefISize(nsRenderingContext *aRenderingContext) override;

  virtual mozilla::IntrinsicSize GetIntrinsicSize() override;
  virtual nsSize  GetIntrinsicRatio() override;

  virtual mozilla::LogicalSize
  ComputeSize(nsRenderingContext *aRenderingContext,
              mozilla::WritingMode aWritingMode,
              const mozilla::LogicalSize& aCBSize,
              nscoord aAvailableISize,
              const mozilla::LogicalSize& aMargin,
              const mozilla::LogicalSize& aBorder,
              const mozilla::LogicalSize& aPadding,
              ComputeSizeFlags aFlags) override;

  virtual void Reflow(nsPresContext*           aPresContext,
                      nsHTMLReflowMetrics&     aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&          aStatus) override;

  virtual void DidReflow(nsPresContext*   aPresContext,
                         const nsHTMLReflowState*  aReflowState,
                         nsDidReflowStatus aStatus) override;

  virtual bool UpdateOverflow() override;

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) override;

  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) override;

  virtual nsSplittableType GetSplittableType() const override;

  




  virtual nsIAtom* GetType() const override;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const override
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGOuterSVG"), aResult);
  }
#endif

  virtual nsresult  AttributeChanged(int32_t         aNameSpaceID,
                                     nsIAtom*        aAttribute,
                                     int32_t         aModType) override;

  virtual nsContainerFrame* GetContentInsertionFrame() override {
    
    MOZ_ASSERT(GetFirstPrincipalChild() &&
               GetFirstPrincipalChild()->GetType() ==
                 nsGkAtoms::svgOuterSVGAnonChildFrame,
               "Where is our anonymous child?");
    return GetFirstPrincipalChild()->GetContentInsertionFrame();
  }

  virtual bool IsSVGTransformed(Matrix *aOwnTransform,
                                Matrix *aFromParentTransform) const override {
    
    
    
    return GetFirstPrincipalChild()->IsSVGTransformed();
  }

  
  virtual void NotifyViewportOrTransformChanged(uint32_t aFlags) override;

  
  virtual nsresult PaintSVG(gfxContext& aContext,
                            const gfxMatrix& aTransform,
                            const nsIntRect* aDirtyRect = nullptr) override;
  virtual SVGBBox GetBBoxContribution(const Matrix &aToBBoxUserspace,
                                      uint32_t aFlags) override;

  
  virtual gfxMatrix GetCanvasTM() override;

  








  void RegisterForeignObject(nsSVGForeignObjectFrame* aFrame);
  void UnregisterForeignObject(nsSVGForeignObjectFrame* aFrame);

  virtual bool HasChildrenOnlyTransform(Matrix *aTransform) const override {
    
    
    
    
    return false;
  }

  



  bool VerticalScrollbarNotNeeded() const;

  bool IsCallingReflowSVG() const {
    return mCallingReflowSVG;
  }

  void InvalidateSVG(const nsRegion& aRegion)
  {
    if (!aRegion.IsEmpty()) {
      mInvalidRegion.Or(mInvalidRegion, aRegion);
      InvalidateFrame();
    }
  }
  
  void ClearInvalidRegion() { mInvalidRegion.SetEmpty(); }

  const nsRegion& GetInvalidRegion() {
    nsRect rect;
    if (!IsInvalid(rect)) {
      mInvalidRegion.SetEmpty();
    }
    return mInvalidRegion;
  }

  nsRegion FindInvalidatedForeignObjectFrameChildren(nsIFrame* aFrame);

protected:

  bool mCallingReflowSVG;

  



  bool IsRootOfReplacedElementSubDoc(nsIFrame **aEmbeddingFrame = nullptr);

  


  bool IsRootOfImage();

  
  
  
  
  
  nsAutoPtr<nsTHashtable<nsPtrHashKey<nsSVGForeignObjectFrame> > > mForeignObjectHash;

  nsAutoPtr<gfxMatrix> mCanvasTM;

  nsRegion mInvalidRegion; 

  float mFullZoom;

  bool mViewportInitialized;
  bool mIsRootContent;
};




typedef nsSVGDisplayContainerFrame nsSVGOuterSVGAnonChildFrameBase;
























class nsSVGOuterSVGAnonChildFrame
  : public nsSVGOuterSVGAnonChildFrameBase
{
  friend nsContainerFrame*
  NS_NewSVGOuterSVGAnonChildFrame(nsIPresShell* aPresShell,
                                  nsStyleContext* aContext);

  explicit nsSVGOuterSVGAnonChildFrame(nsStyleContext* aContext)
    : nsSVGOuterSVGAnonChildFrameBase(aContext)
  {}

public:
  NS_DECL_FRAMEARENA_HELPERS

#ifdef DEBUG
  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) override;
#endif

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const override {
    return MakeFrameName(NS_LITERAL_STRING("SVGOuterSVGAnonChild"), aResult);
  }
#endif

  




  virtual nsIAtom* GetType() const override;

  
  virtual gfxMatrix GetCanvasTM() override {
    
    
    
    return static_cast<nsSVGOuterSVGFrame*>(GetParent())->GetCanvasTM();
  }

  virtual bool HasChildrenOnlyTransform(Matrix *aTransform) const override;
};

#endif
