





































#include "PluginInstanceChild.h"
#include "PluginModuleChild.h"
#include "BrowserStreamChild.h"
#include "PluginStreamChild.h"
#include "StreamNotifyChild.h"

using namespace mozilla::plugins;

#if defined(OS_LINUX)

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdk.h>
#include "gtk2xtbin.h"

#elif defined(OS_WIN)

#include <windows.h>

#endif

namespace {

static const char*
NPNVariableToString(NPNVariable aVar)
{
#define VARSTR(v_)  case v_: return #v_

    switch(aVar) {
        VARSTR(NPNVxDisplay);
        VARSTR(NPNVxtAppContext);
        VARSTR(NPNVnetscapeWindow);
        VARSTR(NPNVjavascriptEnabledBool);
        VARSTR(NPNVasdEnabledBool);
        VARSTR(NPNVisOfflineBool);

        VARSTR(NPNVserviceManager);
        VARSTR(NPNVDOMElement);
        VARSTR(NPNVDOMWindow);
        VARSTR(NPNVToolkit);
        VARSTR(NPNVSupportsXEmbedBool);

        VARSTR(NPNVWindowNPObject);

        VARSTR(NPNVPluginElementNPObject);

        VARSTR(NPNVSupportsWindowless);

        VARSTR(NPNVprivateModeBool);

    default: return "???";
    }
#undef VARSTR
}

} 

PluginInstanceChild::~PluginInstanceChild()
{
#if defined(OS_WIN)
  DestroyPluginWindow();
#endif
}

NPError
PluginInstanceChild::NPN_GetValue(NPNVariable aVar,
                                  void* aValue)
{
    printf ("[PluginInstanceChild] NPN_GetValue(%s)\n",
            NPNVariableToString(aVar));

    switch(aVar) {

    case NPNVSupportsWindowless:
        
        
        *((NPBool*)aValue) = false;
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

    default:
        printf("  unhandled var %s\n", NPNVariableToString(aVar));
        return NPERR_GENERIC_ERROR;   
    }

}

bool
PluginInstanceChild::AnswerNPP_GetValue_NPPVpluginScriptableNPObject(
                                           PPluginScriptableObjectChild** value,
                                           NPError* result)
{

    NPObject* object;
    *result = mPluginIface->getvalue(GetNPP(), NPPVpluginScriptableNPObject,
                                     &object);
    if (*result != NPERR_NO_ERROR) {
        return true;
    }

    PluginScriptableObjectChild* actor = CreateActorForNPObject(object);
    if (!actor) {
        PluginModuleChild::sBrowserFuncs.releaseobject(object);
        *result = NPERR_GENERIC_ERROR;
        return true;
    }

    PluginModuleChild::sBrowserFuncs.releaseobject(object);
    *value = actor;
    return true;
}

bool
PluginInstanceChild::AnswerNPP_HandleEvent(const NPEvent& event,
                                           int16_t* handled)
{
    
    NPEvent evcopy = event;
    *handled = mPluginIface->event(&mData, reinterpret_cast<void*>(&evcopy));
    return true;
}

bool
PluginInstanceChild::AnswerNPP_SetWindow(const NPWindow& aWindow,
                                         NPError* rv)
{
    printf("[PluginInstanceChild] NPP_SetWindow(%lx, %d, %d)\n",
           reinterpret_cast<unsigned long>(aWindow.window),
           aWindow.width, aWindow.height);

#if defined(OS_LINUX)
    
    
    

    GdkNativeWindow handle = reinterpret_cast<uintptr_t>(aWindow.window);

    mWindow.window = (void*) handle;
    mWindow.width = aWindow.width;
    mWindow.height = aWindow.height;
    mWindow.type = NPWindowTypeWindow;

    mWsInfo.display = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());

    
    
    
    
    
#if 0
    mWsInfo.display = GDK_WINDOW_XDISPLAY(gdkWindow);
    mWsInfo.colormap =
        GDK_COLORMAP_XCOLORMAP(gdk_drawable_get_colormap(gdkWindow));
    GdkVisual* gdkVisual = gdk_drawable_get_visual(gdkWindow);
    mWsInfo.visual = GDK_VISUAL_XVISUAL(gdkVisual);
    mWsInfo.depth = gdkVisual->depth;
#endif

    mWindow.ws_info = (void*) &mWsInfo;

    *rv = mPluginIface->setwindow(&mData, &mWindow);

#elif defined(OS_WIN)
    ReparentPluginWindow((HWND)aWindow.window);
    SizePluginWindow(aWindow.width, aWindow.height);

    mWindow.window = (void*)mPluginWindowHWND;
    mWindow.width = aWindow.width;
    mWindow.height = aWindow.height;
    mWindow.type = NPWindowTypeWindow;

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

#else
#  error Implement me for your OS
#endif

    return true;
}

