





































#ifndef __NS_SVGGLYPHFRAME_H__
#define __NS_SVGGLYPHFRAME_H__

#include "nsSVGGeometryFrame.h"
#include "nsISVGGlyphFragmentNode.h"
#include "nsISVGChildFrame.h"
#include "nsSVGUtils.h"
#include "gfxContext.h"
#include "gfxFont.h"
#include "nsTextFragment.h"

class nsSVGTextFrame;
class nsSVGTextPathFrame;
class nsSVGGlyphFrame;
class CharacterIterator;
struct CharacterPosition;

typedef nsSVGGeometryFrame nsSVGGlyphFrameBase;

class nsSVGGlyphFrame : public nsSVGGlyphFrameBase,
                        public nsISVGGlyphFragmentNode,
                        public nsISVGChildFrame
{
  friend nsIFrame*
  NS_NewSVGGlyphFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
protected:
  nsSVGGlyphFrame(nsStyleContext* aContext)
    : nsSVGGlyphFrameBase(aContext),
      mTextRun(nsnull),
      mStartIndex(0),
      mCompressWhitespace(true),
      mTrimLeadingWhitespace(false),
      mTrimTrailingWhitespace(false),
      mPropagateTransform(true)
      {}
  ~nsSVGGlyphFrame()
  {
    ClearTextRun();
  }

public:
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  
  nsresult GetStartPositionOfChar(PRUint32 charnum, nsIDOMSVGPoint **_retval);
  nsresult GetEndPositionOfChar(PRUint32 charnum, nsIDOMSVGPoint **_retval);
  nsresult GetExtentOfChar(PRUint32 charnum, nsIDOMSVGRect **_retval);
  nsresult GetRotationOfChar(PRUint32 charnum, float *_retval);
  



  float GetAdvance(bool aForceGlobalTransform);

  void SetGlyphPosition(gfxPoint *aPosition, bool aForceGlobalTransform);
  nsSVGTextPathFrame* FindTextPathParent();
  bool IsStartOfChunk(); 

  void GetXY(mozilla::SVGUserUnitList *aX, mozilla::SVGUserUnitList *aY);
  void SetStartIndex(PRUint32 aStartIndex);
  



  void GetEffectiveXY(PRInt32 strLength,
                      nsTArray<float> &aX, nsTArray<float> &aY);
  



  void GetEffectiveDxDy(PRInt32 strLength, 
                        nsTArray<float> &aDx,
                        nsTArray<float> &aDy);
  



  void GetEffectiveRotate(PRInt32 strLength,
                          nsTArray<float> &aRotate);
  PRUint16 GetTextAnchor();
  bool IsAbsolutelyPositioned();
  bool IsTextEmpty() const {
    return mContent->GetText()->GetLength() == 0;
  }
  void SetTrimLeadingWhitespace(bool aTrimLeadingWhitespace) {
    mTrimLeadingWhitespace = aTrimLeadingWhitespace;
  }
  void SetTrimTrailingWhitespace(bool aTrimTrailingWhitespace) {
    mTrimTrailingWhitespace = aTrimTrailingWhitespace;
  }
  bool EndsWithWhitespace() const;
  bool IsAllWhitespace() const;

  
  NS_IMETHOD  CharacterDataChanged(CharacterDataChangeInfo* aInfo);

  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext);

  NS_IMETHOD  IsSelectable(bool* aIsSelectable, PRUint8* aSelectStyle) const;

  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);

  




  virtual nsIAtom* GetType() const;

  virtual bool IsFrameOfType(PRUint32 aFlags) const
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
  virtual gfxRect GetBBoxContribution(const gfxMatrix &aToBBoxUserspace,
                                      PRUint32 aFlags);

  NS_IMETHOD_(nsRect) GetCoveredRegion();
  NS_IMETHOD InitialUpdate();
  virtual void NotifySVGChanged(PRUint32 aFlags);
  NS_IMETHOD NotifyRedrawSuspended();
  NS_IMETHOD NotifyRedrawUnsuspended();
  NS_IMETHOD_(bool) IsDisplayContainer() { return false; }
  NS_IMETHOD_(bool) HasValidCoveredRect() {
    return !(GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD);
  }

  
  gfxMatrix GetCanvasTM();

  
  
  virtual PRUint32 GetNumberOfChars();
  virtual float GetComputedTextLength();
  virtual float GetSubStringLength(PRUint32 charnum, PRUint32 fragmentChars);
  virtual PRInt32 GetCharNumAtPosition(nsIDOMSVGPoint *point);
  NS_IMETHOD_(nsSVGGlyphFrame *) GetFirstGlyphFrame();
  NS_IMETHOD_(nsSVGGlyphFrame *) GetNextGlyphFrame();
  NS_IMETHOD_(void) SetWhitespaceCompression(bool aCompressWhitespace) {
    mCompressWhitespace = aCompressWhitespace;
  }

protected:
  friend class CharacterIterator;

  
  
  
  
  static PRUint32 GetTextRunUnitsFactor() { return 64; }
  
  








  bool EnsureTextRun(float *aDrawScale, float *aMetricsScale,
                       bool aForceGlobalTransform);
  void ClearTextRun();

  bool GetCharacterData(nsAString & aCharacterData);
  bool GetCharacterPositions(nsTArray<CharacterPosition>* aCharacterPositions,
                               float aMetricsScale);
  PRUint32 GetTextRunFlags(PRUint32 strLength);

  void AddCharactersToPath(CharacterIterator *aIter,
                           gfxContext *aContext);
  void AddBoundingBoxesToPath(CharacterIterator *aIter,
                              gfxContext *aContext);
  void FillCharacters(CharacterIterator *aIter,
                      gfxContext *aContext);

  void NotifyGlyphMetricsChange();
  bool GetGlobalTransform(gfxMatrix *aMatrix);
  void SetupGlobalTransform(gfxContext *aContext);
  nsresult GetHighlight(PRUint32 *charnum, PRUint32 *nchars,
                        nscolor *foreground, nscolor *background);
  float GetSubStringAdvance(PRUint32 charnum, PRUint32 fragmentChars,
                            float aMetricsScale);
  gfxFloat GetBaselineOffset(float aMetricsScale);

  virtual void GetDxDy(SVGUserUnitList *aDx, SVGUserUnitList *aDy);
  virtual const SVGNumberList *GetRotate();

  
  
  nsAutoPtr<gfxMatrix> mOverrideCanvasTM;

  
  gfxTextRun *mTextRun;
  gfxPoint mPosition;
  
  PRUint32 mStartIndex;
  bool mCompressWhitespace;
  bool mTrimLeadingWhitespace;
  bool mTrimTrailingWhitespace;
  bool mPropagateTransform;
};

#endif
