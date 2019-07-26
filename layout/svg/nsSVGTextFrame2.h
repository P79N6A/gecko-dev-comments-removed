




#ifndef NS_SVGTEXTFRAME2_H
#define NS_SVGTEXTFRAME2_H

#include "gfxMatrix.h"
#include "gfxRect.h"
#include "nsStubMutationObserver.h"
#include "nsSVGTextContainerFrame.h"

class nsDisplaySVGText;
class nsRenderingContext;
class nsTextFrame;

typedef nsSVGDisplayContainerFrame nsSVGTextFrame2Base;

namespace mozilla {

class TextFrameIterator;
class TextNodeCorrespondenceRecorder;
struct TextRenderedRun;
class TextRenderedRunIterator;








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

}



































class nsSVGTextFrame2 : public nsSVGTextFrame2Base
{
  friend nsIFrame*
  NS_NewSVGTextFrame2(nsIPresShell* aPresShell, nsStyleContext* aContext);

  friend class mozilla::TextFrameIterator;
  friend class mozilla::TextNodeCorrespondenceRecorder;
  friend struct mozilla::TextRenderedRun;
  friend class mozilla::TextRenderedRunIterator;
  friend class AutoCanvasTMForMarker;
  friend class MutationObserver;
  friend class nsDisplaySVGText;

protected:
  nsSVGTextFrame2(nsStyleContext* aContext)
    : nsSVGTextFrame2Base(aContext),
      mFontSizeScaleFactor(1.0f),
      mGetCanvasTMForFlag(FOR_OUTERSVG_TM),
      mPositioningDirty(true)
  {
  }

public:
  NS_DECL_QUERYFRAME_TARGET(nsSVGTextFrame2)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  
  NS_IMETHOD Init(nsIContent* aContent,
                  nsIFrame*   aParent,
                  nsIFrame*   aPrevInFlow);

  NS_IMETHOD AttributeChanged(int32_t aNamespaceID,
                              nsIAtom* aAttribute,
                              int32_t aModType);

  virtual nsIFrame* GetContentInsertionFrame()
  {
    return GetFirstPrincipalChild()->GetContentInsertionFrame();
  }

  NS_IMETHOD Reflow(nsPresContext*           aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  




  virtual nsIAtom* GetType() const;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGText2"), aResult);
  }
#endif

  virtual void InvalidateInternal(const nsRect& aDamageRect,
                                  nscoord aX, nscoord aY, nsIFrame* aForChild,
                                  uint32_t aFlags);

  


  virtual void FindCloserFrameForSelection(nsPoint aPoint,
                                          FrameWithDistance* aCurrentBestFrame);


  
  virtual void NotifySVGChanged(uint32_t aFlags);
  NS_IMETHOD PaintSVG(nsRenderingContext* aContext,
                      const nsIntRect* aDirtyRect);
  NS_IMETHOD_(nsIFrame*) GetFrameForPoint(const nsPoint& aPoint);
  virtual void ReflowSVG();
  NS_IMETHOD_(nsRect) GetCoveredRegion();
  virtual SVGBBox GetBBoxContribution(const gfxMatrix& aToBBoxUserspace,
                                      uint32_t aFlags);

  
  virtual gfxMatrix GetCanvasTM(uint32_t aFor);
  
  
  uint32_t GetNumberOfChars(nsIContent* aContent);
  float GetComputedTextLength(nsIContent* aContent);
  float GetSubStringLength(nsIContent* aContent, uint32_t charnum, uint32_t nchars);
  int32_t GetCharNumAtPosition(nsIContent* aContent, mozilla::nsISVGPoint* point);

  nsresult GetStartPositionOfChar(nsIContent* aContent, uint32_t aCharNum,
                                  mozilla::nsISVGPoint** aResult);
  nsresult GetEndPositionOfChar(nsIContent* aContent, uint32_t aCharNum,
                                mozilla::nsISVGPoint** aResult);
  nsresult GetExtentOfChar(nsIContent* aContent, uint32_t aCharNum,
                           nsIDOMSVGRect** aResult);
  nsresult GetRotationOfChar(nsIContent* aContent, uint32_t aCharNum,
                             float* aResult);

  

  



  void NotifyGlyphMetricsChange();

  



  void UpdateFontSizeScaleFactor(bool aForceGlobalTransform);

  double GetFontSizeScaleFactor() const;

  




  gfxPoint TransformFramePointToTextChild(const gfxPoint& aPoint,
                                          nsIFrame* aChildFrame);

  





  gfxRect TransformFrameRectToTextChild(const gfxRect& aRect,
                                        nsIFrame* aChildFrame);

  





  gfxRect TransformFrameRectFromTextChild(const nsRect& aRect,
                                          nsIFrame* aChildFrame);

private:
  




  class AutoCanvasTMForMarker {
  public:
    AutoCanvasTMForMarker(nsSVGTextFrame2* aFrame, uint32_t aFor)
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
    nsSVGTextFrame2* mFrame;
    uint32_t mOldFor;
  };

  



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
    NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTECHANGED

  private:
    nsSVGTextFrame2* mFrame;
  };

  


  void DoReflow(bool aForceGlobalTransform);

  


  void RequestReflow(nsIPresShell::IntrinsicDirty aType, uint32_t aBit);

  






  void UpdateGlyphPositioning(bool aForceGlobalTransform);

  



  void DoGlyphPositioning();

  









  uint32_t ResolvePositions(nsIContent* aContent, uint32_t aIndex,
                            bool aInTextPath, bool& aForceStartOfChunk,
                            nsTArray<gfxPoint>& aDeltas);

  







  bool ResolvePositions(nsTArray<gfxPoint>& aDeltas);

  




  void DetermineCharPositions(nsTArray<nsPoint>& aPositions);

  



  void AdjustChunksForLineBreaks();

  



















  void AdjustPositionsForClusters();

  



  void DoAnchoring();

  



  void DoTextPathLayout();

  




  bool ShouldRenderAsPath(nsRenderingContext* aContext, nsTextFrame* aFrame);

  
  nsIFrame* GetTextPathPathFrame(nsIFrame* aTextPathFrame);
  already_AddRefed<gfxFlattenedPath> GetFlattenedTextPath(nsIFrame* aTextPathFrame);
  gfxFloat GetOffsetScale(nsIFrame* aTextPathFrame);
  gfxFloat GetStartOffset(nsIFrame* aTextPathFrame);

  


  MutationObserver mMutationObserver;

  


  nsAutoPtr<gfxMatrix> mCanvasTM;

  







  uint32_t mTrailingUndisplayedCharacters;

  


  nsTArray<mozilla::CharPosition> mPositions;

  





















  float mFontSizeScaleFactor;

  




  uint32_t mGetCanvasTMForFlag;

  


  bool mPositioningDirty;
};

#endif
