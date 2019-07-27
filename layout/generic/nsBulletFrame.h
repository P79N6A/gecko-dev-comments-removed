






#ifndef nsBulletFrame_h___
#define nsBulletFrame_h___

#include "mozilla/Attributes.h"
#include "nsFrame.h"

#include "imgINotificationObserver.h"

class imgIContainer;
class imgRequestProxy;

class nsBulletFrame;

class nsBulletListener : public imgINotificationObserver
{
public:
  nsBulletListener();

  NS_DECL_ISUPPORTS
  NS_DECL_IMGINOTIFICATIONOBSERVER

  void SetFrame(nsBulletFrame *frame) { mFrame = frame; }

private:
  virtual ~nsBulletListener();

  nsBulletFrame *mFrame;
};





class nsBulletFrame : public nsFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS
#ifdef DEBUG
  NS_DECL_QUERYFRAME_TARGET(nsBulletFrame)
  NS_DECL_QUERYFRAME
#endif

  nsBulletFrame(nsStyleContext* aContext)
    : nsFrame(aContext)
    , mPadding(GetWritingMode())
    , mIntrinsicSize(GetWritingMode())
  {
  }
  virtual ~nsBulletFrame();

  NS_IMETHOD Notify(imgIRequest *aRequest, int32_t aType, const nsIntRect* aData);

  
  virtual void DestroyFrom(nsIFrame* aDestructRoot) MOZ_OVERRIDE;
  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;
  virtual nsIAtom* GetType() const MOZ_OVERRIDE;
  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext) MOZ_OVERRIDE;
#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif

  
  virtual void Reflow(nsPresContext* aPresContext,
                      nsHTMLReflowMetrics& aMetrics,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus& aStatus) MOZ_OVERRIDE;
  virtual nscoord GetMinISize(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;
  virtual nscoord GetPrefISize(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;

  
  int32_t SetListItemOrdinal(int32_t aNextOrdinal, bool* aChanged,
                             int32_t aIncrement);

  
  void GetListItemText(nsAString& aResult);

  void GetSpokenText(nsAString& aText);
                         
  void PaintBullet(nsRenderingContext& aRenderingContext, nsPoint aPt,
                   const nsRect& aDirtyRect, uint32_t aFlags);
  
  virtual bool IsEmpty() MOZ_OVERRIDE;
  virtual bool IsSelfEmpty() MOZ_OVERRIDE;
  virtual nscoord GetLogicalBaseline(mozilla::WritingMode aWritingMode) const MOZ_OVERRIDE;

  float GetFontSizeInflation() const;
  bool HasFontSizeInflation() const {
    return (GetStateBits() & BULLET_FRAME_HAS_FONT_INFLATION) != 0;
  }
  void SetFontSizeInflation(float aInflation);

  int32_t GetOrdinal() { return mOrdinal; }

  already_AddRefed<imgIContainer> GetImage() const;

protected:
  nsresult OnStartContainer(imgIRequest *aRequest, imgIContainer *aImage);

  void AppendSpacingToPadding(nsFontMetrics* aFontMetrics);
  void GetDesiredSize(nsPresContext* aPresContext,
                      nsRenderingContext *aRenderingContext,
                      nsHTMLReflowMetrics& aMetrics,
                      float aFontSizeInflation);

  void GetLoadGroup(nsPresContext *aPresContext, nsILoadGroup **aLoadGroup);

  mozilla::LogicalMargin mPadding;
  nsRefPtr<imgRequestProxy> mImageRequest;
  nsRefPtr<nsBulletListener> mListener;

  mozilla::LogicalSize mIntrinsicSize;
  int32_t mOrdinal;

private:

  
  
  bool mRequestRegistered;
};

#endif 
