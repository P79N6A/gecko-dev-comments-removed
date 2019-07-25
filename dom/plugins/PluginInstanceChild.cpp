






































#include "PluginInstanceChild.h"
#include "PluginModuleChild.h"
#include "BrowserStreamChild.h"
#include "PluginStreamChild.h"
#include "StreamNotifyChild.h"

#include "mozilla/ipc/SyncChannel.h"

using namespace mozilla::plugins;

#ifdef MOZ_WIDGET_GTK2

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdk.h>
#include "gtk2xtbin.h"

#elif defined(MOZ_WIDGET_QT)
#include <QX11Info>
#elif defined(OS_WIN)

using mozilla::gfx::SharedDIB;

#include <windows.h>
#include <windowsx.h>

#define NS_OOPP_DOUBLEPASS_MSGID TEXT("MozDoublePassMsg")





#define CHILD_MODALPUMPTIMEOUT 50
#define CHILD_MODALLOOPTIMER   654321

#endif 

PluginInstanceChild::PluginInstanceChild(const NPPluginFuncs* aPluginIface,
                                         const nsCString& aMimeType) :
    mPluginIface(aPluginIface)
    , mCachedWindowActor(nsnull)
    , mCachedElementActor(nsnull)
    , mQuirks(0)
#if defined(OS_WIN)
    , mPluginWindowHWND(0)
    , mPluginWndProc(0)
    , mPluginParentHWND(0)
    , mNestedEventHook(0)
    , mNestedPumpHook(0)
    , mNestedEventLevelDepth(0)
    , mNestedEventState(false)
    , mCachedWinlessPluginHWND(0)
    , mEventPumpTimer(0)
#endif 
{
    memset(&mWindow, 0, sizeof(mWindow));
    mData.ndata = (void*) this;
#if defined(MOZ_X11) && defined(XP_UNIX) && !defined(XP_MACOSX)
    mWindow.ws_info = &mWsInfo;
    memset(&mWsInfo, 0, sizeof(mWsInfo));
#ifdef MOZ_WIDGET_GTK2
    mWsInfo.display = GDK_DISPLAY();
#elif defined(MOZ_WIDGET_QT)
    mWsInfo.display = QX11Info::display();
#endif 
#endif 
#if defined(OS_WIN)
    memset(&mAlphaExtract, 0, sizeof(mAlphaExtract));
    mAlphaExtract.doublePassEvent = ::RegisterWindowMessage(NS_OOPP_DOUBLEPASS_MSGID);
#endif 
    InitQuirksModes(aMimeType);
}

PluginInstanceChild::~PluginInstanceChild()
{
#if defined(OS_WIN)
  DestroyPluginWindow();
#endif
}

void
PluginInstanceChild::InitQuirksModes(const nsCString& aMimeType)
{
#ifdef OS_WIN
    
    
    NS_NAMED_LITERAL_CSTRING(silverlight, "application/x-silverlight");
    if (FindInReadable(silverlight, aMimeType)) {
        mQuirks |= QUIRK_SILVERLIGHT_WINLESS_INPUT_TRANSLATION;
    }
#endif
}

NPError
PluginInstanceChild::InternalGetNPObjectForValue(NPNVariable aValue,
                                                 NPObject** aObject)
{
    PluginScriptableObjectChild* actor;
    NPError result = NPERR_NO_ERROR;

    switch (aValue) {
        case NPNVWindowNPObject:
            if (!(actor = mCachedWindowActor)) {
                PPluginScriptableObjectChild* actorProtocol;
                CallNPN_GetValue_NPNVWindowNPObject(&actorProtocol, &result);
                if (result == NPERR_NO_ERROR) {
                    actor = mCachedWindowActor =
                        static_cast<PluginScriptableObjectChild*>(actorProtocol);
                    NS_ASSERTION(actor, "Null actor!");
                    PluginModuleChild::sBrowserFuncs.retainobject(
                        actor->GetObject(false));
                }
            }
            break;

        case NPNVPluginElementNPObject:
            if (!(actor = mCachedElementActor)) {
                PPluginScriptableObjectChild* actorProtocol;
                CallNPN_GetValue_NPNVPluginElementNPObject(&actorProtocol,
                                                           &result);
                if (result == NPERR_NO_ERROR) {
                    actor = mCachedElementActor =
                        static_cast<PluginScriptableObjectChild*>(actorProtocol);
                    NS_ASSERTION(actor, "Null actor!");
                    PluginModuleChild::sBrowserFuncs.retainobject(
                        actor->GetObject(false));
                }
            }
            break;

        default:
            NS_NOTREACHED("Don't know what to do with this value type!");
    }

#ifdef DEBUG
    {
        NPError currentResult;
        PPluginScriptableObjectChild* currentActor;

        switch (aValue) {
            case NPNVWindowNPObject:
                CallNPN_GetValue_NPNVWindowNPObject(&currentActor,
                                                    &currentResult);
                break;
            case NPNVPluginElementNPObject:
                CallNPN_GetValue_NPNVPluginElementNPObject(&currentActor,
                                                           &currentResult);
                break;
            default:
                NS_NOTREACHED("Don't know what to do with this value type!");
        }

        
        
        NS_ASSERTION(static_cast<PluginScriptableObjectChild*>(currentActor) ==
                     actor, "Cached actor is out of date!");
        NS_ASSERTION(currentResult == result, "Results don't match?!");
    }
#endif

    if (result != NPERR_NO_ERROR) {
        return result;
    }

    NPObject* object = actor->GetObject(false);
    NS_ASSERTION(object, "Null object?!");

    *aObject = PluginModuleChild::sBrowserFuncs.retainobject(object);
    return NPERR_NO_ERROR;

}

