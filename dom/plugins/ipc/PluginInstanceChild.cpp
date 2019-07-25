






































#include "PluginBackgroundDestroyer.h"
#include "PluginInstanceChild.h"
#include "PluginModuleChild.h"
#include "BrowserStreamChild.h"
#include "PluginStreamChild.h"
#include "StreamNotifyChild.h"
#include "PluginProcessChild.h"
#include "gfxASurface.h"
#include "gfxContext.h"
#ifdef MOZ_X11
#include "gfxXlibSurface.h"
#endif
#ifdef XP_WIN
#include "mozilla/gfx/SharedDIBSurface.h"
#include "nsCrashOnException.h"
extern const PRUnichar* kFlashFullscreenClass;
using mozilla::gfx::SharedDIBSurface;
#endif
#include "gfxSharedImageSurface.h"
#include "gfxUtils.h"
#include "gfxAlphaRecovery.h"

#include "mozilla/ipc/SyncChannel.h"

using mozilla::ipc::ProcessChild;
using namespace mozilla::plugins;

#ifdef MOZ_WIDGET_GTK2

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdk.h>
#include "gtk2xtbin.h"

#elif defined(MOZ_WIDGET_QT)
#include <QX11Info>
#elif defined(OS_WIN)
#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL     0x020E
#endif

#include "nsWindowsDllInterceptor.h"

typedef BOOL (WINAPI *User32TrackPopupMenu)(HMENU hMenu,
                                            UINT uFlags,
                                            int x,
                                            int y,
                                            int nReserved,
                                            HWND hWnd,
                                            CONST RECT *prcRect);
static WindowsDllInterceptor sUser32Intercept;
static HWND sWinlessPopupSurrogateHWND = NULL;
static User32TrackPopupMenu sUser32TrackPopupMenuStub = NULL;

using mozilla::gfx::SharedDIB;

#include <windows.h>
#include <windowsx.h>




const int kFlashWMUSERMessageThrottleDelayMs = 5;

static const TCHAR kPluginIgnoreSubclassProperty[] = TEXT("PluginIgnoreSubclassProperty");

#elif defined(XP_MACOSX)
#include <ApplicationServices/ApplicationServices.h>
#endif 

template<>
struct RunnableMethodTraits<PluginInstanceChild>
{
    static void RetainCallee(PluginInstanceChild* obj) { }
    static void ReleaseCallee(PluginInstanceChild* obj) { }
};

PluginInstanceChild::PluginInstanceChild(const NPPluginFuncs* aPluginIface)
    : mPluginIface(aPluginIface)
    , mCachedWindowActor(nsnull)
    , mCachedElementActor(nsnull)
#if defined(OS_WIN)
    , mPluginWindowHWND(0)
    , mPluginWndProc(0)
    , mPluginParentHWND(0)
    , mCachedWinlessPluginHWND(0)
    , mWinlessPopupSurrogateHWND(0)
    , mWinlessThrottleOldWndProc(0)
    , mWinlessHiddenMsgHWND(0)
#endif 
    , mAsyncCallMutex("PluginInstanceChild::mAsyncCallMutex")
#if defined(OS_MACOSX)
#if defined(__i386__)
    , mEventModel(NPEventModelCarbon)
#endif
    , mShColorSpace(nsnull)
    , mShContext(nsnull)
    , mDrawingModel(NPDrawingModelCoreGraphics)
    , mCurrentEvent(nsnull)
#endif
    , mLayersRendering(false)
#ifdef XP_WIN
    , mCurrentSurfaceActor(NULL)
    , mBackSurfaceActor(NULL)
#endif
    , mAccumulatedInvalidRect(0,0,0,0)
    , mIsTransparent(false)
    , mSurfaceType(gfxASurface::SurfaceTypeMax)
    , mCurrentInvalidateTask(nsnull)
    , mCurrentAsyncSetWindowTask(nsnull)
    , mPendingPluginCall(false)
    , mDoAlphaExtraction(false)
    , mHasPainted(false)
    , mSurfaceDifferenceRect(0,0,0,0)
#if (MOZ_PLATFORM_MAEMO == 5) || (MOZ_PLATFORM_MAEMO == 6)
    , mMaemoImageRendering(PR_FALSE)
#endif
{
    memset(&mWindow, 0, sizeof(mWindow));
    mData.ndata = (void*) this;
    mData.pdata = nsnull;
#if defined(MOZ_X11) && defined(XP_UNIX) && !defined(XP_MACOSX)
    mWindow.ws_info = &mWsInfo;
    memset(&mWsInfo, 0, sizeof(mWsInfo));
    mWsInfo.display = DefaultXDisplay();
#endif 
#if defined(OS_WIN)
    memset(&mAlphaExtract, 0, sizeof(mAlphaExtract));
#endif 
#if defined(OS_WIN)
    InitPopupMenuHook();
#endif 
}

PluginInstanceChild::~PluginInstanceChild()
{
#if defined(OS_WIN)
    NS_ASSERTION(!mPluginWindowHWND, "Destroying PluginInstanceChild without NPP_Destroy?");
#endif
#if defined(OS_MACOSX)
    if (mShColorSpace) {
        ::CGColorSpaceRelease(mShColorSpace);
    }
    if (mShContext) {
        ::CGContextRelease(mShContext);
    }
#endif
}

int
PluginInstanceChild::GetQuirks()
{
    return PluginModuleChild::current()->GetQuirks();
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

#if (MOZ_PLATFORM_MAEMO == 5) || (MOZ_PLATFORM_MAEMO == 6)
    case NPNVSupportsWindowlessLocal: {
#ifdef MOZ_WIDGET_QT
        const char *graphicsSystem = PR_GetEnv("MOZ_QT_GRAPHICSSYSTEM");
        
        
        mMaemoImageRendering = (!(graphicsSystem && !strcmp(graphicsSystem, "native")));
#endif
        *((NPBool*)aValue) = mMaemoImageRendering;
        return NPERR_NO_ERROR;
    }
#endif
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

#ifdef XP_MACOSX
   case NPNVsupportsCoreGraphicsBool: {
        *((NPBool*)aValue) = true;
        return NPERR_NO_ERROR;
    }

    case NPNVsupportsCoreAnimationBool: {
        *((NPBool*)aValue) = true;
        return NPERR_NO_ERROR;
    }

    case NPNVsupportsInvalidatingCoreAnimationBool: {
        *((NPBool*)aValue) = true;
        return NPERR_NO_ERROR;
    }

    case NPNVsupportsCocoaBool: {
        *((NPBool*)aValue) = true;
        return NPERR_NO_ERROR;
    }

#ifndef NP_NO_CARBON
    case NPNVsupportsCarbonBool: {
      *((NPBool*)aValue) = false;
      return NPERR_NO_ERROR;
    }
#endif

    case NPNVsupportsUpdatedCocoaTextInputBool: {
      *static_cast<NPBool*>(aValue) = true;
      return NPERR_NO_ERROR;
    }

#ifndef NP_NO_QUICKDRAW
    case NPNVsupportsQuickDrawBool: {
        *((NPBool*)aValue) = false;
        return NPERR_NO_ERROR;
    }
#endif 
#endif 

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
        mIsTransparent = (!!aValue);

        if (!CallNPN_SetValue_NPPVpluginTransparent(mIsTransparent, &rv))
            return NPERR_GENERIC_ERROR;

        return rv;
    }

    case NPPVpluginUsesDOMForCursorBool: {
        NPError rv = NPERR_GENERIC_ERROR;
        if (!CallNPN_SetValue_NPPVpluginUsesDOMForCursor((NPBool)(intptr_t)aValue, &rv)) {
            return NPERR_GENERIC_ERROR;
        }
        return rv;
    }

#ifdef XP_MACOSX
    case NPPVpluginDrawingModel: {
        NPError rv;
        int drawingModel = (int16) (intptr_t) aValue;

        if (!CallNPN_SetValue_NPPVpluginDrawingModel(drawingModel, &rv))
            return NPERR_GENERIC_ERROR;
        mDrawingModel = drawingModel;

        return rv;
    }

    case NPPVpluginEventModel: {
        NPError rv;
        int eventModel = (int16) (intptr_t) aValue;

        if (!CallNPN_SetValue_NPPVpluginEventModel(eventModel, &rv))
            return NPERR_GENERIC_ERROR;
#if defined(__i386__)
        mEventModel = static_cast<NPEventModel>(eventModel);
#endif

        return rv;
    }
#endif

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

    NPObject* object = nsnull;
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
    else {
        result = NPERR_GENERIC_ERROR;
    }

    *aValue = nsnull;
    *aResult = result;
    return true;
}

bool
PluginInstanceChild::AnswerNPP_GetValue_NPPVpluginNativeAccessibleAtkPlugId(
                                          nsCString* aPlugId,
                                          NPError* aResult)
{
    AssertPluginThread();

#if MOZ_ACCESSIBILITY_ATK

    char* plugId = NULL;
    NPError result = NPERR_GENERIC_ERROR;
    if (mPluginIface->getvalue) {
        result = mPluginIface->getvalue(GetNPP(),
                                        NPPVpluginNativeAccessibleAtkPlugId,
                                        &plugId);
    }

    *aPlugId = nsCString(plugId);
    *aResult = result;
    return true;

#else

    NS_RUNTIMEABORT("shouldn't be called on non-ATK platforms");
    return false;

#endif
}

bool
PluginInstanceChild::AnswerNPP_SetValue_NPNVprivateModeBool(const bool& value,
                                                            NPError* result)
{
    if (!mPluginIface->setvalue) {
        *result = NPERR_GENERIC_ERROR;
        return true;
    }

    NPBool v = value;
    *result = mPluginIface->setvalue(GetNPP(), NPNVprivateModeBool, &v);
    return true;
}

