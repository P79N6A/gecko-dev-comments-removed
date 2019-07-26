




#ifndef NS_SVGTEXTFRAME_H
#define NS_SVGTEXTFRAME_H

#include "gfxMatrix.h"
#include "gfxRect.h"
#include "nsSVGTextContainerFrame.h"

class nsRenderingContext;

typedef nsSVGTextContainerFrame nsSVGTextFrameBase;

class nsSVGTextFrame : public nsSVGTextFrameBase
{
  friend nsIFrame*
  NS_NewSVGTextFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
protected:
  nsSVGTextFrame(nsStyleContext* aContext)
    : nsSVGTextFrameBase(aContext),
      mPositioningDirty(true) {}

public:
  NS_DECL_FRAMEARENA_HELPERS

  
#ifdef DEBUG
  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);
#endif

  NS_IMETHOD  AttributeChanged(int32_t         aNameSpaceID,
                               nsIAtom*        aAttribute,
                               int32_t         aModType);

  




  virtual nsIAtom* GetType() const;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGText"), aResult);
  }
#endif

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  
  virtual void NotifySVGChanged(uint32_t aFlags);
  
  
  NS_IMETHOD PaintSVG(nsRenderingContext* aContext,
                      const nsIntRect *aDirtyRect);
  NS_IMETHOD_(nsIFrame*) GetFrameForPoint(const nsPoint & aPoint);
  virtual void ReflowSVG();
  virtual SVGBBox GetBBoxContribution(const gfxMatrix &aToBBoxUserspace,
                                      uint32_t aFlags);
  
  
  virtual gfxMatrix GetCanvasTM(uint32_t aFor);
  
  
  virtual uint32_t GetNumberOfChars();
  virtual float GetComputedTextLength();
  virtual float GetSubStringLength(uint32_t charnum, uint32_t nchars);
  virtual int32_t GetCharNumAtPosition(mozilla::nsISVGPoint *point);

  NS_IMETHOD GetStartPositionOfChar(uint32_t charnum, nsISupports **_retval);
  NS_IMETHOD GetEndPositionOfChar(uint32_t charnum, nsISupports **_retval);
  NS_IMETHOD GetExtentOfChar(uint32_t charnum, nsIDOMSVGRect **_retval);
  NS_IMETHOD GetRotationOfChar(uint32_t charnum, float *_retval);

  
  void NotifyGlyphMetricsChange();

private:
  




  void UpdateGlyphPositioning(bool aForceGlobalTransform);

  void SetWhitespaceHandling(nsSVGGlyphFrame *aFrame);

  nsAutoPtr<gfxMatrix> mCanvasTM;

  bool mPositioningDirty;
};

#endif
