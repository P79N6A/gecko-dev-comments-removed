





































#include "NPPInstanceChild.h"

#if defined(OS_LINUX)

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdk.h>
#include "gtk2xtbin.h"

#elif defined(OS_WIN)

#include <windows.h>

#endif

namespace mozilla {
namespace plugins {


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

NPPInstanceChild::~NPPInstanceChild()
{
#if defined(OS_WIN)
  DestroyPluginWindow();
#endif
}

NPError
NPPInstanceChild::NPN_GetValue(NPNVariable aVar, void* aValue)
{
    printf ("[NPPInstanceChild] NPN_GetValue(%s)\n",
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
    default:
        printf("  unhandled var %s\n", NPNVariableToString(aVar));
        return NPERR_GENERIC_ERROR;   
    }

}

nsresult
NPPInstanceChild::AnswerNPP_GetValue(const String& key, String* value)
{
    return NPERR_GENERIC_ERROR;
}

nsresult
NPPInstanceChild::AnswerNPP_SetWindow(const NPWindow& aWindow, NPError* rv)
{
    printf("[NPPInstanceChild] NPP_SetWindow(%lx, %d, %d)\n",
           reinterpret_cast<unsigned long>(aWindow.window),
           aWindow.width, aWindow.height);

#if defined(OS_LINUX)
    
    
    

    GdkNativeWindow handle = (GdkNativeWindow) aWindow.window;
    GdkWindow* gdkWindow = gdk_window_lookup(handle);

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

    return NS_OK;
}

bool
NPPInstanceChild::Initialize()
{
#if defined(OS_WIN)
  if (!CreatePluginWindow())
      return false;
#endif

  return true;
}

#if defined(OS_WIN)

static const TCHAR kWindowClassName[] = TEXT("GeckoPluginWindow");
static const TCHAR kNPPInstanceChildProperty[] = TEXT("NPPInstanceChildProperty");


bool
NPPInstanceChild::RegisterWindowClass()
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
NPPInstanceChild::CreatePluginWindow()
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
        if (!SetProp(mPluginWindowHWND, kNPPInstanceChildProperty, this))
            return false;

        
        SetWindowLongPtrA(mPluginWindowHWND, GWLP_WNDPROC,
                          reinterpret_cast<LONG>(DefWindowProcA));
    }

    return true;
}

void
NPPInstanceChild::DestroyPluginWindow()
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

        RemoveProp(mPluginWindowHWND, kNPPInstanceChildProperty);
        DestroyWindow(mPluginWindowHWND);
        mPluginWindowHWND = 0;
    }
}

void
NPPInstanceChild::ReparentPluginWindow(HWND hWndParent)
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
NPPInstanceChild::SizePluginWindow(int width,
                                   int height)
{
    if (mPluginWindowHWND) {
        SetWindowPos(mPluginWindowHWND, NULL, 0, 0, width, height,
                     SWP_NOZORDER | SWP_NOREPOSITION);
    }
}



LRESULT CALLBACK
NPPInstanceChild::DummyWindowProc(HWND hWnd,
                                  UINT message,
                                  WPARAM wParam,
                                  LPARAM lParam)
{
    return CallWindowProc(DefWindowProc, hWnd, message, wParam, lParam);
}


LRESULT CALLBACK
NPPInstanceChild::PluginWindowProc(HWND hWnd,
                                   UINT message,
                                   WPARAM wParam,
                                   LPARAM lParam)
{
    NPPInstanceChild* self = reinterpret_cast<NPPInstanceChild*>(
        GetProp(hWnd, kNPPInstanceChildProperty));
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
        RemoveProp(hWnd, kNPPInstanceChildProperty);

    return res;
}

#endif 

} 
} 
