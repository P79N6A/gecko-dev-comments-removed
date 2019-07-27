






#ifndef nsObjectFrame_h___
#define nsObjectFrame_h___

#include "mozilla/Attributes.h"
#include "nsIObjectFrame.h"
#include "nsFrame.h"
#include "nsRegion.h"
#include "nsDisplayList.h"
#include "nsIReflowCallback.h"

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
class nsIOSurface;
class PluginBackgroundSink;
class nsPluginInstanceOwner;

namespace mozilla {
namespace layers {
class ImageContainer;
class Layer;
class LayerManager;
}
}

typedef nsFrame nsObjectFrameSuper;

class nsObjectFrame : public nsObjectFrameSuper,
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
  NS_DECL_QUERYFRAME_TARGET(nsObjectFrame)

  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) MOZ_OVERRIDE;
  virtual nscoord GetMinISize(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;
  virtual nscoord GetPrefISize(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;
  virtual void Reflow(nsPresContext* aPresContext,
                      nsHTMLReflowMetrics& aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus& aStatus) MOZ_OVERRIDE;
  virtual void DidReflow(nsPresContext* aPresContext,
                         const nsHTMLReflowState* aReflowState,
                         nsDidReflowStatus aStatus) MOZ_OVERRIDE;
  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  virtual nsresult  HandleEvent(nsPresContext* aPresContext,
                                mozilla::WidgetGUIEvent* aEvent,
                                nsEventStatus* aEventStatus) MOZ_OVERRIDE;

  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

  virtual bool IsFrameOfType(uint32_t aFlags) const MOZ_OVERRIDE
  {
    return nsObjectFrameSuper::IsFrameOfType(aFlags & ~(nsIFrame::eReplaced));
  }

  virtual bool NeedsView() MOZ_OVERRIDE { return true; }

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif

  virtual void DestroyFrom(nsIFrame* aDestructRoot) MOZ_OVERRIDE;

  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext) MOZ_OVERRIDE;

  NS_METHOD GetPluginInstance(nsNPAPIPluginInstance** aPluginInstance) MOZ_OVERRIDE;

  virtual void SetIsDocumentActive(bool aIsActive) MOZ_OVERRIDE;

  virtual nsresult GetCursor(const nsPoint& aPoint, 
                             nsIFrame::Cursor& aCursor) MOZ_OVERRIDE;

  
  
  







  void SetEmptyWidgetConfiguration()
  {
    mNextConfigurationBounds = nsIntRect(0,0,0,0);
    mNextConfigurationClipRegion.Clear();
  }
  


  void GetWidgetConfiguration(nsTArray<nsIWidget::Configuration>* aConfigurations)
  {
    if (mWidget) {
      if (!mWidget->GetParent()) {
        
        
        
        
        NS_ERROR("Plugin widgets registered for geometry updates should not be toplevel");
        return;
      }
      nsIWidget::Configuration* configuration = aConfigurations->AppendElement();
      configuration->mChild = mWidget;
      configuration->mBounds = mNextConfigurationBounds;
      configuration->mClipRegion = mNextConfigurationClipRegion;
    }
  }
  



  void DidSetWidgetGeometry();

  
#ifdef ACCESSIBILITY
  virtual mozilla::a11y::AccType AccessibleType() MOZ_OVERRIDE;
#ifdef XP_WIN
  NS_IMETHOD GetPluginPort(HWND *aPort);
#endif
#endif

  
  nsresult PrepForDrawing(nsIWidget *aWidget);

  
  static nsIObjectFrame* GetNextObjectFrame(nsPresContext* aPresContext,
                                            nsIFrame* aRoot);

  
  virtual bool ReflowFinished() MOZ_OVERRIDE;
  virtual void ReflowCallbackCanceled() MOZ_OVERRIDE;

  



  already_AddRefed<Layer> BuildLayer(nsDisplayListBuilder* aBuilder,
                                     LayerManager* aManager,
                                     nsDisplayItem* aItem,
                                     const ContainerLayerParameters& aContainerParameters);

  LayerState GetLayerState(nsDisplayListBuilder* aBuilder,
                           LayerManager* aManager);

  




  nsRect GetPaintedRect(nsDisplayPlugin* aItem);

  





  static void BeginSwapDocShells(nsISupports* aSupports, void*);
  



  static void EndSwapDocShells(nsISupports* aSupports, void*);

  nsIWidget* GetWidget() MOZ_OVERRIDE { return mInnerView ? mWidget : nullptr; }

  



  void FixupWindow(const nsSize& aSize);

  


  nsresult CallSetWindow(bool aCheckIsHidden = true);

  void SetInstanceOwner(nsPluginInstanceOwner* aOwner);

protected:
  explicit nsObjectFrame(nsStyleContext* aContext);
  virtual ~nsObjectFrame();

  
  
  void GetDesiredSize(nsPresContext* aPresContext,
                      const nsHTMLReflowState& aReflowState,
                      nsHTMLReflowMetrics& aDesiredSize);

  bool IsFocusable(int32_t *aTabIndex = nullptr, 
                   bool aWithMouse = false) MOZ_OVERRIDE;

  
  bool IsHidden(bool aCheckVisibilityStyle = true) const;

  bool IsOpaque() const;
  bool IsTransparentMode() const;
  bool IsPaintedByGecko() const;

  nsIntPoint GetWindowOriginInPixels(bool aWindowless);

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

  class PluginEventNotifier : public nsRunnable {
  public:
    explicit PluginEventNotifier(const nsString &aEventType) : 
      mEventType(aEventType) {}
    
    NS_IMETHOD Run() MOZ_OVERRIDE;
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

  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) MOZ_OVERRIDE;
  virtual nsRegion GetOpaqueRegion(nsDisplayListBuilder* aBuilder,
                                   bool* aSnap) MOZ_OVERRIDE;
  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx) MOZ_OVERRIDE;
  virtual bool ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                 nsRegion* aVisibleRegion) MOZ_OVERRIDE;

  NS_DISPLAY_DECL_NAME("Plugin", TYPE_PLUGIN)

  virtual already_AddRefed<Layer> BuildLayer(nsDisplayListBuilder* aBuilder,
                                             LayerManager* aManager,
                                             const ContainerLayerParameters& aContainerParameters) MOZ_OVERRIDE
  {
    return static_cast<nsObjectFrame*>(mFrame)->BuildLayer(aBuilder,
                                                           aManager, 
                                                           this,
                                                           aContainerParameters);
  }

  virtual LayerState GetLayerState(nsDisplayListBuilder* aBuilder,
                                   LayerManager* aManager,
                                   const ContainerLayerParameters& aParameters) MOZ_OVERRIDE
  {
    return static_cast<nsObjectFrame*>(mFrame)->GetLayerState(aBuilder,
                                                              aManager);
  }
};

#endif 