bool
PluginInstanceChild::AnswerNPP_HandleEvent(const NPRemoteEvent& event,
                                           int16_t* handled)
{
    PLUGIN_LOG_DEBUG_FUNCTION;
    AssertPluginThread();

#if defined(MOZ_X11) && defined(DEBUG)
    if (GraphicsExpose == event.event.type)
        PLUGIN_LOG_DEBUG(("  received drawable 0x%lx\n",
                          event.event.xgraphicsexpose.drawable));
#endif

#ifdef XP_MACOSX
    
    NPCocoaEvent evcopy = event.event;

    
    AutoRestore<const NPCocoaEvent*> savePreviousEvent(mCurrentEvent);

    
    mCurrentEvent = &event.event;
#else
    
    NPEvent evcopy = event.event;
#endif

#ifdef OS_WIN
    
    if (WM_NULL == evcopy.event)
        return true;

    
    if (mWindow.type == NPWindowTypeDrawable) {
       if (evcopy.event == WM_PAINT) {
          *handled = SharedSurfacePaint(evcopy);
          return true;
       }
       else if (DoublePassRenderingEvent() == evcopy.event) {
            
            
            
            mAlphaExtract.doublePass = RENDER_BACK_ONE;
            *handled = true;
            return true;
       }
    }
    *handled = WinlessHandleEvent(evcopy);
    return true;
#endif

    
    
    
    if (!mPluginIface->event)
        *handled = false;
    else
        *handled = mPluginIface->event(&mData, reinterpret_cast<void*>(&evcopy));

#ifdef XP_MACOSX
    
    if (evcopy.type == NPCocoaEventKeyDown ||
        evcopy.type == NPCocoaEventKeyUp) {
      ::CFRelease((CFStringRef)evcopy.data.key.characters);
      ::CFRelease((CFStringRef)evcopy.data.key.charactersIgnoringModifiers);
    }
    else if (evcopy.type == NPCocoaEventTextInput) {
      ::CFRelease((CFStringRef)evcopy.data.text.text);
    }
#endif

#ifdef MOZ_X11
    if (GraphicsExpose == event.event.type) {
        
        
        
        
        
        
        
        XSync(mWsInfo.display, False);
    }
#endif

    return true;
}

#ifdef XP_MACOSX

bool
PluginInstanceChild::AnswerNPP_HandleEvent_Shmem(const NPRemoteEvent& event,
                                                 Shmem& mem,
                                                 int16_t* handled,
                                                 Shmem* rtnmem)
{
    PLUGIN_LOG_DEBUG_FUNCTION;
    AssertPluginThread();

    PaintTracker pt;

    NPCocoaEvent evcopy = event.event;

    if (evcopy.type == NPCocoaEventDrawRect) {
        if (!mShColorSpace) {
            mShColorSpace = CreateSystemColorSpace();
            if (!mShColorSpace) {
                PLUGIN_LOG_DEBUG(("Could not allocate ColorSpace."));
                *handled = false;
                *rtnmem = mem;
                return true;
            } 
        }
        if (!mShContext) {
            void* cgContextByte = mem.get<char>();
            mShContext = ::CGBitmapContextCreate(cgContextByte, 
                              mWindow.width, mWindow.height, 8, 
                              mWindow.width * 4, mShColorSpace, 
                              kCGImageAlphaPremultipliedFirst |
                              kCGBitmapByteOrder32Host);
    
            if (!mShContext) {
                PLUGIN_LOG_DEBUG(("Could not allocate CGBitmapContext."));
                *handled = false;
                *rtnmem = mem;
                return true;
            }
        }
        CGRect clearRect = ::CGRectMake(0, 0, mWindow.width, mWindow.height);
        ::CGContextClearRect(mShContext, clearRect);
        evcopy.data.draw.context = mShContext; 
    } else {
        PLUGIN_LOG_DEBUG(("Invalid event type for AnswerNNP_HandleEvent_Shmem."));
        *handled = false;
        *rtnmem = mem;
        return true;
    } 

    if (!mPluginIface->event) {
        *handled = false;
    } else {
        ::CGContextSaveGState(evcopy.data.draw.context);
        *handled = mPluginIface->event(&mData, reinterpret_cast<void*>(&evcopy));
        ::CGContextRestoreGState(evcopy.data.draw.context);
    }

    *rtnmem = mem;
    return true;
}

#else
bool
PluginInstanceChild::AnswerNPP_HandleEvent_Shmem(const NPRemoteEvent& event,
                                                 Shmem& mem,
                                                 int16_t* handled,
                                                 Shmem* rtnmem)
{
    NS_RUNTIMEABORT("not reached.");
    *rtnmem = mem;
    return true;
}
#endif

#ifdef XP_MACOSX
bool
PluginInstanceChild::AnswerNPP_HandleEvent_IOSurface(const NPRemoteEvent& event,
                                                     const uint32_t &surfaceid,
                                                     int16_t* handled)
{
    PLUGIN_LOG_DEBUG_FUNCTION;
    AssertPluginThread();

    PaintTracker pt;

    NPCocoaEvent evcopy = event.event;
    nsIOSurface* surf = nsIOSurface::LookupSurface(surfaceid);
    if (!surf) {
        NS_ERROR("Invalid IOSurface.");
        *handled = false;
        return false;
    }

    if (evcopy.type == NPCocoaEventDrawRect) {
        mCARenderer.AttachIOSurface(surf);
        if (!mCARenderer.isInit()) {
            void *caLayer = nsnull;
            NPError result = mPluginIface->getvalue(GetNPP(), 
                                     NPPVpluginCoreAnimationLayer,
                                     &caLayer);
            if (result != NPERR_NO_ERROR || !caLayer) {
                PLUGIN_LOG_DEBUG(("Plugin requested CoreAnimation but did not "
                                  "provide CALayer."));
                *handled = false;
                return false;
            }
            mCARenderer.SetupRenderer(caLayer, mWindow.width, mWindow.height);
            
            if (mPluginIface->setwindow)
                (void) mPluginIface->setwindow(&mData, &mWindow);
        }
    } else {
        PLUGIN_LOG_DEBUG(("Invalid event type for "
                          "AnswerNNP_HandleEvent_IOSurface."));
        *handled = false;
        return false;
    } 

    mCARenderer.Render(mWindow.width, mWindow.height, nsnull);

    return true;

}

#else
bool
PluginInstanceChild::AnswerNPP_HandleEvent_IOSurface(const NPRemoteEvent& event,
                                                     const uint32_t &surfaceid,
                                                     int16_t* handled)
{
    NS_RUNTIMEABORT("NPP_HandleEvent_IOSurface is a OSX-only message");
    return false;
}
#endif

bool
PluginInstanceChild::RecvWindowPosChanged(const NPRemoteEvent& event)
{
    NS_ASSERTION(!mLayersRendering && !mPendingPluginCall,
                 "Shouldn't be receiving WindowPosChanged with layer rendering");

#ifdef OS_WIN
    int16_t dontcare;
    return AnswerNPP_HandleEvent(event, &dontcare);
#else
    NS_RUNTIMEABORT("WindowPosChanged is a windows-only message");
    return false;
#endif
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
PluginInstanceChild::AnswerNPP_SetWindow(const NPRemoteWindow& aWindow)
{
    PLUGIN_LOG_DEBUG(("%s (aWindow=<window: 0x%lx, x: %d, y: %d, width: %d, height: %d>)",
                      FULLFUNCTION,
                      aWindow.window,
                      aWindow.x, aWindow.y,
                      aWindow.width, aWindow.height));
    NS_ASSERTION(!mLayersRendering && !mPendingPluginCall,
                 "Shouldn't be receiving NPP_SetWindow with layer rendering");
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

#ifdef MOZ_WIDGET_GTK2
    if (gtk_check_version(2,18,7) != NULL) { 
        if (aWindow.type == NPWindowTypeWindow) {
            GdkWindow* socket_window = gdk_window_lookup(aWindow.window);
            if (socket_window) {
                
                
                
                g_object_set_data(G_OBJECT(socket_window),
                                  "moz-existed-before-set-window",
                                  GUINT_TO_POINTER(1));
            }
        }

        if (aWindow.visualID != None
            && gtk_check_version(2, 12, 10) != NULL) { 
            
            
            
            GdkVisual *gdkvisual = gdkx_visual_get(aWindow.visualID);
            GdkColormap *gdkcolor =
                gdk_x11_colormap_foreign_new(gdkvisual, aWindow.colormap);

            if (g_object_get_data(G_OBJECT(gdkcolor), "moz-have-extra-ref")) {
                
                g_object_unref(gdkcolor);
            } else {
                
                g_object_set_data(G_OBJECT(gdkcolor),
                                  "moz-have-extra-ref", GUINT_TO_POINTER(1));
            }
        }
    }
#endif

    PLUGIN_LOG_DEBUG(
        ("[InstanceChild][%p] Answer_SetWindow w=<x=%d,y=%d, w=%d,h=%d>, clip=<l=%d,t=%d,r=%d,b=%d>",
         this, mWindow.x, mWindow.y, mWindow.width, mWindow.height,
         mWindow.clipRect.left, mWindow.clipRect.top, mWindow.clipRect.right, mWindow.clipRect.bottom));

    if (mPluginIface->setwindow)
        (void) mPluginIface->setwindow(&mData, &mWindow);

#elif defined(OS_WIN)
    switch (aWindow.type) {
      case NPWindowTypeWindow:
      {
          if ((GetQuirks() & PluginModuleChild::QUIRK_QUICKTIME_AVOID_SETWINDOW) &&
              aWindow.width == 0 &&
              aWindow.height == 0) {
            
            return true;
          }

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

          if (mPluginIface->setwindow) {
              SetProp(mPluginWindowHWND, kPluginIgnoreSubclassProperty, (HANDLE)1);
              (void) mPluginIface->setwindow(&mData, &mWindow);
              WNDPROC wndProc = reinterpret_cast<WNDPROC>(
                  GetWindowLongPtr(mPluginWindowHWND, GWLP_WNDPROC));
              if (wndProc != PluginWindowProc) {
                  mPluginWndProc = reinterpret_cast<WNDPROC>(
                      SetWindowLongPtr(mPluginWindowHWND, GWLP_WNDPROC,
                                       reinterpret_cast<LONG_PTR>(PluginWindowProc)));
                  NS_ASSERTION(mPluginWndProc != PluginWindowProc, "WTF?");
              }
              RemoveProp(mPluginWindowHWND, kPluginIgnoreSubclassProperty);
              HookSetWindowLongPtr();
          }
      }
      break;

      case NPWindowTypeDrawable:
          mWindow.type = aWindow.type;
          if (GetQuirks() & PluginModuleChild::QUIRK_WINLESS_TRACKPOPUP_HOOK)
              CreateWinlessPopupSurrogate();
          if (GetQuirks() & PluginModuleChild::QUIRK_FLASH_THROTTLE_WMUSER_EVENTS)
              SetupFlashMsgThrottle();
          return SharedSurfaceSetWindow(aWindow);
      break;

      default:
          NS_NOTREACHED("Bad plugin window type.");
          return false;
      break;
    }

#elif defined(XP_MACOSX)

    mWindow.x = aWindow.x;
    mWindow.y = aWindow.y;
    mWindow.width = aWindow.width;
    mWindow.height = aWindow.height;
    mWindow.clipRect = aWindow.clipRect;
    mWindow.type = aWindow.type;

    if (mShContext) {
        
        
        ::CGContextRelease(mShContext);
        mShContext = nsnull;
    }

    if (mPluginIface->setwindow)
        (void) mPluginIface->setwindow(&mData, &mWindow);

#elif defined(ANDROID)
#  warning Need Android impl
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
static const TCHAR kFlashThrottleProperty[] = TEXT("MozillaFlashThrottleProperty");


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
                      reinterpret_cast<LONG_PTR>(DefWindowProcA));

    return true;
}

