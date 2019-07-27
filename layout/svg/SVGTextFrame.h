




#ifndef MOZILLA_SVGTEXTFRAME_H
#define MOZILLA_SVGTEXTFRAME_H

#include "mozilla/Attributes.h"
#include "mozilla/RefPtr.h"
#include "mozilla/gfx/2D.h"
#include "gfxMatrix.h"
#include "gfxRect.h"
#include "gfxSVGGlyphs.h"
#include "nsIContent.h" 
#include "nsStubMutationObserver.h"
#include "nsSVGPaintServerFrame.h"

class nsDisplaySVGText;
class nsRenderingContext;
class SVGTextFrame;
class nsTextFrame;

typedef nsSVGDisplayContainerFrame SVGTextFrameBase;

namespace mozilla {

class CharIterator;
class nsISVGPoint;
class TextFrameIterator;
class TextNodeCorrespondenceRecorder;
struct TextRenderedRun;
class TextRenderedRunIterator;

namespace dom {
class SVGIRect;
}








struct CharPosition
{
  CharPosition()
    : mAngle(0),
      mHidden(false),
      mUnaddressable(false),
      mClusterOrLigatureGroupMiddle(false),
      mRunBoundary(false),
      mStartOfChunk(false)
  {
  }

  CharPosition(gfxPoint aPosition, double aAngle)
    : mPosition(aPosition),
      mAngle(aAngle),
      mHidden(false),
      mUnaddressable(false),
      mClusterOrLigatureGroupMiddle(false),
      mRunBoundary(false),
      mStartOfChunk(false)
  {
  }

  static CharPosition Unspecified(bool aUnaddressable)
  {
    CharPosition cp(UnspecifiedPoint(), UnspecifiedAngle());
    cp.mUnaddressable = aUnaddressable;
    return cp;
  }

  bool IsAngleSpecified() const
  {
    return mAngle != UnspecifiedAngle();
  }

  bool IsXSpecified() const
  {
    return mPosition.x != UnspecifiedCoord();
  }

  bool IsYSpecified() const
  {
    return mPosition.y != UnspecifiedCoord();
  }

  gfxPoint mPosition;
  double mAngle;

  
  bool mHidden;

  
  bool mUnaddressable;

  
  bool mClusterOrLigatureGroupMiddle;

  
  bool mRunBoundary;

  
  bool mStartOfChunk;

private:
  static gfxFloat UnspecifiedCoord()
  {
    return std::numeric_limits<gfxFloat>::infinity();
  }

  static double UnspecifiedAngle()
  {
    return std::numeric_limits<double>::infinity();
  }

  static gfxPoint UnspecifiedPoint()
  {
    return gfxPoint(UnspecifiedCoord(), UnspecifiedCoord());
  }
};





class GlyphMetricsUpdater : public nsRunnable {
public:
  NS_DECL_NSIRUNNABLE
  GlyphMetricsUpdater(SVGTextFrame* aFrame) : mFrame(aFrame) { }
  static void Run(SVGTextFrame* aFrame);
  void Revoke() { mFrame = nullptr; }
private:
  SVGTextFrame* mFrame;
};


struct SVGTextContextPaint : public gfxTextContextPaint {
  already_AddRefed<gfxPattern> GetFillPattern(float aOpacity,
                                              const gfxMatrix& aCTM) MOZ_OVERRIDE;
  already_AddRefed<gfxPattern> GetStrokePattern(float aOpacity,
                                                const gfxMatrix& aCTM) MOZ_OVERRIDE;

  void SetFillOpacity(float aOpacity) { mFillOpacity = aOpacity; }
  float GetFillOpacity() MOZ_OVERRIDE { return mFillOpacity; }

  void SetStrokeOpacity(float aOpacity) { mStrokeOpacity = aOpacity; }
  float GetStrokeOpacity() MOZ_OVERRIDE { return mStrokeOpacity; }

  struct Paint {
    Paint() : mPaintType(eStyleSVGPaintType_None) {}

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

    void SetContextPaint(gfxTextContextPaint *aContextPaint,
                         nsStyleSVGPaintType aPaintType) {
      NS_ASSERTION(aPaintType == eStyleSVGPaintType_ContextFill ||
                   aPaintType == eStyleSVGPaintType_ContextStroke,
                   "Invalid context paint type");
      mPaintType = aPaintType;
      mPaintDefinition.mContextPaint = aContextPaint;
    }

