













































#ifndef nsPluginInstanceOwner_h_
#define nsPluginInstanceOwner_h_

#include "prtypes.h"
#include "npapi.h"
#include "nsCOMPtr.h"
#include "nsIPluginInstanceOwner.h"
#include "nsIPluginTagInfo.h"
#include "nsIDOMEventListener.h"
#include "nsIScrollPositionListener.h"
#include "nsPluginHost.h"
#include "nsPluginNativeWindow.h"
#include "gfxRect.h"


#ifdef KeyPress
#undef KeyPress
#endif

#ifdef XP_MACOSX
#include "nsCoreAnimationSupport.h"
#include <ApplicationServices/ApplicationServices.h>
#endif

class nsIInputStream;
struct nsIntRect;
class nsPluginDOMContextMenuListener;
class nsObjectFrame;
class nsDisplayListBuilder;

#ifdef MOZ_X11
class gfxXlibSurface;
#endif

#ifdef MOZ_WIDGET_GTK2
#include "gfxXlibNativeRenderer.h"
#endif

#ifdef MOZ_WIDGET_QT
#ifdef MOZ_X11
#include "gfxQtNativeRenderer.h"
#endif
#endif

#ifdef XP_OS2
#define INCL_PM
#define INCL_GPI
#include <os2.h>
#endif


#ifdef KeyPress
#undef KeyPress
#endif

