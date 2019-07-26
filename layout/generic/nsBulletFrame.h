






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
  virtual ~nsBulletListener();

  NS_DECL_ISUPPORTS
  NS_DECL_IMGINOTIFICATIONOBSERVER

  void SetFrame(nsBulletFrame *frame) { mFrame = frame; }

private:
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
  virtual nscoord GetMinWidth(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;
  virtual nscoord GetPrefWidth(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;

  
  int32_t SetListItemOrdinal(int32_t aNextOrdinal, bool* aChanged,
                             int32_t aIncrement);


  
  static void AppendCounterText(int32_t aListStyleType,
                                int32_t aOrdinal,
                                nsString& aResult,
                                bool& aIsRTL);

  
  static void GetListItemSuffix(int32_t aListStyleType,
                                nsString& aResult,
                                bool& aSuppressPadding);

  
  void GetListItemText(const nsStyleList& aStyleList, nsString& aResult);
                         
  void PaintBullet(nsRenderingContext& aRenderingContext, nsPoint aPt,
                   const nsRect& aDirtyRect, uint32_t aFlags);
  
  virtual bool IsEmpty() MOZ_OVERRIDE;
  virtual bool IsSelfEmpty() MOZ_OVERRIDE;
  virtual nscoord GetBaseline() const MOZ_OVERRIDE;

  float GetFontSizeInflation() const;
  bool HasFontSizeInflation() const {
    return (GetStateBits() & BULLET_FRAME_HAS_FONT_INFLATION) != 0;
  }
  void SetFontSizeInflation(float aInflation);

  int32_t GetOrdinal() { return mOrdinal; }

  already_AddRefed<imgIContainer> GetImage() const;

protected:
  nsresult OnStartContainer(imgIRequest *aRequest, imgIContainer *aImage);

  void GetDesiredSize(nsPresContext* aPresContext,
                      nsRenderingContext *aRenderingContext,
                      nsHTMLReflowMetrics& aMetrics,
                      float aFontSizeInflation);

  void GetLoadGroup(nsPresContext *aPresContext, nsILoadGroup **aLoadGroup);

  nsMargin mPadding;
  nsRefPtr<imgRequestProxy> mImageRequest;
  nsRefPtr<nsBulletListener> mListener;

  nsSize mIntrinsicSize;
  int32_t mOrdinal;
  bool mTextIsRTL;

  
  
  
  
  bool mSuppressPadding;

private:

  
  
  bool mRequestRegistered;
};

#endif 