    union {
      nsSVGPaintServerFrame *mPaintServerFrame;
      gfxTextContextPaint *mContextPaint;
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



































class SVGTextFrame MOZ_FINAL : public SVGTextFrameBase
{
  friend nsIFrame*
  NS_NewSVGTextFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  friend class mozilla::CharIterator;
  friend class mozilla::GlyphMetricsUpdater;
  friend class mozilla::TextFrameIterator;
  friend class mozilla::TextNodeCorrespondenceRecorder;
  friend struct mozilla::TextRenderedRun;
  friend class mozilla::TextRenderedRunIterator;
  friend class MutationObserver;
  friend class nsDisplaySVGText;

  typedef mozilla::gfx::Path Path;
  typedef mozilla::SVGTextContextPaint SVGTextContextPaint;

protected:
  SVGTextFrame(nsStyleContext* aContext)
    : SVGTextFrameBase(aContext),
      mFontSizeScaleFactor(1.0f),
      mLastContextScale(1.0f),
      mLengthAdjustScaleFactor(1.0f)
  {
    AddStateBits(NS_STATE_SVG_POSITIONING_DIRTY);
  }

  ~SVGTextFrame() {}

public:
  NS_DECL_QUERYFRAME_TARGET(SVGTextFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  
  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) MOZ_OVERRIDE;

  virtual nsresult AttributeChanged(int32_t aNamespaceID,
                                    nsIAtom* aAttribute,
                                    int32_t aModType) MOZ_OVERRIDE;

  virtual nsContainerFrame* GetContentInsertionFrame() MOZ_OVERRIDE
  {
    return GetFirstPrincipalChild()->GetContentInsertionFrame();
  }

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  




  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGText"), aResult);
  }
#endif

  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext) MOZ_OVERRIDE;

  


  virtual void FindCloserFrameForSelection(nsPoint aPoint,
                                          FrameWithDistance* aCurrentBestFrame) MOZ_OVERRIDE;



  
  virtual void NotifySVGChanged(uint32_t aFlags) MOZ_OVERRIDE;
  virtual nsresult PaintSVG(nsRenderingContext* aContext,
                            const nsIntRect* aDirtyRect,
                            nsIFrame* aTransformRoot = nullptr) MOZ_OVERRIDE;
  virtual nsIFrame* GetFrameForPoint(const gfxPoint& aPoint) MOZ_OVERRIDE;
  virtual void ReflowSVG() MOZ_OVERRIDE;
  virtual nsRect GetCoveredRegion() MOZ_OVERRIDE;
  virtual SVGBBox GetBBoxContribution(const Matrix& aToBBoxUserspace,
                                      uint32_t aFlags) MOZ_OVERRIDE;

  
  virtual gfxMatrix GetCanvasTM(uint32_t aFor,
                                nsIFrame* aTransformRoot = nullptr) MOZ_OVERRIDE;
  
  
  uint32_t GetNumberOfChars(nsIContent* aContent);
  float GetComputedTextLength(nsIContent* aContent);
  nsresult SelectSubString(nsIContent* aContent, uint32_t charnum, uint32_t nchars);
  nsresult GetSubStringLength(nsIContent* aContent, uint32_t charnum,
                              uint32_t nchars, float* aResult);
  int32_t GetCharNumAtPosition(nsIContent* aContent, mozilla::nsISVGPoint* point);

  nsresult GetStartPositionOfChar(nsIContent* aContent, uint32_t aCharNum,
                                  mozilla::nsISVGPoint** aResult);
  nsresult GetEndPositionOfChar(nsIContent* aContent, uint32_t aCharNum,
                                mozilla::nsISVGPoint** aResult);
  nsresult GetExtentOfChar(nsIContent* aContent, uint32_t aCharNum,
                           mozilla::dom::SVGIRect** aResult);
  nsresult GetRotationOfChar(nsIContent* aContent, uint32_t aCharNum,
                             float* aResult);

  

  



  void NotifyGlyphMetricsChange();

  



