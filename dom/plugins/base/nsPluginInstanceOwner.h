





#ifndef nsPluginInstanceOwner_h_
#define nsPluginInstanceOwner_h_

#include "mozilla/Attributes.h"
#include "npapi.h"
#include "nsCOMPtr.h"
#include "nsIPluginInstanceOwner.h"
#include "nsIPrivacyTransitionObserver.h"
#include "nsIDOMEventListener.h"
#include "nsPluginHost.h"
#include "nsPluginNativeWindow.h"
#include "nsWeakReference.h"
#include "gfxRect.h"

#ifdef XP_MACOSX
#include "mozilla/gfx/QuartzSupport.h"
#include <ApplicationServices/ApplicationServices.h>
#endif

class nsIInputStream;
class nsPluginDOMContextMenuListener;
class nsPluginFrame;
class nsDisplayListBuilder;

namespace mozilla {
namespace dom {
struct MozPluginParameter;
}
namespace widget {
class PuppetWidget;
}
}

using mozilla::widget::PuppetWidget;

#ifdef MOZ_X11
#ifdef MOZ_WIDGET_QT
#include "gfxQtNativeRenderer.h"
#else
#include "gfxXlibNativeRenderer.h"
#endif
#endif

class nsPluginInstanceOwner final : public nsIPluginInstanceOwner,
                                    public nsIDOMEventListener,
                                    public nsIPrivacyTransitionObserver,
                                    public nsSupportsWeakReference
{
public:
  nsPluginInstanceOwner();
  
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPLUGININSTANCEOWNER
  NS_DECL_NSIPRIVACYTRANSITIONOBSERVER
  
  NS_IMETHOD GetURL(const char *aURL, const char *aTarget,
                    nsIInputStream *aPostStream, 
                    void *aHeadersData, uint32_t aHeadersDataLen) override;
  
  NS_IMETHOD ShowStatus(const char16_t *aStatusMsg) override;
  
  
  NPError    ShowNativeContextMenu(NPMenu* menu, void* event) override;
  
  NPBool     ConvertPoint(double sourceX, double sourceY, NPCoordinateSpace sourceSpace,
                          double *destX, double *destY, NPCoordinateSpace destSpace) override;
  
  



  NS_IMETHOD GetTagType(nsPluginTagType *aResult);

  void GetParameters(nsTArray<mozilla::dom::MozPluginParameter>& parameters);
  void GetAttributes(nsTArray<mozilla::dom::MozPluginParameter>& attributes);

  






  NS_IMETHOD GetDOMElement(nsIDOMElement* * aResult);
  
  
  NS_DECL_NSIDOMEVENTLISTENER
  
  nsresult ProcessMouseDown(nsIDOMEvent* aKeyEvent);
  nsresult ProcessKeyPress(nsIDOMEvent* aKeyEvent);
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
#endif

  
  
  nsresult Init(nsIContent* aContent);
  
  void* GetPluginPort();
  void ReleasePluginPort(void* pluginPort);

  nsEventStatus ProcessEvent(const mozilla::WidgetGUIEvent& anEvent);
  
#ifdef XP_MACOSX
  enum { ePluginPaintEnable, ePluginPaintDisable };

  void WindowFocusMayHaveChanged();
  void ResolutionMayHaveChanged();

  bool WindowIsActive();
  void SendWindowFocusChanged(bool aIsActive);
  NPDrawingModel GetDrawingModel();
  bool IsRemoteDrawingCoreAnimation();
  nsresult ContentsScaleFactorChanged(double aContentsScaleFactor);
  NPEventModel GetEventModel();
  static void CARefresh(nsITimer *aTimer, void *aClosure);
  void AddToCARefreshTimer();
  void RemoveFromCARefreshTimer();
  
  void FixUpPluginWindow(int32_t inPaintState);
  void HidePluginWindow();
  
  
  void* GetPluginPortCopy();
  
  
  void SetPluginPort();
  
  
  
  
  
  void BeginCGPaint();
  void EndCGPaint();
#else 
  void UpdateWindowPositionAndClipRect(bool aSetWindow);
  void UpdateWindowVisibility(bool aVisible);
#endif 

  void UpdateDocumentActiveState(bool aIsActive);

  void SetFrame(nsPluginFrame *aFrame);
  nsPluginFrame* GetFrame();

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
      const char* name = nullptr;
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

  
  already_AddRefed<mozilla::layers::ImageContainer> GetImageContainer();

  




  nsIntSize GetCurrentImageSize();
  
  
  
  
  void SetBackgroundUnknown();
  already_AddRefed<gfxContext> BeginUpdateBackground(const nsIntRect& aRect);
  void EndUpdateBackground(gfxContext* aContext, const nsIntRect& aRect);
  
  bool UseAsyncRendering();

  already_AddRefed<nsIURI> GetBaseURI() const;

#ifdef MOZ_WIDGET_ANDROID
  
  void GetVideos(nsTArray<nsNPAPIPluginInstance::VideoInfo*>& aVideos);
  already_AddRefed<mozilla::layers::ImageContainer> GetImageContainerForVideo(nsNPAPIPluginInstance::VideoInfo* aVideoInfo);

  void Invalidate();

  void RequestFullScreen();
  void ExitFullScreen();

  
  static void ExitFullScreen(jobject view);
#endif

  void NotifyHostAsyncInitFailed();
  void NotifyHostCreateWidget();
  void NotifyDestroyPending();

private:
  virtual ~nsPluginInstanceOwner();

  
  bool IsUpToDate()
  {
    nsIntSize size;
    return NS_SUCCEEDED(mInstance->GetImageSize(&size)) &&
    size == nsIntSize(mPluginWindow->width, mPluginWindow->height);
  }

#ifdef MOZ_WIDGET_ANDROID
  mozilla::LayoutDeviceRect GetPluginRect();
  bool AddPluginView(const mozilla::LayoutDeviceRect& aRect = mozilla::LayoutDeviceRect(0, 0, 0, 0));
  void RemovePluginView();

  bool mFullScreen;
  void* mJavaView;
#endif 
 
  nsPluginNativeWindow       *mPluginWindow;
  nsRefPtr<nsNPAPIPluginInstance> mInstance;
  nsPluginFrame              *mPluginFrame;
  nsWeakPtr                   mContent; 
  nsCString                   mDocumentBase;
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
  bool                                      mLastWindowIsActive;
  bool                                      mLastContentFocused;
  double                                    mLastScaleFactor;
  
  bool                                      mShouldBlurOnActivate;
#endif

  
  
  
  uint32_t                    mLastEventloopNestingLevel;
  bool                        mContentFocused;
  bool                        mWidgetVisible;    
#ifdef MOZ_X11
  
  bool                        mFlash10Quirks;
#endif
  bool                        mPluginWindowVisible;
  bool                        mPluginDocumentActiveState;

#ifdef XP_MACOSX
  NPEventModel mEventModel;
  
  
  
  
  bool mUseAsyncRendering;
#endif
  
  
  nsRefPtr<nsPluginDOMContextMenuListener> mCXMenuListener;
  
  nsresult DispatchKeyToPlugin(nsIDOMEvent* aKeyEvent);
  nsresult DispatchMouseToPlugin(nsIDOMEvent* aMouseEvent,
                                 bool aAllowPropagate = false);
  nsresult DispatchFocusToPlugin(nsIDOMEvent* aFocusEvent);

#ifdef XP_MACOSX
  static NPBool ConvertPointPuppet(PuppetWidget *widget, nsPluginFrame* pluginFrame,
                            double sourceX, double sourceY, NPCoordinateSpace sourceSpace,
                            double *destX, double *destY, NPCoordinateSpace destSpace);
  static NPBool ConvertPointNoPuppet(nsIWidget *widget, nsPluginFrame* pluginFrame,
                            double sourceX, double sourceY, NPCoordinateSpace sourceSpace,
                            double *destX, double *destY, NPCoordinateSpace destSpace);
  void PerformDelayedBlurs();
#endif    

  int mLastMouseDownButtonType;

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
    virtual nsresult DrawWithXlib(cairo_surface_t* surface,
                                  nsIntPoint offset,
                                  nsIntRect* clipRects, uint32_t numClipRects) override;
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