void
PluginInstanceChild::DestroyPluginWindow()
{
    if (mPluginWindowHWND) {
        
        WNDPROC wndProc = reinterpret_cast<WNDPROC>(
            GetWindowLongPtr(mPluginWindowHWND, GWLP_WNDPROC));
        
        RemoveProp(mPluginWindowHWND, kPluginInstanceChildProperty);
        if (wndProc == PluginWindowProc) {
            NS_ASSERTION(mPluginWndProc, "Should have old proc here!");
            SetWindowLongPtr(mPluginWindowHWND, GWLP_WNDPROC,
                             reinterpret_cast<LONG_PTR>(mPluginWndProc));
            mPluginWndProc = 0;
        }
        DestroyWindow(mPluginWindowHWND);
        mPluginWindowHWND = 0;
    }
}

void
PluginInstanceChild::ReparentPluginWindow(HWND hWndParent)
{
    if (hWndParent != mPluginParentHWND && IsWindow(hWndParent)) {
        
        LONG_PTR style = GetWindowLongPtr(mPluginWindowHWND, GWL_STYLE);
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
  return mozilla::CallWindowProcCrashProtected(PluginWindowProcInternal, hWnd, message, wParam, lParam);
}


LRESULT CALLBACK
PluginInstanceChild::PluginWindowProcInternal(HWND hWnd,
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
    NS_ASSERTION(self->mPluginWndProc != PluginWindowProc, "Self-referential windowproc. Infinite recursion will happen soon.");

    
    
    
    
    
    
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
      self->CallPluginFocusChange(true);

    
    
    
    if ((InSendMessageEx(NULL)&(ISMEX_REPLIED|ISMEX_SEND)) == ISMEX_SEND) {
        switch(message) {
            case WM_KILLFOCUS:
            case WM_MOUSEHWHEEL:
            case WM_MOUSEWHEEL:
            case WM_HSCROLL:
            case WM_VSCROLL:
            ReplyMessage(0);
            break;
        }
    }

    if (message == WM_KILLFOCUS)
      self->CallPluginFocusChange(false);

    if (message == WM_USER+1 &&
        (self->GetQuirks() & PluginModuleChild::QUIRK_FLASH_THROTTLE_WMUSER_EVENTS)) {
        self->FlashThrottleMessage(hWnd, message, wParam, lParam, true);
        return 0;
    }

    NS_ASSERTION(self->mPluginWndProc != PluginWindowProc,
      "Self-referential windowproc happened inside our hook proc. "
      "Infinite recursion will happen soon.");

    LRESULT res = CallWindowProc(self->mPluginWndProc, hWnd, message, wParam,
                                 lParam);

    
    
    
    
    if (message == WM_LBUTTONDOWN &&
        self->GetQuirks() & PluginModuleChild::QUIRK_FLASH_FIXUP_MOUSE_CAPTURE) {
      PRUnichar szClass[26];
      HWND hwnd = GetForegroundWindow();
      if (hwnd && GetClassNameW(hwnd, szClass,
                                sizeof(szClass)/sizeof(PRUnichar)) &&
          !wcscmp(szClass, kFlashFullscreenClass)) {
        ReleaseCapture();
        SetFocus(hwnd);
      }
    }

    if (message == WM_CLOSE)
        self->DestroyPluginWindow();

    if (message == WM_NCDESTROY)
        RemoveProp(hWnd, kPluginInstanceChildProperty);

    return res;
}












 
#ifdef _WIN64
typedef LONG_PTR
  (WINAPI *User32SetWindowLongPtrA)(HWND hWnd,
                                    int nIndex,
                                    LONG_PTR dwNewLong);
typedef LONG_PTR
  (WINAPI *User32SetWindowLongPtrW)(HWND hWnd,
                                    int nIndex,
                                    LONG_PTR dwNewLong);
static User32SetWindowLongPtrA sUser32SetWindowLongAHookStub = NULL;
static User32SetWindowLongPtrW sUser32SetWindowLongWHookStub = NULL;
#else
typedef LONG
(WINAPI *User32SetWindowLongA)(HWND hWnd,
                               int nIndex,
                               LONG dwNewLong);
typedef LONG
(WINAPI *User32SetWindowLongW)(HWND hWnd,
                               int nIndex,
                               LONG dwNewLong);
static User32SetWindowLongA sUser32SetWindowLongAHookStub = NULL;
static User32SetWindowLongW sUser32SetWindowLongWHookStub = NULL;
#endif

extern LRESULT CALLBACK
NeuteredWindowProc(HWND hwnd,
                   UINT uMsg,
                   WPARAM wParam,
                   LPARAM lParam);

const wchar_t kOldWndProcProp[] = L"MozillaIPCOldWndProc";


PRBool
PluginInstanceChild::SetWindowLongHookCheck(HWND hWnd,
                                            int nIndex,
                                            LONG_PTR newLong)
{
      
  if (nIndex != GWLP_WNDPROC ||
      
      !GetProp(hWnd, kPluginInstanceChildProperty) ||
      
      GetProp(hWnd, kPluginIgnoreSubclassProperty) ||
      
      newLong == reinterpret_cast<LONG_PTR>(PluginWindowProc) ||
      newLong == reinterpret_cast<LONG_PTR>(NeuteredWindowProc) ||
      newLong == reinterpret_cast<LONG_PTR>(DefWindowProcA) ||
      newLong == reinterpret_cast<LONG_PTR>(DefWindowProcW) ||
      
      GetProp(hWnd, kOldWndProcProp))
      return PR_TRUE;
  
  return PR_FALSE;
}

#ifdef _WIN64
LONG_PTR WINAPI
PluginInstanceChild::SetWindowLongPtrAHook(HWND hWnd,
                                           int nIndex,
                                           LONG_PTR newLong)
#else
LONG WINAPI
PluginInstanceChild::SetWindowLongAHook(HWND hWnd,
                                        int nIndex,
                                        LONG newLong)
#endif
{
    if (SetWindowLongHookCheck(hWnd, nIndex, newLong))
        return sUser32SetWindowLongAHookStub(hWnd, nIndex, newLong);

    
    LONG_PTR proc = sUser32SetWindowLongAHookStub(hWnd, nIndex, newLong);

    
    PluginInstanceChild* self = reinterpret_cast<PluginInstanceChild*>(
        GetProp(hWnd, kPluginInstanceChildProperty));

    
    WNDPROC currentProc =
        reinterpret_cast<WNDPROC>(GetWindowLongPtr(hWnd, GWLP_WNDPROC));
    if (currentProc != PluginWindowProc) {
        self->mPluginWndProc =
            reinterpret_cast<WNDPROC>(sUser32SetWindowLongWHookStub(hWnd, nIndex,
                reinterpret_cast<LONG_PTR>(PluginWindowProc)));
        NS_ASSERTION(self->mPluginWndProc != PluginWindowProc, "Infinite recursion coming up!");
    }
    return proc;
}

#ifdef _WIN64
LONG_PTR WINAPI
PluginInstanceChild::SetWindowLongPtrWHook(HWND hWnd,
                                           int nIndex,
                                           LONG_PTR newLong)
#else
LONG WINAPI
PluginInstanceChild::SetWindowLongWHook(HWND hWnd,
                                        int nIndex,
                                        LONG newLong)
#endif
{
    if (SetWindowLongHookCheck(hWnd, nIndex, newLong))
        return sUser32SetWindowLongWHookStub(hWnd, nIndex, newLong);

    
    LONG_PTR proc = sUser32SetWindowLongWHookStub(hWnd, nIndex, newLong);

    
    PluginInstanceChild* self = reinterpret_cast<PluginInstanceChild*>(
        GetProp(hWnd, kPluginInstanceChildProperty));

    
    WNDPROC currentProc =
        reinterpret_cast<WNDPROC>(GetWindowLongPtr(hWnd, GWLP_WNDPROC));
    if (currentProc != PluginWindowProc) {
        self->mPluginWndProc =
            reinterpret_cast<WNDPROC>(sUser32SetWindowLongWHookStub(hWnd, nIndex,
                reinterpret_cast<LONG_PTR>(PluginWindowProc)));
        NS_ASSERTION(self->mPluginWndProc != PluginWindowProc, "Infinite recursion coming up!");
    }
    return proc;
}

void
PluginInstanceChild::HookSetWindowLongPtr()
{
#ifdef _WIN64
    
    
    return;
#endif

    if (!(GetQuirks() & PluginModuleChild::QUIRK_FLASH_HOOK_SETLONGPTR))
        return;

    sUser32Intercept.Init("user32.dll");
#ifdef _WIN64
    sUser32Intercept.AddHook("SetWindowLongPtrA", reinterpret_cast<intptr_t>(SetWindowLongPtrAHook),
                             (void**) &sUser32SetWindowLongAHookStub);
    sUser32Intercept.AddHook("SetWindowLongPtrW", reinterpret_cast<intptr_t>(SetWindowLongPtrWHook),
                             (void**) &sUser32SetWindowLongWHookStub);
#else
    sUser32Intercept.AddHook("SetWindowLongA", reinterpret_cast<intptr_t>(SetWindowLongAHook),
                             (void**) &sUser32SetWindowLongAHookStub);
    sUser32Intercept.AddHook("SetWindowLongW", reinterpret_cast<intptr_t>(SetWindowLongWHook),
                             (void**) &sUser32SetWindowLongWHookStub);
#endif
}



BOOL
WINAPI
PluginInstanceChild::TrackPopupHookProc(HMENU hMenu,
                                        UINT uFlags,
                                        int x,
                                        int y,
                                        int nReserved,
                                        HWND hWnd,
                                        CONST RECT *prcRect)
{
  if (!sUser32TrackPopupMenuStub) {
      NS_ERROR("TrackPopupMenu stub isn't set! Badness!");
      return 0;
  }

  
  
  
  PRUnichar szClass[21];
  bool haveClass = GetClassNameW(hWnd, szClass, NS_ARRAY_LENGTH(szClass));
  if (!haveClass || 
      (wcscmp(szClass, L"MozillaWindowClass") &&
       wcscmp(szClass, L"SWFlash_Placeholder"))) {
      
      return sUser32TrackPopupMenuStub(hMenu, uFlags, x, y, nReserved,
                                       hWnd, prcRect);
  }

  
  if (!sWinlessPopupSurrogateHWND) {
      NS_WARNING(
          "Untraced TrackPopupHookProc call! Menu might not work right!");
      return sUser32TrackPopupMenuStub(hMenu, uFlags, x, y, nReserved,
                                       hWnd, prcRect);
  }

  HWND surrogateHwnd = sWinlessPopupSurrogateHWND;
  sWinlessPopupSurrogateHWND = NULL;

  
  
  
  
  bool isRetCmdCall = (uFlags & TPM_RETURNCMD);

  
  
  HWND focusHwnd = SetFocus(surrogateHwnd);
  DWORD res = sUser32TrackPopupMenuStub(hMenu, uFlags|TPM_RETURNCMD, x, y,
                                        nReserved, surrogateHwnd, prcRect);
  if (IsWindow(focusHwnd)) {
      SetFocus(focusHwnd);
  }

  if (!isRetCmdCall && res) {
      SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(res, 0), 0);
  }

  return res;
}

