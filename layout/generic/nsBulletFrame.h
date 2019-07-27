






#ifndef nsBulletFrame_h___
#define nsBulletFrame_h___

#include "mozilla/Attributes.h"
#include "nsFrame.h"

#include "imgIContainer.h"
#include "imgINotificationObserver.h"
#include "imgIOnloadBlocker.h"

class imgIContainer;
class imgRequestProxy;

class nsBulletFrame;

class nsBulletListener MOZ_FINAL : public imgINotificationObserver,
                                   public imgIOnloadBlocker
{
public:
  nsBulletListener();

  NS_DECL_ISUPPORTS
  NS_DECL_IMGINOTIFICATIONOBSERVER
  NS_DECL_IMGIONLOADBLOCKER

  void SetFrame(nsBulletFrame *frame) { mFrame = frame; }

private:
  virtual ~nsBulletListener();

  nsBulletFrame *mFrame;
};





class nsBulletFrame MOZ_FINAL : public nsFrame {
  typedef mozilla::image::DrawResult DrawResult;

public:
  NS_DECL_FRAMEARENA_HELPERS
#ifdef DEBUG
  NS_DECL_QUERYFRAME_TARGET(nsBulletFrame)
  NS_DECL_QUERYFRAME
#endif

  explicit nsBulletFrame(nsStyleContext* aContext)
    : nsFrame(aContext)
    , mPadding(GetWritingMode())
    , mIntrinsicSize(GetWritingMode())
    , mRequestRegistered(false)
    , mBlockingOnload(false)
  { }
  virtual ~nsBulletFrame();

  NS_IMETHOD Notify(imgIRequest* aRequest, int32_t aType, const nsIntRect* aData);
  NS_IMETHOD BlockOnload(imgIRequest* aRequest);
  NS_IMETHOD UnblockOnload(imgIRequest* aRequest);

  
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
                         
  DrawResult PaintBullet(nsRenderingContext& aRenderingContext, nsPoint aPt,
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
  nsresult OnSizeAvailable(imgIRequest* aRequest, imgIContainer* aImage);

  void AppendSpacingToPadding(nsFontMetrics* aFontMetrics);
  void GetDesiredSize(nsPresContext* aPresContext,
                      nsRenderingContext *aRenderingContext,
                      nsHTMLReflowMetrics& aMetrics,
                      float aFontSizeInflation);

  void GetLoadGroup(nsPresContext *aPresContext, nsILoadGroup **aLoadGroup);
  nsIDocument* GetOurCurrentDoc() const;

  mozilla::LogicalMargin mPadding;
  nsRefPtr<imgRequestProxy> mImageRequest;
  nsRefPtr<nsBulletListener> mListener;

  mozilla::LogicalSize mIntrinsicSize;
  int32_t mOrdinal;

private:
  void RegisterImageRequest(bool aKnownToBeAnimated);
  void DeregisterAndCancelImageRequest();

  
  
  bool mRequestRegistered : 1;

  
  bool mBlockingOnload : 1;
};

#endif 
