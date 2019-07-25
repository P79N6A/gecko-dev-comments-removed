




#ifndef __NS_SVGGLYPHFRAME_H__
#define __NS_SVGGLYPHFRAME_H__

#include "gfxFont.h"
#include "nsISVGGlyphFragmentNode.h"
#include "nsISVGChildFrame.h"
#include "nsSVGGeometryFrame.h"
#include "nsSVGUtils.h"
#include "nsTextFragment.h"

class CharacterIterator;
class gfxContext;
class nsDisplaySVGGlyphs;
class nsIDOMSVGRect;
class nsRenderingContext;
class nsSVGGlyphFrame;
class nsSVGTextFrame;
class nsSVGTextPathFrame;

struct CharacterPosition;

typedef gfxFont::DrawMode DrawMode;

typedef nsSVGGeometryFrame nsSVGGlyphFrameBase;

class nsSVGGlyphFrame : public nsSVGGlyphFrameBase,
                        public nsISVGGlyphFragmentNode,
                        public nsISVGChildFrame
{
  class AutoCanvasTMForMarker;
  friend class AutoCanvasTMForMarker;
  friend class CharacterIterator;
  friend nsIFrame*
  NS_NewSVGGlyphFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
protected:
  nsSVGGlyphFrame(nsStyleContext* aContext)
    : nsSVGGlyphFrameBase(aContext),
      mTextRun(nullptr),
      mStartIndex(0),
      mGetCanvasTMForFlag(nsISVGChildFrame::FOR_OUTERSVG_TM),
      mCompressWhitespace(true),
      mTrimLeadingWhitespace(false),
      mTrimTrailingWhitespace(false)
      {}
  ~nsSVGGlyphFrame()
  {
    ClearTextRun();
  }

public:
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  
  nsresult GetStartPositionOfChar(uint32_t charnum, nsIDOMSVGPoint **_retval);
  nsresult GetEndPositionOfChar(uint32_t charnum, nsIDOMSVGPoint **_retval);
  nsresult GetExtentOfChar(uint32_t charnum, nsIDOMSVGRect **_retval);
  nsresult GetRotationOfChar(uint32_t charnum, float *_retval);
  



  float GetAdvance(bool aForceGlobalTransform);

  void SetGlyphPosition(gfxPoint *aPosition, bool aForceGlobalTransform);
  nsSVGTextPathFrame* FindTextPathParent();
  bool IsStartOfChunk(); 

  void GetXY(mozilla::SVGUserUnitList *aX, mozilla::SVGUserUnitList *aY);
  void SetStartIndex(uint32_t aStartIndex);
  



  void GetEffectiveXY(int32_t strLength,
                      nsTArray<float> &aX, nsTArray<float> &aY);
  



  void GetEffectiveDxDy(int32_t strLength, 
                        nsTArray<float> &aDx,
                        nsTArray<float> &aDy);
  



  void GetEffectiveRotate(int32_t strLength,
                          nsTArray<float> &aRotate);
  uint16_t GetTextAnchor();
  bool IsAbsolutelyPositioned();
  bool IsTextEmpty() const {
    return mContent->GetText()->GetLength() == 0;
  }
  void SetTrimLeadingWhitespace(bool aTrimLeadingWhitespace) {
    if (mTrimLeadingWhitespace != aTrimLeadingWhitespace) {
      mTrimLeadingWhitespace = aTrimLeadingWhitespace;
      ClearTextRun();
    }
  }
  void SetTrimTrailingWhitespace(bool aTrimTrailingWhitespace) {
    if (mTrimTrailingWhitespace != aTrimTrailingWhitespace) {
      mTrimTrailingWhitespace = aTrimTrailingWhitespace;
      ClearTextRun();
    }
  }
  bool EndsWithWhitespace() const;
  bool IsAllWhitespace() const;

  
  NS_IMETHOD  CharacterDataChanged(CharacterDataChangeInfo* aInfo);

  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext);

  NS_IMETHOD  IsSelectable(bool* aIsSelectable, uint8_t* aSelectStyle) const;

  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);

  




  virtual nsIAtom* GetType() const;

  virtual bool IsFrameOfType(uint32_t aFlags) const
  {
    
    

    return nsSVGGlyphFrameBase::IsFrameOfType(aFlags & ~(nsIFrame::eReplaced));
  }

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGGlyph"), aResult);
  }
