




#include <android/log.h>
#include <math.h>
#include <unistd.h>

#include "mozilla/IMEStateManager.h"
#include "mozilla/MiscEvents.h"
#include "mozilla/MouseEvents.h"
#include "mozilla/TextComposition.h"
#include "mozilla/TextEvents.h"
#include "mozilla/TouchEvents.h"

#include "mozilla/dom/ContentParent.h"
#include "mozilla/dom/ContentChild.h"
#include "mozilla/unused.h"
#include "mozilla/Preferences.h"
#include "mozilla/layers/RenderTrace.h"
#include <algorithm>

using mozilla::dom::ContentParent;
using mozilla::dom::ContentChild;
using mozilla::unused;

#include "nsAppShell.h"
#include "nsIdleService.h"
#include "nsWindow.h"
#include "nsIObserverService.h"
#include "nsFocusManager.h"
#include "nsIWidgetListener.h"
#include "nsViewManager.h"
#include "nsISelection.h"

#include "nsIDOMSimpleGestureEvent.h"

#include "nsGkAtoms.h"
#include "nsWidgetsCID.h"
#include "nsGfxCIID.h"

#include "gfxContext.h"

#include "Layers.h"
#include "mozilla/layers/LayerManagerComposite.h"
#include "mozilla/layers/AsyncCompositionManager.h"
#include "mozilla/layers/APZCTreeManager.h"
#include "mozilla/layers/APZThreadUtils.h"
#include "GLContext.h"
#include "GLContextProvider.h"
#include "ScopedGLHelpers.h"
#include "mozilla/layers/CompositorOGL.h"
#include "APZCCallbackHandler.h"

#include "nsTArray.h"

#include "AndroidBridge.h"
#include "AndroidBridgeUtilities.h"
#include "android_npapi.h"

#include "imgIEncoder.h"

#include "nsString.h"
#include "GeckoProfiler.h" 
#include "nsIXULRuntime.h"

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::widget;
using namespace mozilla::layers;

NS_IMPL_ISUPPORTS_INHERITED0(nsWindow, nsBaseWidget)


static gfxIntSize gAndroidBounds = gfxIntSize(0, 0);
static gfxIntSize gAndroidScreenBounds;

#include "mozilla/layers/CompositorChild.h"
#include "mozilla/layers/CompositorParent.h"
#include "mozilla/layers/LayerTransactionParent.h"
#include "mozilla/Mutex.h"
#include "mozilla/Services.h"
#include "nsThreadUtils.h"

class ContentCreationNotifier;
static StaticRefPtr<ContentCreationNotifier> gContentCreationNotifier;



class ContentCreationNotifier final : public nsIObserver
{
private:
    ~ContentCreationNotifier() {}

public:
    NS_DECL_ISUPPORTS

    NS_IMETHOD Observe(nsISupports* aSubject,
                       const char* aTopic,
                       const char16_t* aData)
    {
        if (!strcmp(aTopic, "ipc:content-created")) {
            nsCOMPtr<nsIObserver> cpo = do_QueryInterface(aSubject);
            ContentParent* cp = static_cast<ContentParent*>(cpo.get());
            unused << cp->SendScreenSizeChanged(gAndroidScreenBounds);
        } else if (!strcmp(aTopic, "xpcom-shutdown")) {
            nsCOMPtr<nsIObserverService> obs =
                mozilla::services::GetObserverService();
            if (obs) {
                obs->RemoveObserver(static_cast<nsIObserver*>(this),
                                    "xpcom-shutdown");
                obs->RemoveObserver(static_cast<nsIObserver*>(this),
                                    "ipc:content-created");
            }
            gContentCreationNotifier = nullptr;
        }

        return NS_OK;
    }
};

NS_IMPL_ISUPPORTS(ContentCreationNotifier,
                  nsIObserver)

static bool gMenu;
static bool gMenuConsumed;




static nsTArray<nsWindow*> gTopLevelWindows;

static bool sFailedToCreateGLContext = false;


static const double SWIPE_MAX_PINCH_DELTA_INCHES = 0.4;
static const double SWIPE_MIN_DISTANCE_INCHES = 0.6;

nsWindow*
nsWindow::TopWindow()
{
    if (!gTopLevelWindows.IsEmpty())
        return gTopLevelWindows[0];
    return nullptr;
}

void
nsWindow::LogWindow(nsWindow *win, int index, int indent)
{
#if defined(DEBUG) || defined(FORCE_ALOG)
    char spaces[] = "                    ";
    spaces[indent < 20 ? indent : 20] = 0;
    ALOG("%s [% 2d] 0x%08x [parent 0x%08x] [% 3d,% 3dx% 3d,% 3d] vis %d type %d",
         spaces, index, (intptr_t)win, (intptr_t)win->mParent,
         win->mBounds.x, win->mBounds.y,
         win->mBounds.width, win->mBounds.height,
         win->mIsVisible, win->mWindowType);
#endif
}

void
nsWindow::DumpWindows()
{
    DumpWindows(gTopLevelWindows);
}

void
nsWindow::DumpWindows(const nsTArray<nsWindow*>& wins, int indent)
{
    for (uint32_t i = 0; i < wins.Length(); ++i) {
        nsWindow *w = wins[i];
        LogWindow(w, i, indent);
        DumpWindows(w->mChildren, indent+1);
    }
}

nsWindow::nsWindow() :
    mIsVisible(false),
    mParent(nullptr),
    mFocus(nullptr),
    mIMEMaskSelectionUpdate(false),
    mIMEMaskEventsCount(1), 
    mIMERanges(new TextRangeArray()),
    mIMEUpdatingContext(false),
    mIMESelectionChanged(false)
{
}

nsWindow::~nsWindow()
{
    gTopLevelWindows.RemoveElement(this);
    nsWindow *top = FindTopLevel();
    if (top->mFocus == this)
        top->mFocus = nullptr;
    ALOG("nsWindow %p destructor", (void*)this);
    if (mLayerManager == sLayerManager) {
        
        
        SetCompositor(nullptr, nullptr, nullptr);
    }
}

bool
nsWindow::IsTopLevel()
{
    return mWindowType == eWindowType_toplevel ||
        mWindowType == eWindowType_dialog ||
        mWindowType == eWindowType_invisible;
}

