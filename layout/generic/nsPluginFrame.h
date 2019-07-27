






#ifndef nsPluginFrame_h___
#define nsPluginFrame_h___

#include "mozilla/Attributes.h"
#include "nsIObjectFrame.h"
#include "nsFrame.h"
#include "nsRegion.h"
#include "nsDisplayList.h"
#include "nsIReflowCallback.h"
#include "Units.h"

#ifdef XP_WIN
#include <windows.h> 

#undef GetMessage
#undef CreateEvent
#undef GetClassName
#undef GetBinaryType
#undef RemoveDirectory
#endif

class nsPresContext;
class nsRootPresContext;
class nsDisplayPlugin;
class PluginBackgroundSink;
class nsPluginInstanceOwner;

namespace mozilla {
namespace layers {
class ImageContainer;
class Layer;
class LayerManager;
}
}

typedef nsFrame nsPluginFrameSuper;

class nsPluginFrame : public nsPluginFrameSuper,
                      public nsIObjectFrame,
                      public nsIReflowCallback {
public:
  typedef mozilla::LayerState LayerState;
  typedef mozilla::layers::Layer Layer;
  typedef mozilla::layers::LayerManager LayerManager;
  typedef mozilla::layers::ImageContainer ImageContainer;
  typedef mozilla::ContainerLayerParameters ContainerLayerParameters;

  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewObjectFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  NS_DECL_QUERYFRAME
  NS_DECL_QUERYFRAME_TARGET(nsPluginFrame)

  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) override;
  virtual nscoord GetMinISize(nsRenderingContext *aRenderingContext) override;
  virtual nscoord GetPrefISize(nsRenderingContext *aRenderingContext) override;
  virtual void Reflow(nsPresContext* aPresContext,
                      nsHTMLReflowMetrics& aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus& aStatus) override;
  virtual void DidReflow(nsPresContext* aPresContext,
                         const nsHTMLReflowState* aReflowState,
                         nsDidReflowStatus aStatus) override;
  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) override;

  virtual nsresult  HandleEvent(nsPresContext* aPresContext,
                                mozilla::WidgetGUIEvent* aEvent,
                                nsEventStatus* aEventStatus) override;

  virtual nsIAtom* GetType() const override;

  virtual bool IsFrameOfType(uint32_t aFlags) const override
  {
    return nsPluginFrameSuper::IsFrameOfType(aFlags & ~(nsIFrame::eReplaced));
  }

  virtual bool NeedsView() override { return true; }

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const override;
#endif

  virtual void DestroyFrom(nsIFrame* aDestructRoot) override;

  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext) override;

  NS_METHOD GetPluginInstance(nsNPAPIPluginInstance** aPluginInstance) override;

  virtual void SetIsDocumentActive(bool aIsActive) override;

  virtual nsresult GetCursor(const nsPoint& aPoint, 
                             nsIFrame::Cursor& aCursor) override;

  
  
  







  void SetEmptyWidgetConfiguration()
  {
    mNextConfigurationBounds = nsIntRect(0,0,0,0);
    mNextConfigurationClipRegion.Clear();
  }
  


  void GetWidgetConfiguration(nsTArray<nsIWidget::Configuration>* aConfigurations);

  nsIntRect GetWidgetlessClipRect() {
    return RegionFromArray(mNextConfigurationClipRegion).GetBounds();
  }

  



  void DidSetWidgetGeometry();

  
#ifdef ACCESSIBILITY
  virtual mozilla::a11y::AccType AccessibleType() override;
#ifdef XP_WIN
  NS_IMETHOD GetPluginPort(HWND *aPort);
#endif
#endif

  
  nsresult PrepForDrawing(nsIWidget *aWidget);

  
  static nsIObjectFrame* GetNextObjectFrame(nsPresContext* aPresContext,
                                            nsIFrame* aRoot);

  
  virtual bool ReflowFinished() override;
  virtual void ReflowCallbackCanceled() override;

  



  already_AddRefed<Layer> BuildLayer(nsDisplayListBuilder* aBuilder,
                                     LayerManager* aManager,
                                     nsDisplayItem* aItem,
                                     const ContainerLayerParameters& aContainerParameters);

  LayerState GetLayerState(nsDisplayListBuilder* aBuilder,
                           LayerManager* aManager);

  




  nsRect GetPaintedRect(nsDisplayPlugin* aItem);

  





  static void BeginSwapDocShells(nsISupports* aSupports, void*);
  



  static void EndSwapDocShells(nsISupports* aSupports, void*);

  nsIWidget* GetWidget() override { return mInnerView ? mWidget : nullptr; }

  



  void FixupWindow(const nsSize& aSize);

  


  nsresult CallSetWindow(bool aCheckIsHidden = true);

  void SetInstanceOwner(nsPluginInstanceOwner* aOwner);

