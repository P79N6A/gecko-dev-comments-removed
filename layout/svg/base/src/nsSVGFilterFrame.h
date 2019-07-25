



































#ifndef __NS_SVGFILTERFRAME_H__
#define __NS_SVGFILTERFRAME_H__

#include "nsFrame.h"
#include "nsQueryFrame.h"
#include "nsRect.h"
#include "nsSVGContainerFrame.h"
#include "nsSVGUtils.h"

class nsIAtom;
class nsIContent;
class nsIFrame;
class nsIPresShell;
class nsRenderingContext;
class nsStyleContext;
class nsSVGFilterPaintCallback;
class nsSVGFilterElement;
class nsSVGIntegerPair;
class nsSVGLength2;

typedef nsSVGContainerFrame nsSVGFilterFrameBase;

class nsSVGFilterFrame : public nsSVGFilterFrameBase
{
  friend nsIFrame*
  NS_NewSVGFilterFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
protected:
  nsSVGFilterFrame(nsStyleContext* aContext)
    : nsSVGFilterFrameBase(aContext),
      mLoopFlag(false),
      mNoHRefURI(false)
  {
    AddStateBits(NS_STATE_SVG_NONDISPLAY_CHILD);
  }

public:
  NS_DECL_FRAMEARENA_HELPERS

  NS_IMETHOD AttributeChanged(PRInt32         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              PRInt32         aModType);

  nsresult FilterPaint(nsRenderingContext *aContext,
                       nsIFrame *aTarget, nsSVGFilterPaintCallback *aPaintCallback,
                       const nsIntRect* aDirtyRect);

  




  nsIntRect GetInvalidationBBox(nsIFrame *aTarget, const nsIntRect& aRect);

  





  nsIntRect GetSourceForInvalidArea(nsIFrame *aTarget, const nsIntRect& aRect);

  





  nsIntRect GetFilterBBox(nsIFrame *aTarget, const nsIntRect *aSourceBBox);

#ifdef DEBUG
  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);
#endif

  




  virtual nsIAtom* GetType() const;

private:
  
  
  
  class AutoFilterReferencer;
  friend class nsAutoFilterInstance;
  nsSVGFilterFrame* GetReferencedFilter();
  nsSVGFilterFrame* GetReferencedFilterIfNotInUse();

  
  PRUint16 GetEnumValue(PRUint32 aIndex, nsIContent *aDefault);
  PRUint16 GetEnumValue(PRUint32 aIndex)
  {
    return GetEnumValue(aIndex, mContent);
  }
  const nsSVGIntegerPair *GetIntegerPairValue(PRUint32 aIndex, nsIContent *aDefault);
  const nsSVGIntegerPair *GetIntegerPairValue(PRUint32 aIndex)
  {
    return GetIntegerPairValue(aIndex, mContent);
  }
  const nsSVGLength2 *GetLengthValue(PRUint32 aIndex, nsIContent *aDefault);
  const nsSVGLength2 *GetLengthValue(PRUint32 aIndex)
  {
    return GetLengthValue(aIndex, mContent);
  }
  const nsSVGFilterElement *GetFilterContent(nsIContent *aDefault);
  const nsSVGFilterElement *GetFilterContent()
  {
    return GetFilterContent(mContent);
  }

  
  bool                              mLoopFlag;
  bool                              mNoHRefURI;
};

#endif
