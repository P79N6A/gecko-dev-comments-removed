






































#ifndef nsObjectFrame_h___
#define nsObjectFrame_h___

#ifdef XP_WIN
#include <windows.h>
#endif

#include "nsIObjectFrame.h"
#include "nsFrame.h"

#ifdef ACCESSIBILITY
class nsIAccessible;
#endif

class nsPluginInstanceOwner;
class nsIPluginHost;
class nsIPluginInstance;
class nsPresContext;

#define nsObjectFrameSuper nsFrame

class nsObjectFrame : public nsObjectFrameSuper, public nsIObjectFrame {
public:
  friend nsIFrame* NS_NewObjectFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  
  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);

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

  void PrintPlugin(nsIRenderingContext& aRenderingContext,
                   const nsRect& aDirtyRect);
  void PaintPlugin(nsIRenderingContext& aRenderingContext,
                   const nsRect& aDirtyRect);

  NS_IMETHOD  HandleEvent(nsPresContext* aPresContext,
                          nsGUIEvent* aEvent,
                          nsEventStatus* aEventStatus);

  virtual nsIAtom* GetType() const;

  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
  {
    return nsObjectFrameSuper::IsFrameOfType(aFlags & ~(nsIFrame::eReplaced));
  }

  virtual PRBool SupportsVisibilityHidden() { return PR_FALSE; }
  virtual PRBool NeedsView() { return PR_TRUE; }
  virtual nsresult CreateWidgetForView(nsIView* aView);

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif

  virtual void Destroy();

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
  
  NS_IMETHOD_(nsrefcnt) AddRef(void);
  NS_IMETHOD_(nsrefcnt) Release(void);

  nsObjectFrame(nsStyleContext* aContext) : nsObjectFrameSuper(aContext) {}
  virtual ~nsObjectFrame();

  
  
  void GetDesiredSize(nsPresContext* aPresContext,
                      const nsHTMLReflowState& aReflowState,
                      nsHTMLReflowMetrics& aDesiredSize);

  nsresult InstantiatePlugin(nsIPluginHost* aPluginHost, 
                             const char* aMimetype,
                             nsIURI* aURL);

  



  void FixupWindow(const nsSize& aSize);

  PRBool IsFocusable(PRInt32 *aTabIndex = nsnull, PRBool aWithMouse = PR_FALSE);

  
  PRBool IsHidden(PRBool aCheckVisibilityStyle = PR_TRUE) const;

  void NotifyContentObjectWrapper();

  nsPoint GetWindowOriginInPixels(PRBool aWindowless);

  



  NS_HIDDEN_(nsresult) PrepareInstanceOwner();

  friend class nsPluginInstanceOwner;
private:
  nsPluginInstanceOwner *mInstanceOwner;
  nsRect                mWindowlessRect;

#ifdef DEBUG
  
  
  PRBool mInstantiating;
#endif
};


#endif 
