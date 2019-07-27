




#ifndef __NS_SVGGRADIENTFRAME_H__
#define __NS_SVGGRADIENTFRAME_H__

#include "mozilla/Attributes.h"
#include "gfxMatrix.h"
#include "nsCOMPtr.h"
#include "nsFrame.h"
#include "nsLiteralString.h"
#include "nsSVGPaintServerFrame.h"

class gfxPattern;
class nsIAtom;
class nsIContent;
class nsIFrame;
class nsIPresShell;
class nsStyleContext;

struct gfxRect;

namespace mozilla {
class nsSVGAnimatedTransformList;

namespace dom {
class SVGLinearGradientElement;
class SVGRadialGradientElement;
} 
} 

typedef nsSVGPaintServerFrame nsSVGGradientFrameBase;





class nsSVGGradientFrame : public nsSVGGradientFrameBase
{
protected:
  explicit nsSVGGradientFrame(nsStyleContext* aContext);

public:
  NS_DECL_FRAMEARENA_HELPERS

  
  virtual already_AddRefed<gfxPattern>
    GetPaintServerPattern(nsIFrame *aSource,
                          const gfxMatrix& aContextMatrix,
                          nsStyleSVGPaint nsStyleSVG::*aFillOrStroke,
                          float aGraphicOpacity,
                          const gfxRect *aOverrideBounds) MOZ_OVERRIDE;

  
  virtual nsresult AttributeChanged(int32_t         aNameSpaceID,
                                    nsIAtom*        aAttribute,
                                    int32_t         aModType) MOZ_OVERRIDE;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGGradient"), aResult);
  }
#endif 

private:

  
  
  
  nsSVGGradientFrame* GetReferencedGradient();

  
  void GetStopFrames(nsTArray<nsIFrame*>* aStopFrames);

  const mozilla::nsSVGAnimatedTransformList* GetGradientTransformList(
    nsIContent* aDefault);
  
  gfxMatrix GetGradientTransform(nsIFrame *aSource,
                                 const gfxRect *aOverrideBounds);

protected:
  virtual bool GradientVectorLengthIsZero() = 0;
  virtual already_AddRefed<gfxPattern> CreateGradient() = 0;

  
  class AutoGradientReferencer;
  nsSVGGradientFrame* GetReferencedGradientIfNotInUse();

  
  uint16_t GetEnumValue(uint32_t aIndex, nsIContent *aDefault);
  uint16_t GetEnumValue(uint32_t aIndex)
  {
    return GetEnumValue(aIndex, mContent);
  }
  uint16_t GetGradientUnits();
  uint16_t GetSpreadMethod();

  
  
  virtual mozilla::dom::SVGLinearGradientElement * GetLinearGradientWithLength(
    uint32_t aIndex, mozilla::dom::SVGLinearGradientElement* aDefault);
  virtual mozilla::dom::SVGRadialGradientElement * GetRadialGradientWithLength(
    uint32_t aIndex, mozilla::dom::SVGRadialGradientElement* aDefault);

  
  nsIFrame*                              mSource;

private:
  
  
  
  bool                                   mLoopFlag;
  
  
  bool                                   mNoHRefURI;
};






typedef nsSVGGradientFrame nsSVGLinearGradientFrameBase;

class nsSVGLinearGradientFrame : public nsSVGLinearGradientFrameBase
{
  friend nsIFrame* NS_NewSVGLinearGradientFrame(nsIPresShell* aPresShell,
                                                nsStyleContext* aContext);
protected:
  explicit nsSVGLinearGradientFrame(nsStyleContext* aContext) :
    nsSVGLinearGradientFrameBase(aContext) {}

public:
  NS_DECL_FRAMEARENA_HELPERS

  
#ifdef DEBUG
  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) MOZ_OVERRIDE;
#endif

  virtual nsIAtom* GetType() const MOZ_OVERRIDE;  

  virtual nsresult AttributeChanged(int32_t         aNameSpaceID,
                                    nsIAtom*        aAttribute,
                                    int32_t         aModType) MOZ_OVERRIDE;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGLinearGradient"), aResult);
  }
#endif 

protected:
  float GetLengthValue(uint32_t aIndex);
  virtual mozilla::dom::SVGLinearGradientElement* GetLinearGradientWithLength(
    uint32_t aIndex, mozilla::dom::SVGLinearGradientElement* aDefault) MOZ_OVERRIDE;
  virtual bool GradientVectorLengthIsZero() MOZ_OVERRIDE;
  virtual already_AddRefed<gfxPattern> CreateGradient() MOZ_OVERRIDE;
};





typedef nsSVGGradientFrame nsSVGRadialGradientFrameBase;

class nsSVGRadialGradientFrame : public nsSVGRadialGradientFrameBase
{
  friend nsIFrame* NS_NewSVGRadialGradientFrame(nsIPresShell* aPresShell,
                                                nsStyleContext* aContext);
protected:
  explicit nsSVGRadialGradientFrame(nsStyleContext* aContext) :
    nsSVGRadialGradientFrameBase(aContext) {}

public:
  NS_DECL_FRAMEARENA_HELPERS

  
#ifdef DEBUG
  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) MOZ_OVERRIDE;
#endif

  virtual nsIAtom* GetType() const MOZ_OVERRIDE;  

  virtual nsresult AttributeChanged(int32_t         aNameSpaceID,
                                    nsIAtom*        aAttribute,
                                    int32_t         aModType) MOZ_OVERRIDE;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGRadialGradient"), aResult);
  }
#endif 

protected:
  float GetLengthValue(uint32_t aIndex);
  float GetLengthValue(uint32_t aIndex, float aDefaultValue);
  float GetLengthValueFromElement(uint32_t aIndex,
                                  mozilla::dom::SVGRadialGradientElement& aElement);
  virtual mozilla::dom::SVGRadialGradientElement* GetRadialGradientWithLength(
    uint32_t aIndex, mozilla::dom::SVGRadialGradientElement* aDefault) MOZ_OVERRIDE;
  virtual bool GradientVectorLengthIsZero() MOZ_OVERRIDE;
  virtual already_AddRefed<gfxPattern> CreateGradient() MOZ_OVERRIDE;
};

#endif 