NPError
PluginInstanceChild::NPN_GetValue(NPNVariable aVar,
                                  void* aValue)
{
    PLUGIN_LOG_DEBUG(("%s (aVar=%i)", FULLFUNCTION, (int) aVar));
    AssertPluginThread();

    switch(aVar) {

    case NPNVSupportsWindowless:
#if defined(OS_LINUX) || defined(OS_WIN)
        *((NPBool*)aValue) = true;
#else
        *((NPBool*)aValue) = false;
#endif
        return NPERR_NO_ERROR;

#if defined(OS_LINUX)
    case NPNVSupportsXEmbedBool:
        *((NPBool*)aValue) = true;
        return NPERR_NO_ERROR;

    case NPNVToolkit:
        *((NPNToolkitType*)aValue) = NPNVGtk2;
        return NPERR_NO_ERROR;

#elif defined(OS_WIN)
    case NPNVToolkit:
        return NPERR_GENERIC_ERROR;
#endif
    case NPNVjavascriptEnabledBool: {
        bool v = false;
        NPError result;
        if (!CallNPN_GetValue_NPNVjavascriptEnabledBool(&v, &result)) {
            return NPERR_GENERIC_ERROR;
        }
        *static_cast<NPBool*>(aValue) = v;
        return result;
    }

    case NPNVisOfflineBool: {
        bool v = false;
        NPError result;
        if (!CallNPN_GetValue_NPNVisOfflineBool(&v, &result)) {
            return NPERR_GENERIC_ERROR;
        }
        *static_cast<NPBool*>(aValue) = v;
        return result;
    }

    case NPNVprivateModeBool: {
        bool v = false;
        NPError result;
        if (!CallNPN_GetValue_NPNVprivateModeBool(&v, &result)) {
            return NPERR_GENERIC_ERROR;
        }
        *static_cast<NPBool*>(aValue) = v;
        return result;
    }

    case NPNVWindowNPObject: 
    case NPNVPluginElementNPObject: {
        NPObject* object;
        NPError result = InternalGetNPObjectForValue(aVar, &object);
        if (result == NPERR_NO_ERROR) {
            *((NPObject**)aValue) = object;
        }
        return result;
    }

    case NPNVnetscapeWindow: {
#ifdef XP_WIN
        if (mWindow.type == NPWindowTypeDrawable) {
            if (mCachedWinlessPluginHWND) {
              *static_cast<HWND*>(aValue) = mCachedWinlessPluginHWND;
              return NPERR_NO_ERROR;
            }
            NPError result;
            if (!CallNPN_GetValue_NPNVnetscapeWindow(&mCachedWinlessPluginHWND, &result)) {
                return NPERR_GENERIC_ERROR;
            }
            *static_cast<HWND*>(aValue) = mCachedWinlessPluginHWND;
            return result;
        }
        else {
            *static_cast<HWND*>(aValue) = mPluginWindowHWND;
            return NPERR_NO_ERROR;
        }
#elif defined(MOZ_X11)
        NPError result;
        CallNPN_GetValue_NPNVnetscapeWindow(static_cast<XID*>(aValue), &result);
        return result;
#else
        return NPERR_GENERIC_ERROR;
#endif
    }

    default:
        PR_LOG(gPluginLog, PR_LOG_WARNING,
               ("In PluginInstanceChild::NPN_GetValue: Unhandled NPNVariable %i (%s)",
                (int) aVar, NPNVariableToString(aVar)));
        return NPERR_GENERIC_ERROR;
    }

}


NPError
PluginInstanceChild::NPN_SetValue(NPPVariable aVar, void* aValue)
{
    PR_LOG(gPluginLog, PR_LOG_DEBUG, ("%s (aVar=%i, aValue=%p)",
                                      FULLFUNCTION, (int) aVar, aValue));

    AssertPluginThread();

    switch (aVar) {
    case NPPVpluginWindowBool: {
        NPError rv;
        bool windowed = (NPBool) (intptr_t) aValue;

        if (!CallNPN_SetValue_NPPVpluginWindow(windowed, &rv))
            return NPERR_GENERIC_ERROR;

        return rv;
    }

    case NPPVpluginTransparentBool: {
        NPError rv;
        bool transparent = (NPBool) (intptr_t) aValue;

        if (!CallNPN_SetValue_NPPVpluginTransparent(transparent, &rv))
            return NPERR_GENERIC_ERROR;

        return rv;
    }

    default:
        PR_LOG(gPluginLog, PR_LOG_WARNING,
               ("In PluginInstanceChild::NPN_SetValue: Unhandled NPPVariable %i (%s)",
                (int) aVar, NPPVariableToString(aVar)));
        return NPERR_GENERIC_ERROR;
    }
}

bool
PluginInstanceChild::AnswerNPP_GetValue_NPPVpluginNeedsXEmbed(
    bool* needs, NPError* rv)
{
    AssertPluginThread();

#ifdef MOZ_X11
    
    
    
    
    
    
    
    PRBool needsXEmbed = 0;
    if (!mPluginIface->getvalue) {
        *rv = NPERR_GENERIC_ERROR;
    }
    else {
        *rv = mPluginIface->getvalue(GetNPP(), NPPVpluginNeedsXEmbed,
                                     &needsXEmbed);
    }
    *needs = needsXEmbed;
    return true;

#else

    NS_RUNTIMEABORT("shouldn't be called on non-X11 platforms");
    return false;               

#endif
}