NS_IMETHODIMP
nsWindow::Create(nsIWidget *aParent,
                 nsNativeWidget aNativeParent,
                 const nsIntRect &aRect,
                 nsWidgetInitData *aInitData)
{
    ALOG("nsWindow[%p]::Create %p [%d %d %d %d]", (void*)this, (void*)aParent, aRect.x, aRect.y, aRect.width, aRect.height);
    nsWindow *parent = (nsWindow*) aParent;
    if (aNativeParent) {
        if (parent) {
            ALOG("Ignoring native parent on Android window [%p], since parent was specified (%p %p)", (void*)this, (void*)aNativeParent, (void*)aParent);
        } else {
            parent = (nsWindow*) aNativeParent;
        }
    }

    mBounds = aRect;

    
    if (!parent) {
        mBounds.x = 0;
        mBounds.y = 0;
        mBounds.width = gAndroidBounds.width;
        mBounds.height = gAndroidBounds.height;
    }

    BaseCreate(nullptr, mBounds, aInitData);

    NS_ASSERTION(IsTopLevel() || parent, "non top level windowdoesn't have a parent!");

    if (IsTopLevel()) {
        gTopLevelWindows.AppendElement(this);
    }

    if (parent) {
        parent->mChildren.AppendElement(this);
        mParent = parent;
    }

#ifdef DEBUG_ANDROID_WIDGET
    DumpWindows();
#endif

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::Destroy(void)
{
    nsBaseWidget::mOnDestroyCalled = true;

    while (mChildren.Length()) {
        
        ALOG("### Warning: Destroying window %p and reparenting child %p to null!", (void*)this, (void*)mChildren[0]);
        mChildren[0]->SetParent(nullptr);
    }

    if (IsTopLevel())
        gTopLevelWindows.RemoveElement(this);

    SetParent(nullptr);

    nsBaseWidget::OnDestroy();

#ifdef DEBUG_ANDROID_WIDGET
    DumpWindows();
#endif

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::ConfigureChildren(const nsTArray<nsIWidget::Configuration>& config)
{
    for (uint32_t i = 0; i < config.Length(); ++i) {
        nsWindow *childWin = (nsWindow*) config[i].mChild;
        childWin->Resize(config[i].mBounds.x,
                         config[i].mBounds.y,
                         config[i].mBounds.width,
                         config[i].mBounds.height,
                         false);
    }

    return NS_OK;
}

void
nsWindow::RedrawAll()
{
    if (mFocus) {
        mFocus->RedrawAll();
    }
    if (mWidgetListener) {
        mWidgetListener->RequestRepaint();
    }
}

NS_IMETHODIMP
nsWindow::SetParent(nsIWidget *aNewParent)
{
    if ((nsIWidget*)mParent == aNewParent)
        return NS_OK;

    
    
    if (mParent)
        mParent->mChildren.RemoveElement(this);

    mParent = (nsWindow*)aNewParent;

    if (mParent)
        mParent->mChildren.AppendElement(this);

    
    if (FindTopLevel() == nsWindow::TopWindow())
        RedrawAll();

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::ReparentNativeWidget(nsIWidget *aNewParent)
{
    NS_PRECONDITION(aNewParent, "");
    return NS_OK;
}

nsIWidget*
nsWindow::GetParent()
{
    return mParent;
}

float
nsWindow::GetDPI()
{
    if (AndroidBridge::Bridge())
        return AndroidBridge::Bridge()->GetDPI();
    return 160.0f;
}

double
nsWindow::GetDefaultScaleInternal()
{
    static double density = 0.0;

    if (density != 0.0) {
        return density;
    }

    density = GeckoAppShell::GetDensity();

    if (!density) {
        density = 1.0;
    }

    return density;
}

NS_IMETHODIMP
nsWindow::Show(bool aState)
{
    ALOG("nsWindow[%p]::Show %d", (void*)this, aState);

    if (mWindowType == eWindowType_invisible) {
        ALOG("trying to show invisible window! ignoring..");
        return NS_ERROR_FAILURE;
    }

    if (aState == mIsVisible)
        return NS_OK;

    mIsVisible = aState;

    if (IsTopLevel()) {
        
        

        
        
        
        

        if (aState) {
            
            
            Resize(0, 0, gAndroidBounds.width, gAndroidBounds.height, false);
            BringToFront();
        } else if (nsWindow::TopWindow() == this) {
            
            unsigned int i;
            for (i = 1; i < gTopLevelWindows.Length(); i++) {
                nsWindow *win = gTopLevelWindows[i];
                if (!win->mIsVisible)
                    continue;

                win->BringToFront();
                break;
            }
        }
    } else if (FindTopLevel() == nsWindow::TopWindow()) {
        RedrawAll();
    }

#ifdef DEBUG_ANDROID_WIDGET
    DumpWindows();
#endif

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::SetModal(bool aState)
{
    ALOG("nsWindow[%p]::SetModal %d ignored", (void*)this, aState);

    return NS_OK;
}

bool
nsWindow::IsVisible() const
{
    return mIsVisible;
}

NS_IMETHODIMP
nsWindow::ConstrainPosition(bool aAllowSlop,
                            int32_t *aX,
                            int32_t *aY)
{
    ALOG("nsWindow[%p]::ConstrainPosition %d [%d %d]", (void*)this, aAllowSlop, *aX, *aY);

    
    if (IsTopLevel()) {
        *aX = 0;
        *aY = 0;
    }

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::Move(double aX,
               double aY)
{
    if (IsTopLevel())
        return NS_OK;

    return Resize(aX,
                  aY,
                  mBounds.width,
                  mBounds.height,
                  true);
}

NS_IMETHODIMP
nsWindow::Resize(double aWidth,
                 double aHeight,
                 bool aRepaint)
{
    return Resize(mBounds.x,
                  mBounds.y,
                  aWidth,
                  aHeight,
                  aRepaint);
}

NS_IMETHODIMP
nsWindow::Resize(double aX,
                 double aY,
                 double aWidth,
                 double aHeight,
                 bool aRepaint)
{
    ALOG("nsWindow[%p]::Resize [%f %f %f %f] (repaint %d)", (void*)this, aX, aY, aWidth, aHeight, aRepaint);

    bool needSizeDispatch = aWidth != mBounds.width || aHeight != mBounds.height;

    mBounds.x = NSToIntRound(aX);
    mBounds.y = NSToIntRound(aY);
    mBounds.width = NSToIntRound(aWidth);
    mBounds.height = NSToIntRound(aHeight);

    if (needSizeDispatch)
        OnSizeChanged(gfxIntSize(aWidth, aHeight));

    
    if (aRepaint && FindTopLevel() == nsWindow::TopWindow())
        RedrawAll();

    return NS_OK;
}

void
nsWindow::SetZIndex(int32_t aZIndex)
{
    ALOG("nsWindow[%p]::SetZIndex %d ignored", (void*)this, aZIndex);
}

NS_IMETHODIMP
nsWindow::PlaceBehind(nsTopLevelWidgetZPlacement aPlacement,
                      nsIWidget *aWidget,
                      bool aActivate)
{
    return NS_OK;
}

NS_IMETHODIMP
nsWindow::SetSizeMode(int32_t aMode)
{
    switch (aMode) {
        case nsSizeMode_Minimized:
            GeckoAppShell::MoveTaskToBack();
            break;
        case nsSizeMode_Fullscreen:
            MakeFullScreen(true);
            break;
    }
    return NS_OK;
}

NS_IMETHODIMP
nsWindow::Enable(bool aState)
{
    ALOG("nsWindow[%p]::Enable %d ignored", (void*)this, aState);
    return NS_OK;
}

bool
nsWindow::IsEnabled() const
{
    return true;
}

NS_IMETHODIMP
nsWindow::Invalidate(const nsIntRect &aRect)
{
    return NS_OK;
}

nsWindow*
nsWindow::FindTopLevel()
{
    nsWindow *toplevel = this;
    while (toplevel) {
        if (toplevel->IsTopLevel())
            return toplevel;

        toplevel = toplevel->mParent;
    }

    ALOG("nsWindow::FindTopLevel(): couldn't find a toplevel or dialog window in this [%p] widget's hierarchy!", (void*)this);
    return this;
}

NS_IMETHODIMP
nsWindow::SetFocus(bool aRaise)
{
    if (!aRaise) {
        ALOG("nsWindow::SetFocus: can't set focus without raising, ignoring aRaise = false!");
    }

    nsWindow *top = FindTopLevel();
    top->mFocus = this;
    top->BringToFront();

    return NS_OK;
}

void
nsWindow::BringToFront()
{
    
    
    
    
    nsCOMPtr<nsIFocusManager> fm = do_GetService(FOCUSMANAGER_CONTRACTID);
    nsCOMPtr<nsIDOMWindow> existingTopWindow;
    fm->GetActiveWindow(getter_AddRefs(existingTopWindow));
    if (existingTopWindow && FindTopLevel() == nsWindow::TopWindow())
        return;

    if (!IsTopLevel()) {
        FindTopLevel()->BringToFront();
        return;
    }

    nsRefPtr<nsWindow> kungFuDeathGrip(this);

    nsWindow *oldTop = nullptr;
    nsWindow *newTop = this;
    if (!gTopLevelWindows.IsEmpty())
        oldTop = gTopLevelWindows[0];

    gTopLevelWindows.RemoveElement(this);
    gTopLevelWindows.InsertElementAt(0, this);

    if (oldTop) {
      nsIWidgetListener* listener = oldTop->GetWidgetListener();
      if (listener) {
          listener->WindowDeactivated();
      }
    }

    if (Destroyed()) {
        
        
        
        if (gTopLevelWindows.IsEmpty())
            return;
        newTop = gTopLevelWindows[0];
    }

    if (mWidgetListener) {
        mWidgetListener->WindowActivated();
    }

    
    nsAppShell::gAppShell->ResendLastResizeEvent(newTop);
    RedrawAll();
}

NS_IMETHODIMP
nsWindow::GetScreenBounds(nsIntRect &aRect)
{
    LayoutDeviceIntPoint p = WidgetToScreenOffset();

    aRect.x = p.x;
    aRect.y = p.y;
    aRect.width = mBounds.width;
    aRect.height = mBounds.height;
    
    return NS_OK;
}

LayoutDeviceIntPoint
nsWindow::WidgetToScreenOffset()
{
    LayoutDeviceIntPoint p(0, 0);
    nsWindow *w = this;

    while (w && !w->IsTopLevel()) {
        p.x += w->mBounds.x;
        p.y += w->mBounds.y;

        w = w->mParent;
    }

    return p;
}

NS_IMETHODIMP
nsWindow::DispatchEvent(WidgetGUIEvent* aEvent,
                        nsEventStatus& aStatus)
{
    aStatus = DispatchEvent(aEvent);
    return NS_OK;
}

nsEventStatus
nsWindow::DispatchEvent(WidgetGUIEvent* aEvent)
{
    if (this == TopWindow() && mFocus) {
        
        
        
        
        
        
        
        
        
        return mFocus->DispatchEvent(aEvent);
    }

    aEvent->widget = this;
    if (mWidgetListener) {
        return mWidgetListener->HandleEvent(aEvent, mUseAttachedEvents);
    }
    return nsEventStatus_eIgnore;
}

NS_IMETHODIMP
nsWindow::MakeFullScreen(bool aFullScreen, nsIScreen*)
{
    GeckoAppShell::SetFullScreen(aFullScreen);
    return NS_OK;
}

NS_IMETHODIMP
nsWindow::SetWindowClass(const nsAString& xulWinType)
{
    return NS_OK;
}

mozilla::layers::LayerManager*
nsWindow::GetLayerManager(PLayerTransactionChild*, LayersBackend, LayerManagerPersistence,
                          bool* aAllowRetaining)
{
    if (aAllowRetaining) {
        *aAllowRetaining = true;
    }
    if (mLayerManager) {
        return mLayerManager;
    }
    
    
    if (ShouldUseOffMainThreadCompositing()) {
        return sLayerManager;
    }
    return nullptr;
}

void
nsWindow::CreateLayerManager(int aCompositorWidth, int aCompositorHeight)
{
    if (mLayerManager) {
        return;
    }

    nsWindow *topLevelWindow = FindTopLevel();
    if (!topLevelWindow || topLevelWindow->mWindowType == eWindowType_invisible) {
        
        return;
    }

    mUseLayersAcceleration = ComputeShouldAccelerate(mUseLayersAcceleration);

    if (ShouldUseOffMainThreadCompositing()) {
        if (sLayerManager) {
            return;
        }
        CreateCompositor(aCompositorWidth, aCompositorHeight);
        if (mLayerManager) {
            
            
            SetCompositor(mLayerManager, mCompositorParent, mCompositorChild);
            sCompositorPaused = false;
            return;
        }

        
        sFailedToCreateGLContext = true;
    }

    if (!mUseLayersAcceleration || sFailedToCreateGLContext) {
        printf_stderr(" -- creating basic, not accelerated\n");
        mLayerManager = CreateBasicLayerManager();
    }
}

void
nsWindow::OnGlobalAndroidEvent(AndroidGeckoEvent *ae)
{
    nsWindow *win = TopWindow();
    if (!win)
        return;

    switch (ae->Type()) {
        case AndroidGeckoEvent::FORCED_RESIZE:
            win->mBounds.width = 0;
            win->mBounds.height = 0;
            
            for (uint32_t i = 0; i < win->mChildren.Length(); i++) {
                win->mChildren[i]->mBounds.width = 0;
                win->mChildren[i]->mBounds.height = 0;
            }
        case AndroidGeckoEvent::SIZE_CHANGED: {
            const nsTArray<nsIntPoint>& points = ae->Points();
            NS_ASSERTION(points.Length() == 2, "Size changed does not have enough coordinates");

            int nw = points[0].x;
            int nh = points[0].y;

            if (ae->Type() == AndroidGeckoEvent::FORCED_RESIZE || nw != gAndroidBounds.width ||
                nh != gAndroidBounds.height) {
                gAndroidBounds.width = nw;
                gAndroidBounds.height = nh;

                
                for (size_t i = 0; i < gTopLevelWindows.Length(); ++i) {
                    if (gTopLevelWindows[i]->mIsVisible)
                        gTopLevelWindows[i]->Resize(gAndroidBounds.width,
                                                    gAndroidBounds.height,
                                                    false);
                }
            }

            int newScreenWidth = points[1].x;
            int newScreenHeight = points[1].y;

            if (newScreenWidth == gAndroidScreenBounds.width &&
                newScreenHeight == gAndroidScreenBounds.height)
                break;

            gAndroidScreenBounds.width = newScreenWidth;
            gAndroidScreenBounds.height = newScreenHeight;

            if (XRE_GetProcessType() != GeckoProcessType_Default &&
                !Preferences::GetBool("browser.tabs.remote.desktopbehavior", false)) {
                break;
            }

            
            nsTArray<ContentParent*> cplist;
            ContentParent::GetAll(cplist);
            for (uint32_t i = 0; i < cplist.Length(); ++i)
                unused << cplist[i]->SendScreenSizeChanged(gAndroidScreenBounds);

            if (gContentCreationNotifier)
                break;

            
            
            nsCOMPtr<nsIObserverService> obs =
                mozilla::services::GetObserverService();
            if (!obs)
                break;

            nsRefPtr<ContentCreationNotifier> notifier = new ContentCreationNotifier;
            if (NS_SUCCEEDED(obs->AddObserver(notifier, "ipc:content-created", false))) {
                if (NS_SUCCEEDED(obs->AddObserver(notifier, "xpcom-shutdown", false)))
                    gContentCreationNotifier = notifier;
                else
                    obs->RemoveObserver(notifier, "ipc:content-created");
            }
            break;
        }

        case AndroidGeckoEvent::APZ_INPUT_EVENT: {
            win->UserActivity();

            WidgetTouchEvent touchEvent = ae->MakeTouchEvent(win);
            win->ProcessUntransformedAPZEvent(&touchEvent, ae->ApzGuid(), ae->ApzInputBlockId(), ae->ApzEventStatus());
            break;
        }
        case AndroidGeckoEvent::MOTION_EVENT: {
            win->UserActivity();
            bool preventDefaultActions = win->OnMultitouchEvent(ae);
            if (!preventDefaultActions && ae->Count() < 2) {
                win->OnMouseEvent(ae);
            }
            break;
        }

        
        
        case AndroidGeckoEvent::LONG_PRESS: {
            win->UserActivity();

            nsCOMPtr<nsIObserverService> obsServ = mozilla::services::GetObserverService();
            obsServ->NotifyObservers(nullptr, "before-build-contextmenu", nullptr);

            
            if (!win->OnContextmenuEvent(ae)) {
                
                
                win->OnLongTapEvent(ae);
            }
            break;
        }

        case AndroidGeckoEvent::NATIVE_GESTURE_EVENT: {
            win->OnNativeGestureEvent(ae);
            break;
        }

        case AndroidGeckoEvent::KEY_EVENT:
            win->UserActivity();
            win->OnKeyEvent(ae);
            break;

        case AndroidGeckoEvent::IME_EVENT:
            win->UserActivity();
            win->OnIMEEvent(ae);
            break;

        case AndroidGeckoEvent::IME_KEY_EVENT:
            
            
            
            win->mIMEKeyEvents.AppendElement(*ae);
            break;

        case AndroidGeckoEvent::COMPOSITOR_PAUSE:
            
            
            
            
            if (sCompositorChild) {
                sCompositorChild->SendPause();
            }
            sCompositorPaused = true;
            break;

        case AndroidGeckoEvent::COMPOSITOR_CREATE:
            win->CreateLayerManager(ae->Width(), ae->Height());
            

        case AndroidGeckoEvent::COMPOSITOR_RESUME:
            
            
            
            
            
            
            
            if (!sCompositorPaused) {
                win->RedrawAll();
            }
            break;
    }
}

void
nsWindow::OnSizeChanged(const gfxIntSize& aSize)
{
    ALOG("nsWindow: %p OnSizeChanged [%d %d]", (void*)this, aSize.width, aSize.height);

    mBounds.width = aSize.width;
    mBounds.height = aSize.height;

    if (mWidgetListener) {
        mWidgetListener->WindowResized(this, aSize.width, aSize.height);
    }
}

void
nsWindow::InitEvent(WidgetGUIEvent& event, nsIntPoint* aPoint)
{
    if (aPoint) {
        event.refPoint.x = aPoint->x;
        event.refPoint.y = aPoint->y;
    } else {
        event.refPoint.x = 0;
        event.refPoint.y = 0;
    }

    event.time = PR_Now() / 1000;
}

gfxIntSize
nsWindow::GetAndroidScreenBounds()
{
    if (XRE_GetProcessType() == GeckoProcessType_Content) {
        return ContentChild::GetSingleton()->GetScreenSize();
    }
    return gAndroidScreenBounds;
}

void *
nsWindow::GetNativeData(uint32_t aDataType)
{
    switch (aDataType) {
        
        case NS_NATIVE_DISPLAY:
            return nullptr;

        case NS_NATIVE_WIDGET:
            return (void *) this;
    }

    return nullptr;
}

void
nsWindow::OnMouseEvent(AndroidGeckoEvent *ae)
{
    nsRefPtr<nsWindow> kungFuDeathGrip(this);

    WidgetMouseEvent event = ae->MakeMouseEvent(this);
    if (event.message == NS_EVENT_NULL) {
        
        return;
    }

    DispatchEvent(&event);
}

bool
nsWindow::OnContextmenuEvent(AndroidGeckoEvent *ae)
{
    nsRefPtr<nsWindow> kungFuDeathGrip(this);

    CSSPoint pt;
    const nsTArray<nsIntPoint>& points = ae->Points();
    if (points.Length() > 0) {
        pt = CSSPoint(points[0].x, points[0].y);
    }

    
    WidgetMouseEvent contextMenuEvent(true, NS_CONTEXTMENU, this,
                                      WidgetMouseEvent::eReal, WidgetMouseEvent::eNormal);
    contextMenuEvent.refPoint =
        RoundedToInt(pt * GetDefaultScale()) - WidgetToScreenOffset();
    contextMenuEvent.ignoreRootScrollFrame = true;
    contextMenuEvent.inputSource = nsIDOMMouseEvent::MOZ_SOURCE_TOUCH;

    nsEventStatus contextMenuStatus;
    DispatchEvent(&contextMenuEvent, contextMenuStatus);

    
    
    
    if (contextMenuStatus == nsEventStatus_eConsumeNoDefault) {
        WidgetTouchEvent canceltouchEvent = ae->MakeTouchEvent(this);
        canceltouchEvent.message = NS_TOUCH_CANCEL;
        DispatchEvent(&canceltouchEvent);
        return true;
    }

    return false;
}

void
nsWindow::OnLongTapEvent(AndroidGeckoEvent *ae)
{
    nsRefPtr<nsWindow> kungFuDeathGrip(this);

    CSSPoint pt;
    const nsTArray<nsIntPoint>& points = ae->Points();
    if (points.Length() > 0) {
        pt = CSSPoint(points[0].x, points[0].y);
    }

    
    WidgetMouseEvent event(true, NS_MOUSE_MOZLONGTAP, this,
        WidgetMouseEvent::eReal, WidgetMouseEvent::eNormal);
    event.button = WidgetMouseEvent::eLeftButton;
    event.refPoint =
        RoundedToInt(pt * GetDefaultScale()) - WidgetToScreenOffset();
    event.clickCount = 1;
    event.time = ae->Time();
    event.inputSource = nsIDOMMouseEvent::MOZ_SOURCE_TOUCH;
    event.ignoreRootScrollFrame = true;

    DispatchEvent(&event);
}

bool nsWindow::OnMultitouchEvent(AndroidGeckoEvent *ae)
{
    nsRefPtr<nsWindow> kungFuDeathGrip(this);

    
    
    RemoveIMEComposition();

    
    
    
    static bool sDefaultPreventedNotified = false;
    static bool sLastWasDownEvent = false;

    bool preventDefaultActions = false;
    bool isDownEvent = false;

    WidgetTouchEvent event = ae->MakeTouchEvent(this);
    if (event.message != NS_EVENT_NULL) {
        nsEventStatus status;
        DispatchEvent(&event, status);
        
        
        
        
        preventDefaultActions = (status == nsEventStatus_eConsumeNoDefault ||
                                event.mFlags.mMultipleActionsPrevented);
        isDownEvent = (event.message == NS_TOUCH_START);
    }

    if (isDownEvent && event.touches.Length() == 1) {
        
        
        
        
        WidgetMouseEvent hittest(true, NS_MOUSE_MOZHITTEST, this, WidgetMouseEvent::eReal);
        hittest.refPoint = event.touches[0]->mRefPoint;
        hittest.ignoreRootScrollFrame = true;
        hittest.inputSource = nsIDOMMouseEvent::MOZ_SOURCE_TOUCH;
        nsEventStatus status;
        DispatchEvent(&hittest, status);
    }

    
    
    
    if (sLastWasDownEvent && !sDefaultPreventedNotified) {
        
        
        bool defaultPrevented = isDownEvent ? false : preventDefaultActions;
        if (ae->Type() == AndroidGeckoEvent::APZ_INPUT_EVENT) {
            widget::android::APZCCallbackHandler::GetInstance()->NotifyDefaultPrevented(ae->ApzInputBlockId(), defaultPrevented);
        } else {
            GeckoAppShell::NotifyDefaultPrevented(defaultPrevented);
        }
        sDefaultPreventedNotified = true;
    }

    
    
    
    if (isDownEvent) {
        if (preventDefaultActions) {
            if (ae->Type() == AndroidGeckoEvent::APZ_INPUT_EVENT) {
                widget::android::APZCCallbackHandler::GetInstance()->NotifyDefaultPrevented(ae->ApzInputBlockId(), true);
            } else {
                GeckoAppShell::NotifyDefaultPrevented(true);
            }
            sDefaultPreventedNotified = true;
        } else {
            sDefaultPreventedNotified = false;
        }
    }
    sLastWasDownEvent = isDownEvent;

    return preventDefaultActions;
}

void
nsWindow::OnNativeGestureEvent(AndroidGeckoEvent *ae)
{
    LayoutDeviceIntPoint pt(ae->Points()[0].x,
                            ae->Points()[0].y);
    double delta = ae->X();
    int msg = 0;

    switch (ae->Action()) {
        case AndroidMotionEvent::ACTION_MAGNIFY_START:
            msg = NS_SIMPLE_GESTURE_MAGNIFY_START;
            mStartDist = delta;
            mLastDist = delta;
            break;
        case AndroidMotionEvent::ACTION_MAGNIFY:
            msg = NS_SIMPLE_GESTURE_MAGNIFY_UPDATE;
            delta -= mLastDist;
            mLastDist += delta;
            break;
        case AndroidMotionEvent::ACTION_MAGNIFY_END:
            msg = NS_SIMPLE_GESTURE_MAGNIFY;
            delta -= mStartDist;
            break;
        default:
            return;
    }

    nsRefPtr<nsWindow> kungFuDeathGrip(this);

    WidgetSimpleGestureEvent event(true, msg, this);

    event.direction = 0;
    event.delta = delta;
    event.modifiers = 0;
    event.time = ae->Time();
    event.refPoint = pt;

    DispatchEvent(&event);
}


static unsigned int ConvertAndroidKeyCodeToDOMKeyCode(int androidKeyCode)
{
    
    if (androidKeyCode >= AKEYCODE_A &&
        androidKeyCode <= AKEYCODE_Z) {
        return androidKeyCode - AKEYCODE_A + NS_VK_A;
    }

    if (androidKeyCode >= AKEYCODE_0 &&
        androidKeyCode <= AKEYCODE_9) {
        return androidKeyCode - AKEYCODE_0 + NS_VK_0;
    }

    switch (androidKeyCode) {
        
        case AKEYCODE_BACK:               return NS_VK_ESCAPE;
        
        case AKEYCODE_DPAD_UP:            return NS_VK_UP;
        case AKEYCODE_DPAD_DOWN:          return NS_VK_DOWN;
        case AKEYCODE_DPAD_LEFT:          return NS_VK_LEFT;
        case AKEYCODE_DPAD_RIGHT:         return NS_VK_RIGHT;
        case AKEYCODE_DPAD_CENTER:        return NS_VK_RETURN;
        case AKEYCODE_VOLUME_UP:          return NS_VK_VOLUME_UP;
        case AKEYCODE_VOLUME_DOWN:        return NS_VK_VOLUME_DOWN;
        
        case AKEYCODE_COMMA:              return NS_VK_COMMA;
        case AKEYCODE_PERIOD:             return NS_VK_PERIOD;
        case AKEYCODE_ALT_LEFT:           return NS_VK_ALT;
        case AKEYCODE_ALT_RIGHT:          return NS_VK_ALT;
        case AKEYCODE_SHIFT_LEFT:         return NS_VK_SHIFT;
        case AKEYCODE_SHIFT_RIGHT:        return NS_VK_SHIFT;
        case AKEYCODE_TAB:                return NS_VK_TAB;
        case AKEYCODE_SPACE:              return NS_VK_SPACE;
        
        case AKEYCODE_ENTER:              return NS_VK_RETURN;
        case AKEYCODE_DEL:                return NS_VK_BACK; 
        case AKEYCODE_GRAVE:              return NS_VK_BACK_QUOTE;
        
        case AKEYCODE_EQUALS:             return NS_VK_EQUALS;
        case AKEYCODE_LEFT_BRACKET:       return NS_VK_OPEN_BRACKET;
        case AKEYCODE_RIGHT_BRACKET:      return NS_VK_CLOSE_BRACKET;
        case AKEYCODE_BACKSLASH:          return NS_VK_BACK_SLASH;
        case AKEYCODE_SEMICOLON:          return NS_VK_SEMICOLON;
        
        case AKEYCODE_SLASH:              return NS_VK_SLASH;
        
        case AKEYCODE_MUTE:               return NS_VK_VOLUME_MUTE;
        case AKEYCODE_PAGE_UP:            return NS_VK_PAGE_UP;
        case AKEYCODE_PAGE_DOWN:          return NS_VK_PAGE_DOWN;
        
        case AKEYCODE_ESCAPE:             return NS_VK_ESCAPE;
        case AKEYCODE_FORWARD_DEL:        return NS_VK_DELETE;
        case AKEYCODE_CTRL_LEFT:          return NS_VK_CONTROL;
        case AKEYCODE_CTRL_RIGHT:         return NS_VK_CONTROL;
        case AKEYCODE_CAPS_LOCK:          return NS_VK_CAPS_LOCK;
        case AKEYCODE_SCROLL_LOCK:        return NS_VK_SCROLL_LOCK;
        
        case AKEYCODE_SYSRQ:              return NS_VK_PRINTSCREEN;
        case AKEYCODE_BREAK:              return NS_VK_PAUSE;
        case AKEYCODE_MOVE_HOME:          return NS_VK_HOME;
        case AKEYCODE_MOVE_END:           return NS_VK_END;
        case AKEYCODE_INSERT:             return NS_VK_INSERT;
        
        case AKEYCODE_F1:                 return NS_VK_F1;
        case AKEYCODE_F2:                 return NS_VK_F2;
        case AKEYCODE_F3:                 return NS_VK_F3;
        case AKEYCODE_F4:                 return NS_VK_F4;
        case AKEYCODE_F5:                 return NS_VK_F5;
        case AKEYCODE_F6:                 return NS_VK_F6;
        case AKEYCODE_F7:                 return NS_VK_F7;
        case AKEYCODE_F8:                 return NS_VK_F8;
        case AKEYCODE_F9:                 return NS_VK_F9;
        case AKEYCODE_F10:                return NS_VK_F10;
        case AKEYCODE_F11:                return NS_VK_F11;
        case AKEYCODE_F12:                return NS_VK_F12;
        case AKEYCODE_NUM_LOCK:           return NS_VK_NUM_LOCK;
        case AKEYCODE_NUMPAD_0:           return NS_VK_NUMPAD0;
        case AKEYCODE_NUMPAD_1:           return NS_VK_NUMPAD1;
        case AKEYCODE_NUMPAD_2:           return NS_VK_NUMPAD2;
        case AKEYCODE_NUMPAD_3:           return NS_VK_NUMPAD3;
        case AKEYCODE_NUMPAD_4:           return NS_VK_NUMPAD4;
        case AKEYCODE_NUMPAD_5:           return NS_VK_NUMPAD5;
        case AKEYCODE_NUMPAD_6:           return NS_VK_NUMPAD6;
        case AKEYCODE_NUMPAD_7:           return NS_VK_NUMPAD7;
        case AKEYCODE_NUMPAD_8:           return NS_VK_NUMPAD8;
        case AKEYCODE_NUMPAD_9:           return NS_VK_NUMPAD9;
        case AKEYCODE_NUMPAD_DIVIDE:      return NS_VK_DIVIDE;
        case AKEYCODE_NUMPAD_MULTIPLY:    return NS_VK_MULTIPLY;
        case AKEYCODE_NUMPAD_SUBTRACT:    return NS_VK_SUBTRACT;
        case AKEYCODE_NUMPAD_ADD:         return NS_VK_ADD;
        case AKEYCODE_NUMPAD_DOT:         return NS_VK_DECIMAL;
        case AKEYCODE_NUMPAD_COMMA:       return NS_VK_SEPARATOR;
        case AKEYCODE_NUMPAD_ENTER:       return NS_VK_RETURN;
        case AKEYCODE_NUMPAD_EQUALS:      return NS_VK_EQUALS;
        

        
        
        
        
        case AKEYCODE_ZENKAKU_HANKAKU:    return 0;
        case AKEYCODE_EISU:               return NS_VK_EISU;
        case AKEYCODE_MUHENKAN:           return NS_VK_NONCONVERT;
        case AKEYCODE_HENKAN:             return NS_VK_CONVERT;
        case AKEYCODE_KATAKANA_HIRAGANA:  return 0;
        case AKEYCODE_YEN:                return NS_VK_BACK_SLASH; 
        case AKEYCODE_RO:                 return NS_VK_BACK_SLASH; 
        case AKEYCODE_KANA:               return NS_VK_KANA;
        case AKEYCODE_ASSIST:             return NS_VK_HELP;

        
        case AKEYCODE_BUTTON_A:          return NS_VK_RETURN;

        default:
            ALOG("ConvertAndroidKeyCodeToDOMKeyCode: "
                 "No DOM keycode for Android keycode %d", androidKeyCode);
        return 0;
    }
}

static KeyNameIndex
ConvertAndroidKeyCodeToKeyNameIndex(AndroidGeckoEvent& aAndroidGeckoEvent)
{
    int keyCode = aAndroidGeckoEvent.KeyCode();
    
    if (keyCode >= AKEYCODE_A && keyCode <= AKEYCODE_Z) {
        return KEY_NAME_INDEX_USE_STRING;
    }

    if (keyCode >= AKEYCODE_0 && keyCode <= AKEYCODE_9) {
        return KEY_NAME_INDEX_USE_STRING;
    }

    switch (keyCode) {

#define NS_NATIVE_KEY_TO_DOM_KEY_NAME_INDEX(aNativeKey, aKeyNameIndex) \
        case aNativeKey: return aKeyNameIndex;

#include "NativeKeyToDOMKeyName.h"

#undef NS_NATIVE_KEY_TO_DOM_KEY_NAME_INDEX

        
        case AKEYCODE_STAR:               
        case AKEYCODE_POUND:              

        

        case AKEYCODE_COMMA:              
        case AKEYCODE_PERIOD:             
        case AKEYCODE_SPACE:
        case AKEYCODE_GRAVE:              
        case AKEYCODE_MINUS:              
        case AKEYCODE_EQUALS:             
        case AKEYCODE_LEFT_BRACKET:       
        case AKEYCODE_RIGHT_BRACKET:      
        case AKEYCODE_BACKSLASH:          
        case AKEYCODE_SEMICOLON:          
        case AKEYCODE_APOSTROPHE:         
        case AKEYCODE_SLASH:              
        case AKEYCODE_AT:                 
        case AKEYCODE_PLUS:               

        case AKEYCODE_NUMPAD_0:
        case AKEYCODE_NUMPAD_1:
        case AKEYCODE_NUMPAD_2:
        case AKEYCODE_NUMPAD_3:
        case AKEYCODE_NUMPAD_4:
        case AKEYCODE_NUMPAD_5:
        case AKEYCODE_NUMPAD_6:
        case AKEYCODE_NUMPAD_7:
        case AKEYCODE_NUMPAD_8:
        case AKEYCODE_NUMPAD_9:
        case AKEYCODE_NUMPAD_DIVIDE:
        case AKEYCODE_NUMPAD_MULTIPLY:
        case AKEYCODE_NUMPAD_SUBTRACT:
        case AKEYCODE_NUMPAD_ADD:
        case AKEYCODE_NUMPAD_DOT:
        case AKEYCODE_NUMPAD_COMMA:
        case AKEYCODE_NUMPAD_EQUALS:
        case AKEYCODE_NUMPAD_LEFT_PAREN:
        case AKEYCODE_NUMPAD_RIGHT_PAREN:

        case AKEYCODE_YEN:                
        case AKEYCODE_RO:                 
            return KEY_NAME_INDEX_USE_STRING;

        case AKEYCODE_SOFT_LEFT:
        case AKEYCODE_SOFT_RIGHT:
        case AKEYCODE_CALL:
        case AKEYCODE_ENDCALL:
        case AKEYCODE_NUM:                
        case AKEYCODE_HEADSETHOOK:
        case AKEYCODE_NOTIFICATION:       
        case AKEYCODE_PICTSYMBOLS:

        case AKEYCODE_BUTTON_A:
        case AKEYCODE_BUTTON_B:
        case AKEYCODE_BUTTON_C:
        case AKEYCODE_BUTTON_X:
        case AKEYCODE_BUTTON_Y:
        case AKEYCODE_BUTTON_Z:
        case AKEYCODE_BUTTON_L1:
        case AKEYCODE_BUTTON_R1:
        case AKEYCODE_BUTTON_L2:
        case AKEYCODE_BUTTON_R2:
        case AKEYCODE_BUTTON_THUMBL:
        case AKEYCODE_BUTTON_THUMBR:
        case AKEYCODE_BUTTON_START:
        case AKEYCODE_BUTTON_SELECT:
        case AKEYCODE_BUTTON_MODE:

        case AKEYCODE_MUTE: 
        case AKEYCODE_MEDIA_CLOSE:

        case AKEYCODE_DVR:

        case AKEYCODE_BUTTON_1:
        case AKEYCODE_BUTTON_2:
        case AKEYCODE_BUTTON_3:
        case AKEYCODE_BUTTON_4:
        case AKEYCODE_BUTTON_5:
        case AKEYCODE_BUTTON_6:
        case AKEYCODE_BUTTON_7:
        case AKEYCODE_BUTTON_8:
        case AKEYCODE_BUTTON_9:
        case AKEYCODE_BUTTON_10:
        case AKEYCODE_BUTTON_11:
        case AKEYCODE_BUTTON_12:
        case AKEYCODE_BUTTON_13:
        case AKEYCODE_BUTTON_14:
        case AKEYCODE_BUTTON_15:
        case AKEYCODE_BUTTON_16:

        case AKEYCODE_MANNER_MODE:
        case AKEYCODE_3D_MODE:
        case AKEYCODE_CONTACTS:
            return KEY_NAME_INDEX_Unidentified;

        case AKEYCODE_UNKNOWN:
            MOZ_ASSERT(
                aAndroidGeckoEvent.Action() != AKEY_EVENT_ACTION_MULTIPLE,
                "Don't call this when action is AKEY_EVENT_ACTION_MULTIPLE!");
            
            
            return aAndroidGeckoEvent.DOMPrintableKeyValue() ?
                KEY_NAME_INDEX_USE_STRING : KEY_NAME_INDEX_Unidentified;

        default:
            ALOG("ConvertAndroidKeyCodeToKeyNameIndex: "
                 "No DOM key name index for Android keycode %d", keyCode);
            return KEY_NAME_INDEX_Unidentified;
    }
}

static CodeNameIndex
ConvertAndroidScanCodeToCodeNameIndex(AndroidGeckoEvent& aAndroidGeckoEvent)
{
    switch (aAndroidGeckoEvent.ScanCode()) {

#define NS_NATIVE_KEY_TO_DOM_CODE_NAME_INDEX(aNativeKey, aCodeNameIndex) \
        case aNativeKey: return aCodeNameIndex;

#include "NativeKeyToDOMCodeName.h"

#undef NS_NATIVE_KEY_TO_DOM_CODE_NAME_INDEX

        default:
          return CODE_NAME_INDEX_UNKNOWN;
    }
}

static void InitPluginEvent(ANPEvent* pluginEvent, ANPKeyActions keyAction,
                            AndroidGeckoEvent& key)
{
    int androidKeyCode = key.KeyCode();
    uint32_t domKeyCode = ConvertAndroidKeyCodeToDOMKeyCode(androidKeyCode);

    int modifiers = 0;
    if (key.IsAltPressed())
      modifiers |= kAlt_ANPKeyModifier;
    if (key.IsShiftPressed())
      modifiers |= kShift_ANPKeyModifier;

    pluginEvent->inSize = sizeof(ANPEvent);
    pluginEvent->eventType = kKey_ANPEventType;
    pluginEvent->data.key.action = keyAction;
    pluginEvent->data.key.nativeCode = androidKeyCode;
    pluginEvent->data.key.virtualCode = domKeyCode;
    pluginEvent->data.key.unichar = key.UnicodeChar();
    pluginEvent->data.key.modifiers = modifiers;
    pluginEvent->data.key.repeatCount = key.RepeatCount();
}

void
nsWindow::InitKeyEvent(WidgetKeyboardEvent& event, AndroidGeckoEvent& key,
                       ANPEvent* pluginEvent)
{
    event.mKeyNameIndex = ConvertAndroidKeyCodeToKeyNameIndex(key);
    if (event.mKeyNameIndex == KEY_NAME_INDEX_USE_STRING) {
        int keyValue = key.DOMPrintableKeyValue();
        if (keyValue) {
            event.mKeyValue = static_cast<char16_t>(keyValue);
        }
    }
    event.mCodeNameIndex = ConvertAndroidScanCodeToCodeNameIndex(key);
    uint32_t domKeyCode = ConvertAndroidKeyCodeToDOMKeyCode(key.KeyCode());

    if (event.message == NS_KEY_PRESS) {
        
        int charCode = key.UnicodeChar();
        if (!charCode) {
            charCode = key.BaseUnicodeChar();
        }
        event.isChar = (charCode >= ' ');
        event.charCode = event.isChar ? charCode : 0;
        event.keyCode = (event.charCode > 0) ? 0 : domKeyCode;
        event.mPluginEvent.Clear();
    } else {
#ifdef DEBUG
        if (event.message != NS_KEY_DOWN && event.message != NS_KEY_UP) {
            ALOG("InitKeyEvent: unexpected event.message %d", event.message);
        }
#endif 

        
        ANPKeyActions action = event.message == NS_KEY_DOWN
                             ? kDown_ANPKeyAction
                             : kUp_ANPKeyAction;
        InitPluginEvent(pluginEvent, action, key);

        event.isChar = false;
        event.charCode = 0;
        event.keyCode = domKeyCode;
        event.mPluginEvent.Copy(*pluginEvent);
    }

    event.modifiers = key.DOMModifiers();
    if (gMenu) {
        event.modifiers |= MODIFIER_CONTROL;
    }
    
    
    
    
    
    
    if (event.message == NS_KEY_PRESS &&
        key.UnicodeChar() && key.UnicodeChar() != key.BaseUnicodeChar()) {
        event.modifiers &= ~(MODIFIER_ALT | MODIFIER_CONTROL | MODIFIER_META);
    }

    event.mIsRepeat =
        (event.message == NS_KEY_DOWN || event.message == NS_KEY_PRESS) &&
        (!!(key.Flags() & AKEY_EVENT_FLAG_LONG_PRESS) || !!key.RepeatCount());
    event.location =
        WidgetKeyboardEvent::ComputeLocationFromCodeValue(event.mCodeNameIndex);
    event.time = key.Time();

    if (gMenu)
        gMenuConsumed = true;
}

void
nsWindow::HandleSpecialKey(AndroidGeckoEvent *ae)
{
    nsRefPtr<nsWindow> kungFuDeathGrip(this);
    nsCOMPtr<nsIAtom> command;
    bool isDown = ae->Action() == AKEY_EVENT_ACTION_DOWN;
    bool isLongPress = !!(ae->Flags() & AKEY_EVENT_FLAG_LONG_PRESS);
    bool doCommand = false;
    uint32_t keyCode = ae->KeyCode();

    if (isDown) {
        switch (keyCode) {
            case AKEYCODE_BACK:
                if (isLongPress) {
                    command = nsGkAtoms::Clear;
                    doCommand = true;
                }
                break;
            case AKEYCODE_MENU:
                gMenu = true;
                gMenuConsumed = isLongPress;
                break;
        }
    } else {
        switch (keyCode) {
            case AKEYCODE_BACK: {
                
                WidgetKeyboardEvent pressEvent(true, NS_KEY_PRESS, this);
                ANPEvent pluginEvent;
                InitKeyEvent(pressEvent, *ae, &pluginEvent);
                DispatchEvent(&pressEvent);
                return;
            }
            case AKEYCODE_MENU:
                gMenu = false;
                if (!gMenuConsumed) {
                    command = nsGkAtoms::Menu;
                    doCommand = true;
                }
                break;
            case AKEYCODE_SEARCH:
                command = nsGkAtoms::Search;
                doCommand = true;
                break;
            default:
                ALOG("Unknown special key code!");
                return;
        }
    }
    if (doCommand) {
        WidgetCommandEvent event(true, nsGkAtoms::onAppCommand, command, this);
        InitEvent(event);
        DispatchEvent(&event);
    }
}

void
nsWindow::OnKeyEvent(AndroidGeckoEvent *ae)
{
    nsRefPtr<nsWindow> kungFuDeathGrip(this);
    RemoveIMEComposition();
    uint32_t msg;
    switch (ae->Action()) {
    case AKEY_EVENT_ACTION_DOWN:
        msg = NS_KEY_DOWN;
        break;
    case AKEY_EVENT_ACTION_UP:
        msg = NS_KEY_UP;
        break;
    case AKEY_EVENT_ACTION_MULTIPLE:
        
        
        MOZ_CRASH("Cannot handle key with multiple action");
    default:
        ALOG("Unknown key action event!");
        return;
    }

    bool firePress = ae->Action() == AKEY_EVENT_ACTION_DOWN;
    switch (ae->KeyCode()) {
    case AKEYCODE_SHIFT_LEFT:
    case AKEYCODE_SHIFT_RIGHT:
    case AKEYCODE_ALT_LEFT:
    case AKEYCODE_ALT_RIGHT:
    case AKEYCODE_CTRL_LEFT:
    case AKEYCODE_CTRL_RIGHT:
        firePress = false;
        break;
    case AKEYCODE_BACK:
    case AKEYCODE_MENU:
    case AKEYCODE_SEARCH:
        HandleSpecialKey(ae);
        return;
    }

    nsEventStatus status;
    WidgetKeyboardEvent event(true, msg, this);
    ANPEvent pluginEvent;
    InitKeyEvent(event, *ae, &pluginEvent);
    DispatchEvent(&event, status);

    if (Destroyed())
        return;
    if (!firePress || status == nsEventStatus_eConsumeNoDefault) {
        return;
    }

    WidgetKeyboardEvent pressEvent(true, NS_KEY_PRESS, this);
    InitKeyEvent(pressEvent, *ae, &pluginEvent);
#ifdef DEBUG_ANDROID_WIDGET
    __android_log_print(ANDROID_LOG_INFO, "Gecko", "Dispatching key pressEvent with keyCode %d charCode %d shift %d alt %d sym/ctrl %d metamask %d", pressEvent.keyCode, pressEvent.charCode, pressEvent.IsShift(), pressEvent.IsAlt(), pressEvent.IsControl(), ae->MetaState());
#endif
    DispatchEvent(&pressEvent);
}

#ifdef DEBUG_ANDROID_IME
#define ALOGIME(args...) ALOG(args)
#else
#define ALOGIME(args...) ((void)0)
#endif

static nscolor
ConvertAndroidColor(uint32_t argb)
{
    return NS_RGBA((argb & 0x00ff0000) >> 16,
                   (argb & 0x0000ff00) >> 8,
                   (argb & 0x000000ff),
                   (argb & 0xff000000) >> 24);
}

class AutoIMEMask {
private:
    bool mOldMask, *mMask;
public:
    AutoIMEMask(bool &mask) : mOldMask(mask), mMask(&mask) {
        mask = true;
    }
    ~AutoIMEMask() {
        *mMask = mOldMask;
    }
};




nsRefPtr<mozilla::TextComposition>
nsWindow::GetIMEComposition()
{
    
    
    
    
    
    MOZ_ASSERT(this == TopWindow());
    nsWindow* win = this;
    if (mFocus) {
        win = mFocus;
    }
    return mozilla::IMEStateManager::GetTextCompositionFor(win);
}




void
nsWindow::RemoveIMEComposition()
{
    
    if (!GetIMEComposition()) {
        return;
    }

    nsRefPtr<nsWindow> kungFuDeathGrip(this);
    AutoIMEMask selMask(mIMEMaskSelectionUpdate);

    WidgetCompositionEvent compositionCommitEvent(true,
                                                  NS_COMPOSITION_COMMIT_AS_IS,
                                                  this);
    InitEvent(compositionCommitEvent, nullptr);
    DispatchEvent(&compositionCommitEvent);
}

void
nsWindow::OnIMEEvent(AndroidGeckoEvent *ae)
{
    MOZ_ASSERT(!mIMEMaskSelectionUpdate);
    









    nsRefPtr<nsWindow> kungFuDeathGrip(this);

    if (ae->Action() == AndroidGeckoEvent::IME_ACKNOWLEDGE_FOCUS) {
        MOZ_ASSERT(mIMEMaskEventsCount > 0);
        mIMEMaskEventsCount--;
        if (!mIMEMaskEventsCount) {
            
            
            mIMETextChanges.Clear();
            mIMESelectionChanged = false;
            
            
            
            
            IMENotification notification(NOTIFY_IME_OF_TEXT_CHANGE);
            notification.mTextChangeData.mOldEndOffset =
                notification.mTextChangeData.mNewEndOffset = INT32_MAX / 2;
            NotifyIMEOfTextChange(notification);
            FlushIMEChanges();
        }
        GeckoAppShell::NotifyIME(AndroidBridge::NOTIFY_IME_REPLY_EVENT);
        return;

    } else if (ae->Action() == AndroidGeckoEvent::IME_UPDATE_CONTEXT) {
        GeckoAppShell::NotifyIMEContext(mInputContext.mIMEState.mEnabled,
                                        mInputContext.mHTMLInputType,
                                        mInputContext.mHTMLInputInputmode,
                                        mInputContext.mActionHint);
        mIMEUpdatingContext = false;
        return;
    }

    if (mIMEMaskEventsCount > 0) {
        
        if (ae->Action() == AndroidGeckoEvent::IME_SYNCHRONIZE ||
            ae->Action() == AndroidGeckoEvent::IME_COMPOSE_TEXT ||
            ae->Action() == AndroidGeckoEvent::IME_REPLACE_TEXT) {
            GeckoAppShell::NotifyIME(AndroidBridge::NOTIFY_IME_REPLY_EVENT);
        }
        return;
    }

    switch (ae->Action()) {
    case AndroidGeckoEvent::IME_FLUSH_CHANGES:
        {
            FlushIMEChanges();
        }
        break;

    case AndroidGeckoEvent::IME_SYNCHRONIZE:
        {
            FlushIMEChanges();
            GeckoAppShell::NotifyIME(AndroidBridge::NOTIFY_IME_REPLY_EVENT);
        }
        break;

    case AndroidGeckoEvent::IME_REPLACE_TEXT:
    case AndroidGeckoEvent::IME_COMPOSE_TEXT:
        {
            






            AutoIMEMask selMask(mIMEMaskSelectionUpdate);
            const auto composition(GetIMEComposition());
            MOZ_ASSERT(!composition || !composition->IsEditorHandlingEvent());

            if (!mIMEKeyEvents.IsEmpty() || !composition ||
                uint32_t(ae->Start()) !=
                    composition->NativeOffsetOfStartComposition() ||
                uint32_t(ae->End()) !=
                    composition->NativeOffsetOfStartComposition() +
                    composition->String().Length()) {

                
                
                
                RemoveIMEComposition();

                {
                    WidgetSelectionEvent event(true, NS_SELECTION_SET, this);
                    InitEvent(event, nullptr);
                    event.mOffset = uint32_t(ae->Start());
                    event.mLength = uint32_t(ae->End() - ae->Start());
                    event.mExpandToClusterBoundary = false;
                    DispatchEvent(&event);
                }

                if (!mIMEKeyEvents.IsEmpty()) {
                    for (uint32_t i = 0; i < mIMEKeyEvents.Length(); i++) {
                        OnKeyEvent(&mIMEKeyEvents[i]);
                    }
                    mIMEKeyEvents.Clear();
                    FlushIMEChanges();
                    GeckoAppShell::NotifyIME(AndroidBridge::NOTIFY_IME_REPLY_EVENT);
                    
                    break;
                }

                {
                    WidgetCompositionEvent event(
                        true, NS_COMPOSITION_START, this);
                    InitEvent(event, nullptr);
                    DispatchEvent(&event);
                }

            } else if (composition->String().Equals(ae->Characters())) {
                



                IMEChange dummyChange;
                dummyChange.mStart = ae->Start();
                dummyChange.mOldEnd = dummyChange.mNewEnd = ae->End();
                AddIMETextChange(dummyChange);
            }

            {
                WidgetCompositionEvent event(true, NS_COMPOSITION_CHANGE, this);
                InitEvent(event, nullptr);
                event.mData = ae->Characters();

                
                TextRange range;
                range.mStartOffset = 0;
                range.mEndOffset = event.mData.Length();
                range.mRangeType = NS_TEXTRANGE_RAWINPUT;
                event.mRanges = new TextRangeArray();
                event.mRanges->AppendElement(range);

                DispatchEvent(&event);
            }

            
            if (ae->Action() != AndroidGeckoEvent::IME_COMPOSE_TEXT) {
                WidgetCompositionEvent compositionCommitEvent(
                        true, NS_COMPOSITION_COMMIT_AS_IS, this);
                InitEvent(compositionCommitEvent, nullptr);
                DispatchEvent(&compositionCommitEvent);
            }

            FlushIMEChanges();
            GeckoAppShell::NotifyIME(AndroidBridge::NOTIFY_IME_REPLY_EVENT);
        }
        break;

    case AndroidGeckoEvent::IME_SET_SELECTION:
        {
            





            AutoIMEMask selMask(mIMEMaskSelectionUpdate);
            RemoveIMEComposition();
            WidgetSelectionEvent selEvent(true, NS_SELECTION_SET, this);
            InitEvent(selEvent, nullptr);

            int32_t start = ae->Start(), end = ae->End();

            if (start < 0 || end < 0) {
                WidgetQueryContentEvent event(true, NS_QUERY_SELECTED_TEXT,
                                              this);
                InitEvent(event, nullptr);
                DispatchEvent(&event);
                MOZ_ASSERT(event.mSucceeded && !event.mWasAsync);

                if (start < 0)
                    start = int32_t(event.GetSelectionStart());
                if (end < 0)
                    end = int32_t(event.GetSelectionEnd());
            }

            selEvent.mOffset = std::min(start, end);
            selEvent.mLength = std::max(start, end) - selEvent.mOffset;
            selEvent.mReversed = start > end;
            selEvent.mExpandToClusterBoundary = false;

            DispatchEvent(&selEvent);
        }
        break;
    case AndroidGeckoEvent::IME_ADD_COMPOSITION_RANGE:
        {
            TextRange range;
            range.mStartOffset = ae->Start();
            range.mEndOffset = ae->End();
            range.mRangeType = ae->RangeType();
            range.mRangeStyle.mDefinedStyles = ae->RangeStyles();
            range.mRangeStyle.mLineStyle = ae->RangeLineStyle();
            range.mRangeStyle.mIsBoldLine = ae->RangeBoldLine();
            range.mRangeStyle.mForegroundColor =
                    ConvertAndroidColor(uint32_t(ae->RangeForeColor()));
            range.mRangeStyle.mBackgroundColor =
                    ConvertAndroidColor(uint32_t(ae->RangeBackColor()));
            range.mRangeStyle.mUnderlineColor =
                    ConvertAndroidColor(uint32_t(ae->RangeLineColor()));
            mIMERanges->AppendElement(range);
        }
        break;
    case AndroidGeckoEvent::IME_UPDATE_COMPOSITION:
        {
            










            AutoIMEMask selMask(mIMEMaskSelectionUpdate);
            const auto composition(GetIMEComposition());
            MOZ_ASSERT(!composition || !composition->IsEditorHandlingEvent());

            WidgetCompositionEvent event(true, NS_COMPOSITION_CHANGE, this);
            InitEvent(event, nullptr);

            event.mRanges = new TextRangeArray();
            mIMERanges.swap(event.mRanges);

            if (!composition ||
                uint32_t(ae->Start()) !=
                    composition->NativeOffsetOfStartComposition() ||
                uint32_t(ae->End()) !=
                    composition->NativeOffsetOfStartComposition() +
                    composition->String().Length()) {

                
                
                RemoveIMEComposition();

                {
                    WidgetSelectionEvent event(true, NS_SELECTION_SET, this);
                    InitEvent(event, nullptr);
                    event.mOffset = uint32_t(ae->Start());
                    event.mLength = uint32_t(ae->End() - ae->Start());
                    event.mExpandToClusterBoundary = false;
                    DispatchEvent(&event);
                }

                {
                    WidgetQueryContentEvent queryEvent(true,
                                                       NS_QUERY_SELECTED_TEXT,
                                                       this);
                    InitEvent(queryEvent, nullptr);
                    DispatchEvent(&queryEvent);
                    MOZ_ASSERT(queryEvent.mSucceeded && !queryEvent.mWasAsync);
                    event.mData = queryEvent.mReply.mString;
                }

                {
                    WidgetCompositionEvent event(
                        true, NS_COMPOSITION_START, this);
                    InitEvent(event, nullptr);
                    DispatchEvent(&event);
                }

            } else {
                
                
                event.mData = composition->String();
            }

#ifdef DEBUG_ANDROID_IME
            const NS_ConvertUTF16toUTF8 data(event.mData);
            const char* text = data.get();
            ALOGIME("IME: IME_SET_TEXT: text=\"%s\", length=%u, range=%u",
                    text, event.mData.Length(), event.mRanges->Length());
#endif 

            DispatchEvent(&event);
        }
        break;

    case AndroidGeckoEvent::IME_REMOVE_COMPOSITION:
        {
            






            AutoIMEMask selMask(mIMEMaskSelectionUpdate);
            RemoveIMEComposition();
            mIMERanges->Clear();
        }
        break;
    }
}

void
nsWindow::UserActivity()
{
  if (!mIdleService) {
    mIdleService = do_GetService("@mozilla.org/widget/idleservice;1");
  }

  if (mIdleService) {
    mIdleService->ResetIdleTimeOut(0);
  }
}

nsresult
nsWindow::NotifyIMEInternal(const IMENotification& aIMENotification)
{
    
    
    
    nsWindow* top = TopWindow();
    if (top && top != this) {
        return top->NotifyIMEInternal(aIMENotification);
    }

    switch (aIMENotification.mMessage) {
        case REQUEST_TO_COMMIT_COMPOSITION:
            
            RemoveIMEComposition();
            GeckoAppShell::NotifyIME(REQUEST_TO_COMMIT_COMPOSITION);
            return NS_OK;

        case REQUEST_TO_CANCEL_COMPOSITION:
            ALOGIME("IME: REQUEST_TO_CANCEL_COMPOSITION");

            
            if (GetIMEComposition()) {
                nsRefPtr<nsWindow> kungFuDeathGrip(this);

                WidgetCompositionEvent compositionCommitEvent(
                                         true, NS_COMPOSITION_COMMIT, this);
                InitEvent(compositionCommitEvent, nullptr);
                
                
                DispatchEvent(&compositionCommitEvent);
            }

            GeckoAppShell::NotifyIME(REQUEST_TO_CANCEL_COMPOSITION);
            return NS_OK;

        case NOTIFY_IME_OF_FOCUS:
            ALOGIME("IME: NOTIFY_IME_OF_FOCUS");
            GeckoAppShell::NotifyIME(NOTIFY_IME_OF_FOCUS);
            return NS_OK;

        case NOTIFY_IME_OF_BLUR:
            ALOGIME("IME: NOTIFY_IME_OF_BLUR");

            
            
            
            mIMEMaskEventsCount++;

            GeckoAppShell::NotifyIME(NOTIFY_IME_OF_BLUR);
            return NS_OK;

        case NOTIFY_IME_OF_SELECTION_CHANGE:
            if (mIMEMaskSelectionUpdate) {
                return NS_OK;
            }

            ALOGIME("IME: NOTIFY_IME_OF_SELECTION_CHANGE");

            PostFlushIMEChanges();
            mIMESelectionChanged = true;
            return NS_OK;

        case NOTIFY_IME_OF_TEXT_CHANGE:
            return NotifyIMEOfTextChange(aIMENotification);

        default:
            return NS_ERROR_NOT_IMPLEMENTED;
    }
}

NS_IMETHODIMP_(void)
nsWindow::SetInputContext(const InputContext& aContext,
                          const InputContextAction& aAction)
{
    nsWindow *top = TopWindow();
    if (top && this != top) {
        
        
        
        
        top->SetInputContext(aContext, aAction);
        return;
    }

    ALOGIME("IME: SetInputContext: s=0x%X, 0x%X, action=0x%X, 0x%X",
            aContext.mIMEState.mEnabled, aContext.mIMEState.mOpen,
            aAction.mCause, aAction.mFocusChange);

    mInputContext = aContext;

    
    
    if (aContext.mIMEState.mEnabled != IMEState::DISABLED && 
        aContext.mIMEState.mEnabled != IMEState::PLUGIN &&
        Preferences::GetBool("content.ime.strict_policy", false) &&
        !aAction.ContentGotFocusByTrustedCause() &&
        !aAction.UserMightRequestOpenVKB()) {
        return;
    }

    IMEState::Enabled enabled = aContext.mIMEState.mEnabled;

    
    
    if (aContext.mIMEState.mEnabled == IMEState::PLUGIN &&
        aContext.mIMEState.mOpen != IMEState::OPEN) {
        enabled = IMEState::DISABLED;
    }

    mInputContext.mIMEState.mEnabled = enabled;

    if (enabled == IMEState::ENABLED && aAction.UserMightRequestOpenVKB()) {
        
        GeckoAppShell::NotifyIME(AndroidBridge::NOTIFY_IME_OPEN_VKB);
        return;
    }

    if (mIMEUpdatingContext) {
        return;
    }
    AndroidGeckoEvent *event = AndroidGeckoEvent::MakeIMEEvent(
            AndroidGeckoEvent::IME_UPDATE_CONTEXT);
    nsAppShell::gAppShell->PostEvent(event);
    mIMEUpdatingContext = true;
}

NS_IMETHODIMP_(InputContext)
nsWindow::GetInputContext()
{
    nsWindow *top = TopWindow();
    if (top && this != top) {
        
        
        return top->GetInputContext();
    }
    InputContext context = mInputContext;
    context.mIMEState.mOpen = IMEState::OPEN_STATE_NOT_SUPPORTED;
    
    context.mNativeIMEContext = nullptr;
    return context;
}

void
nsWindow::PostFlushIMEChanges()
{
    if (!mIMETextChanges.IsEmpty() || mIMESelectionChanged) {
        
        return;
    }
    AndroidGeckoEvent *event = AndroidGeckoEvent::MakeIMEEvent(
            AndroidGeckoEvent::IME_FLUSH_CHANGES);
    nsAppShell::gAppShell->PostEvent(event);
}

void
nsWindow::FlushIMEChanges()
{
    
    
    NS_ENSURE_TRUE_VOID(!mIMEMaskEventsCount);

    nsCOMPtr<nsISelection> imeSelection;
    nsCOMPtr<nsIContent> imeRoot;

    
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(IMEStateManager::GetFocusSelectionAndRoot(
            getter_AddRefs(imeSelection), getter_AddRefs(imeRoot))));

    nsRefPtr<nsWindow> kungFuDeathGrip(this);

    for (uint32_t i = 0; i < mIMETextChanges.Length(); i++) {
        IMEChange &change = mIMETextChanges[i];

        if (change.mStart == change.mOldEnd &&
                change.mStart == change.mNewEnd) {
            continue;
        }

        WidgetQueryContentEvent event(true, NS_QUERY_TEXT_CONTENT, this);

        if (change.mNewEnd != change.mStart) {
            InitEvent(event, nullptr);
            event.InitForQueryTextContent(change.mStart,
                                          change.mNewEnd - change.mStart);
            DispatchEvent(&event);
            NS_ENSURE_TRUE_VOID(event.mSucceeded);
            NS_ENSURE_TRUE_VOID(event.mReply.mContentsRoot == imeRoot.get());
        }

        GeckoAppShell::NotifyIMEChange(event.mReply.mString, change.mStart,
                                       change.mOldEnd, change.mNewEnd);
    }
    mIMETextChanges.Clear();

    if (mIMESelectionChanged) {
        WidgetQueryContentEvent event(true, NS_QUERY_SELECTED_TEXT, this);
        InitEvent(event, nullptr);

        DispatchEvent(&event);
        NS_ENSURE_TRUE_VOID(event.mSucceeded);
        NS_ENSURE_TRUE_VOID(event.mReply.mContentsRoot == imeRoot.get());

        GeckoAppShell::NotifyIMEChange(EmptyString(),
                                       int32_t(event.GetSelectionStart()),
                                       int32_t(event.GetSelectionEnd()), -1);
        mIMESelectionChanged = false;
    }
}

nsresult
nsWindow::NotifyIMEOfTextChange(const IMENotification& aIMENotification)
{
    MOZ_ASSERT(aIMENotification.mMessage == NOTIFY_IME_OF_TEXT_CHANGE,
               "NotifyIMEOfTextChange() is called with invaild notification");

    ALOGIME("IME: NotifyIMEOfTextChange: s=%d, oe=%d, ne=%d",
            aIMENotification.mTextChangeData.mStartOffset,
            aIMENotification.mTextChangeData.mOldEndOffset,
            aIMENotification.mTextChangeData.mNewEndOffset);

    
    mIMESelectionChanged = false;
    NotifyIME(NOTIFY_IME_OF_SELECTION_CHANGE);

    PostFlushIMEChanges();
    AddIMETextChange(IMEChange(aIMENotification));
    return NS_OK;
}

void
nsWindow::AddIMETextChange(const IMEChange& aChange) {

    mIMETextChanges.AppendElement(aChange);

    
    
    
    
    const int32_t delta = aChange.mNewEnd - aChange.mOldEnd;
    for (int32_t i = mIMETextChanges.Length() - 2; i >= 0; i--) {
        IMEChange &previousChange = mIMETextChanges[i];
        if (previousChange.mStart > aChange.mOldEnd) {
            previousChange.mStart += delta;
            previousChange.mOldEnd += delta;
            previousChange.mNewEnd += delta;
        }
    }

    
    
    
    int32_t srcIndex = mIMETextChanges.Length() - 1;
    int32_t dstIndex = srcIndex;

    while (--dstIndex >= 0) {
        IMEChange &src = mIMETextChanges[srcIndex];
        IMEChange &dst = mIMETextChanges[dstIndex];
        
        
        
        if (src.mOldEnd < dst.mStart || dst.mNewEnd < src.mStart) {
            
            continue;
        }
        
        
        
        
        
        
        
        
        
        
        dst.mStart = std::min(dst.mStart, src.mStart);
        if (src.mOldEnd < dst.mNewEnd) {
            
            dst.mNewEnd += src.mNewEnd - src.mOldEnd;
        } else { 
            
            dst.mOldEnd += src.mOldEnd - dst.mNewEnd;
            dst.mNewEnd = src.mNewEnd;
        }
        
        mIMETextChanges.RemoveElementAt(srcIndex);
        
        
        srcIndex = dstIndex;
    }
}

nsIMEUpdatePreference
nsWindow::GetIMEUpdatePreference()
{
    return nsIMEUpdatePreference(
        nsIMEUpdatePreference::NOTIFY_SELECTION_CHANGE |
        nsIMEUpdatePreference::NOTIFY_TEXT_CHANGE);
}

void
nsWindow::DrawWindowUnderlay(LayerManagerComposite* aManager, nsIntRect aRect)
{
    GeckoLayerClient::LocalRef client = AndroidBridge::Bridge()->GetLayerClient();
    if (!client) {
        ALOG_BRIDGE("Exceptional Exit: %s", __PRETTY_FUNCTION__);
        return;
    }

    AutoLocalJNIFrame jniFrame(client.Env());
    auto frameObj = client->CreateFrame();
    if (!frameObj) {
        NS_WARNING("Warning: unable to obtain a LayerRenderer frame; aborting window underlay draw");
        return;
    }

    mLayerRendererFrame.Init(client.Env(), frameObj.Get());
    if (!WidgetPaintsBackground()) {
        return;
    }

    CompositorOGL *compositor = static_cast<CompositorOGL*>(aManager->GetCompositor());
    compositor->ResetProgram();
    gl::GLContext* gl = compositor->gl();
    bool scissorEnabled = gl->fIsEnabled(LOCAL_GL_SCISSOR_TEST);
    GLint scissorRect[4];
    gl->fGetIntegerv(LOCAL_GL_SCISSOR_BOX, scissorRect);

    client->ActivateProgram();
    if (!mLayerRendererFrame.BeginDrawing(&jniFrame)) return;
    if (!mLayerRendererFrame.DrawBackground(&jniFrame)) return;
    client->DeactivateProgramAndRestoreState(scissorEnabled,
        scissorRect[0], scissorRect[1], scissorRect[2], scissorRect[3]);
}

void
nsWindow::DrawWindowOverlay(LayerManagerComposite* aManager, nsIntRect aRect)
{
    PROFILER_LABEL("nsWindow", "DrawWindowOverlay",
        js::ProfileEntry::Category::GRAPHICS);

    if (mLayerRendererFrame.isNull()) {
        NS_WARNING("Warning: do not have a LayerRenderer frame; aborting window overlay draw");
        return;
    }

    GeckoLayerClient::LocalRef client = AndroidBridge::Bridge()->GetLayerClient();

    AutoLocalJNIFrame jniFrame(client.Env());
    CompositorOGL *compositor = static_cast<CompositorOGL*>(aManager->GetCompositor());
    compositor->ResetProgram();
    gl::GLContext* gl = compositor->gl();
    bool scissorEnabled = gl->fIsEnabled(LOCAL_GL_SCISSOR_TEST);
    GLint scissorRect[4];
    gl->fGetIntegerv(LOCAL_GL_SCISSOR_BOX, scissorRect);

    client->ActivateProgram();
    if (!mLayerRendererFrame.DrawForeground(&jniFrame)) return;
    if (!mLayerRendererFrame.EndDrawing(&jniFrame)) return;
    client->DeactivateProgramAndRestoreState(scissorEnabled,
        scissorRect[0], scissorRect[1], scissorRect[2], scissorRect[3]);
    mLayerRendererFrame.Dispose(client.Env());
}



StaticRefPtr<mozilla::layers::APZCTreeManager> nsWindow::sApzcTreeManager;
StaticRefPtr<mozilla::layers::LayerManager> nsWindow::sLayerManager;
StaticRefPtr<mozilla::layers::CompositorParent> nsWindow::sCompositorParent;
StaticRefPtr<mozilla::layers::CompositorChild> nsWindow::sCompositorChild;
bool nsWindow::sCompositorPaused = true;

void
nsWindow::SetCompositor(mozilla::layers::LayerManager* aLayerManager,
                        mozilla::layers::CompositorParent* aCompositorParent,
                        mozilla::layers::CompositorChild* aCompositorChild)
{
    sLayerManager = aLayerManager;
    sCompositorParent = aCompositorParent;
    sCompositorChild = aCompositorChild;
}

void
nsWindow::ScheduleComposite()
{
    if (sCompositorParent) {
        sCompositorParent->ScheduleRenderOnCompositorThread();
    }
}

bool
nsWindow::IsCompositionPaused()
{
    return sCompositorPaused;
}

void
nsWindow::SchedulePauseComposition()
{
    if (sCompositorParent) {
        sCompositorParent->SchedulePauseOnCompositorThread();
        sCompositorPaused = true;
    }
}

void
nsWindow::ScheduleResumeComposition()
{
    if (sCompositorParent && sCompositorParent->ScheduleResumeOnCompositorThread()) {
        sCompositorPaused = false;
    }
}

void
nsWindow::ScheduleResumeComposition(int width, int height)
{
    if (sCompositorParent && sCompositorParent->ScheduleResumeOnCompositorThread(width, height)) {
        sCompositorPaused = false;
    }
}

void
nsWindow::ForceIsFirstPaint()
{
    if (sCompositorParent) {
        sCompositorParent->ForceIsFirstPaint();
    }
}

float
nsWindow::ComputeRenderIntegrity()
{
    if (sCompositorParent) {
        return sCompositorParent->ComputeRenderIntegrity();
    }

    return 1.f;
}

bool
nsWindow::WidgetPaintsBackground()
{
    static bool sWidgetPaintsBackground = true;
    static bool sWidgetPaintsBackgroundPrefCached = false;

    if (!sWidgetPaintsBackgroundPrefCached) {
        sWidgetPaintsBackgroundPrefCached = true;
        mozilla::Preferences::AddBoolVarCache(&sWidgetPaintsBackground,
                                              "android.widget_paints_background",
                                              true);
    }

    return sWidgetPaintsBackground;
}

bool
nsWindow::NeedsPaint()
{
  if (sCompositorPaused || FindTopLevel() != nsWindow::TopWindow() || !GetLayerManager(nullptr)) {
    return false;
  }
  return nsIWidget::NeedsPaint();
}

CompositorParent*
nsWindow::NewCompositorParent(int aSurfaceWidth, int aSurfaceHeight)
{
    return new CompositorParent(this, true, aSurfaceWidth, aSurfaceHeight);
}

mozilla::layers::APZCTreeManager*
nsWindow::GetAPZCTreeManager()
{
    return sApzcTreeManager;
}

void
nsWindow::ConfigureAPZCTreeManager()
{
    nsBaseWidget::ConfigureAPZCTreeManager();
    if (!sApzcTreeManager) {
        sApzcTreeManager = mAPZC;
    }
}

void
nsWindow::ConfigureAPZControllerThread()
{
    APZThreadUtils::SetControllerThread(nullptr);
}

already_AddRefed<GeckoContentController>
nsWindow::CreateRootContentController()
{
    widget::android::APZCCallbackHandler::Initialize(this, mAPZEventState);

    nsRefPtr<widget::android::APZCCallbackHandler> controller =
        widget::android::APZCCallbackHandler::GetInstance();
    return controller.forget();
}

uint64_t
nsWindow::RootLayerTreeId()
{
    MOZ_ASSERT(sCompositorParent);
    return sCompositorParent->RootLayerTreeId();
}
