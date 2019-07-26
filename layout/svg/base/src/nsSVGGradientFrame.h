




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

  NS_IMETHOD AttributeChanged(PRInt32         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              PRInt32         aModType);

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGGradient"), aResult);
  }
#endif 

private:

  
  
  
  nsSVGGradientFrame* GetReferencedGradient();

  
  PRInt32 GetStopFrame(PRInt32 aIndex, nsIFrame * *aStopFrame);

  PRUint32 GetStopCount();
  void GetStopInformation(PRInt32 aIndex,
                          float *aOffset, nscolor *aColor, float *aStopOpacity);

  const mozilla::SVGAnimatedTransformList* GetGradientTransformList(
    nsIContent* aDefault);
  
  gfxMatrix GetGradientTransform(nsIFrame *aSource,
                                 const gfxRect *aOverrideBounds);

protected:
  virtual already_AddRefed<gfxPattern> CreateGradient() = 0;

  
  class AutoGradientReferencer;
  nsSVGGradientFrame* GetReferencedGradientIfNotInUse();

  
  PRUint16 GetEnumValue(PRUint32 aIndex, nsIContent *aDefault);
  PRUint16 GetEnumValue(PRUint32 aIndex)
  {
    return GetEnumValue(aIndex, mContent);
  }
  PRUint16 GetGradientUnits();
  PRUint16 GetSpreadMethod();

  
  
  virtual nsSVGLinearGradientElement * GetLinearGradientWithLength(
    PRUint32 aIndex, nsSVGLinearGradientElement* aDefault);
  virtual nsSVGRadialGradientElement * GetRadialGradientWithLength(
    PRUint32 aIndex, nsSVGRadialGradientElement* aDefault);

  
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

  NS_IMETHOD AttributeChanged(PRInt32         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              PRInt32         aModType);

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGLinearGradient"), aResult);
  }
#endif 

protected:
  float GetLengthValue(PRUint32 aIndex);
  virtual nsSVGLinearGradientElement * GetLinearGradientWithLength(
    PRUint32 aIndex, nsSVGLinearGradientElement* aDefault);
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

  NS_IMETHOD AttributeChanged(PRInt32         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              PRInt32         aModType);

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGRadialGradient"), aResult);
  }
#endif 

protected:
  float GetLengthValue(PRUint32 aIndex);
  float GetLengthValue(PRUint32 aIndex, float aDefaultValue);
  float GetLengthValueFromElement(PRUint32 aIndex,
                                  nsSVGRadialGradientElement& aElement);
  virtual nsSVGRadialGradientElement * GetRadialGradientWithLength(
    PRUint32 aIndex, nsSVGRadialGradientElement* aDefault);
  virtual already_AddRefed<gfxPattern> CreateGradient();
};

#endif 

