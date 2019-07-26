




#ifndef NS_SVGTEXTFRAME2_H
#define NS_SVGTEXTFRAME2_H

#include "mozilla/Attributes.h"
#include "gfxMatrix.h"
#include "gfxRect.h"
#include "gfxSVGGlyphs.h"
#include "nsStubMutationObserver.h"
#include "nsSVGGlyphFrame.h" 
#include "nsSVGTextContainerFrame.h"

class nsDisplaySVGText;
class nsRenderingContext;
class nsSVGTextFrame2;
class nsTextFrame;
class gfxFlattenedPath;

typedef nsSVGDisplayContainerFrame nsSVGTextFrame2Base;

namespace mozilla {

class CharIterator;
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
  GlyphMetricsUpdater(nsSVGTextFrame2* aFrame) : mFrame(aFrame) { }
  static void Run(nsSVGTextFrame2* aFrame);
  void Revoke() { mFrame = nullptr; }
private:
  nsSVGTextFrame2* mFrame;
};

}



































class nsSVGTextFrame2 : public nsSVGTextFrame2Base
{
  friend nsIFrame*
  NS_NewSVGTextFrame2(nsIPresShell* aPresShell, nsStyleContext* aContext);

  friend class mozilla::CharIterator;
  friend class mozilla::GlyphMetricsUpdater;
  friend class mozilla::TextFrameIterator;
  friend class mozilla::TextNodeCorrespondenceRecorder;
  friend struct mozilla::TextRenderedRun;
  friend class mozilla::TextRenderedRunIterator;
  friend class MutationObserver;
  friend class nsDisplaySVGText;

protected:
  nsSVGTextFrame2(nsStyleContext* aContext)
    : nsSVGTextFrame2Base(aContext),
      mFontSizeScaleFactor(1.0f),
      mLastContextScale(1.0f),
      mLengthAdjustScaleFactor(1.0f)
  {
    AddStateBits(NS_STATE_SVG_POSITIONING_DIRTY);
  }

public:
  NS_DECL_QUERYFRAME_TARGET(nsSVGTextFrame2)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  
  virtual void Init(nsIContent* aContent,
                    nsIFrame*   aParent,
                    nsIFrame*   aPrevInFlow) MOZ_OVERRIDE;

  NS_IMETHOD AttributeChanged(int32_t aNamespaceID,
                              nsIAtom* aAttribute,
                              int32_t aModType) MOZ_OVERRIDE;

  virtual nsIFrame* GetContentInsertionFrame() MOZ_OVERRIDE
  {
    return GetFirstPrincipalChild()->GetContentInsertionFrame();
  }

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  




  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const MOZ_OVERRIDE
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGText2"), aResult);
  }
#endif

  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext) MOZ_OVERRIDE;

  


  virtual void FindCloserFrameForSelection(nsPoint aPoint,
                                          FrameWithDistance* aCurrentBestFrame) MOZ_OVERRIDE;



  
  virtual void NotifySVGChanged(uint32_t aFlags) MOZ_OVERRIDE;
  NS_IMETHOD PaintSVG(nsRenderingContext* aContext,
                      const nsIntRect* aDirtyRect,
                      nsIFrame* aTransformRoot = nullptr) MOZ_OVERRIDE;
  NS_IMETHOD_(nsIFrame*) GetFrameForPoint(const nsPoint& aPoint) MOZ_OVERRIDE;
  virtual void ReflowSVG() MOZ_OVERRIDE;
  NS_IMETHOD_(nsRect) GetCoveredRegion() MOZ_OVERRIDE;
  virtual SVGBBox GetBBoxContribution(const gfxMatrix& aToBBoxUserspace,
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
  



  class MutationObserver : public nsStubMutationObserver {
  public:
    MutationObserver()
      : mFrame(nullptr)
    {
    }

    void StartObserving(nsSVGTextFrame2* aFrame)
    {
      NS_ASSERTION(!mFrame, "should not be observing yet!");
      mFrame = aFrame;
      aFrame->GetContent()->AddMutationObserver(this);
    }

    virtual ~MutationObserver()
    {
      if (mFrame) {
        mFrame->GetContent()->RemoveMutationObserver(this);
      }
    }

    
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
    NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
    NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED
    NS_DECL_NSIMUTATIONOBSERVER_CHARACTERDATACHANGED
    NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTECHANGED

  private:
    nsSVGTextFrame2* mFrame;
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
  already_AddRefed<gfxFlattenedPath> GetFlattenedTextPath(nsIFrame* aTextPathFrame);
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

  




  bool SetupContextPaint(gfxContext* aContext,
                        nsIFrame* aFrame,
                        nsStyleSVGPaint nsStyleSVG::*aFillOrStroke,
                        float& aOpacity,
                        gfxTextContextPaint* aContextPaint);

  









  void SetupInheritablePaint(gfxContext* aContext,
                             nsIFrame* aFrame,
                             float& aOpacity,
                             gfxTextContextPaint* aOuterContextPaint,
                             SVGTextContextPaint::Paint& aTargetPaint,
                             nsStyleSVGPaint nsStyleSVG::*aFillOrStroke,
                             const FramePropertyDescriptor* aProperty);

  


  MutationObserver mMutationObserver;

  


  nsAutoPtr<gfxMatrix> mCanvasTM;

  







  uint32_t mTrailingUndisplayedCharacters;

  


  nsTArray<mozilla::CharPosition> mPositions;

  





















  float mFontSizeScaleFactor;

  




  float mLastContextScale;

  



  float mLengthAdjustScaleFactor;
};

#endif
