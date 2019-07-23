





































#ifndef __NS_SVGGLYPHFRAME_H__
#define __NS_SVGGLYPHFRAME_H__

#include "nsSVGGeometryFrame.h"
#include "nsISVGGlyphFragmentLeaf.h"
#include "nsISVGChildFrame.h"
#include "gfxContext.h"
#include "gfxFont.h"

class nsSVGTextFrame;
class nsSVGGlyphFrame;
class CharacterIterator;
struct CharacterPosition;

typedef nsSVGGeometryFrame nsSVGGlyphFrameBase;

class nsSVGGlyphFrame : public nsSVGGlyphFrameBase,
                        public nsISVGGlyphFragmentLeaf, 
                        public nsISVGChildFrame
{
  friend nsIFrame*
  NS_NewSVGGlyphFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
protected:
  nsSVGGlyphFrame(nsStyleContext* aContext)
    : nsSVGGlyphFrameBase(aContext),
      mTextRun(nsnull),
      mWhitespaceHandling(COMPRESS_WHITESPACE)
      {}
  ~nsSVGGlyphFrame()
  {
    ClearTextRun();
  }

public:
  NS_DECL_QUERYFRAME

  
  NS_IMETHOD  CharacterDataChanged(nsPresContext*  aPresContext,
                                   nsIContent*     aChild,
                                   PRBool          aAppend);

  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext);

  NS_IMETHOD  SetSelected(nsPresContext* aPresContext,
                          nsIDOMRange*    aRange,
                          PRBool          aSelected,
                          nsSpread        aSpread,
                          SelectionType   aType);
  NS_IMETHOD  GetSelected(PRBool *aSelected) const;
  NS_IMETHOD  IsSelectable(PRBool* aIsSelectable, PRUint8* aSelectStyle) const;

#ifdef DEBUG
  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);
#endif

  




  virtual nsIAtom* GetType() const;

  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
  {
    
    

    return nsSVGGlyphFrameBase::IsFrameOfType(aFlags & ~(nsIFrame::eReplaced));
  }

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGGlyph"), aResult);
  }
#endif

  
  
  NS_IMETHOD PaintSVG(nsSVGRenderState *aContext,
                      const nsIntRect *aDirtyRect);
  NS_IMETHOD_(nsIFrame*) GetFrameForPoint(const nsPoint &aPoint);
  NS_IMETHOD UpdateCoveredRegion();
  NS_IMETHOD GetBBox(nsIDOMSVGRect **_retval);

  NS_IMETHOD_(nsRect) GetCoveredRegion();
  NS_IMETHOD InitialUpdate();
  virtual void NotifySVGChanged(PRUint32 aFlags);
  NS_IMETHOD NotifyRedrawSuspended();
  NS_IMETHOD NotifyRedrawUnsuspended();
  NS_IMETHOD SetMatrixPropagation(PRBool aPropagate);
  virtual PRBool GetMatrixPropagation();
  NS_IMETHOD_(PRBool) IsDisplayContainer() { return PR_FALSE; }
  NS_IMETHOD_(PRBool) HasValidCoveredRect() { return PR_TRUE; }

  
  NS_IMETHOD GetCanvasTM(nsIDOMSVGMatrix * *aCTM);

  
  
  NS_IMETHOD GetStartPositionOfChar(PRUint32 charnum, nsIDOMSVGPoint **_retval);
  NS_IMETHOD GetEndPositionOfChar(PRUint32 charnum, nsIDOMSVGPoint **_retval);
  NS_IMETHOD GetExtentOfChar(PRUint32 charnum, nsIDOMSVGRect **_retval);
  NS_IMETHOD GetRotationOfChar(PRUint32 charnum, float *_retval);
  



  NS_IMETHOD_(float) GetBaselineOffset(PRUint16 baselineIdentifier,
                                       PRBool aForceGlobalTransform);
  



  NS_IMETHOD_(float) GetAdvance(PRBool aForceGlobalTransform);

  NS_IMETHOD_(void) SetGlyphPosition(float x, float y);
  NS_IMETHOD_(nsSVGTextPathFrame*) FindTextPathParent();
  NS_IMETHOD_(PRBool) IsStartOfChunk(); 
  NS_IMETHOD_(void) GetAdjustedPosition( float &x,  float &y);

  NS_IMETHOD_(already_AddRefed<nsIDOMSVGLengthList>) GetX();
  NS_IMETHOD_(already_AddRefed<nsIDOMSVGLengthList>) GetY();
  NS_IMETHOD_(already_AddRefed<nsIDOMSVGLengthList>) GetDx();
  NS_IMETHOD_(already_AddRefed<nsIDOMSVGLengthList>) GetDy();
  NS_IMETHOD_(PRUint16) GetTextAnchor();
  NS_IMETHOD_(PRBool) IsAbsolutelyPositioned();

  
  
  NS_IMETHOD_(PRUint32) GetNumberOfChars();
  NS_IMETHOD_(float) GetComputedTextLength();
  NS_IMETHOD_(float) GetSubStringLength(PRUint32 charnum, PRUint32 fragmentChars);
  NS_IMETHOD_(PRInt32) GetCharNumAtPosition(nsIDOMSVGPoint *point);
  NS_IMETHOD_(nsISVGGlyphFragmentLeaf *) GetFirstGlyphFragment();
  NS_IMETHOD_(nsISVGGlyphFragmentLeaf *) GetNextGlyphFragment();
  NS_IMETHOD_(void) SetWhitespaceHandling(PRUint8 aWhitespaceHandling);

protected:
  friend class CharacterIterator;

  
  
  
  
  static PRUint32 GetTextRunUnitsFactor() { return 64; }
  
  








  PRBool EnsureTextRun(float *aDrawScale, float *aMetricsScale,
                       PRBool aForceGlobalTransform);
  void ClearTextRun();

  PRBool GetCharacterData(nsAString & aCharacterData);
  PRBool GetCharacterPositions(nsTArray<CharacterPosition>* aCharacterPositions,
                               float aMetricsScale);

  void AddCharactersToPath(CharacterIterator *aIter,
                           gfxContext *aContext);
  void AddBoundingBoxesToPath(CharacterIterator *aIter,
                              gfxContext *aContext);
  void FillCharacters(CharacterIterator *aIter,
                      gfxContext *aContext);

  void NotifyGlyphMetricsChange();
  PRBool ContainsPoint(const nsPoint &aPoint);
  PRBool GetGlobalTransform(gfxMatrix *aMatrix);
  void SetupGlobalTransform(gfxContext *aContext);
  nsresult GetHighlight(PRUint32 *charnum, PRUint32 *nchars,
                        nscolor *foreground, nscolor *background);

  
  gfxTextRun *mTextRun;
  gfxPoint mPosition;
  PRUint8 mWhitespaceHandling;
};

#endif