bool
PluginInstanceChild::AnswerNPP_GetValue_NPPVpluginScriptableNPObject(
                                          PPluginScriptableObjectChild** aValue,
                                          NPError* aResult)
{
    AssertPluginThread();

    NPObject* object;
    NPError result = NPERR_GENERIC_ERROR;
    if (mPluginIface->getvalue) {
        result = mPluginIface->getvalue(GetNPP(), NPPVpluginScriptableNPObject,
                                        &object);
    }
    if (result == NPERR_NO_ERROR && object) {
        PluginScriptableObjectChild* actor = GetActorForNPObject(object);

        
        
        PluginModuleChild::sBrowserFuncs.releaseobject(object);
        if (actor) {
            *aValue = actor;
            *aResult = NPERR_NO_ERROR;
            return true;
        }

        NS_ERROR("Failed to get actor!");
        result = NPERR_GENERIC_ERROR;
    }

    *aValue = nsnull;
    *aResult = result;
    return true;
}

bool
PluginInstanceChild::AnswerNPP_SetValue_NPNVprivateModeBool(const bool& value,
                                                            NPError* result)
{
    if (!mPluginIface->setvalue) {
        *result = NPERR_GENERIC_ERROR;
        return true;
    }

    
    
    long v = value;
    *result = mPluginIface->setvalue(GetNPP(), NPNVprivateModeBool, &v);
    return true;
}

bool
PluginInstanceChild::AnswerNPP_HandleEvent(const NPRemoteEvent& event,
                                           int16_t* handled)
{
    PLUGIN_LOG_DEBUG_FUNCTION;
    AssertPluginThread();

#if defined(OS_LINUX) && defined(DEBUG)
    if (GraphicsExpose == event.event.type)
        PLUGIN_LOG_DEBUG(("  received drawable 0x%lx\n",
                          event.event.xgraphicsexpose.drawable));
#endif

    
    NPEvent evcopy = event.event;

#ifdef OS_WIN
    
    if (mWindow.type == NPWindowTypeDrawable) {
       if (evcopy.event == WM_PAINT) {
          *handled = SharedSurfacePaint(evcopy);
          return true;
       }
       else if (evcopy.event == mAlphaExtract.doublePassEvent) {
            
            
            
            mAlphaExtract.doublePass = RENDER_BACK_ONE;
            *handled = true;
            return true;
       }
    }
    *handled = WinlessHandleEvent(evcopy);
    return true;
#endif

    *handled = mPluginIface->event(&mData, reinterpret_cast<void*>(&evcopy));

#ifdef MOZ_X11
    if (GraphicsExpose == event.event.type) {
        
        
        
        
        
        
        
        XSync(mWsInfo.display, False);
    }
#endif

    return true;
}

#if defined(MOZ_X11) && defined(XP_UNIX) && !defined(XP_MACOSX)
static bool
XVisualIDToInfo(Display* aDisplay, VisualID aVisualID,
                Visual** aVisual, unsigned int* aDepth)
{
    if (aVisualID == None) {
        *aVisual = NULL;
        *aDepth = 0;
        return true;
    }

    const Screen* screen = DefaultScreenOfDisplay(aDisplay);

    for (int d = 0; d < screen->ndepths; d++) {
        Depth *d_info = &screen->depths[d];
        for (int v = 0; v < d_info->nvisuals; v++) {
            Visual* visual = &d_info->visuals[v];
            if (visual->visualid == aVisualID) {
                *aVisual = visual;
                *aDepth = d_info->depth;
                return true;
            }
        }
    }

    NS_ERROR("VisualID not on Screen.");
    return false;
}
#endif

bool
PluginInstanceChild::AnswerNPP_SetWindow(const NPRemoteWindow& aWindow,
                                         NPError* rv)
{
    PLUGIN_LOG_DEBUG(("%s (aWindow=<window: 0x%lx, x: %d, y: %d, width: %d, height: %d>)",
                      FULLFUNCTION,
                      aWindow.window,
                      aWindow.x, aWindow.y,
                      aWindow.width, aWindow.height));
    AssertPluginThread();

#if defined(MOZ_X11) && defined(XP_UNIX) && !defined(XP_MACOSX)
    
    

    mWindow.window = reinterpret_cast<void*>(aWindow.window);
    mWindow.x = aWindow.x;
    mWindow.y = aWindow.y;
    mWindow.width = aWindow.width;
    mWindow.height = aWindow.height;
    mWindow.clipRect = aWindow.clipRect;
    mWindow.type = aWindow.type;

    mWsInfo.colormap = aWindow.colormap;
    if (!XVisualIDToInfo(mWsInfo.display, aWindow.visualID,
                         &mWsInfo.visual, &mWsInfo.depth))
        return false;

    if (aWindow.type == NPWindowTypeWindow) {
#ifdef MOZ_WIDGET_GTK2
        if (GdkWindow* socket_window = gdk_window_lookup(aWindow.window)) {
            
            
            
            g_object_set_data(G_OBJECT(socket_window),
                              "moz-existed-before-set-window",
                              GUINT_TO_POINTER(1));
        }
#endif
    }

    *rv = mPluginIface->setwindow(&mData, &mWindow);

#elif defined(OS_WIN)
    switch (aWindow.type) {
      case NPWindowTypeWindow:
      {
          if (!CreatePluginWindow())
              return false;

          ReparentPluginWindow((HWND)aWindow.window);
          SizePluginWindow(aWindow.width, aWindow.height);

          mWindow.window = (void*)mPluginWindowHWND;
          mWindow.x = aWindow.x;
          mWindow.y = aWindow.y;
          mWindow.width = aWindow.width;
          mWindow.height = aWindow.height;
          mWindow.type = aWindow.type;

          *rv = mPluginIface->setwindow(&mData, &mWindow);
          if (*rv == NPERR_NO_ERROR) {
              WNDPROC wndProc = reinterpret_cast<WNDPROC>(
                  GetWindowLongPtr(mPluginWindowHWND, GWLP_WNDPROC));
              if (wndProc != PluginWindowProc) {
                  mPluginWndProc = reinterpret_cast<WNDPROC>(
                      SetWindowLongPtr(mPluginWindowHWND, GWLP_WNDPROC,
                                       reinterpret_cast<LONG>(PluginWindowProc)));
              }
          }
      }
      break;

      case NPWindowTypeDrawable:
          return SharedSurfaceSetWindow(aWindow, rv);
      break;

      default:
          NS_NOTREACHED("Bad plugin window type.");
          return false;
      break;
    }

#elif defined(OS_MACOSX)
#  warning This is only a stub implementation IMPLEMENT ME

#else
#  error Implement me for your OS
#endif

    return true;
}

