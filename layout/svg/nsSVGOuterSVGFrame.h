




#ifndef __NS_SVGOUTERSVGFRAME_H__
#define __NS_SVGOUTERSVGFRAME_H__

#include "gfxMatrix.h"
#include "nsISVGSVGFrame.h"
#include "nsSVGContainerFrame.h"

class nsSVGForeignObjectFrame;




typedef nsSVGDisplayContainerFrame nsSVGOuterSVGFrameBase;

class nsSVGOuterSVGFrame : public nsSVGOuterSVGFrameBase,
                           public nsISVGSVGFrame
{
  friend nsIFrame*
  NS_NewSVGOuterSVGFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
protected:
  nsSVGOuterSVGFrame(nsStyleContext* aContext);

public:
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

#ifdef DEBUG
  ~nsSVGOuterSVGFrame() {
    NS_ASSERTION(mForeignObjectHash.Count() == 0,
                 "foreignObject(s) still registered!");
  }
#endif

  
  virtual nscoord GetMinWidth(nsRenderingContext *aRenderingContext);
  virtual nscoord GetPrefWidth(nsRenderingContext *aRenderingContext);

  virtual IntrinsicSize GetIntrinsicSize();
  virtual nsSize  GetIntrinsicRatio();

  virtual nsSize ComputeSize(nsRenderingContext *aRenderingContext,
                             nsSize aCBSize, nscoord aAvailableWidth,
                             nsSize aMargin, nsSize aBorder, nsSize aPadding,
                             uint32_t aFlags) MOZ_OVERRIDE;

  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  NS_IMETHOD  DidReflow(nsPresContext*   aPresContext,
                        const nsHTMLReflowState*  aReflowState,
                        nsDidReflowStatus aStatus);

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);

  virtual nsSplittableType GetSplittableType() const;

  




  virtual nsIAtom* GetType() const;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGOuterSVG"), aResult);
  }
#endif

  NS_IMETHOD  AttributeChanged(int32_t         aNameSpaceID,
                               nsIAtom*        aAttribute,
                               int32_t         aModType);

  virtual nsIFrame* GetContentInsertionFrame() {
    
    NS_ABORT_IF_FALSE(GetFirstPrincipalChild() &&
                      GetFirstPrincipalChild()->GetType() ==
                        nsGkAtoms::svgOuterSVGAnonChildFrame,
                      "Where is our anonymous child?");
    return GetFirstPrincipalChild()->GetContentInsertionFrame();
  }

  virtual bool IsSVGTransformed(gfxMatrix *aOwnTransform,
                                gfxMatrix *aFromParentTransform) const {
    
    
    return false;
  }

  
  virtual void NotifyViewportOrTransformChanged(uint32_t aFlags);

  
  NS_IMETHOD PaintSVG(nsRenderingContext* aContext,
                      const nsIntRect *aDirtyRect);

  virtual SVGBBox GetBBoxContribution(const gfxMatrix &aToBBoxUserspace,
                                      uint32_t aFlags);

  
  virtual gfxMatrix GetCanvasTM(uint32_t aFor);

  








  void RegisterForeignObject(nsSVGForeignObjectFrame* aFrame);
  void UnregisterForeignObject(nsSVGForeignObjectFrame* aFrame);

  virtual bool HasChildrenOnlyTransform(gfxMatrix *aTransform) const {
    
    
    
    
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

  
  
  
  
  
  nsTHashtable<nsPtrHashKey<nsSVGForeignObjectFrame> > mForeignObjectHash;

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
  friend nsIFrame*
  NS_NewSVGOuterSVGAnonChildFrame(nsIPresShell* aPresShell,
                                  nsStyleContext* aContext);

  nsSVGOuterSVGAnonChildFrame(nsStyleContext* aContext)
    : nsSVGOuterSVGAnonChildFrameBase(aContext)
  {}

public:
  NS_DECL_FRAMEARENA_HELPERS

#ifdef DEBUG
  NS_IMETHOD Init(nsIContent* aContent,
                  nsIFrame* aParent,
                  nsIFrame* aPrevInFlow);

  NS_IMETHOD GetFrameName(nsAString& aResult) const {
    return MakeFrameName(NS_LITERAL_STRING("SVGOuterSVGAnonChild"), aResult);
  }
#endif

  




  virtual nsIAtom* GetType() const;

  virtual bool IsSVGTransformed(gfxMatrix *aOwnTransform,
                                gfxMatrix *aFromParentTransform) const {
    
    
    return false;
  }

  
  virtual gfxMatrix GetCanvasTM(uint32_t aFor) {
    
    
    
    return static_cast<nsSVGOuterSVGFrame*>(mParent)->GetCanvasTM(aFor);
  }

  virtual bool HasChildrenOnlyTransform(gfxMatrix *aTransform) const;
};

#endif