  void ScheduleReflowSVG();

  










  void ReflowSVGNonDisplayText();

  












  void ScheduleReflowSVGNonDisplayText();

  





  bool UpdateFontSizeScaleFactor();

  double GetFontSizeScaleFactor() const;

  




  gfxPoint TransformFramePointToTextChild(const gfxPoint& aPoint,
                                          nsIFrame* aChildFrame);

  





  gfxRect TransformFrameRectToTextChild(const gfxRect& aRect,
                                        nsIFrame* aChildFrame);

  





  gfxRect TransformFrameRectFromTextChild(const nsRect& aRect,
                                          nsIFrame* aChildFrame);

private:
  



  class MutationObserver MOZ_FINAL : public nsStubMutationObserver {
  public:
    MutationObserver(SVGTextFrame* aFrame)
      : mFrame(aFrame)
    {
      MOZ_ASSERT(mFrame, "MutationObserver needs a non-null frame");
      mFrame->GetContent()->AddMutationObserver(this);
    }

    
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
    NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
    NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED
    NS_DECL_NSIMUTATIONOBSERVER_CHARACTERDATACHANGED
    NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTECHANGED

  private:
    ~MutationObserver()
    {
      mFrame->GetContent()->RemoveMutationObserver(this);
    }

    SVGTextFrame* const mFrame;
  };

  



  void MaybeReflowAnonymousBlockChild();

  


  void DoReflow();

  



  void UpdateGlyphPositioning();

  



  void DoGlyphPositioning();

  











  int32_t
  ConvertTextElementCharIndexToAddressableIndex(int32_t aIndex,
                                                nsIContent* aContent);

  









  uint32_t ResolvePositions(nsIContent* aContent, uint32_t aIndex,
                            bool aInTextPath, bool& aForceStartOfChunk,
                            nsTArray<gfxPoint>& aDeltas);

  









  bool ResolvePositions(nsTArray<gfxPoint>& aDeltas, bool aRunPerGlyph);

  




  void DetermineCharPositions(nsTArray<nsPoint>& aPositions);

  



  void AdjustChunksForLineBreaks();

  



















  void AdjustPositionsForClusters();

  



  void DoAnchoring();

  



  void DoTextPathLayout();

  







  bool ShouldRenderAsPath(nsRenderingContext* aContext, nsTextFrame* aFrame,
                          bool& aShouldPaintSVGGlyphs);

  
  nsIFrame* GetTextPathPathFrame(nsIFrame* aTextPathFrame);
  mozilla::TemporaryRef<Path> GetTextPath(nsIFrame* aTextPathFrame);
  gfxFloat GetOffsetScale(nsIFrame* aTextPathFrame);
  gfxFloat GetStartOffset(nsIFrame* aTextPathFrame);

  DrawMode SetupCairoState(gfxContext* aContext,
                           nsIFrame* aFrame,
                           gfxTextContextPaint* aOuterContextPaint,
                           gfxTextContextPaint** aThisContextPaint);

  



  bool SetupCairoStroke(gfxContext* aContext,
                        nsIFrame* aFrame,
                        gfxTextContextPaint* aOuterContextPaint,
                        SVGTextContextPaint* aThisContextPaint);

  



  bool SetupCairoFill(gfxContext* aContext,
                      nsIFrame* aFrame,
                      gfxTextContextPaint* aOuterContextPaint,
                      SVGTextContextPaint* aThisContextPaint);

  









  void SetupInheritablePaint(gfxContext* aContext,
                             nsIFrame* aFrame,
                             float& aOpacity,
                             gfxTextContextPaint* aOuterContextPaint,
                             SVGTextContextPaint::Paint& aTargetPaint,
                             nsStyleSVGPaint nsStyleSVG::*aFillOrStroke,
                             const FramePropertyDescriptor* aProperty);

  


  nsRefPtr<MutationObserver> mMutationObserver;

  


  nsAutoPtr<gfxMatrix> mCanvasTM;

  







  uint32_t mTrailingUndisplayedCharacters;

  


  nsTArray<mozilla::CharPosition> mPositions;

  





















  float mFontSizeScaleFactor;

  




  float mLastContextScale;

  



  float mLengthAdjustScaleFactor;
};

#endif
