




#ifndef __NS_SVGOUTERSVGFRAME_H__
#define __NS_SVGOUTERSVGFRAME_H__

#include "mozilla/Attributes.h"
#include "nsISVGSVGFrame.h"
#include "nsSVGContainerFrame.h"
#include "nsRegion.h"

class nsSVGForeignObjectFrame;




typedef nsSVGDisplayContainerFrame nsSVGOuterSVGFrameBase;

class nsSVGOuterSVGFrame MOZ_FINAL : public nsSVGOuterSVGFrameBase,
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

  
  virtual nscoord GetMinISize(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;
  virtual nscoord GetPrefISize(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;

  virtual mozilla::IntrinsicSize GetIntrinsicSize() MOZ_OVERRIDE;
  virtual nsSize  GetIntrinsicRatio() MOZ_OVERRIDE;

  virtual mozilla::LogicalSize
  ComputeSize(nsRenderingContext *aRenderingContext,
              mozilla::WritingMode aWritingMode,
              const mozilla::LogicalSize& aCBSize,
              nscoord aAvailableISize,
              const mozilla::LogicalSize& aMargin,
              const mozilla::LogicalSize& aBorder,
              const mozilla::LogicalSize& aPadding,
              uint32_t aFlags) MOZ_OVERRIDE;

  virtual void Reflow(nsPresContext*           aPresContext,
                      nsHTMLReflowMetrics&     aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&          aStatus) MOZ_OVERRIDE;

  virtual void DidReflow(nsPresContext*   aPresContext,
                         const nsHTMLReflowState*  aReflowState,
                         nsDidReflowStatus aStatus) MOZ_OVERRIDE;

  virtual bool UpdateOverflow() MOZ_OVERRIDE;

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) MOZ_OVERRIDE;

  virtual nsSplittableType GetSplittableType() const MOZ_OVERRIDE;

  




  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGOuterSVG"), aResult);
  }
#endif

  virtual nsresult  AttributeChanged(int32_t         aNameSpaceID,
                                     nsIAtom*        aAttribute,
                                     int32_t         aModType) MOZ_OVERRIDE;

  virtual nsContainerFrame* GetContentInsertionFrame() MOZ_OVERRIDE {
    
    NS_ABORT_IF_FALSE(GetFirstPrincipalChild() &&
                      GetFirstPrincipalChild()->GetType() ==
                        nsGkAtoms::svgOuterSVGAnonChildFrame,
                      "Where is our anonymous child?");
    return GetFirstPrincipalChild()->GetContentInsertionFrame();
  }

  virtual bool IsSVGTransformed(Matrix *aOwnTransform,
                                Matrix *aFromParentTransform) const MOZ_OVERRIDE {
    
    
    
    return GetFirstPrincipalChild()->IsSVGTransformed();
  }

  
  virtual void NotifyViewportOrTransformChanged(uint32_t aFlags) MOZ_OVERRIDE;

  
  virtual nsresult PaintSVG(nsRenderingContext* aContext,
                            const nsIntRect *aDirtyRect,
                            nsIFrame* aTransformRoot = nullptr) MOZ_OVERRIDE;
  virtual SVGBBox GetBBoxContribution(const Matrix &aToBBoxUserspace,
                                      uint32_t aFlags) MOZ_OVERRIDE;

  
  virtual gfxMatrix GetCanvasTM(uint32_t aFor,
                                nsIFrame* aTransformRoot = nullptr) MOZ_OVERRIDE;

  








  void RegisterForeignObject(nsSVGForeignObjectFrame* aFrame);
  void UnregisterForeignObject(nsSVGForeignObjectFrame* aFrame);

  virtual bool HasChildrenOnlyTransform(Matrix *aTransform) const MOZ_OVERRIDE {
    
    
    
    
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
                    nsIFrame*         aPrevInFlow) MOZ_OVERRIDE;
#endif

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE {
    return MakeFrameName(NS_LITERAL_STRING("SVGOuterSVGAnonChild"), aResult);
  }
#endif

  




  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

  
  virtual gfxMatrix GetCanvasTM(uint32_t aFor,
                                nsIFrame* aTransformRoot) MOZ_OVERRIDE {
    
    
    
    return static_cast<nsSVGOuterSVGFrame*>(GetParent())->GetCanvasTM(aFor, aTransformRoot);
  }

  virtual bool HasChildrenOnlyTransform(Matrix *aTransform) const MOZ_OVERRIDE;
};

#endif
