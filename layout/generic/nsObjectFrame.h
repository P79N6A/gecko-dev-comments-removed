






































#ifndef nsObjectFrame_h___
#define nsObjectFrame_h___

#include "nsPluginInstanceOwner.h"
#include "nsIObjectFrame.h"
#include "nsFrame.h"
#include "nsRegion.h"
#include "nsDisplayList.h"
#include "nsIReflowCallback.h"
#include "Layers.h"
#include "ImageLayers.h"

#ifdef ACCESSIBILITY
class nsIAccessible;
#endif

class nsPluginHost;
class nsPresContext;
class nsDisplayPlugin;
class nsIOSurface;
class PluginBackgroundSink;

#define nsObjectFrameSuper nsFrame

class nsObjectFrame : public nsObjectFrameSuper,
                      public nsIObjectFrame,
                      public nsIReflowCallback {
public:
  typedef mozilla::LayerState LayerState;
  typedef mozilla::layers::Layer Layer;
  typedef mozilla::layers::LayerManager LayerManager;
  typedef mozilla::layers::ImageContainer ImageContainer;

  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewObjectFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  NS_DECL_QUERYFRAME

  NS_IMETHOD Init(nsIContent* aContent,
                  nsIFrame* aParent,
                  nsIFrame* aPrevInFlow);
  virtual nscoord GetMinWidth(nsRenderingContext *aRenderingContext);
  virtual nscoord GetPrefWidth(nsRenderingContext *aRenderingContext);
  NS_IMETHOD Reflow(nsPresContext* aPresContext,
                    nsHTMLReflowMetrics& aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus& aStatus);
  NS_IMETHOD DidReflow(nsPresContext* aPresContext,
                       const nsHTMLReflowState* aReflowState,
                       nsDidReflowStatus aStatus);
  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  NS_IMETHOD  HandleEvent(nsPresContext* aPresContext,
                          nsGUIEvent* aEvent,
                          nsEventStatus* aEventStatus);

#ifdef XP_MACOSX
  NS_IMETHOD HandlePress(nsPresContext* aPresContext,
                         nsGUIEvent*    aEvent,
                         nsEventStatus* aEventStatus);
#endif

  virtual nsIAtom* GetType() const;

  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
  {
    return nsObjectFrameSuper::IsFrameOfType(aFlags & ~(nsIFrame::eReplaced));
  }

  virtual PRBool NeedsView() { return PR_TRUE; }

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif

  virtual void DestroyFrom(nsIFrame* aDestructRoot);

  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext);

  NS_METHOD GetPluginInstance(nsNPAPIPluginInstance** aPluginInstance);

  virtual void SetIsDocumentActive(PRBool aIsActive);

  NS_IMETHOD GetCursor(const nsPoint& aPoint, nsIFrame::Cursor& aCursor);

  
  
  
  
  
  
  
  
  void GetEmptyClipConfiguration(nsTArray<nsIWidget::Configuration>* aConfigurations) {
    ComputeWidgetGeometry(nsRegion(), nsPoint(0,0), aConfigurations);
  }

  void DidSetWidgetGeometry();

  
#ifdef ACCESSIBILITY
  virtual already_AddRefed<nsAccessible> CreateAccessible();
#ifdef XP_WIN
  NS_IMETHOD GetPluginPort(HWND *aPort);
#endif
#endif

  
  nsresult SetWidget(nsIWidget *aWidget);

  
  static nsIObjectFrame* GetNextObjectFrame(nsPresContext* aPresContext,
                                            nsIFrame* aRoot);

  
  virtual PRBool ReflowFinished();
  virtual void ReflowCallbackCanceled();

  void UpdateImageLayer(ImageContainer* aContainer, const gfxRect& aRect);

  



  already_AddRefed<Layer> BuildLayer(nsDisplayListBuilder* aBuilder,
                                     LayerManager* aManager,
                                     nsDisplayItem* aItem);

  virtual LayerState GetLayerState(nsDisplayListBuilder* aBuilder,
                                   LayerManager* aManager);

  already_AddRefed<ImageContainer> GetImageContainer(LayerManager* aManager = nsnull);
  




  nsRect GetPaintedRect(nsDisplayPlugin* aItem);

  





  static void BeginSwapDocShells(nsIContent* aContent, void*);
  



  static void EndSwapDocShells(nsIContent* aContent, void*);

  nsIWidget* GetWidget() { return mWidget; }

  



  void FixupWindow(const nsSize& aSize);

  


  nsresult CallSetWindow(PRBool aCheckIsHidden = PR_TRUE);

  void SetInstanceOwner(nsPluginInstanceOwner* aOwner);