void
PluginInstanceChild::InitPopupMenuHook()
{
    if (!(GetQuirks() & PluginModuleChild::QUIRK_WINLESS_TRACKPOPUP_HOOK) ||
        sUser32TrackPopupMenuStub)
        return;

    
    
    
    
    sUser32Intercept.Init("user32.dll");
    sUser32Intercept.AddHook("TrackPopupMenu", reinterpret_cast<intptr_t>(TrackPopupHookProc),
                             (void**) &sUser32TrackPopupMenuStub);
}

void
PluginInstanceChild::CreateWinlessPopupSurrogate()
{
    
    if (mWinlessPopupSurrogateHWND)
        return;

    HWND hwnd = NULL;
    NPError result;
    if (!CallNPN_GetValue_NPNVnetscapeWindow(&hwnd, &result)) {
        NS_ERROR("CallNPN_GetValue_NPNVnetscapeWindow failed.");
        return;
    }

    mWinlessPopupSurrogateHWND =
        CreateWindowEx(WS_EX_NOPARENTNOTIFY, L"Static", NULL, WS_CHILD, 0, 0,
                       0, 0, hwnd, 0, GetModuleHandle(NULL), 0);
    if (!mWinlessPopupSurrogateHWND) {
        NS_ERROR("CreateWindowEx failed for winless placeholder!");
        return;
    }
    return;
}

void
PluginInstanceChild::DestroyWinlessPopupSurrogate()
{
    if (mWinlessPopupSurrogateHWND)
        DestroyWindow(mWinlessPopupSurrogateHWND);
    mWinlessPopupSurrogateHWND = NULL;
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
    if (!mPluginIface->event)
        return false;

    if (!NeedsNestedEventCoverage(event.event)) {
        return mPluginIface->event(&mData, reinterpret_cast<void*>(&event));
    }

    
    
    int16_t handled;

    
    
    
    if ((GetQuirks() & PluginModuleChild::QUIRK_WINLESS_TRACKPOPUP_HOOK) && 
          (event.event == WM_RBUTTONDOWN || 
           event.event == WM_RBUTTONUP)) {  
      sWinlessPopupSurrogateHWND = mWinlessPopupSurrogateHWND;
    }

    handled = mPluginIface->event(&mData, reinterpret_cast<void*>(&event));

    sWinlessPopupSurrogateHWND = NULL;

    return handled;
}