bool
PluginInstanceChild::Initialize()
{
    return true;
}

#if defined(OS_WIN)

static const TCHAR kWindowClassName[] = TEXT("GeckoPluginWindow");
static const TCHAR kPluginInstanceChildProperty[] = TEXT("PluginInstanceChildProperty");


bool
PluginInstanceChild::RegisterWindowClass()
{
    static bool alreadyRegistered = false;
    if (alreadyRegistered)
        return true;

    alreadyRegistered = true;

    WNDCLASSEX wcex;
    wcex.cbSize         = sizeof(WNDCLASSEX);
    wcex.style          = CS_DBLCLKS;
    wcex.lpfnWndProc    = DummyWindowProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = GetModuleHandle(NULL);
    wcex.hIcon          = 0;
    wcex.hCursor        = 0;
    wcex.hbrBackground  = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wcex.lpszMenuName   = 0;
    wcex.lpszClassName  = kWindowClassName;
    wcex.hIconSm        = 0;

    return RegisterClassEx(&wcex) ? true : false;
}

bool
PluginInstanceChild::CreatePluginWindow()
{
    
    if (mPluginWindowHWND)
        return true;
        
    if (!RegisterWindowClass())
        return false;

    mPluginWindowHWND =
        CreateWindowEx(WS_EX_LEFT | WS_EX_LTRREADING |
                       WS_EX_NOPARENTNOTIFY | 
                       WS_EX_RIGHTSCROLLBAR,
                       kWindowClassName, 0,
                       WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, 0,
                       0, 0, NULL, 0, GetModuleHandle(NULL), 0);
    if (!mPluginWindowHWND)
        return false;
    if (!SetProp(mPluginWindowHWND, kPluginInstanceChildProperty, this))
        return false;

    
    SetWindowLongPtrA(mPluginWindowHWND, GWLP_WNDPROC,
                      reinterpret_cast<LONG>(DefWindowProcA));

    return true;
}

void
PluginInstanceChild::DestroyPluginWindow()
{
    if (mPluginWindowHWND) {
        
        WNDPROC wndProc = reinterpret_cast<WNDPROC>(
            GetWindowLongPtr(mPluginWindowHWND, GWLP_WNDPROC));
        if (wndProc == PluginWindowProc) {
            NS_ASSERTION(mPluginWndProc, "Should have old proc here!");
            SetWindowLongPtr(mPluginWindowHWND, GWLP_WNDPROC,
                             reinterpret_cast<LONG>(mPluginWndProc));
            mPluginWndProc = 0;
        }

        RemoveProp(mPluginWindowHWND, kPluginInstanceChildProperty);
        DestroyWindow(mPluginWindowHWND);
        mPluginWindowHWND = 0;
    }
}

void
PluginInstanceChild::ReparentPluginWindow(HWND hWndParent)
{
    if (hWndParent != mPluginParentHWND && IsWindow(hWndParent)) {
        
        LONG style = GetWindowLongPtr(mPluginWindowHWND, GWL_STYLE);
        style |= WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
        style &= ~WS_POPUP;
        SetWindowLongPtr(mPluginWindowHWND, GWL_STYLE, style);

        
        SetParent(mPluginWindowHWND, hWndParent);

        
        ShowWindow(mPluginWindowHWND, SW_SHOWNA);
    }
    mPluginParentHWND = hWndParent;
}

void
PluginInstanceChild::SizePluginWindow(int width,
                                      int height)
{
    if (mPluginWindowHWND) {
        mPluginSize.x = width;
        mPluginSize.y = height;
        SetWindowPos(mPluginWindowHWND, NULL, 0, 0, width, height,
                     SWP_NOZORDER | SWP_NOREPOSITION);
    }
}



LRESULT CALLBACK
PluginInstanceChild::DummyWindowProc(HWND hWnd,
                                     UINT message,
                                     WPARAM wParam,
                                     LPARAM lParam)
{
    return CallWindowProc(DefWindowProc, hWnd, message, wParam, lParam);
}


