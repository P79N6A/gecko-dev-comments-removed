




#ifndef NSSVGTSPANFRAME_H
#define NSSVGTSPANFRAME_H

#include "mozilla/Attributes.h"
#include "gfxMatrix.h"
#include "nsFrame.h"
#include "nsISVGGlyphFragmentNode.h"
#include "nsLiteralString.h"
#include "nsQueryFrame.h"
#include "nsSVGTextContainerFrame.h"

class nsIAtom;
class nsIContent;
class nsIFrame;
class nsIPresShell;
class nsStyleContext;
class nsSVGGlyphFrame;

namespace mozilla {
class nsISVGPoint;
}

typedef nsSVGTextContainerFrame nsSVGTSpanFrameBase;

class nsSVGTSpanFrame : public nsSVGTSpanFrameBase,
                        public nsISVGGlyphFragmentNode
{
  friend nsIFrame*
  NS_NewSVGTSpanFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
protected:
  nsSVGTSpanFrame(nsStyleContext* aContext) :
    nsSVGTSpanFrameBase(aContext) {}

public:
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  
#ifdef DEBUG
  virtual void Init(nsIContent*      aContent,
                    nsIFrame*        aParent,
                    nsIFrame*        aPrevInFlow) MOZ_OVERRIDE;
#endif

  NS_IMETHOD  AttributeChanged(int32_t         aNameSpaceID,
                               nsIAtom*        aAttribute,
                               int32_t         aModType);

  




  virtual nsIAtom* GetType() const;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGTSpan"), aResult);
  }
#endif
  
  virtual gfxMatrix GetCanvasTM(uint32_t aFor);
  
  
  virtual uint32_t GetNumberOfChars();
  virtual float GetComputedTextLength() MOZ_OVERRIDE;
  virtual float GetSubStringLength(uint32_t charnum, uint32_t fragmentChars) MOZ_OVERRIDE;
  virtual int32_t GetCharNumAtPosition(mozilla::nsISVGPoint *point) MOZ_OVERRIDE;
  NS_IMETHOD_(nsSVGGlyphFrame *) GetFirstGlyphFrame() MOZ_OVERRIDE;
  NS_IMETHOD_(nsSVGGlyphFrame *) GetNextGlyphFrame() MOZ_OVERRIDE;
  NS_IMETHOD_(void) SetWhitespaceCompression(bool aCompressWhitespace) MOZ_OVERRIDE;
};

#endif