bool
PluginInstanceChild::SharedSurfaceSetWindow(const NPRemoteWindow& aWindow)
{
    
    
    
    if (!aWindow.surfaceHandle) {
        if (!mSharedSurfaceDib.IsValid()) {
            return false;
        }
    }
    else {
        
        if (NS_FAILED(mSharedSurfaceDib.Attach((SharedDIB::Handle)aWindow.surfaceHandle,
                                               aWindow.width, aWindow.height, false)))
          return false;
        
        
        AlphaExtractCacheRelease();
    }
      
    
    mWindow.x      = aWindow.x;
    mWindow.y      = aWindow.y;
    mWindow.width  = aWindow.width;
    mWindow.height = aWindow.height;
    mWindow.type   = aWindow.type;

    mWindow.window = reinterpret_cast<void*>(mSharedSurfaceDib.GetHDC());
    ::SetViewportOrgEx(mSharedSurfaceDib.GetHDC(), -aWindow.x, -aWindow.y, NULL);

    if (mPluginIface->setwindow)
        mPluginIface->setwindow(&mData, &mWindow);

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
    if (!mPluginIface->event)
        return false;

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

              
              UpdatePaintClipRect(pRect);
              ::FillRect(mSharedSurfaceDib.GetHDC(), pRect, (HBRUSH)GetStockObject(WHITE_BRUSH));
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
              
              UpdatePaintClipRect(pRect);
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










void
PluginInstanceChild::UnhookWinlessFlashThrottle()
{
  
  if (!mWinlessThrottleOldWndProc)
      return;

  WNDPROC tmpProc = mWinlessThrottleOldWndProc;
  mWinlessThrottleOldWndProc = nsnull;

  NS_ASSERTION(mWinlessHiddenMsgHWND,
               "Missing mWinlessHiddenMsgHWND w/subclass set??");

  
  SetWindowLongPtr(mWinlessHiddenMsgHWND, GWLP_WNDPROC,
                   reinterpret_cast<LONG_PTR>(tmpProc));

  
  RemoveProp(mWinlessHiddenMsgHWND, kFlashThrottleProperty);
  mWinlessHiddenMsgHWND = nsnull;
}


LRESULT CALLBACK
PluginInstanceChild::WinlessHiddenFlashWndProc(HWND hWnd,
                                               UINT message,
                                               WPARAM wParam,
                                               LPARAM lParam)
{
    PluginInstanceChild* self = reinterpret_cast<PluginInstanceChild*>(
        GetProp(hWnd, kFlashThrottleProperty));
    if (!self) {
        NS_NOTREACHED("Badness!");
        return 0;
    }

    NS_ASSERTION(self->mWinlessThrottleOldWndProc,
                 "Missing subclass procedure!!");

    
    if (message == WM_USER+1) {
        self->FlashThrottleMessage(hWnd, message, wParam, lParam, false);
        return 0;
     }

    
    if (message == WM_CLOSE || message == WM_NCDESTROY) {
        WNDPROC tmpProc = self->mWinlessThrottleOldWndProc;
        self->UnhookWinlessFlashThrottle();
        LRESULT res = CallWindowProc(tmpProc, hWnd, message, wParam, lParam);
        return res;
    }

    return CallWindowProc(self->mWinlessThrottleOldWndProc,
                          hWnd, message, wParam, lParam);
}




BOOL CALLBACK
PluginInstanceChild::EnumThreadWindowsCallback(HWND hWnd,
                                               LPARAM aParam)
{
    PluginInstanceChild* self = reinterpret_cast<PluginInstanceChild*>(aParam);
    if (!self) {
        NS_NOTREACHED("Enum befuddled!");
        return FALSE;
    }

    PRUnichar className[64];
    if (!GetClassNameW(hWnd, className, sizeof(className)/sizeof(PRUnichar)))
      return TRUE;
    
    if (!wcscmp(className, L"SWFlash_PlaceholderX")) {
        WNDPROC oldWndProc =
            reinterpret_cast<WNDPROC>(GetWindowLongPtr(hWnd, GWLP_WNDPROC));
        
        if (oldWndProc != WinlessHiddenFlashWndProc) {
            if (self->mWinlessThrottleOldWndProc) {
                NS_WARNING("mWinlessThrottleWndProc already set???");
                return FALSE;
            }
            
            self->mWinlessHiddenMsgHWND = hWnd;
            self->mWinlessThrottleOldWndProc =
                reinterpret_cast<WNDPROC>(SetWindowLongPtr(hWnd, GWLP_WNDPROC,
                reinterpret_cast<LONG_PTR>(WinlessHiddenFlashWndProc)));
            SetProp(hWnd, kFlashThrottleProperty, self);
            NS_ASSERTION(self->mWinlessThrottleOldWndProc,
                         "SetWindowLongPtr failed?!");
        }
        
        return FALSE;
    }

    return TRUE;
}


void
PluginInstanceChild::SetupFlashMsgThrottle()
{
    if (mWindow.type == NPWindowTypeDrawable) {
        
        
        if (mWinlessThrottleOldWndProc)
            return;
        EnumThreadWindows(GetCurrentThreadId(), EnumThreadWindowsCallback,
                          reinterpret_cast<LPARAM>(this));
    }
    else {
        
        return;
    }
}

WNDPROC
PluginInstanceChild::FlashThrottleAsyncMsg::GetProc()
{ 
    if (mInstance) {
        return mWindowed ? mInstance->mPluginWndProc :
                           mInstance->mWinlessThrottleOldWndProc;
    }
    return nsnull;
}
 
void
PluginInstanceChild::FlashThrottleAsyncMsg::Run()
{
    RemoveFromAsyncList();

    
    
    
    if (!GetProc())
        return;
  
    
    CallWindowProc(GetProc(), GetWnd(), GetMsg(), GetWParam(), GetLParam());
}

void
PluginInstanceChild::FlashThrottleMessage(HWND aWnd,
                                          UINT aMsg,
                                          WPARAM aWParam,
                                          LPARAM aLParam,
                                          bool isWindowed)
{
    
    
    FlashThrottleAsyncMsg* task = new FlashThrottleAsyncMsg(this,
        aWnd, aMsg, aWParam, aLParam, isWindowed);
    if (!task)
        return; 

    {
        MutexAutoLock lock(mAsyncCallMutex);
        mPendingAsyncCalls.AppendElement(task);
    }
    MessageLoop::current()->PostDelayedTask(FROM_HERE,
        task, kFlashWMUSERMessageThrottleDelayMs);
}

#endif 

bool
PluginInstanceChild::AnswerSetPluginFocus()
{
    PR_LOG(gPluginLog, PR_LOG_DEBUG, ("%s", FULLFUNCTION));

#if defined(OS_WIN)
    
    
    
    
    
    if (::GetFocus() == mPluginWindowHWND || ::GetFocus() != mPluginParentHWND)
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
    if (mPluginWindowHWND) {
        RECT rect;
        if (GetUpdateRect(GetParent(mPluginWindowHWND), &rect, FALSE)) {
            ::InvalidateRect(mPluginWindowHWND, &rect, FALSE); 
        }
        UpdateWindow(mPluginWindowHWND);
    }
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
          ->StreamConstructed(mimeType, seekable, stype);
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
    return new BrowserStreamChild(this, url, length, lastmodified,
                                  static_cast<StreamNotifyChild*>(notifyData),
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

void
StreamNotifyChild::ActorDestroy(ActorDestroyReason why)
{
    if (AncestorDeletion == why && mBrowserStream) {
        NS_ERROR("Pending NPP_URLNotify not called when closing an instance.");

        
        mBrowserStream->mStreamNotify = NULL;
        mBrowserStream = NULL;
    }
}


void
StreamNotifyChild::SetAssociatedStream(BrowserStreamChild* bs)
{
    NS_ASSERTION(bs, "Shouldn't be null");
    NS_ASSERTION(!mBrowserStream, "Two streams for one streamnotify?");

    mBrowserStream = bs;
}

bool
StreamNotifyChild::Recv__delete__(const NPReason& reason)
{
    AssertPluginThread();

    if (mBrowserStream)
        mBrowserStream->NotifyPending();
    else
        NPP_URLNotify(reason);

    return true;
}

bool
StreamNotifyChild::RecvRedirectNotify(const nsCString& url, const int32_t& status)
{
    
    
    
    
    if (!mClosure) {
        SendRedirectNotifyResponse(false);
    }

    PluginInstanceChild* instance = static_cast<PluginInstanceChild*>(Manager());
    if (instance->mPluginIface->urlredirectnotify)
      instance->mPluginIface->urlredirectnotify(instance->GetNPP(), url.get(), status, mClosure);

    return true;
}

void
StreamNotifyChild::NPP_URLNotify(NPReason reason)
{
    PluginInstanceChild* instance = static_cast<PluginInstanceChild*>(Manager());

    if (mClosure)
        instance->mPluginIface->urlnotify(instance->GetNPP(), mURL.get(),
                                          reason, mClosure);
}

bool
PluginInstanceChild::DeallocPStreamNotify(PStreamNotifyChild* notifyData)
{
    AssertPluginThread();

    if (!static_cast<StreamNotifyChild*>(notifyData)->mBrowserStream)
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
PluginInstanceChild::NPN_URLRedirectResponse(void* notifyData, NPBool allow)
{
    if (!notifyData) {
        return;
    }

    InfallibleTArray<PStreamNotifyChild*> notifyStreams;
    ManagedPStreamNotifyChild(notifyStreams);
    PRUint32 notifyStreamCount = notifyStreams.Length();
    for (PRUint32 i = 0; i < notifyStreamCount; i++) {
        StreamNotifyChild* sn = static_cast<StreamNotifyChild*>(notifyStreams[i]);
        if (sn->mClosure == notifyData) {
            sn->SendRedirectNotifyResponse(static_cast<bool>(allow));
            return;
        }
    }
    NS_ASSERTION(PR_FALSE, "Couldn't find stream for redirect response!");
}

bool
PluginInstanceChild::RecvAsyncSetWindow(const gfxSurfaceType& aSurfaceType,
                                        const NPRemoteWindow& aWindow)
{
    AssertPluginThread();

    NS_ASSERTION(!aWindow.window, "Remote window should be null.");

    if (mCurrentAsyncSetWindowTask) {
        mCurrentAsyncSetWindowTask->Cancel();
        mCurrentAsyncSetWindowTask = nsnull;
    }

    if (mPendingPluginCall) {
        
        mCurrentAsyncSetWindowTask =
            NewRunnableMethod<PluginInstanceChild,
                              void (PluginInstanceChild::*)(const gfxSurfaceType&, const NPRemoteWindow&, bool),
                              gfxSurfaceType, NPRemoteWindow, bool>
                (this, &PluginInstanceChild::DoAsyncSetWindow,
                 aSurfaceType, aWindow, true);
        MessageLoop::current()->PostTask(FROM_HERE, mCurrentAsyncSetWindowTask);
    } else {
        DoAsyncSetWindow(aSurfaceType, aWindow, false);
    }

    return true;
}

void
PluginInstanceChild::DoAsyncSetWindow(const gfxSurfaceType& aSurfaceType,
                                      const NPRemoteWindow& aWindow,
                                      bool aIsAsync)
{
    PLUGIN_LOG_DEBUG(
        ("[InstanceChild][%p] AsyncSetWindow to <x=%d,y=%d, w=%d,h=%d>",
         this, aWindow.x, aWindow.y, aWindow.width, aWindow.height));

    AssertPluginThread();
    NS_ASSERTION(!aWindow.window, "Remote window should be null.");
    NS_ASSERTION(!mPendingPluginCall, "Can't do SetWindow during plugin call!");

    if (aIsAsync) {
        if (!mCurrentAsyncSetWindowTask) {
            return;
        }
        mCurrentAsyncSetWindowTask = nsnull;
    }

    mWindow.window = NULL;
    if (mWindow.width != aWindow.width || mWindow.height != aWindow.height ||
        mWindow.clipRect.top != aWindow.clipRect.top ||
        mWindow.clipRect.left != aWindow.clipRect.left ||
        mWindow.clipRect.bottom != aWindow.clipRect.bottom ||
        mWindow.clipRect.right != aWindow.clipRect.right)
        mAccumulatedInvalidRect = nsIntRect(0, 0, aWindow.width, aWindow.height);

    mWindow.x = aWindow.x;
    mWindow.y = aWindow.y;
    mWindow.width = aWindow.width;
    mWindow.height = aWindow.height;
    mWindow.clipRect = aWindow.clipRect;
    mWindow.type = aWindow.type;

    if (GetQuirks() & PluginModuleChild::QUIRK_SILVERLIGHT_DEFAULT_TRANSPARENT)
        mIsTransparent = true;

    mLayersRendering = true;
    mSurfaceType = aSurfaceType;
    UpdateWindowAttributes(true);

#ifdef XP_WIN
    if (GetQuirks() & PluginModuleChild::QUIRK_WINLESS_TRACKPOPUP_HOOK)
        CreateWinlessPopupSurrogate();
    if (GetQuirks() & PluginModuleChild::QUIRK_FLASH_THROTTLE_WMUSER_EVENTS)
        SetupFlashMsgThrottle();
#endif

    if (!mAccumulatedInvalidRect.IsEmpty()) {
        AsyncShowPluginFrame();
    }
}

static inline gfxRect
GfxFromNsRect(const nsIntRect& aRect)
{
    return gfxRect(aRect.x, aRect.y, aRect.width, aRect.height);
}

bool
PluginInstanceChild::CreateOptSurface(void)
{
    NS_ABORT_IF_FALSE(mSurfaceType != gfxASurface::SurfaceTypeMax,
                      "Need a valid surface type here");
    NS_ASSERTION(!mCurrentSurface, "mCurrentSurfaceActor can get out of sync.");

    nsRefPtr<gfxASurface> retsurf;
    
    
    gfxASurface::gfxImageFormat format =
        (mIsTransparent && !mBackground) ? gfxASurface::ImageFormatARGB32 :
                                           gfxASurface::ImageFormatRGB24;

#if (MOZ_PLATFORM_MAEMO == 5) || (MOZ_PLATFORM_MAEMO == 6)
    
    if (mMaemoImageRendering) {
        NPEvent pluginEvent;
        XVisibilityEvent& visibilityEvent = pluginEvent.xvisibility;
        visibilityEvent.type = VisibilityNotify;
        visibilityEvent.display = 0;
        visibilityEvent.state = VisibilityUnobscured;
        mPluginIface->event(&mData, reinterpret_cast<void*>(&pluginEvent));
    }
#endif
#ifdef MOZ_X11
    Display* dpy = mWsInfo.display;
    Screen* screen = DefaultScreenOfDisplay(dpy);
    if (format == gfxASurface::ImageFormatRGB24 &&
        DefaultDepth(dpy, DefaultScreen(dpy)) == 16) {
        format = gfxASurface::ImageFormatRGB16_565;
    }

    if (mSurfaceType == gfxASurface::SurfaceTypeXlib) {
        XRenderPictFormat* xfmt = gfxXlibSurface::FindRenderFormat(dpy, format);
        if (!xfmt) {
            NS_ERROR("Need X falback surface, but FindRenderFormat failed");
            return false;
        }
        mCurrentSurface =
            gfxXlibSurface::Create(screen, xfmt,
                                   gfxIntSize(mWindow.width,
                                              mWindow.height));
        return mCurrentSurface != nsnull;
    }
#endif

#ifdef XP_WIN
    if (mSurfaceType == gfxASurface::SurfaceTypeWin32 ||
        mSurfaceType == gfxASurface::SurfaceTypeD2D) {
        bool willHaveTransparentPixels = mIsTransparent && !mBackground;

        SharedDIBSurface* s = new SharedDIBSurface();
        if (!s->Create(reinterpret_cast<HDC>(mWindow.window),
                       mWindow.width, mWindow.height,
                       willHaveTransparentPixels))
            return false;

        mCurrentSurface = s;
        return true;
    }

    NS_RUNTIMEABORT("Shared-memory drawing not expected on Windows.");
#endif

    
    mCurrentSurface =
        gfxSharedImageSurface::CreateUnsafe(this, gfxIntSize(mWindow.width, mWindow.height), format);
    return !!mCurrentSurface;
}

bool
PluginInstanceChild::MaybeCreatePlatformHelperSurface(void)
{
    if (!mCurrentSurface) {
        NS_ERROR("Cannot create helper surface without mCurrentSurface");
        return false;
    }

#ifdef MOZ_PLATFORM_MAEMO
    
    bool supportNonDefaultVisual = true;
#else
    bool supportNonDefaultVisual = false;
#endif
#ifdef MOZ_X11
    Screen* screen = DefaultScreenOfDisplay(mWsInfo.display);
    Visual* defaultVisual = DefaultVisualOfScreen(screen);
    Visual* visual = nsnull;
    Colormap colormap = 0;
    mDoAlphaExtraction = false;
    bool createHelperSurface = false;

    if (mCurrentSurface->GetType() == gfxASurface::SurfaceTypeXlib) {
        static_cast<gfxXlibSurface*>(mCurrentSurface.get())->
            GetColormapAndVisual(&colormap, &visual);
        
        
        if (!visual || (defaultVisual != visual && !supportNonDefaultVisual)) {
            createHelperSurface = true;
            visual = defaultVisual;
            mDoAlphaExtraction = mIsTransparent;
        }
    } else if (mCurrentSurface->GetType() == gfxASurface::SurfaceTypeImage) {
#if (MOZ_PLATFORM_MAEMO == 5) || (MOZ_PLATFORM_MAEMO == 6)
        if (mMaemoImageRendering) {
            
            
            
            return PR_TRUE;
        }
#endif
        
        createHelperSurface = true;
        
        visual = gfxXlibSurface::FindVisual(screen,
            static_cast<gfxImageSurface*>(mCurrentSurface.get())->Format());
        if (visual && defaultVisual != visual && !supportNonDefaultVisual) {
            visual = defaultVisual;
            mDoAlphaExtraction = mIsTransparent;
        }
    }

    if (createHelperSurface) {
        if (!visual) {
            NS_ERROR("Need X falback surface, but visual failed");
            return false;
        }
        mHelperSurface =
            gfxXlibSurface::Create(screen, visual,
                                   mCurrentSurface->GetSize());
        if (!mHelperSurface) {
            NS_WARNING("Fail to create create helper surface");
            return false;
        }
    }
#elif defined(XP_WIN)
    mDoAlphaExtraction = mIsTransparent && !mBackground;
#endif

    return true;
}

bool
PluginInstanceChild::EnsureCurrentBuffer(void)
{
    nsIntRect toInvalidate(0, 0, 0, 0);
    gfxIntSize winSize = gfxIntSize(mWindow.width, mWindow.height);

    if (mBackground && mBackground->GetSize() != winSize) {
        
        
        
        mBackground = nsnull;
        toInvalidate.UnionRect(toInvalidate,
                               nsIntRect(0, 0, winSize.width, winSize.height));
    }

    if (mCurrentSurface) {
        gfxIntSize surfSize = mCurrentSurface->GetSize();
        if (winSize != surfSize ||
            (mBackground && !CanPaintOnBackground()) ||
            (mBackground &&
             gfxASurface::CONTENT_COLOR != mCurrentSurface->GetContentType()) ||
            (!mBackground && mIsTransparent &&
             gfxASurface::CONTENT_COLOR == mCurrentSurface->GetContentType())) {
            
            mWindow.window = nsnull;
            ClearCurrentSurface();
            toInvalidate.UnionRect(toInvalidate,
                                   nsIntRect(0, 0, winSize.width, winSize.height));
        }
    }

    mAccumulatedInvalidRect.UnionRect(mAccumulatedInvalidRect, toInvalidate);

    if (mCurrentSurface) {
       return true;
    }

    if (!CreateOptSurface()) {
        NS_ERROR("Cannot create optimized surface");
        return false;
    }

    if (!MaybeCreatePlatformHelperSurface()) {
        NS_ERROR("Cannot create helper surface");
        return false;
    }

    return true;
}

void
PluginInstanceChild::UpdateWindowAttributes(bool aForceSetWindow)
{
    nsRefPtr<gfxASurface> curSurface = mHelperSurface ? mHelperSurface : mCurrentSurface;
    bool needWindowUpdate = aForceSetWindow;
#ifdef MOZ_X11
    Visual* visual = nsnull;
    Colormap colormap = 0;
    if (curSurface && curSurface->GetType() == gfxASurface::SurfaceTypeXlib) {
        static_cast<gfxXlibSurface*>(curSurface.get())->
            GetColormapAndVisual(&colormap, &visual);
        if (visual != mWsInfo.visual || colormap != mWsInfo.colormap) {
            mWsInfo.visual = visual;
            mWsInfo.colormap = colormap;
            needWindowUpdate = true;
        }
    }
#if (MOZ_PLATFORM_MAEMO == 5) || (MOZ_PLATFORM_MAEMO == 6)
    else if (curSurface && curSurface->GetType() == gfxASurface::SurfaceTypeImage
             && mMaemoImageRendering) {
        
        
        gfxImageSurface* img = static_cast<gfxImageSurface*>(curSurface.get());
        if (mWindow.window ||
            mWsInfo.depth != gfxUtils::ImageFormatToDepth(img->Format()) ||
            mWsInfo.colormap) {
            mWindow.window = nsnull;
            mWsInfo.depth = gfxUtils::ImageFormatToDepth(img->Format());
            mWsInfo.colormap = 0;
            needWindowUpdate = true;
        }
    }
#endif 
#endif 
#ifdef XP_WIN
    HDC dc = NULL;

    if (curSurface) {
        if (!SharedDIBSurface::IsSharedDIBSurface(curSurface))
            NS_RUNTIMEABORT("Expected SharedDIBSurface!");

        SharedDIBSurface* dibsurf = static_cast<SharedDIBSurface*>(curSurface.get());
        dc = dibsurf->GetHDC();
    }
    if (mWindow.window != dc) {
        mWindow.window = dc;
        needWindowUpdate = true;
    }
#endif 

    if (!needWindowUpdate) {
        return;
    }

#ifndef XP_WIN
    
    
    mWindow.x = mWindow.y = 0;
#endif

    if (IsVisible()) {
        
        nsIntRect clipRect;

        
        
        
        clipRect.SetRect(0, 0, mWindow.width, mWindow.height);

        mWindow.clipRect.left = 0;
        mWindow.clipRect.top = 0;
        mWindow.clipRect.right = clipRect.XMost();
        mWindow.clipRect.bottom = clipRect.YMost();
    }

#ifdef XP_WIN
    
    
    

    if (mPluginIface->event) {
        WINDOWPOS winpos = {
            0, 0,
            mWindow.x, mWindow.y,
            mWindow.width, mWindow.height,
            0
        };
        NPEvent pluginEvent = {
            WM_WINDOWPOSCHANGED, 0,
            (LPARAM) &winpos
        };
        mPluginIface->event(&mData, &pluginEvent);
    }
#endif

    PLUGIN_LOG_DEBUG(
        ("[InstanceChild][%p] UpdateWindow w=<x=%d,y=%d, w=%d,h=%d>, clip=<l=%d,t=%d,r=%d,b=%d>",
         this, mWindow.x, mWindow.y, mWindow.width, mWindow.height,
         mWindow.clipRect.left, mWindow.clipRect.top, mWindow.clipRect.right, mWindow.clipRect.bottom));

    if (mPluginIface->setwindow) {
        mPluginIface->setwindow(&mData, &mWindow);
    }
}

void
PluginInstanceChild::PaintRectToPlatformSurface(const nsIntRect& aRect,
                                                gfxASurface* aSurface)
{
    UpdateWindowAttributes();

#ifdef MOZ_X11
#if (MOZ_PLATFORM_MAEMO == 5) || (MOZ_PLATFORM_MAEMO == 6)
    
    if (mMaemoImageRendering &&
        aSurface->GetType() == gfxASurface::SurfaceTypeImage) {
        aSurface->Flush();
        gfxImageSurface* image = static_cast<gfxImageSurface*>(aSurface);
        NPImageExpose imgExp;
        imgExp.depth = gfxUtils::ImageFormatToDepth(image->Format());
        imgExp.x = aRect.x;
        imgExp.y = aRect.y;
        imgExp.width = aRect.width;
        imgExp.height = aRect.height;
        imgExp.stride = image->Stride();
        imgExp.data = (char *)image->Data();
        imgExp.dataSize.width = image->Width();
        imgExp.dataSize.height = image->Height();
        imgExp.translateX = 0;
        imgExp.translateY = 0;
        imgExp.scaleX = 1;
        imgExp.scaleY = 1;
        NPEvent pluginEvent;
        XGraphicsExposeEvent& exposeEvent = pluginEvent.xgraphicsexpose;
        exposeEvent.type = GraphicsExpose;
        exposeEvent.display = 0;
        
        exposeEvent.drawable = (Drawable)&imgExp;
        exposeEvent.x = imgExp.x;
        exposeEvent.y = imgExp.y;
        exposeEvent.width = imgExp.width;
        exposeEvent.height = imgExp.height;
        exposeEvent.count = 0;
        
        exposeEvent.serial = 0;
        exposeEvent.send_event = False;
        exposeEvent.major_code = 0;
        exposeEvent.minor_code = 0;
        mPluginIface->event(&mData, reinterpret_cast<void*>(&exposeEvent));
    } else
#endif
    {
        NS_ASSERTION(aSurface->GetType() == gfxASurface::SurfaceTypeXlib,
                     "Non supported platform surface type");

        NPEvent pluginEvent;
        XGraphicsExposeEvent& exposeEvent = pluginEvent.xgraphicsexpose;
        exposeEvent.type = GraphicsExpose;
        exposeEvent.display = mWsInfo.display;
        exposeEvent.drawable = static_cast<gfxXlibSurface*>(aSurface)->XDrawable();
        exposeEvent.x = aRect.x;
        exposeEvent.y = aRect.y;
        exposeEvent.width = aRect.width;
        exposeEvent.height = aRect.height;
        exposeEvent.count = 0;
        
        exposeEvent.serial = 0;
        exposeEvent.send_event = False;
        exposeEvent.major_code = 0;
        exposeEvent.minor_code = 0;
        mPluginIface->event(&mData, reinterpret_cast<void*>(&exposeEvent));
    }
#elif defined(XP_WIN)
    NS_ASSERTION(SharedDIBSurface::IsSharedDIBSurface(aSurface),
                 "Expected (SharedDIB) image surface.");

    
    
    RECT rect = {
        mWindow.x + aRect.x,
        mWindow.y + aRect.y,
        mWindow.x + aRect.XMost(),
        mWindow.y + aRect.YMost()
    };
    NPEvent paintEvent = {
        WM_PAINT,
        uintptr_t(mWindow.window),
        uintptr_t(&rect)
    };

    ::SetViewportOrgEx((HDC) mWindow.window, -mWindow.x, -mWindow.y, NULL);
    ::SelectClipRgn((HDC) mWindow.window, NULL);
    ::IntersectClipRect((HDC) mWindow.window, rect.left, rect.top, rect.right, rect.bottom);
    mPluginIface->event(&mData, reinterpret_cast<void*>(&paintEvent));
#else
    NS_RUNTIMEABORT("Surface type not implemented.");
#endif
}

void
PluginInstanceChild::PaintRectToSurface(const nsIntRect& aRect,
                                        gfxASurface* aSurface,
                                        const gfxRGBA& aColor)
{
    
    nsIntRect plPaintRect(aRect);
    nsRefPtr<gfxASurface> renderSurface = aSurface;
#ifdef MOZ_X11
    if (mIsTransparent && (GetQuirks() & PluginModuleChild::QUIRK_FLASH_EXPOSE_COORD_TRANSLATION)) {
        
        
        
        
        plPaintRect.SetRect(0, 0, aRect.XMost(), aRect.YMost());
    }
    if (renderSurface->GetType() != gfxASurface::SurfaceTypeXlib) {
        
#if (MOZ_PLATFORM_MAEMO == 5) || (MOZ_PLATFORM_MAEMO == 6)
        
        if (!mMaemoImageRendering ||
            renderSurface->GetType() != gfxASurface::SurfaceTypeImage)
#endif
        renderSurface = mHelperSurface;
    }
#endif

    if (aColor.a > 0.0) {
       
       nsRefPtr<gfxContext> ctx = new gfxContext(renderSurface);
       ctx->SetColor(aColor);
       ctx->SetOperator(gfxContext::OPERATOR_SOURCE);
       ctx->Rectangle(GfxFromNsRect(plPaintRect));
       ctx->Fill();
    }

    PaintRectToPlatformSurface(plPaintRect, renderSurface);

    if (renderSurface != aSurface) {
        
        nsRefPtr<gfxContext> ctx = new gfxContext(aSurface);
        ctx->SetSource(renderSurface);
        ctx->SetOperator(gfxContext::OPERATOR_SOURCE);
        ctx->Rectangle(GfxFromNsRect(aRect));
        ctx->Fill();
    }
}

void
PluginInstanceChild::PaintRectWithAlphaExtraction(const nsIntRect& aRect,
                                                  gfxASurface* aSurface)
{
    NS_ABORT_IF_FALSE(aSurface->GetContentType() == gfxASurface::CONTENT_COLOR_ALPHA,
                      "Refusing to pointlessly recover alpha");

    nsIntRect rect(aRect);
    
    
    
    bool useSurfaceSubimageForBlack = false;
    if (gfxASurface::SurfaceTypeImage == aSurface->GetType()) {
        gfxImageSurface* surfaceAsImage =
            static_cast<gfxImageSurface*>(aSurface);
        useSurfaceSubimageForBlack =
            (surfaceAsImage->Format() == gfxASurface::ImageFormatARGB32);
        
        
        
        
        if (useSurfaceSubimageForBlack) {
            rect =
                gfxAlphaRecovery::AlignRectForSubimageRecovery(aRect,
                                                               surfaceAsImage);
        }
    }

    nsRefPtr<gfxImageSurface> whiteImage;
    nsRefPtr<gfxImageSurface> blackImage;
    gfxRect targetRect(rect.x, rect.y, rect.width, rect.height);
    gfxIntSize targetSize(rect.width, rect.height);
    gfxPoint deviceOffset = -targetRect.TopLeft();

    
    whiteImage = new gfxImageSurface(targetSize, gfxASurface::ImageFormatRGB24);
    if (whiteImage->CairoStatus()) {
        return;
    }

#ifdef XP_WIN
    
    
    
    if (!SharedDIBSurface::IsSharedDIBSurface(aSurface))
        NS_RUNTIMEABORT("Expected SharedDIBSurface!");

    
    
    PaintRectToSurface(rect, aSurface, gfxRGBA(1.0, 1.0, 1.0));
    {
        gfxRect copyRect(gfxPoint(0, 0), targetRect.Size());
        nsRefPtr<gfxContext> ctx = new gfxContext(whiteImage);
        ctx->SetOperator(gfxContext::OPERATOR_SOURCE);
        ctx->SetSource(aSurface, deviceOffset);
        ctx->Rectangle(copyRect);
        ctx->Fill();
    }

    
    
    PaintRectToSurface(rect, aSurface, gfxRGBA(0.0, 0.0, 0.0));

    
    
    gfxImageSurface *image = static_cast<gfxImageSurface*>(aSurface);
    blackImage = image->GetSubimage(targetRect);

#else
    
    whiteImage->SetDeviceOffset(deviceOffset);
    PaintRectToSurface(rect, whiteImage, gfxRGBA(1.0, 1.0, 1.0));

    if (useSurfaceSubimageForBlack) {
        gfxImageSurface *surface = static_cast<gfxImageSurface*>(aSurface);
        blackImage = surface->GetSubimage(targetRect);
    } else {
        blackImage = new gfxImageSurface(targetSize,
                                         gfxASurface::ImageFormatARGB32);
    }

    
    blackImage->SetDeviceOffset(deviceOffset);
    PaintRectToSurface(rect, blackImage, gfxRGBA(0.0, 0.0, 0.0));
#endif

    NS_ABORT_IF_FALSE(whiteImage && blackImage, "Didn't paint enough!");

    
    
    if (!gfxAlphaRecovery::RecoverAlpha(blackImage, whiteImage)) {
        return;
    }

    
    
    if (!useSurfaceSubimageForBlack) {
        nsRefPtr<gfxContext> ctx = new gfxContext(aSurface);
        ctx->SetOperator(gfxContext::OPERATOR_SOURCE);
        ctx->SetSource(blackImage);
        ctx->Rectangle(targetRect);
        ctx->Fill();
    }
}

bool
PluginInstanceChild::CanPaintOnBackground()
{
    return (mBackground &&
            mCurrentSurface &&
            mCurrentSurface->GetSize() == mBackground->GetSize());
}

bool
PluginInstanceChild::ShowPluginFrame()
{
    
    
    
    
    if (!mLayersRendering || mPendingPluginCall) {
        return false;
    }

    AutoRestore<bool> pending(mPendingPluginCall);
    mPendingPluginCall = true;

    bool temporarilyMakeVisible = !IsVisible() && !mHasPainted;
    if (temporarilyMakeVisible && mWindow.width && mWindow.height) {
        mWindow.clipRect.right = mWindow.width;
        mWindow.clipRect.bottom = mWindow.height;
    } else if (!IsVisible()) {
        
        
        
        ClearCurrentSurface();
        return true;
    }

    if (!EnsureCurrentBuffer()) {
        return false;
    }

    NS_ASSERTION(mWindow.width == (mWindow.clipRect.right - mWindow.clipRect.left) &&
                 mWindow.height == (mWindow.clipRect.bottom - mWindow.clipRect.top),
                 "Clip rect should be same size as window when using layers");

    
    
    nsIntRect rect = mAccumulatedInvalidRect;
    mAccumulatedInvalidRect.SetEmpty();

    
    
    gfxIntSize surfaceSize = mCurrentSurface->GetSize();
    rect.IntersectRect(rect,
                       nsIntRect(0, 0, surfaceSize.width, surfaceSize.height));

    if (!ReadbackDifferenceRect(rect)) {
        
        
        
        rect.SetRect(0, 0, mWindow.width, mWindow.height);
    }

    bool haveTransparentPixels =
        gfxASurface::CONTENT_COLOR_ALPHA == mCurrentSurface->GetContentType();
    PLUGIN_LOG_DEBUG(
        ("[InstanceChild][%p] Painting%s <x=%d,y=%d, w=%d,h=%d> on surface <w=%d,h=%d>",
         this, haveTransparentPixels ? " with alpha" : "",
         rect.x, rect.y, rect.width, rect.height,
         mCurrentSurface->GetSize().width, mCurrentSurface->GetSize().height));

    if (CanPaintOnBackground()) {
        PLUGIN_LOG_DEBUG(("  (on background)"));
        
        {
            nsRefPtr<gfxContext> ctx = new gfxContext(mCurrentSurface);
            ctx->SetSource(mBackground);
            ctx->SetOperator(gfxContext::OPERATOR_SOURCE);
            ctx->Rectangle(gfxRect(rect.x, rect.y, rect.width, rect.height));
            ctx->Fill();
        }
        
        
        PaintRectToSurface(rect, mCurrentSurface, gfxRGBA(0.0, 0.0, 0.0, 0.0));
    } else if (!temporarilyMakeVisible && mDoAlphaExtraction) {
        
        
        PLUGIN_LOG_DEBUG(("  (with alpha recovery)"));
        PaintRectWithAlphaExtraction(rect, mCurrentSurface);
    } else {
        PLUGIN_LOG_DEBUG(("  (onto opaque surface)"));

        
        
        
        
        nsRefPtr<gfxASurface> target =
            (temporarilyMakeVisible && mHelperSurface) ?
            mHelperSurface : mCurrentSurface;

        PaintRectToSurface(rect, target, gfxRGBA(0.0, 0.0, 0.0, 0.0));
    }
    mHasPainted = true;

    if (temporarilyMakeVisible) {
        mWindow.clipRect.right = mWindow.clipRect.bottom = 0;

        PLUGIN_LOG_DEBUG(
            ("[InstanceChild][%p] Undoing temporary clipping w=<x=%d,y=%d, w=%d,h=%d>, clip=<l=%d,t=%d,r=%d,b=%d>",
             this, mWindow.x, mWindow.y, mWindow.width, mWindow.height,
             mWindow.clipRect.left, mWindow.clipRect.top, mWindow.clipRect.right, mWindow.clipRect.bottom));

        if (mPluginIface->setwindow) {
            mPluginIface->setwindow(&mData, &mWindow);
        }

        
        
        
        
        
        mAccumulatedInvalidRect.SetRect(0, 0, mWindow.width, mWindow.height);
        return true;
    }

    NPRect r = { (uint16_t)rect.y, (uint16_t)rect.x,
                 (uint16_t)rect.YMost(), (uint16_t)rect.XMost() };
    SurfaceDescriptor currSurf;
#ifdef MOZ_X11
    if (mCurrentSurface->GetType() == gfxASurface::SurfaceTypeXlib) {
        gfxXlibSurface *xsurf = static_cast<gfxXlibSurface*>(mCurrentSurface.get());
        currSurf = SurfaceDescriptorX11(xsurf->XDrawable(), xsurf->XRenderFormat()->id,
                                        mCurrentSurface->GetSize());
        
        
        XSync(mWsInfo.display, False);
    } else
#endif
#ifdef XP_WIN
    if (SharedDIBSurface::IsSharedDIBSurface(mCurrentSurface)) {
        SharedDIBSurface* s = static_cast<SharedDIBSurface*>(mCurrentSurface.get());
        if (!mCurrentSurfaceActor) {
            base::SharedMemoryHandle handle = NULL;
            s->ShareToProcess(PluginModuleChild::current()->OtherProcess(), &handle);

            mCurrentSurfaceActor =
                SendPPluginSurfaceConstructor(handle,
                                              mCurrentSurface->GetSize(),
                                              haveTransparentPixels);
        }
        currSurf = mCurrentSurfaceActor;
        s->Flush();
    } else
#endif
    if (gfxSharedImageSurface::IsSharedImage(mCurrentSurface)) {
        currSurf = static_cast<gfxSharedImageSurface*>(mCurrentSurface.get())->GetShmem();
    } else {
        NS_RUNTIMEABORT("Surface type is not remotable");
        return false;
    }

    
    SurfaceDescriptor returnSurf;

    if (!SendShow(r, currSurf, &returnSurf)) {
        return false;
    }

    SwapSurfaces();
    mSurfaceDifferenceRect = rect;
    return true;
}

bool
PluginInstanceChild::ReadbackDifferenceRect(const nsIntRect& rect)
{
    if (!mBackSurface)
        return false;

    
    
#if defined(MOZ_X11)
    if (mBackSurface->GetType() != gfxASurface::SurfaceTypeXlib &&
        !gfxSharedImageSurface::IsSharedImage(mBackSurface))
        return false;
#elif defined(XP_WIN)
    if (!SharedDIBSurface::IsSharedDIBSurface(mBackSurface))
        return false;
#else
    return false;
#endif

    if (mCurrentSurface->GetContentType() != mBackSurface->GetContentType())
        return false;

    if (mSurfaceDifferenceRect.IsEmpty())
        return true;

    PLUGIN_LOG_DEBUG(
        ("[InstanceChild][%p] Reading back part of <x=%d,y=%d, w=%d,h=%d>",
         this, mSurfaceDifferenceRect.x, mSurfaceDifferenceRect.y,
         mSurfaceDifferenceRect.width, mSurfaceDifferenceRect.height));

    
    nsRefPtr<gfxContext> ctx = new gfxContext(mCurrentSurface);
    ctx->SetOperator(gfxContext::OPERATOR_SOURCE);
    ctx->SetSource(mBackSurface);
    
    nsIntRegion result;
    result.Sub(mSurfaceDifferenceRect, nsIntRegion(rect));
    nsIntRegionRectIterator iter(result);
    const nsIntRect* r;
    while ((r = iter.Next()) != nsnull) {
        ctx->Rectangle(GfxFromNsRect(*r));
    }
    ctx->Fill();

    return true;
}

void
PluginInstanceChild::InvalidateRectDelayed(void)
{
    if (!mCurrentInvalidateTask) {
        return;
    }

    mCurrentInvalidateTask = nsnull;
    if (mAccumulatedInvalidRect.IsEmpty()) {
        return;
    }

    if (!ShowPluginFrame()) {
        AsyncShowPluginFrame();
    }
}

void
PluginInstanceChild::AsyncShowPluginFrame(void)
{
    if (mCurrentInvalidateTask) {
        return;
    }

    mCurrentInvalidateTask =
        NewRunnableMethod(this, &PluginInstanceChild::InvalidateRectDelayed);
    MessageLoop::current()->PostTask(FROM_HERE, mCurrentInvalidateTask);
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

    if (mLayersRendering) {
        nsIntRect r(aInvalidRect->left, aInvalidRect->top,
                    aInvalidRect->right - aInvalidRect->left,
                    aInvalidRect->bottom - aInvalidRect->top);

        mAccumulatedInvalidRect.UnionRect(r, mAccumulatedInvalidRect);
        
        
        AsyncShowPluginFrame();
        return;
    }
    
    
    
    
    
    
    SendNPN_InvalidateRect(*aInvalidRect);
}

bool
PluginInstanceChild::RecvUpdateBackground(const SurfaceDescriptor& aBackground,
                                          const nsIntRect& aRect)
{
    NS_ABORT_IF_FALSE(mIsTransparent, "Only transparent plugins use backgrounds");

    if (!mBackground) {
        
        switch (aBackground.type()) {
#ifdef MOZ_X11
        case SurfaceDescriptor::TSurfaceDescriptorX11: {
            SurfaceDescriptorX11 xdesc = aBackground.get_SurfaceDescriptorX11();
            XRenderPictFormat pf;
            pf.id = xdesc.xrenderPictID();
            XRenderPictFormat *incFormat =
                XRenderFindFormat(DefaultXDisplay(), PictFormatID, &pf, 0);
            mBackground =
                new gfxXlibSurface(DefaultScreenOfDisplay(DefaultXDisplay()),
                                   xdesc.XID(), incFormat, xdesc.size());
            break;
        }
#endif
        case SurfaceDescriptor::TShmem: {
            mBackground = gfxSharedImageSurface::Open(aBackground.get_Shmem());
            break;
        }
        default:
            NS_RUNTIMEABORT("Unexpected background surface descriptor");
        }

        if (!mBackground) {
            return false;
        }

        gfxIntSize bgSize = mBackground->GetSize();
        mAccumulatedInvalidRect.UnionRect(mAccumulatedInvalidRect,
                                          nsIntRect(0, 0, bgSize.width, bgSize.height));
        AsyncShowPluginFrame();
        return true;
    }

    
    mAccumulatedInvalidRect.UnionRect(aRect, mAccumulatedInvalidRect);

    
    
    
    if (!ShowPluginFrame()) {
        NS_WARNING("Couldn't immediately repaint plugin instance");
        AsyncShowPluginFrame();
    }

    return true;
}

PPluginBackgroundDestroyerChild*
PluginInstanceChild::AllocPPluginBackgroundDestroyer()
{
    return new PluginBackgroundDestroyerChild();
}

bool
PluginInstanceChild::RecvPPluginBackgroundDestroyerConstructor(
    PPluginBackgroundDestroyerChild* aActor)
{
    
    
    
    
    
    
    
    
    
    if (mBackground) {
        gfxIntSize bgsize = mBackground->GetSize();
        mAccumulatedInvalidRect.UnionRect(
            nsIntRect(0, 0, bgsize.width, bgsize.height), mAccumulatedInvalidRect);

        
        
        mBackground = nsnull;
        AsyncShowPluginFrame();
    }

    return PPluginBackgroundDestroyerChild::Send__delete__(aActor);
}

bool
PluginInstanceChild::DeallocPPluginBackgroundDestroyer(
    PPluginBackgroundDestroyerChild* aActor)
{
    delete aActor;
    return true;
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

void
PluginInstanceChild::AsyncCall(PluginThreadCallback aFunc, void* aUserData)
{
    ChildAsyncCall* task = new ChildAsyncCall(this, aFunc, aUserData);

    {
        MutexAutoLock lock(mAsyncCallMutex);
        mPendingAsyncCalls.AppendElement(task);
    }
    ProcessChild::message_loop()->PostTask(FROM_HERE, task);
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

void
PluginInstanceChild::SwapSurfaces()
{
    nsRefPtr<gfxASurface> tmpsurf = mCurrentSurface;
#ifdef XP_WIN
    PPluginSurfaceChild* tmpactor = mCurrentSurfaceActor;
#endif

    mCurrentSurface = mBackSurface;
#ifdef XP_WIN
    mCurrentSurfaceActor = mBackSurfaceActor;
#endif

    mBackSurface = tmpsurf;
#ifdef XP_WIN
    mBackSurfaceActor = tmpactor;
#endif

    
    
    if (mCurrentSurface && mBackSurface &&
        (mCurrentSurface->GetSize() != mBackSurface->GetSize() ||
         mCurrentSurface->GetContentType() != mBackSurface->GetContentType())) {
        mCurrentSurface = nsnull;
#ifdef XP_WIN
        if (mCurrentSurfaceActor) {
            PPluginSurfaceChild::Send__delete__(mCurrentSurfaceActor);
            mCurrentSurfaceActor = NULL;
        }
#endif
    }
}

void
PluginInstanceChild::ClearCurrentSurface()
{
    mCurrentSurface = nsnull;
#ifdef XP_WIN
    if (mCurrentSurfaceActor) {
        PPluginSurfaceChild::Send__delete__(mCurrentSurfaceActor);
        mCurrentSurfaceActor = NULL;
    }
#endif
    mHelperSurface = nsnull;
}

void
PluginInstanceChild::ClearAllSurfaces()
{
    if (mBackSurface) {
        
        SurfaceDescriptor temp = null_t();
        NPRect r = { 0, 0, 1, 1 };
        SendShow(r, temp, &temp);
    }
    if (gfxSharedImageSurface::IsSharedImage(mCurrentSurface))
        DeallocShmem(static_cast<gfxSharedImageSurface*>(mCurrentSurface.get())->GetShmem());
    if (gfxSharedImageSurface::IsSharedImage(mBackSurface))
        DeallocShmem(static_cast<gfxSharedImageSurface*>(mBackSurface.get())->GetShmem());
    mCurrentSurface = nsnull;
    mBackSurface = nsnull;

#ifdef XP_WIN
    if (mCurrentSurfaceActor) {
        PPluginSurfaceChild::Send__delete__(mCurrentSurfaceActor);
        mCurrentSurfaceActor = NULL;
    }
    if (mBackSurfaceActor) {
        PPluginSurfaceChild::Send__delete__(mBackSurfaceActor);
        mBackSurfaceActor = NULL;
    }
#endif
}

bool
PluginInstanceChild::AnswerNPP_Destroy(NPError* aResult)
{
    PLUGIN_LOG_DEBUG_METHOD;
    AssertPluginThread();

#if defined(OS_WIN)
    SetProp(mPluginWindowHWND, kPluginIgnoreSubclassProperty, (HANDLE)1);
#endif

    InfallibleTArray<PBrowserStreamChild*> streams;
    ManagedPBrowserStreamChild(streams);

    
    for (PRUint32 i = 0; i < streams.Length(); ) {
        if (static_cast<BrowserStreamChild*>(streams[i])->InstanceDying())
            ++i;
        else
            streams.RemoveElementAt(i);
    }
    for (PRUint32 i = 0; i < streams.Length(); ++i)
        static_cast<BrowserStreamChild*>(streams[i])->FinishDelivery();

    mTimers.Clear();

    
    
    
    PluginModuleChild::current()->NPP_Destroy(this);
    mData.ndata = 0;

    if (mCurrentInvalidateTask) {
        mCurrentInvalidateTask->Cancel();
        mCurrentInvalidateTask = nsnull;
    }
    if (mCurrentAsyncSetWindowTask) {
        mCurrentAsyncSetWindowTask->Cancel();
        mCurrentAsyncSetWindowTask = nsnull;
    }

    ClearAllSurfaces();

    mDeletingHash = new nsTHashtable<DeletingObjectEntry>;
    mDeletingHash->Init();
    PluginModuleChild::current()->FindNPObjectsForInstance(this);

    mDeletingHash->EnumerateEntries(InvalidateObject, NULL);
    mDeletingHash->EnumerateEntries(DeleteObject, NULL);

    
    
    mCachedWindowActor = nsnull;
    mCachedElementActor = nsnull;

#if defined(OS_WIN)
    SharedSurfaceRelease();
    DestroyWinlessPopupSurrogate();
    UnhookWinlessFlashThrottle();
    DestroyPluginWindow();
#endif

    
    
    for (PRUint32 i = 0; i < mPendingAsyncCalls.Length(); ++i)
        mPendingAsyncCalls[i]->Cancel();

    mPendingAsyncCalls.Clear();

    return true;
}
