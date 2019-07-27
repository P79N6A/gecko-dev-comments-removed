





#include "nsIAnonymousContentCreator.h"
#include "nsSVGEffects.h"
#include "nsSVGGFrame.h"
#include "mozilla/dom/SVGUseElement.h"
#include "nsContentList.h"

typedef nsSVGGFrame nsSVGUseFrameBase;

using namespace mozilla::dom;

class nsSVGUseFrame : public nsSVGUseFrameBase,
                      public nsIAnonymousContentCreator
{
  friend nsIFrame*
  NS_NewSVGUseFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

protected:
  explicit nsSVGUseFrame(nsStyleContext* aContext) :
    nsSVGUseFrameBase(aContext),
    mHasValidDimensions(true)
  {}

public:
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  
  
  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) MOZ_OVERRIDE;

  virtual nsresult  AttributeChanged(int32_t         aNameSpaceID,
                                     nsIAtom*        aAttribute,
                                     int32_t         aModType) MOZ_OVERRIDE;

  virtual void DestroyFrom(nsIFrame* aDestructRoot) MOZ_OVERRIDE;

  




  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

  virtual bool IsLeaf() const MOZ_OVERRIDE;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGUse"), aResult);
  }
#endif

  
  virtual void ReflowSVG() MOZ_OVERRIDE;
  virtual void NotifySVGChanged(uint32_t aFlags) MOZ_OVERRIDE;

  
  virtual nsresult CreateAnonymousContent(nsTArray<ContentInfo>& aElements) MOZ_OVERRIDE;
  virtual void AppendAnonymousContentTo(nsTArray<nsIContent*>& aElements,
                                        uint32_t aFilter) MOZ_OVERRIDE;

private:
  bool mHasValidDimensions;
};




nsIFrame*
NS_NewSVGUseFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsSVGUseFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGUseFrame)

nsIAtom *
nsSVGUseFrame::GetType() const
{
  return nsGkAtoms::svgUseFrame;
}




NS_QUERYFRAME_HEAD(nsSVGUseFrame)
  NS_QUERYFRAME_ENTRY(nsIAnonymousContentCreator)
NS_QUERYFRAME_TAIL_INHERITING(nsSVGUseFrameBase)




void
nsSVGUseFrame::Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow)
{
  NS_ASSERTION(aContent->IsSVG(nsGkAtoms::use),
               "Content is not an SVG use!");

  mHasValidDimensions =
    static_cast<SVGUseElement*>(aContent)->HasValidDimensions();

  nsSVGUseFrameBase::Init(aContent, aParent, aPrevInFlow);
}

nsresult
nsSVGUseFrame::AttributeChanged(int32_t         aNameSpaceID,
                                nsIAtom*        aAttribute,
                                int32_t         aModType)
{
  SVGUseElement *useElement = static_cast<SVGUseElement*>(mContent);

  if (aNameSpaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::x ||
        aAttribute == nsGkAtoms::y) {
      
      mCanvasTM = nullptr;
      nsSVGEffects::InvalidateRenderingObservers(this);
      nsSVGUtils::ScheduleReflowSVG(this);
      nsSVGUtils::NotifyChildrenOfSVGChange(this, TRANSFORM_CHANGED);
    } else if (aAttribute == nsGkAtoms::width ||
               aAttribute == nsGkAtoms::height) {
      bool invalidate = false;
      if (mHasValidDimensions != useElement->HasValidDimensions()) {
        mHasValidDimensions = !mHasValidDimensions;
        invalidate = true;
      }
      if (useElement->OurWidthAndHeightAreUsed()) {
        invalidate = true;
        useElement->SyncWidthOrHeight(aAttribute);
      }
      if (invalidate) {
        nsSVGEffects::InvalidateRenderingObservers(this);
        nsSVGUtils::ScheduleReflowSVG(this);
      }
    }
  } else if (aNameSpaceID == kNameSpaceID_XLink &&
             aAttribute == nsGkAtoms::href) {
    
    nsSVGEffects::InvalidateRenderingObservers(this);
    nsSVGUtils::ScheduleReflowSVG(this);
    useElement->mOriginal = nullptr;
    useElement->UnlinkSource();
    useElement->TriggerReclone();
  }

  return nsSVGUseFrameBase::AttributeChanged(aNameSpaceID,
                                             aAttribute, aModType);
}

void
nsSVGUseFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  nsRefPtr<SVGUseElement> use = static_cast<SVGUseElement*>(mContent);
  nsSVGUseFrameBase::DestroyFrom(aDestructRoot);
  use->DestroyAnonymousContent();
}

bool
nsSVGUseFrame::IsLeaf() const
{
  return true;
}





void
nsSVGUseFrame::ReflowSVG()
{
  
  
  
  float x, y;
  static_cast<SVGUseElement*>(mContent)->
    GetAnimatedLengthValues(&x, &y, nullptr);
  mRect.MoveTo(nsLayoutUtils::RoundGfxRectToAppRect(
                 gfxRect(x, y, 0.0, 0.0),
                 PresContext()->AppUnitsPerCSSPixel()).TopLeft());

  
  
  if (StyleSVGReset()->HasFilters()) {
    InvalidateFrame();
  }

  nsSVGUseFrameBase::ReflowSVG();
}

void
nsSVGUseFrame::NotifySVGChanged(uint32_t aFlags)
{
  if (aFlags & COORD_CONTEXT_CHANGED &&
      !(aFlags & TRANSFORM_CHANGED)) {
    
    
    SVGUseElement *use = static_cast<SVGUseElement*>(mContent);
    if (use->mLengthAttributes[SVGUseElement::ATTR_X].IsPercentage() ||
        use->mLengthAttributes[SVGUseElement::ATTR_Y].IsPercentage()) {
      aFlags |= TRANSFORM_CHANGED;
      
      
      
      
      
      
      nsSVGUtils::ScheduleReflowSVG(this);
    }
  }

  
  
  

  nsSVGUseFrameBase::NotifySVGChanged(aFlags);
}




nsresult
nsSVGUseFrame::CreateAnonymousContent(nsTArray<ContentInfo>& aElements)
{
  SVGUseElement *use = static_cast<SVGUseElement*>(mContent);

  nsIContent* clone = use->CreateAnonymousContent();
  nsSVGEffects::InvalidateRenderingObservers(this);
  if (!clone)
    return NS_ERROR_FAILURE;
  if (!aElements.AppendElement(clone))
    return NS_ERROR_OUT_OF_MEMORY;
  return NS_OK;
}

void
nsSVGUseFrame::AppendAnonymousContentTo(nsTArray<nsIContent*>& aElements,
                                        uint32_t aFilter)
{
  SVGUseElement *use = static_cast<SVGUseElement*>(mContent);
  nsIContent* clone = use->GetAnonymousContent();
  if (clone) {
    aElements.AppendElement(clone);
  }
}