LRESULT CALLBACK
PluginInstanceChild::PluginWindowProc(HWND hWnd,
                                      UINT message,
                                      WPARAM wParam,
                                      LPARAM lParam)
{
    NS_ASSERTION(!mozilla::ipc::SyncChannel::IsPumpingMessages(),
                 "Failed to prevent a nonqueued message from running!");

    PluginInstanceChild* self = reinterpret_cast<PluginInstanceChild*>(
        GetProp(hWnd, kPluginInstanceChildProperty));
    if (!self) {
        NS_NOTREACHED("Badness!");
        return 0;
    }

    NS_ASSERTION(self->mPluginWindowHWND == hWnd, "Wrong window!");

    
    
    
    
    
    
    if (message == WM_WINDOWPOSCHANGING) {
      WINDOWPOS* pos = reinterpret_cast<WINDOWPOS*>(lParam);
      if (pos && (!(pos->flags & SWP_NOMOVE) || !(pos->flags & SWP_NOSIZE))) {
        pos->x = pos->y = 0;
        pos->cx = self->mPluginSize.x;
        pos->cy = self->mPluginSize.y;
        LRESULT res = CallWindowProc(self->mPluginWndProc, hWnd, message, wParam,
                                     lParam);
        pos->x = pos->y = 0;
        pos->cx = self->mPluginSize.x;
        pos->cy = self->mPluginSize.y;
        return res;
      }
    }

    
    if (message == WM_MOUSEACTIVATE)
        self->CallPluginGotFocus();

    
    
    
    if (message == WM_KILLFOCUS && 
        ((InSendMessageEx(NULL) & (ISMEX_REPLIED|ISMEX_SEND)) == ISMEX_SEND)) {
        ReplyMessage(0); 
    }

    LRESULT res = CallWindowProc(self->mPluginWndProc, hWnd, message, wParam,
                                 lParam);

    if (message == WM_CLOSE)
        self->DestroyPluginWindow();

    if (message == WM_NCDESTROY)
        RemoveProp(hWnd, kPluginInstanceChildProperty);

    return res;
}



VOID CALLBACK
PluginInstanceChild::PumpTimerProc(HWND hwnd,
                                   UINT uMsg,
                                   UINT_PTR idEvent,
                                   DWORD dwTime)
{
    MessageLoop::current()->ScheduleWork();
}

