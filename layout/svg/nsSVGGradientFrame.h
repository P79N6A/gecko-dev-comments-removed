




#ifndef __NS_SVGGRADIENTFRAME_H__
#define __NS_SVGGRADIENTFRAME_H__

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
class nsSVGLinearGradientElement;
class nsSVGRadialGradientElement;

struct gfxRect;

namespace mozilla {
class SVGAnimatedTransformList;
}

typedef nsSVGPaintServerFrame nsSVGGradientFrameBase;





class nsSVGGradientFrame : public nsSVGGradientFrameBase
{
protected:
  nsSVGGradientFrame(nsStyleContext* aContext);

public:
  NS_DECL_FRAMEARENA_HELPERS

  
  virtual already_AddRefed<gfxPattern>
    GetPaintServerPattern(nsIFrame *aSource,
                          const gfxMatrix& aContextMatrix,
                          nsStyleSVGPaint nsStyleSVG::*aFillOrStroke,
                          float aGraphicOpacity,
                          const gfxRect *aOverrideBounds);

  
  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext);

  NS_IMETHOD AttributeChanged(int32_t         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              int32_t         aModType);

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGGradient"), aResult);
  }
#endif 

private:

  
  
  
  nsSVGGradientFrame* GetReferencedGradient();

  
  int32_t GetStopFrame(int32_t aIndex, nsIFrame * *aStopFrame);

  uint32_t GetStopCount();
  void GetStopInformation(int32_t aIndex,
                          float *aOffset, nscolor *aColor, float *aStopOpacity);

  const mozilla::SVGAnimatedTransformList* GetGradientTransformList(
    nsIContent* aDefault);
  
  gfxMatrix GetGradientTransform(nsIFrame *aSource,
                                 const gfxRect *aOverrideBounds);

protected:
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

  
  
  virtual nsSVGLinearGradientElement * GetLinearGradientWithLength(
    uint32_t aIndex, nsSVGLinearGradientElement* aDefault);
  virtual nsSVGRadialGradientElement * GetRadialGradientWithLength(
    uint32_t aIndex, nsSVGRadialGradientElement* aDefault);

  
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
  nsSVGLinearGradientFrame(nsStyleContext* aContext) :
    nsSVGLinearGradientFrameBase(aContext) {}

public:
  NS_DECL_FRAMEARENA_HELPERS

  
#ifdef DEBUG
  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);
#endif

  virtual nsIAtom* GetType() const;  

  NS_IMETHOD AttributeChanged(int32_t         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              int32_t         aModType);

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGLinearGradient"), aResult);
  }
#endif 

protected:
  float GetLengthValue(uint32_t aIndex);
  virtual nsSVGLinearGradientElement * GetLinearGradientWithLength(
    uint32_t aIndex, nsSVGLinearGradientElement* aDefault);
  virtual already_AddRefed<gfxPattern> CreateGradient();
};





typedef nsSVGGradientFrame nsSVGRadialGradientFrameBase;

class nsSVGRadialGradientFrame : public nsSVGRadialGradientFrameBase
{
  friend nsIFrame* NS_NewSVGRadialGradientFrame(nsIPresShell* aPresShell,
                                                nsStyleContext* aContext);
protected:
  nsSVGRadialGradientFrame(nsStyleContext* aContext) :
    nsSVGRadialGradientFrameBase(aContext) {}

public:
  NS_DECL_FRAMEARENA_HELPERS

  
#ifdef DEBUG
  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);
#endif

  virtual nsIAtom* GetType() const;  

  NS_IMETHOD AttributeChanged(int32_t         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              int32_t         aModType);

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGRadialGradient"), aResult);
  }
#endif 

protected:
  float GetLengthValue(uint32_t aIndex);
  float GetLengthValue(uint32_t aIndex, float aDefaultValue);
  float GetLengthValueFromElement(uint32_t aIndex,
                                  nsSVGRadialGradientElement& aElement);
  virtual nsSVGRadialGradientElement * GetRadialGradientWithLength(
    uint32_t aIndex, nsSVGRadialGradientElement* aDefault);
  virtual already_AddRefed<gfxPattern> CreateGradient();
};

#endif 

