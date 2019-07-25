




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

  
  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists) {
    return NS_OK;
  }

  NS_IMETHOD AttributeChanged(int32_t         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              int32_t         aModType);

  





  nsresult PaintFilteredFrame(nsRenderingContext *aContext,
                              nsIFrame *aFilteredFrame,
                              nsSVGFilterPaintCallback *aPaintCallback,
                              const nsRect* aDirtyArea);

  





  nsRect GetPostFilterDirtyArea(nsIFrame *aFilteredFrame,
                                const nsRect& aPreFilterDirtyRect);

  





  nsRect GetPreFilterNeededArea(nsIFrame *aFilteredFrame,
                                const nsRect& aPostFilterDirtyRect);

  







  nsRect GetPostFilterBounds(nsIFrame *aFilteredFrame,
                             const gfxRect *aOverrideBBox = nullptr,
                             const nsRect *aPreFilterBounds = nullptr);

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

  
  uint16_t GetEnumValue(uint32_t aIndex, nsIContent *aDefault);
  uint16_t GetEnumValue(uint32_t aIndex)
  {
    return GetEnumValue(aIndex, mContent);
  }
  const nsSVGIntegerPair *GetIntegerPairValue(uint32_t aIndex, nsIContent *aDefault);
  const nsSVGIntegerPair *GetIntegerPairValue(uint32_t aIndex)
  {
    return GetIntegerPairValue(aIndex, mContent);
  }
  const nsSVGLength2 *GetLengthValue(uint32_t aIndex, nsIContent *aDefault);
  const nsSVGLength2 *GetLengthValue(uint32_t aIndex)
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
