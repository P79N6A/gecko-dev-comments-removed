




#ifndef __NS_SVGGLYPHFRAME_H__
#define __NS_SVGGLYPHFRAME_H__

#include "mozilla/Attributes.h"
#include "gfxFont.h"
#include "gfxSVGGlyphs.h"
#include "nsISVGChildFrame.h"
#include "nsISVGGlyphFragmentNode.h"
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
class gfxTextObjectPaint;

struct CharacterPosition;

namespace mozilla {


struct SVGTextObjectPaint : public gfxTextObjectPaint {
  already_AddRefed<gfxPattern> GetFillPattern(float aOpacity,
                                              const gfxMatrix& aCTM);
  already_AddRefed<gfxPattern> GetStrokePattern(float aOpacity,
                                                const gfxMatrix& aCTM);

  void SetFillOpacity(float aOpacity) { mFillOpacity = aOpacity; }
  float GetFillOpacity() { return mFillOpacity; }

  void SetStrokeOpacity(float aOpacity) { mStrokeOpacity = aOpacity; }
  float GetStrokeOpacity() { return mStrokeOpacity; }

  struct Paint {
    Paint() {
      mPatternCache.Init();
    }

    void SetPaintServer(nsIFrame *aFrame, const gfxMatrix& aContextMatrix,
                        nsSVGPaintServerFrame *aPaintServerFrame) {
      mPaintType = eStyleSVGPaintType_Server;
      mPaintDefinition.mPaintServerFrame = aPaintServerFrame;
      mFrame = aFrame;
      mContextMatrix = aContextMatrix;
    }

    void SetColor(const nscolor &aColor) {
      mPaintType = eStyleSVGPaintType_Color;
      mPaintDefinition.mColor = aColor;
    }

    void SetObjectPaint(gfxTextObjectPaint *aObjectPaint,
                        nsStyleSVGPaintType aPaintType) {
      NS_ASSERTION(aPaintType == eStyleSVGPaintType_ObjectFill ||
                   aPaintType == eStyleSVGPaintType_ObjectStroke,
                   "Invalid object paint type");
      mPaintType = aPaintType;
      mPaintDefinition.mObjectPaint = aObjectPaint;
    }

    union {
      nsSVGPaintServerFrame *mPaintServerFrame;
      gfxTextObjectPaint *mObjectPaint;
      nscolor mColor;
    } mPaintDefinition;

    nsIFrame *mFrame;
    
    gfxMatrix mContextMatrix;
    nsStyleSVGPaintType mPaintType;

    
    gfxMatrix mPatternMatrix;
    nsRefPtrHashtable<nsFloatHashKey, gfxPattern> mPatternCache;

    already_AddRefed<gfxPattern> GetPattern(float aOpacity,
                                            nsStyleSVGPaint nsStyleSVG::*aFillOrStroke,
                                            const gfxMatrix& aCTM);
  };

  Paint mFillPaint;
  Paint mStrokePaint;

  float mFillOpacity;
  float mStrokeOpacity;
};

} 

using namespace mozilla;

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

  
  nsresult GetStartPositionOfChar(uint32_t charnum, nsISupports **_retval);
  nsresult GetEndPositionOfChar(uint32_t charnum, nsISupports **_retval);
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

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  
  
  NS_IMETHOD PaintSVG(nsRenderingContext *aContext,
                      const nsIntRect *aDirtyRect);
  NS_IMETHOD_(nsIFrame*) GetFrameForPoint(const nsPoint &aPoint) MOZ_OVERRIDE;
  virtual SVGBBox GetBBoxContribution(const gfxMatrix &aToBBoxUserspace,
                                      uint32_t aFlags) MOZ_OVERRIDE;

  NS_IMETHOD_(nsRect) GetCoveredRegion() MOZ_OVERRIDE;
  virtual void ReflowSVG() MOZ_OVERRIDE;
  virtual void NotifySVGChanged(uint32_t aFlags) MOZ_OVERRIDE;
  NS_IMETHOD_(bool) IsDisplayContainer() MOZ_OVERRIDE { return false; }

  
  gfxMatrix GetCanvasTM(uint32_t aFor);

  
  
  virtual uint32_t GetNumberOfChars();
  virtual float GetComputedTextLength() MOZ_OVERRIDE;
  virtual float GetSubStringLength(uint32_t charnum, uint32_t fragmentChars) MOZ_OVERRIDE;
  virtual int32_t GetCharNumAtPosition(mozilla::nsISVGPoint *point) MOZ_OVERRIDE;
  NS_IMETHOD_(nsSVGGlyphFrame *) GetFirstGlyphFrame() MOZ_OVERRIDE;
  NS_IMETHOD_(nsSVGGlyphFrame *) GetNextGlyphFrame() MOZ_OVERRIDE;
  NS_IMETHOD_(void) SetWhitespaceCompression(bool aCompressWhitespace) MOZ_OVERRIDE {
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

  
  
  
  
  static int32_t GetTextRunUnitsFactor() { return 64; }

  








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
                      gfxTextObjectPaint *aObjectPaint = nullptr);

  void NotifyGlyphMetricsChange();
  void SetupGlobalTransform(gfxContext *aContext, uint32_t aFor);
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
                           gfxTextObjectPaint *aOuterObjectPaint,
                           gfxTextObjectPaint **aThisObjectPaint);

  



  bool SetupCairoStroke(gfxContext *aContext,
                        gfxTextObjectPaint *aOuterObjectPaint,
                        SVGTextObjectPaint *aThisObjectPaint);

  



  bool SetupCairoFill(gfxContext *aContext,
                      gfxTextObjectPaint *aOuterObjectPaint,
                      SVGTextObjectPaint *aThisObjectPaint);

  




  bool SetupObjectPaint(gfxContext *aContext,
                        nsStyleSVGPaint nsStyleSVG::*aFillOrStroke,
                        float& aOpacity,
                        gfxTextObjectPaint *aObjectPaint);

  









  void SetupInheritablePaint(gfxContext *aContext,
                             float& aOpacity,
                             gfxTextObjectPaint *aOuterObjectPaint,
                             SVGTextObjectPaint::Paint& aTargetPaint,
                             nsStyleSVGPaint nsStyleSVG::*aFillOrStroke,
                             const FramePropertyDescriptor *aProperty);

};

#endif
