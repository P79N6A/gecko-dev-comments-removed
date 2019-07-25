




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
                             PRUint32 aFlags) MOZ_OVERRIDE;

  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  NS_IMETHOD  DidReflow(nsPresContext*   aPresContext,
                        const nsHTMLReflowState*  aReflowState,
                        nsDidReflowStatus aStatus);

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);

  virtual nsSplittableType GetSplittableType() const;

  




  virtual nsIAtom* GetType() const;

  void Paint(const nsDisplayListBuilder* aBuilder,
             nsRenderingContext* aContext,
             const nsRect& aDirtyRect, nsPoint aPt);

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGOuterSVG"), aResult);
  }
#endif

  NS_IMETHOD  AttributeChanged(PRInt32         aNameSpaceID,
                               nsIAtom*        aAttribute,
                               PRInt32         aModType);

  virtual bool IsSVGTransformed(gfxMatrix *aOwnTransform,
                                gfxMatrix *aFromParentTransform) const {
    
    
    return false;
  }

  
  virtual void NotifyViewportOrTransformChanged(PRUint32 aFlags);

  
  virtual gfxMatrix GetCanvasTM();

  








  void RegisterForeignObject(nsSVGForeignObjectFrame* aFrame);
  void UnregisterForeignObject(nsSVGForeignObjectFrame* aFrame);

  virtual bool HasChildrenOnlyTransform(gfxMatrix *aTransform) const;

#ifdef XP_MACOSX
  bool BitmapFallbackEnabled() const {
    return mEnableBitmapFallback;
  }
  void SetBitmapFallbackEnabled(bool aVal) {
    NS_NOTREACHED("don't think me need this any more"); 
    mEnableBitmapFallback = aVal;
  }
#endif

  



  bool VerticalScrollbarNotNeeded() const;

#ifdef DEBUG
  bool IsCallingUpdateBounds() const {
    return mCallingUpdateBounds;
  }
#endif

  void InvalidateSVG(const nsRegion& aRegion)
  {
    if (!aRegion.IsEmpty()) {
      mInvalidRegion.Or(mInvalidRegion, aRegion);
      InvalidateFrame();
    }
  }
  
  void ClearInvalidRegion() { mInvalidRegion.SetEmpty(); }

  const nsRegion& GetInvalidRegion() {
    if (!IsInvalid()) {
      mInvalidRegion.SetEmpty();
    }
    return mInvalidRegion;
  }

  nsRegion FindInvalidatedForeignObjectFrameChildren(nsIFrame* aFrame);

protected:

#ifdef DEBUG
  bool mCallingUpdateBounds;
#endif

  



  bool IsRootOfReplacedElementSubDoc(nsIFrame **aEmbeddingFrame = nsnull);

  


  bool IsRootOfImage();

  
  
  
  
  
  nsTHashtable<nsPtrHashKey<nsSVGForeignObjectFrame> > mForeignObjectHash;

  nsAutoPtr<gfxMatrix> mCanvasTM;

  nsRegion mInvalidRegion; 

  float mFullZoom;

  bool mViewportInitialized;
#ifdef XP_MACOSX
  bool mEnableBitmapFallback;
#endif
  bool mIsRootContent;
};

#endif