LRESULT CALLBACK
PluginInstanceChild::NestedInputPumpHook(int nCode,
                                         WPARAM wParam,
                                         LPARAM lParam)
{
    if (nCode >= 0) {
        MessageLoop::current()->ScheduleWork();
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}






static PluginInstanceChild* gTempChildPointer;

LRESULT CALLBACK
PluginInstanceChild::NestedInputEventHook(int nCode,
                                          WPARAM wParam,
                                          LPARAM lParam)
{
    if (!gTempChildPointer) {
        return CallNextHookEx(NULL, nCode, wParam, lParam);
    }

    if (nCode >= 0) {
        NS_ASSERTION(gTempChildPointer, "Never should be null here!");
        gTempChildPointer->ResetNestedEventHook();
        gTempChildPointer->SetNestedInputPumpHook();
        gTempChildPointer->InternalCallSetNestedEventState(true);

        gTempChildPointer = NULL;
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

void
PluginInstanceChild::SetNestedInputPumpHook()
{
    NS_ASSERTION(!mNestedPumpHook,
        "mNestedPumpHook already setup in call to SetNestedInputPumpHook?");

    PLUGIN_LOG_DEBUG(("%s", FULLFUNCTION));

    mNestedPumpHook = SetWindowsHookEx(WH_CALLWNDPROC,
                                       NestedInputPumpHook,
                                       NULL,
                                       GetCurrentThreadId());
    mEventPumpTimer = 
        SetTimer(NULL,
                 CHILD_MODALLOOPTIMER,
                 CHILD_MODALPUMPTIMEOUT,
                 PumpTimerProc);
}

void
PluginInstanceChild::ResetPumpHooks()
{
    if (mNestedPumpHook)
        UnhookWindowsHookEx(mNestedPumpHook);
    mNestedPumpHook = NULL;
    if (mEventPumpTimer)
        KillTimer(NULL, mEventPumpTimer);
}

void
PluginInstanceChild::SetNestedInputEventHook()
{
    NS_ASSERTION(!mNestedEventHook,
        "mNestedEventHook already setup in call to SetNestedInputEventHook?");

    PLUGIN_LOG_DEBUG(("%s", FULLFUNCTION));

    
    
    
    
    mNestedEventHook = SetWindowsHookEx(WH_MSGFILTER,
                                        NestedInputEventHook,
                                        NULL,
                                        GetCurrentThreadId());
}

void
PluginInstanceChild::ResetNestedEventHook()
{
    PLUGIN_LOG_DEBUG(("%s", FULLFUNCTION));
    if (mNestedEventHook)
        UnhookWindowsHookEx(mNestedEventHook);
    mNestedEventHook = NULL;
}

void
PluginInstanceChild::InternalCallSetNestedEventState(bool aState)
{
    if (aState != mNestedEventState) {
        PLUGIN_LOG_DEBUG(
            ("PluginInstanceChild::InternalCallSetNestedEventState(%i)",
            (int)aState));
        mNestedEventState = aState;
        SendSetNestedEventState(mNestedEventState);
    }
}



static bool
NeedsNestedEventCoverage(UINT msg)
{
    
    switch (msg) {
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_CONTEXTMENU:
            return true;
    }
    return false;
}

static bool
IsMouseInputEvent(UINT msg)
{
    switch (msg) {
        case WM_MOUSEMOVE:
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
        case WM_MBUTTONDBLCLK:
        case WM_RBUTTONDBLCLK:
            return true;
    }
    return false;
}

int16_t
PluginInstanceChild::WinlessHandleEvent(NPEvent& event)
{
    
    
    
    
    
    
    if (mQuirks & QUIRK_SILVERLIGHT_WINLESS_INPUT_TRANSLATION) {
        if (event.event == WM_WINDOWPOSCHANGED && event.lParam) {
            WINDOWPOS* pos = reinterpret_cast<WINDOWPOS*>(event.lParam);
            mPluginOffset.x = pos->x;
            mPluginOffset.y = pos->y;
        }
        else if (IsMouseInputEvent(event.event)) {
            event.lParam =
                MAKELPARAM((GET_X_LPARAM(event.lParam) - mPluginOffset.x),
                           (GET_Y_LPARAM(event.lParam) - mPluginOffset.y));
        }
    }

    if (!NeedsNestedEventCoverage(event.event)) {
        return mPluginIface->event(&mData, reinterpret_cast<void*>(&event));
    }

    
    
    int16_t handled;

    mNestedEventLevelDepth++;
    PLUGIN_LOG_DEBUG(("WinlessHandleEvent start depth: %i", mNestedEventLevelDepth));

    
    if (mNestedEventLevelDepth == 1) {
        NS_ASSERTION(!gTempChildPointer, "valid gTempChildPointer here?");
        gTempChildPointer = this;
        SetNestedInputEventHook();
    }

    bool old_state = MessageLoop::current()->NestableTasksAllowed();
    MessageLoop::current()->SetNestableTasksAllowed(true);
    handled = mPluginIface->event(&mData, reinterpret_cast<void*>(&event));
    MessageLoop::current()->SetNestableTasksAllowed(old_state);

    gTempChildPointer = NULL;

    mNestedEventLevelDepth--;
    PLUGIN_LOG_DEBUG(("WinlessHandleEvent end depth: %i", mNestedEventLevelDepth));

    NS_ASSERTION(!(mNestedEventLevelDepth < 0), "mNestedEventLevelDepth < 0?");
    if (mNestedEventLevelDepth <= 0) {
        ResetNestedEventHook();
        ResetPumpHooks();
        InternalCallSetNestedEventState(false);
    }
    return handled;
}



bool
PluginInstanceChild::SharedSurfaceSetWindow(const NPRemoteWindow& aWindow,
                                            NPError* rv)
{
    
    
    
    if (!aWindow.surfaceHandle) {
        if (!mSharedSurfaceDib.IsValid()) {
            return false;
        }
    }
    else {
        
        if (NS_FAILED(mSharedSurfaceDib.Attach((SharedDIB::Handle)aWindow.surfaceHandle,
                                               aWindow.width, aWindow.height, 32)))
          return false;
        
        
        AlphaExtractCacheRelease();
    }
      
    
    mWindow.x      = 0;
    mWindow.y      = 0;
    mWindow.width  = aWindow.width;
    mWindow.height = aWindow.height;
    mWindow.type   = aWindow.type;

    mWindow.window = reinterpret_cast<void*>(mSharedSurfaceDib.GetHDC());
    *rv = mPluginIface->setwindow(&mData, &mWindow);

    return true;
}

void
PluginInstanceChild::SharedSurfaceRelease()
{
    mSharedSurfaceDib.Close();
    AlphaExtractCacheRelease();
}



 
bool
PluginInstanceChild::AlphaExtractCacheSetup()
{
    AlphaExtractCacheRelease();

    mAlphaExtract.hdc = ::CreateCompatibleDC(NULL);

    if (!mAlphaExtract.hdc)
        return false;

    BITMAPINFOHEADER bmih;
    memset((void*)&bmih, 0, sizeof(BITMAPINFOHEADER));
    bmih.biSize        = sizeof(BITMAPINFOHEADER);
    bmih.biWidth       = mWindow.width;
    bmih.biHeight      = mWindow.height;
    bmih.biPlanes      = 1;
    bmih.biBitCount    = 32;
    bmih.biCompression = BI_RGB;

    void* ppvBits = nsnull;
    mAlphaExtract.bmp = ::CreateDIBSection(mAlphaExtract.hdc,
                                           (BITMAPINFO*)&bmih,
                                           DIB_RGB_COLORS,
                                           (void**)&ppvBits,
                                           NULL,
                                           (unsigned long)sizeof(BITMAPINFOHEADER));
    if (!mAlphaExtract.bmp)
      return false;

    DeleteObject(::SelectObject(mAlphaExtract.hdc, mAlphaExtract.bmp));
    return true;
}

void
PluginInstanceChild::AlphaExtractCacheRelease()
{
    if (mAlphaExtract.bmp)
        ::DeleteObject(mAlphaExtract.bmp);

    if (mAlphaExtract.hdc)
        ::DeleteObject(mAlphaExtract.hdc);

    mAlphaExtract.bmp = NULL;
    mAlphaExtract.hdc = NULL;
}

void
PluginInstanceChild::UpdatePaintClipRect(RECT* aRect)
{
    if (aRect) {
        
        HRGN clip = ::CreateRectRgnIndirect(aRect);
        ::SelectClipRgn(mSharedSurfaceDib.GetHDC(), clip);
        ::DeleteObject(clip);
    }
}

int16_t
PluginInstanceChild::SharedSurfacePaint(NPEvent& evcopy)
{
    RECT* pRect = reinterpret_cast<RECT*>(evcopy.lParam);

    switch(mAlphaExtract.doublePass) {
        case RENDER_NATIVE:
            
            UpdatePaintClipRect(pRect);
            evcopy.wParam = WPARAM(mSharedSurfaceDib.GetHDC());
            return mPluginIface->event(&mData, reinterpret_cast<void*>(&evcopy));
        break;
        case RENDER_BACK_ONE:
              
              
              
              
              
              if (!mAlphaExtract.bmp && !AlphaExtractCacheSetup()) {
                  mAlphaExtract.doublePass = RENDER_NATIVE;
                  return false;
              }

              
              ::FillRect(mSharedSurfaceDib.GetHDC(), pRect, (HBRUSH)GetStockObject(WHITE_BRUSH));
              UpdatePaintClipRect(pRect);
              evcopy.wParam = WPARAM(mSharedSurfaceDib.GetHDC());
              if (!mPluginIface->event(&mData, reinterpret_cast<void*>(&evcopy))) {
                  mAlphaExtract.doublePass = RENDER_NATIVE;
                  return false;
              }

              
              
              ::BitBlt(mAlphaExtract.hdc,
                       pRect->left,
                       pRect->top,
                       pRect->right - pRect->left,
                       pRect->bottom - pRect->top,
                       mSharedSurfaceDib.GetHDC(),
                       pRect->left,
                       pRect->top,
                       SRCCOPY);

              ::FillRect(mSharedSurfaceDib.GetHDC(), pRect, (HBRUSH)GetStockObject(BLACK_BRUSH));
              if (!mPluginIface->event(&mData, reinterpret_cast<void*>(&evcopy))) {
                  mAlphaExtract.doublePass = RENDER_NATIVE;
                  return false;
              }
              mAlphaExtract.doublePass = RENDER_BACK_TWO;
              return true;
        break;
        case RENDER_BACK_TWO:
              
              ::BitBlt(mSharedSurfaceDib.GetHDC(),
                       pRect->left,
                       pRect->top,
                       pRect->right - pRect->left,
                       pRect->bottom - pRect->top,
                       mAlphaExtract.hdc,
                       pRect->left,
                       pRect->top,
                       SRCCOPY);
              mAlphaExtract.doublePass = RENDER_NATIVE;
              return true;
        break;
    }
    return false;
}

#endif 

bool
PluginInstanceChild::AnswerSetPluginFocus()
{
    PR_LOG(gPluginLog, PR_LOG_DEBUG, ("%s", FULLFUNCTION));

#if defined(OS_WIN)
    
    if (::GetFocus() == mPluginWindowHWND)
        return true;
    ::SetFocus(mPluginWindowHWND);
    return true;
#else
    NS_NOTREACHED("PluginInstanceChild::AnswerSetPluginFocus not implemented!");
    return false;
#endif
}

bool
PluginInstanceChild::AnswerUpdateWindow()
{
    PR_LOG(gPluginLog, PR_LOG_DEBUG, ("%s", FULLFUNCTION));

#if defined(OS_WIN)
    if (mPluginWindowHWND)
      UpdateWindow(mPluginWindowHWND);
    return true;
#else
    NS_NOTREACHED("PluginInstanceChild::AnswerUpdateWindow not implemented!");
    return false;
#endif
}

PPluginScriptableObjectChild*
PluginInstanceChild::AllocPPluginScriptableObject()
{
    AssertPluginThread();
    return new PluginScriptableObjectChild(Proxy);
}

bool
PluginInstanceChild::DeallocPPluginScriptableObject(
    PPluginScriptableObjectChild* aObject)
{
    AssertPluginThread();
    delete aObject;
    return true;
}

bool
PluginInstanceChild::RecvPPluginScriptableObjectConstructor(
                                           PPluginScriptableObjectChild* aActor)
{
    AssertPluginThread();

    
    
    
    PluginScriptableObjectChild* actor =
        static_cast<PluginScriptableObjectChild*>(aActor);
    NS_ASSERTION(!actor->GetObject(false), "Actor already has an object?!");

    actor->InitializeProxy();
    NS_ASSERTION(actor->GetObject(false), "Actor should have an object!");

    return true;
}

bool
PluginInstanceChild::AnswerPBrowserStreamConstructor(
    PBrowserStreamChild* aActor,
    const nsCString& url,
    const uint32_t& length,
    const uint32_t& lastmodified,
    PStreamNotifyChild* notifyData,
    const nsCString& headers,
    const nsCString& mimeType,
    const bool& seekable,
    NPError* rv,
    uint16_t* stype)
{
    AssertPluginThread();
    *rv = static_cast<BrowserStreamChild*>(aActor)
          ->StreamConstructed(url, length, lastmodified,
                              notifyData, headers, mimeType, seekable,
                              stype);
    return true;
}

PBrowserStreamChild*
PluginInstanceChild::AllocPBrowserStream(const nsCString& url,
                                         const uint32_t& length,
                                         const uint32_t& lastmodified,
                                         PStreamNotifyChild* notifyData,
                                         const nsCString& headers,
                                         const nsCString& mimeType,
                                         const bool& seekable,
                                         NPError* rv,
                                         uint16_t *stype)
{
    AssertPluginThread();
    return new BrowserStreamChild(this, url, length, lastmodified, notifyData,
                                  headers, mimeType, seekable, rv, stype);
}

bool
PluginInstanceChild::DeallocPBrowserStream(PBrowserStreamChild* stream)
{
    AssertPluginThread();
    delete stream;
    return true;
}

PPluginStreamChild*
PluginInstanceChild::AllocPPluginStream(const nsCString& mimeType,
                                        const nsCString& target,
                                        NPError* result)
{
    NS_RUNTIMEABORT("not callable");
    return NULL;
}

bool
PluginInstanceChild::DeallocPPluginStream(PPluginStreamChild* stream)
{
    AssertPluginThread();
    delete stream;
    return true;
}

PStreamNotifyChild*
PluginInstanceChild::AllocPStreamNotify(const nsCString& url,
                                        const nsCString& target,
                                        const bool& post,
                                        const nsCString& buffer,
                                        const bool& file,
                                        NPError* result)
{
    AssertPluginThread();
    NS_RUNTIMEABORT("not reached");
    return NULL;
}

bool
StreamNotifyChild::Answer__delete__(const NPReason& reason)
{
    AssertPluginThread();
    return static_cast<PluginInstanceChild*>(Manager())
        ->NotifyStream(this, reason);
}

bool
PluginInstanceChild::NotifyStream(StreamNotifyChild* notifyData,
                                  NPReason reason)
{
    if (notifyData->mClosure)
        mPluginIface->urlnotify(&mData, notifyData->mURL.get(), reason,
                                notifyData->mClosure);
    return true;
}

bool
PluginInstanceChild::DeallocPStreamNotify(PStreamNotifyChild* notifyData)
{
    AssertPluginThread();
    delete notifyData;
    return true;
}

PluginScriptableObjectChild*
PluginInstanceChild::GetActorForNPObject(NPObject* aObject)
{
    AssertPluginThread();
    NS_ASSERTION(aObject, "Null pointer!");

    if (aObject->_class == PluginScriptableObjectChild::GetClass()) {
        
        ChildNPObject* object = static_cast<ChildNPObject*>(aObject);
        NS_ASSERTION(object->parent, "Null actor!");
        return object->parent;
    }

    PluginScriptableObjectChild* actor =
        PluginModuleChild::current()->GetActorForNPObject(aObject);
    if (actor) {
        
        return actor;
    }

    actor = new PluginScriptableObjectChild(LocalObject);
    if (!SendPPluginScriptableObjectConstructor(actor)) {
        NS_ERROR("Failed to send constructor message!");
        return nsnull;
    }

    actor->InitializeLocal(aObject);
    return actor;
}

NPError
PluginInstanceChild::NPN_NewStream(NPMIMEType aMIMEType, const char* aWindow,
                                   NPStream** aStream)
{
    AssertPluginThread();

    PluginStreamChild* ps = new PluginStreamChild();

    NPError result;
    CallPPluginStreamConstructor(ps, nsDependentCString(aMIMEType),
                                 NullableString(aWindow), &result);
    if (NPERR_NO_ERROR != result) {
        *aStream = NULL;
        PPluginStreamChild::Call__delete__(ps, NPERR_GENERIC_ERROR, true);
        return result;
    }

    *aStream = &ps->mStream;
    return NPERR_NO_ERROR;
}

void
PluginInstanceChild::InvalidateRect(NPRect* aInvalidRect)
{
    NS_ASSERTION(aInvalidRect, "Null pointer!");

#ifdef OS_WIN
    
    if (mWindow.type == NPWindowTypeWindow) {
      NS_ASSERTION(IsWindow(mPluginWindowHWND), "Bad window?!");
      RECT rect = { aInvalidRect->left, aInvalidRect->top,
                    aInvalidRect->right, aInvalidRect->bottom };
      ::InvalidateRect(mPluginWindowHWND, &rect, FALSE);
      return;
    }
#endif

    SendNPN_InvalidateRect(*aInvalidRect);
}

uint32_t
PluginInstanceChild::ScheduleTimer(uint32_t interval, bool repeat,
                                   TimerFunc func)
{
    ChildTimer* t = new ChildTimer(this, interval, repeat, func);
    if (0 == t->ID()) {
        delete t;
        return 0;
    }

    mTimers.AppendElement(t);
    return t->ID();
}

void
PluginInstanceChild::UnscheduleTimer(uint32_t id)
{
    if (0 == id)
        return;

    mTimers.RemoveElement(id, ChildTimer::IDComparator());
}

static PLDHashOperator
InvalidateObject(DeletingObjectEntry* e, void* userArg)
{
    NPObject* o = e->GetKey();
    if (!e->mDeleted && o->_class && o->_class->invalidate)
        o->_class->invalidate(o);

    return PL_DHASH_NEXT;
}

static PLDHashOperator
DeleteObject(DeletingObjectEntry* e, void* userArg)
{
    NPObject* o = e->GetKey();
    if (!e->mDeleted) {
        e->mDeleted = true;

#ifdef NS_BUILD_REFCNT_LOGGING
        {
            int32_t refcnt = o->referenceCount;
            while (refcnt) {
                --refcnt;
                NS_LOG_RELEASE(o, refcnt, "NPObject");
            }
        }
#endif

        PluginModuleChild::DeallocNPObject(o);
    }

    return PL_DHASH_NEXT;
}

bool
PluginInstanceChild::AnswerNPP_Destroy(NPError* aResult)
{
    PLUGIN_LOG_DEBUG_METHOD;
    AssertPluginThread();

    for (PRUint32 i = 0; i < mPendingAsyncCalls.Length(); ++i)
        mPendingAsyncCalls[i]->Cancel();
    mPendingAsyncCalls.TruncateLength(0);

    mTimers.Clear();

    PluginModuleChild::current()->NPP_Destroy(this);
    mData.ndata = 0;

    mDeletingHash = new nsTHashtable<DeletingObjectEntry>;
    mDeletingHash->Init();
    PluginModuleChild::current()->FindNPObjectsForInstance(this);

    mDeletingHash->EnumerateEntries(InvalidateObject, NULL);
    mDeletingHash->EnumerateEntries(DeleteObject, NULL);

    
    
    mCachedWindowActor = nsnull;
    mCachedElementActor = nsnull;

#if defined(OS_WIN)
    SharedSurfaceRelease();
    ResetNestedEventHook();
    ResetPumpHooks();
#endif

    return true;
}