class nsPluginInstanceOwner : public nsIPluginInstanceOwner,
                              public nsIPluginTagInfo,
                              public nsIDOMEventListener,
                              public nsIScrollPositionListener
{
public:
  nsPluginInstanceOwner();
  virtual ~nsPluginInstanceOwner();
  
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPLUGININSTANCEOWNER
  
  NS_IMETHOD GetURL(const char *aURL, const char *aTarget,
                    nsIInputStream *aPostStream, 
                    void *aHeadersData, PRUint32 aHeadersDataLen);
  
  NS_IMETHOD ShowStatus(const PRUnichar *aStatusMsg);
  
  NPError    ShowNativeContextMenu(NPMenu* menu, void* event);
  
  NPBool     ConvertPoint(double sourceX, double sourceY, NPCoordinateSpace sourceSpace,
                          double *destX, double *destY, NPCoordinateSpace destSpace);
  
  
  NS_DECL_NSIPLUGINTAGINFO
  
  
  NS_DECL_NSIDOMEVENTLISTENER
  
  nsresult MouseDown(nsIDOMEvent* aKeyEvent);
  nsresult KeyPress(nsIDOMEvent* aKeyEvent);
#if defined(MOZ_WIDGET_QT) && (MOZ_PLATFORM_MAEMO == 6)
  nsresult Text(nsIDOMEvent* aTextEvent);
#endif

  nsresult Destroy();  
  
  void PrepareToStop(bool aDelayedStop);
  
#ifdef XP_WIN
  void Paint(const RECT& aDirty, HDC aDC);
#elif defined(XP_MACOSX)
  void Paint(const gfxRect& aDirtyRect, CGContextRef cgContext);  
  void RenderCoreAnimation(CGContextRef aCGContext, int aWidth, int aHeight);
  void DoCocoaEventDrawRect(const gfxRect& aDrawRect, CGContextRef cgContext);
#elif defined(MOZ_X11) || defined(ANDROID)
  void Paint(gfxContext* aContext,
             const gfxRect& aFrameRect,
             const gfxRect& aDirtyRect);
#elif defined(XP_OS2)
  void Paint(const nsRect& aDirtyRect, HPS aHPS);
#endif
  
#ifdef MAC_CARBON_PLUGINS
  void CancelTimer();
  void StartTimer(bool isVisible);
#endif
  void SendIdleEvent();
  
  
  virtual void ScrollPositionWillChange(nscoord aX, nscoord aY);
  virtual void ScrollPositionDidChange(nscoord aX, nscoord aY);
  
  
  
  nsresult Init(nsPresContext* aPresContext, nsObjectFrame* aFrame,
                nsIContent* aContent);
  
  void* GetPluginPortFromWidget();
  void ReleasePluginPort(void* pluginPort);
  
  void SetPluginHost(nsIPluginHost* aHost);
  
  nsEventStatus ProcessEvent(const nsGUIEvent & anEvent);
  
#ifdef XP_MACOSX
  enum { ePluginPaintEnable, ePluginPaintDisable };
  
  NPDrawingModel GetDrawingModel();
  bool IsRemoteDrawingCoreAnimation();
  NPEventModel GetEventModel();
  static void CARefresh(nsITimer *aTimer, void *aClosure);
  static void AddToCARefreshTimer(nsPluginInstanceOwner *aPluginInstance);
  static void RemoveFromCARefreshTimer(nsPluginInstanceOwner *aPluginInstance);
  void SetupCARefresh();
  void* FixUpPluginWindow(PRInt32 inPaintState);
  void HidePluginWindow();
  
  
  void SetPluginPortChanged(bool aState) { mPluginPortChanged = aState; }
  
  
  void* GetPluginPortCopy();
  
  
  
  
  void* SetPluginPortAndDetectChange();
  
  
  
  
  
  void BeginCGPaint();
  void EndCGPaint();
#else 
  void UpdateWindowPositionAndClipRect(bool aSetWindow);
  void UpdateWindowVisibility(bool aVisible);
  void UpdateDocumentActiveState(bool aIsActive);
#endif 
  void CallSetWindow();
  
  void SetOwner(nsObjectFrame *aOwner)
  {
    mObjectFrame = aOwner;
  }
  nsObjectFrame* GetOwner() {
    return mObjectFrame;
  }
  
  PRUint32 GetLastEventloopNestingLevel() const {
    return mLastEventloopNestingLevel; 
  }
  
  static PRUint32 GetEventloopNestingLevel();
  
  void ConsiderNewEventloopNestingLevel() {
    PRUint32 currentLevel = GetEventloopNestingLevel();
    
    if (currentLevel < mLastEventloopNestingLevel) {
      mLastEventloopNestingLevel = currentLevel;
    }
  }
  
  const char* GetPluginName()
  {
    if (mInstance && mPluginHost) {
      const char* name = NULL;
      if (NS_SUCCEEDED(mPluginHost->GetPluginName(mInstance, &name)) && name)
        return name;
    }
    return "";
  }
  
#ifdef MOZ_X11
  void GetPluginDescription(nsACString& aDescription)
  {
    aDescription.Truncate();
    if (mInstance && mPluginHost) {
      nsCOMPtr<nsIPluginTag> pluginTag;
      
      mPluginHost->GetPluginTagForInstance(mInstance,
                                           getter_AddRefs(pluginTag));
      if (pluginTag) {
        pluginTag->GetDescription(aDescription);
      }
    }
  }
#endif
  
  bool SendNativeEvents()
  {
#ifdef XP_WIN
    
    return mPluginWindow->type == NPWindowTypeDrawable &&
    (MatchPluginName("Shockwave Flash") ||
     MatchPluginName("Test Plug-in"));
#elif defined(MOZ_X11) || defined(XP_MACOSX)
    return PR_TRUE;
#else
    return PR_FALSE;
#endif
  }
  
  bool MatchPluginName(const char *aPluginName)
  {
    return strncmp(GetPluginName(), aPluginName, strlen(aPluginName)) == 0;
  }
  
  void NotifyPaintWaiter(nsDisplayListBuilder* aBuilder);
  
  bool SetCurrentImage(ImageContainer* aContainer);
  




  nsIntSize GetCurrentImageSize();
  
  
  
  
  void SetBackgroundUnknown();
  already_AddRefed<gfxContext> BeginUpdateBackground(const nsIntRect& aRect);
  void EndUpdateBackground(gfxContext* aContext, const nsIntRect& aRect);
  
  bool UseAsyncRendering();
  
private:
  
  
  bool IsUpToDate()
  {
    nsIntSize size;
    return NS_SUCCEEDED(mInstance->GetImageSize(&size)) &&
    size == nsIntSize(mPluginWindow->width, mPluginWindow->height);
  }
  
  void FixUpURLS(const nsString &name, nsAString &value);
#ifdef ANDROID
  void RemovePluginView();
#endif 
 
  nsPluginNativeWindow       *mPluginWindow;
  nsRefPtr<nsNPAPIPluginInstance> mInstance;
  nsObjectFrame              *mObjectFrame; 
  nsCOMPtr<nsIContent>        mContent;
  nsCString                   mDocumentBase;
  char                       *mTagText;
  nsCOMPtr<nsIWidget>         mWidget;
  nsRefPtr<nsPluginHost>      mPluginHost;
  
#ifdef XP_MACOSX
  NP_CGContext                              mCGPluginPortCopy;
#ifndef NP_NO_QUICKDRAW
  NP_Port                                   mQDPluginPortCopy;
#endif
  PRInt32                                   mInCGPaintLevel;
  nsRefPtr<nsIOSurface>                     mIOSurface;
  nsCARenderer                              mCARenderer;
  CGColorSpaceRef                           mColorProfile;
  static nsCOMPtr<nsITimer>                *sCATimer;
  static nsTArray<nsPluginInstanceOwner*>  *sCARefreshListeners;
  bool                                      mSentInitialTopLevelWindowEvent;
#endif
  
  
  bool                                      mAsyncHidePluginWindow;
  
  
  
  
  PRUint32                    mLastEventloopNestingLevel;
  bool                        mContentFocused;
  bool                        mWidgetVisible;    
#ifdef XP_MACOSX
  bool                        mPluginPortChanged;
#endif
#ifdef MOZ_X11
  
  bool                        mFlash10Quirks;
#endif
  bool                        mPluginWindowVisible;
  bool                        mPluginDocumentActiveState;
  
  
  
  bool                        mDestroyWidget;
  PRUint16          mNumCachedAttrs;
  PRUint16          mNumCachedParams;
  char              **mCachedAttrParamNames;
  char              **mCachedAttrParamValues;
  
#ifdef XP_MACOSX
  NPEventModel mEventModel;
#endif
  
  
  nsRefPtr<nsPluginDOMContextMenuListener> mCXMenuListener;
  
  nsresult DispatchKeyToPlugin(nsIDOMEvent* aKeyEvent);
  nsresult DispatchMouseToPlugin(nsIDOMEvent* aMouseEvent);
  nsresult DispatchFocusToPlugin(nsIDOMEvent* aFocusEvent);
  
  nsresult EnsureCachedAttrParamArrays();
  
#ifdef MOZ_X11
  class Renderer
#if defined(MOZ_WIDGET_GTK2)
  : public gfxXlibNativeRenderer
#elif defined(MOZ_WIDGET_QT)
  : public gfxQtNativeRenderer
#endif
  {
  public:
    Renderer(NPWindow* aWindow, nsPluginInstanceOwner* aInstanceOwner,
             const nsIntSize& aPluginSize, const nsIntRect& aDirtyRect)
    : mWindow(aWindow), mInstanceOwner(aInstanceOwner),
    mPluginSize(aPluginSize), mDirtyRect(aDirtyRect)
    {}
    virtual nsresult DrawWithXlib(gfxXlibSurface* surface, nsIntPoint offset, 
                                  nsIntRect* clipRects, PRUint32 numClipRects);
  private:
    NPWindow* mWindow;
    nsPluginInstanceOwner* mInstanceOwner;
    const nsIntSize& mPluginSize;
    const nsIntRect& mDirtyRect;
  };
#endif

  bool mWaitingForPaint;
};

#endif 

