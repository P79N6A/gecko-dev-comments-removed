





































#ifndef NSSVGTSPANFRAME_H
#define NSSVGTSPANFRAME_H

#include "nsSVGTextContainerFrame.h"
#include "nsISVGGlyphFragmentNode.h"

typedef nsSVGTextContainerFrame nsSVGTSpanFrameBase;

class nsSVGTSpanFrame : public nsSVGTSpanFrameBase,
                        public nsISVGGlyphFragmentNode
{
  friend nsIFrame*
  NS_NewSVGTSpanFrame(nsIPresShell* aPresShell, nsIContent* aContent,
                      nsIFrame* parentFrame, nsStyleContext* aContext);
protected:
  nsSVGTSpanFrame(nsStyleContext* aContext) :
    nsSVGTextContainerFrame(aContext),
    mPropagateTransform(PR_TRUE) {}

   
  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);
private:
  NS_IMETHOD_(nsrefcnt) AddRef() { return 1; }
  NS_IMETHOD_(nsrefcnt) Release() { return 1; }

public:
  
  NS_IMETHOD  AttributeChanged(PRInt32         aNameSpaceID,
                               nsIAtom*        aAttribute,
                               PRInt32         aModType);

  




  virtual nsIAtom* GetType() const;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGTSpan"), aResult);
  }
#endif
  
  NS_IMETHOD SetMatrixPropagation(PRBool aPropagate);
  NS_IMETHOD SetOverrideCTM(nsIDOMSVGMatrix *aCTM);
  
  
  virtual already_AddRefed<nsIDOMSVGMatrix> GetCanvasTM();
  
  
  NS_IMETHOD_(PRUint32) GetNumberOfChars();
  NS_IMETHOD_(float) GetComputedTextLength();
  NS_IMETHOD_(float) GetSubStringLength(PRUint32 charnum, PRUint32 fragmentChars);
  NS_IMETHOD_(PRInt32) GetCharNumAtPosition(nsIDOMSVGPoint *point);
  NS_IMETHOD_(nsISVGGlyphFragmentLeaf *) GetFirstGlyphFragment();
  NS_IMETHOD_(nsISVGGlyphFragmentLeaf *) GetNextGlyphFragment();
  NS_IMETHOD_(void) SetWhitespaceHandling(PRUint8 aWhitespaceHandling);

protected:
  nsCOMPtr<nsIDOMSVGMatrix> mOverrideCTM;
  PRPackedBool mPropagateTransform;
};

#endif