bool
PluginInstanceChild::Initialize()
{
#if defined(OS_WIN)
  if (!CreatePluginWindow())
      return false;
#endif

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
    if (!RegisterWindowClass())
        return false;

    if (!mPluginWindowHWND) {
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
    }

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
    PluginInstanceChild* self = reinterpret_cast<PluginInstanceChild*>(
        GetProp(hWnd, kPluginInstanceChildProperty));
    if (!self) {
        NS_NOTREACHED("Badness!");
        return 0;
    }

    NS_ASSERTION(self->mPluginWindowHWND == hWnd, "Wrong window!");

    LRESULT res = CallWindowProc(self->mPluginWndProc, hWnd, message, wParam,
                                 lParam);

    if (message == WM_CLOSE)
        self->DestroyPluginWindow();

    if (message == WM_NCDESTROY)
        RemoveProp(hWnd, kPluginInstanceChildProperty);

    return res;
}

#endif 

PPluginScriptableObjectChild*
PluginInstanceChild::PPluginScriptableObjectConstructor()
{
    nsAutoPtr<PluginScriptableObjectChild>* object =
        mScriptableObjects.AppendElement();
    NS_ENSURE_TRUE(object, nsnull);

    *object = new PluginScriptableObjectChild();
    NS_ENSURE_TRUE(*object, nsnull);

    return object->get();
}

bool
PluginInstanceChild::PPluginScriptableObjectDestructor(PPluginScriptableObjectChild* aObject)
{
    PluginScriptableObjectChild* object =
        reinterpret_cast<PluginScriptableObjectChild*>(aObject);

    PRUint32 count = mScriptableObjects.Length();
    for (PRUint32 index = 0; index < count; index++) {
        if (mScriptableObjects[index] == object) {
            mScriptableObjects.RemoveElementAt(index);
            break;
        }
    }
    return true;
}

PBrowserStreamChild*
PluginInstanceChild::PBrowserStreamConstructor(const nsCString& url,
                                               const uint32_t& length,
                                               const uint32_t& lastmodified,
                                               const PStreamNotifyChild* notifyData,
                                               const nsCString& headers,
                                               const nsCString& mimeType,
                                               const bool& seekable,
                                               NPError* rv,
                                               uint16_t *stype)
{
    return new BrowserStreamChild(this, url, length, lastmodified, headers,
                                  mimeType, seekable, rv, stype);
}

bool
PluginInstanceChild::AnswerPBrowserStreamDestructor(PBrowserStreamChild* stream,
                                                    const NPError& reason,
                                                    const bool& artificial)
{
    if (!artificial)
        static_cast<BrowserStreamChild*>(stream)->NPP_DestroyStream(reason);
    return true;
}

bool
PluginInstanceChild::PBrowserStreamDestructor(PBrowserStreamChild* stream,
                                              const NPError& reason,
                                              const bool& artificial)
{
    delete stream;
    return true;
}

PPluginStreamChild*
PluginInstanceChild::PPluginStreamConstructor(const nsCString& mimeType,
                                              const nsCString& target,
                                              NPError* result)
{
    NS_RUNTIMEABORT("not callable");
    return NULL;
}

bool
PluginInstanceChild::AnswerPPluginStreamDestructor(PPluginStreamChild* stream,
                                                   const NPReason& reason,
                                                   const bool& artificial)
{
    if (!artificial) {
        static_cast<PluginStreamChild*>(stream)->NPP_DestroyStream(reason);
    }
    return true;
}

bool
PluginInstanceChild::PPluginStreamDestructor(PPluginStreamChild* stream,
                                             const NPError& reason,
                                             const bool& artificial)
{
    delete stream;
    return true;
}

PStreamNotifyChild*
PluginInstanceChild::PStreamNotifyConstructor(const nsCString& url,
                                              const nsCString& target,
                                              const bool& post,
                                              const nsCString& buffer,
                                              const bool& file,
                                              NPError* result)
{
    NS_RUNTIMEABORT("not reached");
    return NULL;
}

bool
PluginInstanceChild::PStreamNotifyDestructor(PStreamNotifyChild* notifyData,
                                             const NPReason& reason)
{
    StreamNotifyChild* sn = static_cast<StreamNotifyChild*>(notifyData);
    mPluginIface->urlnotify(&mData, sn->mURL.get(), reason, sn->mClosure);
    delete sn;

    return true;
}

PluginScriptableObjectChild*
PluginInstanceChild::CreateActorForNPObject(NPObject* aObject)
{
  NS_ASSERTION(aObject, "Null pointer!");

  PluginScriptableObjectChild* actor =
      reinterpret_cast<PluginScriptableObjectChild*>(
          CallPPluginScriptableObjectConstructor());
  NS_ENSURE_TRUE(actor, nsnull);

  actor->Initialize(this, aObject);

#ifdef DEBUG
  bool ok =
#endif
  PluginModuleChild::current()->RegisterNPObject(aObject, actor);
  NS_ASSERTION(ok, "Out of memory?");

  return actor;
}

NPError
PluginInstanceChild::NPN_NewStream(NPMIMEType aMIMEType, const char* aWindow,
                                   NPStream** aStream)
{
    PluginStreamChild* ps = new PluginStreamChild(this);

    NPError result;
    CallPPluginStreamConstructor(ps, nsDependentCString(aMIMEType),
                                 nsDependentCString(aWindow), &result);
    if (NPERR_NO_ERROR != result) {
        *aStream = NULL;
        CallPPluginStreamDestructor(ps, NPERR_GENERIC_ERROR, true);
        return result;
    }

    *aStream = &ps->mStream;
    return NPERR_NO_ERROR;
}