protected:
  nsObjectFrame(nsStyleContext* aContext);
  virtual ~nsObjectFrame();

  
  
  void GetDesiredSize(nsPresContext* aPresContext,
                      const nsHTMLReflowState& aReflowState,
                      nsHTMLReflowMetrics& aDesiredSize);

  PRBool IsFocusable(PRInt32 *aTabIndex = nsnull, PRBool aWithMouse = PR_FALSE);

  
  PRBool IsHidden(PRBool aCheckVisibilityStyle = PR_TRUE) const;

  PRBool IsOpaque() const;
  PRBool IsTransparentMode() const;

  nsIntPoint GetWindowOriginInPixels(PRBool aWindowless);

  static void PaintPrintPlugin(nsIFrame* aFrame,
                               nsRenderingContext* aRenderingContext,
                               const nsRect& aDirtyRect, nsPoint aPt);
  void PrintPlugin(nsRenderingContext& aRenderingContext,
                   const nsRect& aDirtyRect);
  void PaintPlugin(nsDisplayListBuilder* aBuilder,
                   nsRenderingContext& aRenderingContext,
                   const nsRect& aDirtyRect, const nsRect& aPluginRect);

  








  void ComputeWidgetGeometry(const nsRegion& aRegion,
                             const nsPoint& aPluginOrigin,
                             nsTArray<nsIWidget::Configuration>* aConfigurations);

  void NotifyPluginReflowObservers();

  friend class nsPluginInstanceOwner;
  friend class nsDisplayPlugin;
  friend class PluginBackgroundSink;

private:
  
  class PluginEventNotifier : public nsRunnable {
  public:
    PluginEventNotifier(const nsString &aEventType) : 
      mEventType(aEventType) {}
    
    NS_IMETHOD Run();
  private:
    nsString mEventType;
  };

  nsPluginInstanceOwner*          mInstanceOwner; 
  nsIView*                        mInnerView;
  nsCOMPtr<nsIWidget>             mWidget;
  nsIntRect                       mWindowlessRect;
  



  PluginBackgroundSink*           mBackgroundSink;

  PRPackedBool mReflowCallbackPosted;

  
  
  nsRefPtr<ImageContainer> mImageContainer;
};

class nsDisplayPlugin : public nsDisplayItem {
public:
  nsDisplayPlugin(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame)
    : nsDisplayItem(aBuilder, aFrame)
  {
    MOZ_COUNT_CTOR(nsDisplayPlugin);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayPlugin() {
    MOZ_COUNT_DTOR(nsDisplayPlugin);
  }
#endif

  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder);
  virtual nsRegion GetOpaqueRegion(nsDisplayListBuilder* aBuilder,
                                   PRBool* aForceTransparentSurface = nsnull);
  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx);
  virtual PRBool ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                   nsRegion* aVisibleRegion,
                                   const nsRect& aAllowVisibleRegionExpansion);

  NS_DISPLAY_DECL_NAME("Plugin", TYPE_PLUGIN)

  
  
  
  
  
  
  
  void GetWidgetConfiguration(nsDisplayListBuilder* aBuilder,
                              nsTArray<nsIWidget::Configuration>* aConfigurations);

  virtual already_AddRefed<Layer> BuildLayer(nsDisplayListBuilder* aBuilder,
                                             LayerManager* aManager,
                                             const ContainerParameters& aContainerParameters)
  {
    return static_cast<nsObjectFrame*>(mFrame)->BuildLayer(aBuilder,
                                                           aManager, 
                                                           this);
  }

  virtual LayerState GetLayerState(nsDisplayListBuilder* aBuilder,
                                   LayerManager* aManager)
  {
    return static_cast<nsObjectFrame*>(mFrame)->GetLayerState(aBuilder,
                                                              aManager);
  }

private:
  nsRegion mVisibleRegion;
};

#endif 