protected:
  explicit nsPluginFrame(nsStyleContext* aContext);
  virtual ~nsPluginFrame();

  
  
  void GetDesiredSize(nsPresContext* aPresContext,
                      const nsHTMLReflowState& aReflowState,
                      nsHTMLReflowMetrics& aDesiredSize);

  bool IsFocusable(int32_t *aTabIndex = nullptr, 
                   bool aWithMouse = false) override;

  
  bool IsHidden(bool aCheckVisibilityStyle = true) const;

  bool IsOpaque() const;
  bool IsTransparentMode() const;
  bool IsPaintedByGecko() const;

  nsIntPoint GetWindowOriginInPixels(bool aWindowless);
  
  




  mozilla::LayoutDeviceIntPoint GetRemoteTabChromeOffset();

  static void PaintPrintPlugin(nsIFrame* aFrame,
                               nsRenderingContext* aRenderingContext,
                               const nsRect& aDirtyRect, nsPoint aPt);
  void PrintPlugin(nsRenderingContext& aRenderingContext,
                   const nsRect& aDirtyRect);
  void PaintPlugin(nsDisplayListBuilder* aBuilder,
                   nsRenderingContext& aRenderingContext,
                   const nsRect& aDirtyRect, const nsRect& aPluginRect);

  void NotifyPluginReflowObservers();

  friend class nsPluginInstanceOwner;
  friend class nsDisplayPlugin;
  friend class PluginBackgroundSink;

private:
  
  
  
  
  void RegisterPluginForGeometryUpdates();

  
  
  void UnregisterPluginForGeometryUpdates();

  static const nsIntRegion RegionFromArray(const nsTArray<nsIntRect>& aRects)
  {
    nsIntRegion region;
    for (uint32_t i = 0; i < aRects.Length(); ++i) {
      region.Or(region, aRects[i]);
    }
    return region;
  }

  class PluginEventNotifier : public nsRunnable {
  public:
    explicit PluginEventNotifier(const nsString &aEventType) : 
      mEventType(aEventType) {}
    
    NS_IMETHOD Run() override;
  private:
    nsString mEventType;
  };

  nsPluginInstanceOwner*          mInstanceOwner; 
  nsView*                        mInnerView;
  nsCOMPtr<nsIWidget>             mWidget;
  nsIntRect                       mWindowlessRect;
  



  PluginBackgroundSink*           mBackgroundSink;

  




  nsIntRect                       mNextConfigurationBounds;
  



  nsTArray<nsIntRect>             mNextConfigurationClipRegion;

  bool mReflowCallbackPosted;

  
  
  
  
  nsRefPtr<nsRootPresContext> mRootPresContextRegisteredWith;
};

class nsDisplayPlugin : public nsDisplayItem {
public:
  nsDisplayPlugin(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame)
    : nsDisplayItem(aBuilder, aFrame)
  {
    MOZ_COUNT_CTOR(nsDisplayPlugin);
    aBuilder->SetContainsPluginItem();
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayPlugin() {
    MOZ_COUNT_DTOR(nsDisplayPlugin);
  }
#endif

  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) override;
  virtual nsRegion GetOpaqueRegion(nsDisplayListBuilder* aBuilder,
                                   bool* aSnap) override;
  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx) override;
  virtual bool ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                 nsRegion* aVisibleRegion) override;

  NS_DISPLAY_DECL_NAME("Plugin", TYPE_PLUGIN)

  virtual already_AddRefed<Layer> BuildLayer(nsDisplayListBuilder* aBuilder,
                                             LayerManager* aManager,
                                             const ContainerLayerParameters& aContainerParameters) override
  {
    return static_cast<nsPluginFrame*>(mFrame)->BuildLayer(aBuilder,
                                                           aManager, 
                                                           this,
                                                           aContainerParameters);
  }

  virtual LayerState GetLayerState(nsDisplayListBuilder* aBuilder,
                                   LayerManager* aManager,
                                   const ContainerLayerParameters& aParameters) override
  {
    return static_cast<nsPluginFrame*>(mFrame)->GetLayerState(aBuilder,
                                                              aManager);
  }
};

#endif 
