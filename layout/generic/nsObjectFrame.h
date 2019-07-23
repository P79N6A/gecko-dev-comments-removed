






































#ifndef nsObjectFrame_h___
#define nsObjectFrame_h___

#ifdef XP_WIN
#include <windows.h>
#endif

#include "nsIObjectFrame.h"
#include "nsFrame.h"
#include "nsRegion.h"
#include "nsDisplayList.h"

#ifdef ACCESSIBILITY
class nsIAccessible;
#endif

class nsPluginInstanceOwner;
class nsIPluginHost;
class nsIPluginInstance;
class nsPresContext;
class nsDisplayPlugin;
class nsIDOMElement;

#define nsObjectFrameSuper nsFrame

class nsObjectFrame : public nsObjectFrameSuper, public nsIObjectFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewObjectFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  NS_DECL_QUERYFRAME

  NS_IMETHOD Init(nsIContent* aContent,
                  nsIFrame* aParent,
                  nsIFrame* aPrevInFlow);
  virtual nscoord GetMinWidth(nsIRenderingContext *aRenderingContext);
  virtual nscoord GetPrefWidth(nsIRenderingContext *aRenderingContext);
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

  virtual nsIAtom* GetType() const;

  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
  {
    return nsObjectFrameSuper::IsFrameOfType(aFlags & ~(nsIFrame::eReplaced));
  }

  virtual PRBool NeedsView() { return PR_TRUE; }

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif

  virtual void Destroy();

  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext);

  NS_IMETHOD GetPluginInstance(nsIPluginInstance*& aPluginInstance);
  virtual nsresult Instantiate(nsIChannel* aChannel, nsIStreamListener** aStreamListener);
  virtual nsresult Instantiate(const char* aMimeType, nsIURI* aURI);
  virtual void TryNotifyContentObjectWrapper();
  virtual void StopPlugin();

  





  void StopPluginInternal(PRBool aDelayedStop);

  
  NS_IMETHOD GetCursor(const nsPoint& aPoint, nsIFrame::Cursor& aCursor) 
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  
  
  
  
  
  
  
  
  void GetEmptyClipConfiguration(nsTArray<nsIWidget::Configuration>* aConfigurations) {
    ComputeWidgetGeometry(nsRegion(), nsPoint(0,0), aConfigurations);
  }

  void DidSetWidgetGeometry();

  
#ifdef ACCESSIBILITY
  NS_IMETHOD GetAccessible(nsIAccessible** aAccessible);
#ifdef XP_WIN
  NS_IMETHOD GetPluginPort(HWND *aPort);
#endif
#endif

  
  nsresult CreateWidget(nscoord aWidth, nscoord aHeight, PRBool aViewOnly);

  
  static nsIObjectFrame* GetNextObjectFrame(nsPresContext* aPresContext,
                                            nsIFrame* aRoot);

protected:
  nsObjectFrame(nsStyleContext* aContext);
  virtual ~nsObjectFrame();

  
  
  void GetDesiredSize(nsPresContext* aPresContext,
                      const nsHTMLReflowState& aReflowState,
                      nsHTMLReflowMetrics& aDesiredSize);

  nsresult InstantiatePlugin(nsIPluginHost* aPluginHost, 
                             const char* aMimetype,
                             nsIURI* aURL);

  



  void FixupWindow(const nsSize& aSize);

  


  void CallSetWindow();

  PRBool IsFocusable(PRInt32 *aTabIndex = nsnull, PRBool aWithMouse = PR_FALSE);

  
  PRBool IsHidden(PRBool aCheckVisibilityStyle = PR_TRUE) const;

  PRBool IsOpaque() const;

  void NotifyContentObjectWrapper();

  nsIntPoint GetWindowOriginInPixels(PRBool aWindowless);

  static void PaintPrintPlugin(nsIFrame* aFrame,
                               nsIRenderingContext* aRenderingContext,
                               const nsRect& aDirtyRect, nsPoint aPt);
  void PrintPlugin(nsIRenderingContext& aRenderingContext,
                   const nsRect& aDirtyRect);
  void PaintPlugin(nsIRenderingContext& aRenderingContext,
                   const nsRect& aDirtyRect, const nsRect& aPluginRect);

  



  NS_HIDDEN_(nsresult) PrepareInstanceOwner();

  








  void ComputeWidgetGeometry(const nsRegion& aRegion,
                             const nsPoint& aPluginOrigin,
                             nsTArray<nsIWidget::Configuration>* aConfigurations);

  nsIWidget* GetWidget() { return mWidget; }

  nsresult SetAbsoluteScreenPosition(nsIDOMElement* element,
                                     nsIDOMClientRect* position,
                                     nsIDOMClientRect* clip);

  void NotifyPluginEventObservers(const PRUnichar *eventType);

  friend class nsPluginInstanceOwner;
  friend class nsDisplayPlugin;

private:
  nsRefPtr<nsPluginInstanceOwner> mInstanceOwner;
  nsIView*                        mInnerView;
  nsCOMPtr<nsIWidget>             mWidget;
  nsIntRect                       mWindowlessRect;
#ifdef XP_WIN
  PRUint32                        mDoublePassEvent;
#endif

  
  
  
  PRBool mPreventInstantiation;
};

class nsDisplayPlugin : public nsDisplayItem {
public:
  nsDisplayPlugin(nsIFrame* aFrame)
    : nsDisplayItem(aFrame)
  {
    MOZ_COUNT_CTOR(nsDisplayPlugin);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayPlugin() {
    MOZ_COUNT_DTOR(nsDisplayPlugin);
  }
#endif

  virtual Type GetType() { return TYPE_PLUGIN; }
  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder);
  virtual PRBool IsOpaque(nsDisplayListBuilder* aBuilder);
  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsIRenderingContext* aCtx);
  virtual PRBool ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                   nsRegion* aVisibleRegion,
                                   nsRegion* aVisibleRegionBeforeMove);

  NS_DISPLAY_DECL_NAME("Plugin")

  
  
  
  
  
  
  
  void GetWidgetConfiguration(nsDisplayListBuilder* aBuilder,
                              nsTArray<nsIWidget::Configuration>* aConfigurations);

private:
  nsRegion mVisibleRegion;
};

#endif 