#endif

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  
  
  NS_IMETHOD PaintSVG(nsRenderingContext *aContext,
                      const nsIntRect *aDirtyRect);
  NS_IMETHOD_(nsIFrame*) GetFrameForPoint(const nsPoint &aPoint);
  virtual SVGBBox GetBBoxContribution(const gfxMatrix &aToBBoxUserspace,
                                      uint32_t aFlags);

  NS_IMETHOD_(nsRect) GetCoveredRegion();
  virtual void ReflowSVG();
  virtual void NotifySVGChanged(uint32_t aFlags);
  NS_IMETHOD_(bool) IsDisplayContainer() { return false; }

  
  gfxMatrix GetCanvasTM(uint32_t aFor);

  
  
  virtual uint32_t GetNumberOfChars();
  virtual float GetComputedTextLength();
  virtual float GetSubStringLength(uint32_t charnum, uint32_t fragmentChars);
  virtual int32_t GetCharNumAtPosition(nsIDOMSVGPoint *point);
  NS_IMETHOD_(nsSVGGlyphFrame *) GetFirstGlyphFrame();
  NS_IMETHOD_(nsSVGGlyphFrame *) GetNextGlyphFrame();
  NS_IMETHOD_(void) SetWhitespaceCompression(bool aCompressWhitespace) {
    if (mCompressWhitespace != aCompressWhitespace) {
      mCompressWhitespace = aCompressWhitespace;
      ClearTextRun();
    }
  }

private:

  




  class AutoCanvasTMForMarker {
  public:
    AutoCanvasTMForMarker(nsSVGGlyphFrame *aFrame, uint32_t aFor)
      : mFrame(aFrame)
    {
      mOldFor = mFrame->mGetCanvasTMForFlag;
      mFrame->mGetCanvasTMForFlag = aFor;
    }
    ~AutoCanvasTMForMarker()
    {
      
      mFrame->mGetCanvasTMForFlag = mOldFor;
    }
  private:
    nsSVGGlyphFrame *mFrame;
    uint32_t mOldFor;
  };

  
  
  
  
  static uint32_t GetTextRunUnitsFactor() { return 64; }
  
  








  bool EnsureTextRun(float *aDrawScale, float *aMetricsScale,
                       bool aForceGlobalTransform);
  void ClearTextRun();

  bool GetCharacterData(nsAString & aCharacterData);
  bool GetCharacterPositions(nsTArray<CharacterPosition>* aCharacterPositions,
                               float aMetricsScale);
  uint32_t GetTextRunFlags(uint32_t strLength);

  void AddCharactersToPath(CharacterIterator *aIter,
                           gfxContext *aContext);
  void AddBoundingBoxesToPath(CharacterIterator *aIter,
                              gfxContext *aContext);
  void DrawCharacters(CharacterIterator *aIter,
                      gfxContext *aContext,
                      DrawMode aDrawMode,
                      gfxPattern *aStrokePattern = nullptr);

  void NotifyGlyphMetricsChange();
  void SetupGlobalTransform(gfxContext *aContext, uint32_t aFor);
  nsresult GetHighlight(uint32_t *charnum, uint32_t *nchars,
                        nscolor *foreground, nscolor *background);
  float GetSubStringAdvance(uint32_t charnum, uint32_t fragmentChars,
                            float aMetricsScale);
  gfxFloat GetBaselineOffset(float aMetricsScale);

  virtual void GetDxDy(SVGUserUnitList *aDx, SVGUserUnitList *aDy);
  virtual const SVGNumberList *GetRotate();

  
  
  nsAutoPtr<gfxMatrix> mOverrideCanvasTM;

  
  gfxTextRun *mTextRun;
  gfxPoint mPosition;
  
  uint32_t mStartIndex;
  uint32_t mGetCanvasTMForFlag;
  bool mCompressWhitespace;
  bool mTrimLeadingWhitespace;
  bool mTrimTrailingWhitespace;

private:
  DrawMode SetupCairoState(gfxContext *aContext,
                           gfxPattern **aStrokePattern);
};

#endif
