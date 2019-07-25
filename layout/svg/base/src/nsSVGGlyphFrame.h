





































#ifndef __NS_SVGGLYPHFRAME_H__
#define __NS_SVGGLYPHFRAME_H__

#include "nsSVGGeometryFrame.h"
#include "nsISVGGlyphFragmentLeaf.h"
#include "nsISVGChildFrame.h"
#include "gfxContext.h"
#include "gfxFont.h"
#include "gfxRect.h"
#include "gfxMatrix.h"
#include "nsSVGMatrix.h"

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
      mStartIndex(0),
      mCompressWhitespace(PR_TRUE),
      mTrimLeadingWhitespace(PR_FALSE),
      mTrimTrailingWhitespace(PR_FALSE),
      mPropagateTransform(PR_TRUE)
      {}
  ~nsSVGGlyphFrame()
  {
    ClearTextRun();
  }

public:
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  
  NS_IMETHOD  CharacterDataChanged(CharacterDataChangeInfo* aInfo);

  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext);

  virtual void SetSelected(PRBool        aSelected,
                           SelectionType aType);
  NS_IMETHOD  GetSelected(PRBool *aSelected) const;
  NS_IMETHOD  IsSelectable(PRBool* aIsSelectable, PRUint8* aSelectStyle) const;

  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);

  




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
  virtual gfxRect GetBBoxContribution(const gfxMatrix &aToBBoxUserspace);

  NS_IMETHOD_(nsRect) GetCoveredRegion();
  NS_IMETHOD InitialUpdate();
  virtual void NotifySVGChanged(PRUint32 aFlags);
  NS_IMETHOD NotifyRedrawSuspended();
  NS_IMETHOD NotifyRedrawUnsuspended();
  NS_IMETHOD_(PRBool) IsDisplayContainer() { return PR_FALSE; }
  NS_IMETHOD_(PRBool) HasValidCoveredRect() { return PR_TRUE; }

  
  gfxMatrix GetCanvasTM();

  
  
  NS_IMETHOD GetStartPositionOfChar(PRUint32 charnum, nsIDOMSVGPoint **_retval);
  NS_IMETHOD GetEndPositionOfChar(PRUint32 charnum, nsIDOMSVGPoint **_retval);
  NS_IMETHOD GetExtentOfChar(PRUint32 charnum, nsIDOMSVGRect **_retval);
  NS_IMETHOD GetRotationOfChar(PRUint32 charnum, float *_retval);
  



  NS_IMETHOD_(float) GetAdvance(PRBool aForceGlobalTransform);

  NS_IMETHOD_(void) SetGlyphPosition(gfxPoint *aPosition, PRBool aForceGlobalTransform);
  NS_IMETHOD_(nsSVGTextPathFrame*) FindTextPathParent();
  NS_IMETHOD_(PRBool) IsStartOfChunk(); 

  NS_IMETHOD_(void) GetXY(mozilla::SVGUserUnitList *aX, mozilla::SVGUserUnitList *aY);
  NS_IMETHOD_(void) SetStartIndex(PRUint32 aStartIndex);
  NS_IMETHOD_(void) GetEffectiveXY(PRInt32 strLength,
                                   nsTArray<float> &aX, nsTArray<float> &aY);
  NS_IMETHOD_(void) GetEffectiveDxDy(PRInt32 strLength, 
                                     nsTArray<float> &aDx,
                                     nsTArray<float> &aDy);
  NS_IMETHOD_(void) GetEffectiveRotate(PRInt32 strLength,
                                       nsTArray<float> &aRotate);
  NS_IMETHOD_(PRUint16) GetTextAnchor();
  NS_IMETHOD_(PRBool) IsAbsolutelyPositioned();
  NS_IMETHOD_(PRBool) IsEmpty() const {
    return mContent->GetText()->GetLength() == 0;
  }
  NS_IMETHOD_(void) SetTrimLeadingWhitespace(PRBool aTrimLeadingWhitespace) {
    mTrimLeadingWhitespace = aTrimLeadingWhitespace;
  }
  NS_IMETHOD_(void) SetTrimTrailingWhitespace(PRBool aTrimTrailingWhitespace) {
    mTrimTrailingWhitespace = aTrimTrailingWhitespace;
  }
  NS_IMETHOD_(PRBool) EndsWithWhitespace() const;
  NS_IMETHOD_(PRBool) IsAllWhitespace() const;

  
  
  virtual PRUint32 GetNumberOfChars();
  virtual float GetComputedTextLength();
  virtual float GetSubStringLength(PRUint32 charnum, PRUint32 fragmentChars);
  virtual PRInt32 GetCharNumAtPosition(nsIDOMSVGPoint *point);
  NS_IMETHOD_(nsISVGGlyphFragmentLeaf *) GetFirstGlyphFragment();
  NS_IMETHOD_(nsISVGGlyphFragmentLeaf *) GetNextGlyphFragment();
  NS_IMETHOD_(void) SetWhitespaceCompression(PRBool aCompressWhitespace) {
    mCompressWhitespace = aCompressWhitespace;
  }

protected:
  friend class CharacterIterator;

  
  
  
  
  static PRUint32 GetTextRunUnitsFactor() { return 64; }
  
  








  PRBool EnsureTextRun(float *aDrawScale, float *aMetricsScale,
                       PRBool aForceGlobalTransform);
  void ClearTextRun();

  PRBool GetCharacterData(nsAString & aCharacterData);
  PRBool GetCharacterPositions(nsTArray<CharacterPosition>* aCharacterPositions,
                               float aMetricsScale);
  PRUint32 GetTextRunFlags(PRUint32 strLength);

  void AddCharactersToPath(CharacterIterator *aIter,
                           gfxContext *aContext);
  void AddBoundingBoxesToPath(CharacterIterator *aIter,
                              gfxContext *aContext);
  void FillCharacters(CharacterIterator *aIter,
                      gfxContext *aContext);

  void NotifyGlyphMetricsChange();
  PRBool GetGlobalTransform(gfxMatrix *aMatrix);
  void SetupGlobalTransform(gfxContext *aContext);
  nsresult GetHighlight(PRUint32 *charnum, PRUint32 *nchars,
                        nscolor *foreground, nscolor *background);
  float GetSubStringAdvance(PRUint32 charnum, PRUint32 fragmentChars,
                            float aMetricsScale);
  gfxFloat GetBaselineOffset(float aMetricsScale);

  virtual void GetDxDy(SVGUserUnitList *aDx, SVGUserUnitList *aDy);
  virtual const SVGNumberList *GetRotate();

  
  
  nsCOMPtr<nsIDOMSVGMatrix> mOverrideCanvasTM;

  
  gfxTextRun *mTextRun;
  gfxPoint mPosition;
  
  PRUint32 mStartIndex;
  PRPackedBool mCompressWhitespace;
  PRPackedBool mTrimLeadingWhitespace;
  PRPackedBool mTrimTrailingWhitespace;
  PRPackedBool mPropagateTransform;
};

#endif
