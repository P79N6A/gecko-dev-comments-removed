





#ifndef nsPluginInstanceOwner_h_
#define nsPluginInstanceOwner_h_

#include "prtypes.h"
#include "npapi.h"
#include "nsCOMPtr.h"
#include "nsIPluginInstanceOwner.h"
#include "nsIPluginTagInfo.h"
#include "nsIPrivacyTransitionObserver.h"
#include "nsIDOMEventListener.h"
#include "nsIScrollPositionListener.h"
#include "nsPluginHost.h"
#include "nsPluginNativeWindow.h"
#include "nsWeakReference.h"
#include "gfxRect.h"


#ifdef KeyPress
#undef KeyPress
#endif

#ifdef XP_MACOSX
#include "mozilla/gfx/QuartzSupport.h"
#include <ApplicationServices/ApplicationServices.h>
#endif

class nsIInputStream;
struct nsIntRect;
class nsPluginDOMContextMenuListener;
class nsObjectFrame;
class nsDisplayListBuilder;

#ifdef MOZ_X11
class gfxXlibSurface;
#ifdef MOZ_WIDGET_QT
#include "gfxQtNativeRenderer.h"
#else
#include "gfxXlibNativeRenderer.h"
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
                              public nsIScrollPositionListener,
                              public nsIPrivacyTransitionObserver,
                              public nsSupportsWeakReference
{
public:
  nsPluginInstanceOwner();
  virtual ~nsPluginInstanceOwner();
  
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPLUGININSTANCEOWNER
  NS_DECL_NSIPRIVACYTRANSITIONOBSERVER
  
  NS_IMETHOD GetURL(const char *aURL, const char *aTarget,
                    nsIInputStream *aPostStream, 
                    void *aHeadersData, uint32_t aHeadersDataLen);
  
  NS_IMETHOD ShowStatus(const PRUnichar *aStatusMsg);
  
  NPError    ShowNativeContextMenu(NPMenu* menu, void* event);
  
  NPBool     ConvertPoint(double sourceX, double sourceY, NPCoordinateSpace sourceSpace,
                          double *destX, double *destY, NPCoordinateSpace destSpace);
  
  virtual NPError InitAsyncSurface(NPSize *size, NPImageFormat format,
                                   void *initData, NPAsyncSurface *surface);
  virtual NPError FinalizeAsyncSurface(NPAsyncSurface *surface);
  virtual void SetCurrentAsyncSurface(NPAsyncSurface *surface, NPRect *changed);

  
  NS_DECL_NSIPLUGINTAGINFO
  
  
  NS_DECL_NSIDOMEVENTLISTENER
  
  nsresult MouseDown(nsIDOMEvent* aKeyEvent);
  nsresult KeyPress(nsIDOMEvent* aKeyEvent);
#if defined(MOZ_WIDGET_QT) && (MOZ_PLATFORM_MAEMO == 6)
  nsresult Text(nsIDOMEvent* aTextEvent);
#endif

  nsresult Destroy();  

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
  
  
  
  nsresult Init(nsIContent* aContent);
  
  void* GetPluginPortFromWidget();
  void ReleasePluginPort(void* pluginPort);

  nsEventStatus ProcessEvent(const nsGUIEvent & anEvent);
  
#ifdef XP_MACOSX
  enum { ePluginPaintEnable, ePluginPaintDisable };
  
  NPDrawingModel GetDrawingModel();
  bool IsRemoteDrawingCoreAnimation();
  NPEventModel GetEventModel();
  static void CARefresh(nsITimer *aTimer, void *aClosure);
  void AddToCARefreshTimer();
  void RemoveFromCARefreshTimer();
  
  void* FixUpPluginWindow(int32_t inPaintState);
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

  void SetFrame(nsObjectFrame *aFrame);
  nsObjectFrame* GetFrame();

  uint32_t GetLastEventloopNestingLevel() const {
    return mLastEventloopNestingLevel; 
  }
  
  static uint32_t GetEventloopNestingLevel();
  
  void ConsiderNewEventloopNestingLevel() {
    uint32_t currentLevel = GetEventloopNestingLevel();
    
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
    return true;
#else
    return false;
#endif
  }
  
  bool MatchPluginName(const char *aPluginName)
  {
    return strncmp(GetPluginName(), aPluginName, strlen(aPluginName)) == 0;
  }
  
  void NotifyPaintWaiter(nsDisplayListBuilder* aBuilder);

  
  already_AddRefed<ImageContainer> GetImageContainer();

  




  nsIntSize GetCurrentImageSize();
  
  
  
  
  void SetBackgroundUnknown();
  already_AddRefed<gfxContext> BeginUpdateBackground(const nsIntRect& aRect);
  void EndUpdateBackground(gfxContext* aContext, const nsIntRect& aRect);
  
  bool UseAsyncRendering();

#ifdef MOZ_WIDGET_ANDROID
  
  void GetVideos(nsTArray<nsNPAPIPluginInstance::VideoInfo*>& aVideos);
  already_AddRefed<ImageContainer> GetImageContainerForVideo(nsNPAPIPluginInstance::VideoInfo* aVideoInfo);

  nsIntRect GetVisibleRect();

  void Invalidate();

  void RequestFullScreen();
  void ExitFullScreen();

  
  static void ExitFullScreen(jobject view);
#endif
  
private:
  
  
  bool IsUpToDate()
  {
    nsIntSize size;
    return NS_SUCCEEDED(mInstance->GetImageSize(&size)) &&
    size == nsIntSize(mPluginWindow->width, mPluginWindow->height);
  }
  
  void FixUpURLS(const nsString &name, nsAString &value);
#ifdef MOZ_WIDGET_ANDROID
  gfxRect GetPluginRect();
  bool AddPluginView(const gfxRect& aRect = gfxRect(0, 0, 0, 0));
  void RemovePluginView();

  bool mFullScreen;
  void* mJavaView;
#endif 
 
  nsPluginNativeWindow       *mPluginWindow;
  nsRefPtr<nsNPAPIPluginInstance> mInstance;
  nsObjectFrame              *mObjectFrame;
  nsIContent                 *mContent; 
  nsCString                   mDocumentBase;
  char                       *mTagText;
  bool                        mWidgetCreationComplete;
  nsCOMPtr<nsIWidget>         mWidget;
  nsRefPtr<nsPluginHost>      mPluginHost;
  
#ifdef XP_MACOSX
  NP_CGContext                              mCGPluginPortCopy;
  int32_t                                   mInCGPaintLevel;
  mozilla::RefPtr<MacIOSurface>             mIOSurface;
  mozilla::RefPtr<nsCARenderer>             mCARenderer;
  CGColorSpaceRef                           mColorProfile;
  static nsCOMPtr<nsITimer>                *sCATimer;
  static nsTArray<nsPluginInstanceOwner*>  *sCARefreshListeners;
  bool                                      mSentInitialTopLevelWindowEvent;
#endif

  
  
  
  uint32_t                    mLastEventloopNestingLevel;
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

  uint16_t          mNumCachedAttrs;
  uint16_t          mNumCachedParams;
  char              **mCachedAttrParamNames;
  char              **mCachedAttrParamValues;
  
#ifdef XP_MACOSX
  NPEventModel mEventModel;
  
  
  
  
  bool mUseAsyncRendering;
#endif
  
  
  nsRefPtr<nsPluginDOMContextMenuListener> mCXMenuListener;
  
  nsresult DispatchKeyToPlugin(nsIDOMEvent* aKeyEvent);
  nsresult DispatchMouseToPlugin(nsIDOMEvent* aMouseEvent);
  nsresult DispatchFocusToPlugin(nsIDOMEvent* aFocusEvent);
  
  nsresult EnsureCachedAttrParamArrays();
  
#ifdef MOZ_X11
  class Renderer
#if defined(MOZ_WIDGET_QT)
  : public gfxQtNativeRenderer
#else
  : public gfxXlibNativeRenderer
#endif
  {
  public:
    Renderer(NPWindow* aWindow, nsPluginInstanceOwner* aInstanceOwner,
             const nsIntSize& aPluginSize, const nsIntRect& aDirtyRect)
    : mWindow(aWindow), mInstanceOwner(aInstanceOwner),
    mPluginSize(aPluginSize), mDirtyRect(aDirtyRect)
    {}
    virtual nsresult DrawWithXlib(gfxXlibSurface* surface, nsIntPoint offset, 
                                  nsIntRect* clipRects, uint32_t numClipRects);
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

