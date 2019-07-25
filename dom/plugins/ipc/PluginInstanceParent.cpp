






































#include "PluginInstanceParent.h"

#include "BrowserStreamParent.h"
#include "PluginBackgroundDestroyer.h"
#include "PluginModuleParent.h"
#include "PluginStreamParent.h"
#include "StreamNotifyParent.h"
#include "npfunctions.h"
#include "nsAutoPtr.h"
#include "mozilla/unused.h"
#include "gfxASurface.h"
#include "gfxContext.h"
#include "gfxPlatform.h"
#include "gfxSharedImageSurface.h"
#ifdef MOZ_X11
#include "gfxXlibSurface.h"
#endif
#include "gfxContext.h"
#include "gfxColor.h"
#include "gfxUtils.h"
#include "nsNPAPIPluginInstance.h"

#if defined(OS_WIN)
#include <windowsx.h>
#include "mozilla/plugins/PluginSurfaceParent.h"


extern const PRUnichar* kOOPPPluginFocusEventId;
UINT gOOPPPluginFocusEvent =
    RegisterWindowMessage(kOOPPPluginFocusEventId);
extern const PRUnichar* kFlashFullscreenClass;
#elif defined(MOZ_WIDGET_GTK2)
#include <gdk/gdk.h>
#elif defined(XP_MACOSX)
#include <ApplicationServices/ApplicationServices.h>
#endif 

using namespace mozilla::plugins;

bool
StreamNotifyParent::RecvRedirectNotifyResponse(const bool& allow)
{
  PluginInstanceParent* instance = static_cast<PluginInstanceParent*>(Manager());
  instance->mNPNIface->urlredirectresponse(instance->mNPP, this, static_cast<NPBool>(allow));
  return true;
}

PluginInstanceParent::PluginInstanceParent(PluginModuleParent* parent,
                                           NPP npp,
                                           const nsCString& aMimeType,
                                           const NPNetscapeFuncs* npniface)
  : mParent(parent)
    , mNPP(npp)
    , mNPNIface(npniface)
    , mWindowType(NPWindowTypeWindow)
#if defined(OS_WIN)
    , mPluginHWND(NULL)
    , mPluginWndProc(NULL)
    , mNestedEventState(false)
#endif 
    , mQuirks(0)
#if defined(XP_MACOSX)
    , mShWidth(0)
    , mShHeight(0)
    , mShColorSpace(nsnull)
    , mDrawingModel(NPDrawingModelCoreGraphics)
#endif
{
    InitQuirksModes(aMimeType);
}

void
PluginInstanceParent::InitQuirksModes(const nsCString& aMimeType)
{
#ifdef MOZ_WIDGET_COCOA
    NS_NAMED_LITERAL_CSTRING(flash, "application/x-shockwave-flash");
    
    
    if (!FindInReadable(flash, aMimeType)) {
        mQuirks |= COREANIMATION_REFRESH_TIMER;
    }
#endif
}

PluginInstanceParent::~PluginInstanceParent()
{
    if (mNPP)
        mNPP->pdata = NULL;

#if defined(OS_WIN)
    NS_ASSERTION(!(mPluginHWND || mPluginWndProc),
        "Subclass was not reset correctly before the dtor was reached!");
#endif
#if defined(MOZ_WIDGET_COCOA)
    if (mShWidth != 0 && mShHeight != 0) {
        DeallocShmem(mShSurface);
    }
    if (mShColorSpace)
        ::CGColorSpaceRelease(mShColorSpace);
    if (mDrawingModel == NPDrawingModelCoreAnimation) {
        mParent->RemoveFromRefreshTimer(this);
    }
#endif
}

bool
PluginInstanceParent::Init()
{
    return !!mScriptableObjects.Init();
}

namespace {

PLDHashOperator
ActorCollect(const void* aKey,
             PluginScriptableObjectParent* aData,
             void* aUserData)
{
    nsTArray<PluginScriptableObjectParent*>* objects =
        reinterpret_cast<nsTArray<PluginScriptableObjectParent*>*>(aUserData);
    return objects->AppendElement(aData) ? PL_DHASH_NEXT : PL_DHASH_STOP;
}

} 

void
PluginInstanceParent::ActorDestroy(ActorDestroyReason why)
{
#if defined(OS_WIN)
    if (why == AbnormalShutdown) {
        
        
        SharedSurfaceRelease();
        UnsubclassPluginWindow();
    }
#endif
    
    
    
    
    
    if (mFrontSurface) {
        mFrontSurface = NULL;
        const NPRect rect = {0, 0, 0, 0};
        RecvNPN_InvalidateRect(rect);
#ifdef MOZ_X11
        XSync(DefaultXDisplay(), False);
#endif
    }
}

NPError
PluginInstanceParent::Destroy()
{
    NPError retval;
    if (!CallNPP_Destroy(&retval))
        retval = NPERR_GENERIC_ERROR;

#if defined(OS_WIN)
    SharedSurfaceRelease();
    UnsubclassPluginWindow();
#endif

    return retval;
}

PBrowserStreamParent*
PluginInstanceParent::AllocPBrowserStream(const nsCString& url,
                                          const uint32_t& length,
                                          const uint32_t& lastmodified,
                                          PStreamNotifyParent* notifyData,
                                          const nsCString& headers,
                                          const nsCString& mimeType,
                                          const bool& seekable,
                                          NPError* rv,
                                          uint16_t *stype)
{
    NS_RUNTIMEABORT("Not reachable");
    return NULL;
}

bool
PluginInstanceParent::DeallocPBrowserStream(PBrowserStreamParent* stream)
{
    delete stream;
    return true;
}

PPluginStreamParent*
PluginInstanceParent::AllocPPluginStream(const nsCString& mimeType,
                                         const nsCString& target,
                                         NPError* result)
{
    return new PluginStreamParent(this, mimeType, target, result);
}

bool
PluginInstanceParent::DeallocPPluginStream(PPluginStreamParent* stream)
{
    delete stream;
    return true;
}

bool
PluginInstanceParent::AnswerNPN_GetValue_NPNVjavascriptEnabledBool(
                                                       bool* value,
                                                       NPError* result)
{
    NPBool v;
    *result = mNPNIface->getvalue(mNPP, NPNVjavascriptEnabledBool, &v);
    *value = v;
    return true;
}

bool
PluginInstanceParent::AnswerNPN_GetValue_NPNVisOfflineBool(bool* value,
                                                           NPError* result)
{
    NPBool v;
    *result = mNPNIface->getvalue(mNPP, NPNVisOfflineBool, &v);
    *value = v;
    return true;
}

bool
PluginInstanceParent::AnswerNPN_GetValue_NPNVnetscapeWindow(NativeWindowHandle* value,
                                                            NPError* result)
{
#ifdef XP_WIN
    HWND id;
#elif defined(MOZ_X11)
    XID id;
#elif defined(XP_MACOSX)
    intptr_t id;
#elif defined(ANDROID)
#warning Need Android impl
    int id;
#else
#warning Implement me
#endif

    *result = mNPNIface->getvalue(mNPP, NPNVnetscapeWindow, &id);
    *value = id;
    return true;
}

bool
PluginInstanceParent::InternalGetValueForNPObject(
                                         NPNVariable aVariable,
                                         PPluginScriptableObjectParent** aValue,
                                         NPError* aResult)
{
    NPObject* npobject;
    NPError result = mNPNIface->getvalue(mNPP, aVariable, (void*)&npobject);
    if (result == NPERR_NO_ERROR) {
        NS_ASSERTION(npobject, "Shouldn't return null and NPERR_NO_ERROR!");

        PluginScriptableObjectParent* actor = GetActorForNPObject(npobject);
        mNPNIface->releaseobject(npobject);
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
PluginInstanceParent::AnswerNPN_GetValue_NPNVWindowNPObject(
                                         PPluginScriptableObjectParent** aValue,
                                         NPError* aResult)
{
    return InternalGetValueForNPObject(NPNVWindowNPObject, aValue, aResult);
}

bool
PluginInstanceParent::AnswerNPN_GetValue_NPNVPluginElementNPObject(
                                         PPluginScriptableObjectParent** aValue,
                                         NPError* aResult)
{
    return InternalGetValueForNPObject(NPNVPluginElementNPObject, aValue,
                                       aResult);
}

bool
PluginInstanceParent::AnswerNPN_GetValue_NPNVprivateModeBool(bool* value,
                                                             NPError* result)
{
    NPBool v;
    *result = mNPNIface->getvalue(mNPP, NPNVprivateModeBool, &v);
    *value = v;
    return true;
}

bool
PluginInstanceParent::AnswerNPN_SetValue_NPPVpluginWindow(
    const bool& windowed, NPError* result)
{
    NPBool isWindowed = windowed;
    *result = mNPNIface->setvalue(mNPP, NPPVpluginWindowBool,
                                  (void*)isWindowed);
    return true;
}

bool
PluginInstanceParent::AnswerNPN_SetValue_NPPVpluginTransparent(
    const bool& transparent, NPError* result)
{
    NPBool isTransparent = transparent;
    *result = mNPNIface->setvalue(mNPP, NPPVpluginTransparentBool,
                                  (void*)isTransparent);
    return true;
}

bool
PluginInstanceParent::AnswerNPN_SetValue_NPPVpluginUsesDOMForCursor(
    const bool& useDOMForCursor, NPError* result)
{
    *result = mNPNIface->setvalue(mNPP, NPPVpluginUsesDOMForCursorBool,
                                  (void*)(NPBool)useDOMForCursor);
    return true;
}

bool
PluginInstanceParent::AnswerNPN_SetValue_NPPVpluginDrawingModel(
    const int& drawingModel, NPError* result)
{
#ifdef XP_MACOSX
    if (drawingModel == NPDrawingModelCoreAnimation ||
        drawingModel == NPDrawingModelInvalidatingCoreAnimation) {
        
        
        
        mDrawingModel = drawingModel;
        *result = mNPNIface->setvalue(mNPP, NPPVpluginDrawingModel,
                                  (void*)NPDrawingModelCoreGraphics);
        if (drawingModel == NPDrawingModelCoreAnimation &&
            mQuirks & COREANIMATION_REFRESH_TIMER) {
            mParent->AddToRefreshTimer(this);
        }
    } else {
        mDrawingModel = drawingModel;
        *result = mNPNIface->setvalue(mNPP, NPPVpluginDrawingModel,
                                  (void*)drawingModel);
    }
    return true;
#else
    *result = NPERR_GENERIC_ERROR;
    return true;
#endif
}

bool
PluginInstanceParent::AnswerNPN_SetValue_NPPVpluginEventModel(
    const int& eventModel, NPError* result)
{
#ifdef XP_MACOSX
    *result = mNPNIface->setvalue(mNPP, NPPVpluginEventModel,
                                  (void*)eventModel);
    return true;
#else
    *result = NPERR_GENERIC_ERROR;
    return true;
#endif
}

bool
PluginInstanceParent::AnswerNPN_GetURL(const nsCString& url,
                                       const nsCString& target,
                                       NPError* result)
{
    *result = mNPNIface->geturl(mNPP,
                                NullableStringGet(url),
                                NullableStringGet(target));
    return true;
}

bool
PluginInstanceParent::AnswerNPN_PostURL(const nsCString& url,
                                        const nsCString& target,
                                        const nsCString& buffer,
                                        const bool& file,
                                        NPError* result)
{
    *result = mNPNIface->posturl(mNPP, url.get(), NullableStringGet(target),
                                 buffer.Length(), buffer.get(), file);
    return true;
}

PStreamNotifyParent*
PluginInstanceParent::AllocPStreamNotify(const nsCString& url,
                                         const nsCString& target,
                                         const bool& post,
                                         const nsCString& buffer,
                                         const bool& file,
                                         NPError* result)
{
    return new StreamNotifyParent();
}

bool
PluginInstanceParent::AnswerPStreamNotifyConstructor(PStreamNotifyParent* actor,
                                                     const nsCString& url,
                                                     const nsCString& target,
                                                     const bool& post,
                                                     const nsCString& buffer,
                                                     const bool& file,
                                                     NPError* result)
{
    bool streamDestroyed = false;
    static_cast<StreamNotifyParent*>(actor)->
        SetDestructionFlag(&streamDestroyed);

    if (!post) {
        *result = mNPNIface->geturlnotify(mNPP,
                                          NullableStringGet(url),
                                          NullableStringGet(target),
                                          actor);
    }
    else {
        *result = mNPNIface->posturlnotify(mNPP,
                                           NullableStringGet(url),
                                           NullableStringGet(target),
                                           buffer.Length(),
                                           NullableStringGet(buffer),
                                           file, actor);
    }

    if (!streamDestroyed) {
        static_cast<StreamNotifyParent*>(actor)->ClearDestructionFlag();
        if (*result != NPERR_NO_ERROR)
            return PStreamNotifyParent::Send__delete__(actor,
                                                       NPERR_GENERIC_ERROR);
    }

    return true;
}

bool
PluginInstanceParent::DeallocPStreamNotify(PStreamNotifyParent* notifyData)
{
    delete notifyData;
    return true;
}

bool
PluginInstanceParent::RecvNPN_InvalidateRect(const NPRect& rect)
{
    mNPNIface->invalidaterect(mNPP, const_cast<NPRect*>(&rect));
    return true;
}

bool
PluginInstanceParent::RecvShow(const NPRect& updatedRect,
                               const SurfaceDescriptor& newSurface,
                               SurfaceDescriptor* prevSurface)
{
    PLUGIN_LOG_DEBUG(
        ("[InstanceParent][%p] RecvShow for <x=%d,y=%d, w=%d,h=%d>",
         this, updatedRect.left, updatedRect.top,
         updatedRect.right - updatedRect.left,
         updatedRect.bottom - updatedRect.top));

    nsRefPtr<gfxASurface> surface;
    if (newSurface.type() == SurfaceDescriptor::TShmem) {
        if (!newSurface.get_Shmem().IsReadable()) {
            NS_WARNING("back surface not readable");
            return false;
        }
        surface = gfxSharedImageSurface::Open(newSurface.get_Shmem());
    }
#ifdef XP_MACOSX
    else if (newSurface.type() == SurfaceDescriptor::TIOSurfaceDescriptor) {
        IOSurfaceDescriptor iodesc = newSurface.get_IOSurfaceDescriptor();
    
        nsIOSurface *newIOSurface = nsIOSurface::LookupSurface(iodesc.surfaceId());

        if (!newIOSurface) {
            NS_WARNING("Got bad IOSurfaceDescriptor in RecvShow");
            return false;
        }
      
        mIOSurface = newIOSurface;

        RecvNPN_InvalidateRect(updatedRect);

        *prevSurface = null_t();

        PLUGIN_LOG_DEBUG(("   (RecvShow invalidated for surface %p)",
                          mFrontSurface.get()));

        return true;
    }
#endif
#ifdef MOZ_X11
    else if (newSurface.type() == SurfaceDescriptor::TSurfaceDescriptorX11) {
        SurfaceDescriptorX11 xdesc = newSurface.get_SurfaceDescriptorX11();
        XRenderPictFormat pf;
        pf.id = xdesc.xrenderPictID();
        XRenderPictFormat *incFormat =
            XRenderFindFormat(DefaultXDisplay(), PictFormatID, &pf, 0);
        surface =
            new gfxXlibSurface(DefaultScreenOfDisplay(DefaultXDisplay()),
                               xdesc.XID(), incFormat, xdesc.size());
    }
#endif
#ifdef XP_WIN
    else if (newSurface.type() == SurfaceDescriptor::TPPluginSurfaceParent) {
        PluginSurfaceParent* s =
            static_cast<PluginSurfaceParent*>(newSurface.get_PPluginSurfaceParent());
        surface = s->Surface();
    }
#endif

#ifdef MOZ_X11
    if (mFrontSurface &&
        mFrontSurface->GetType() == gfxASurface::SurfaceTypeXlib)
        
        
        
        
        
        XSync(DefaultXDisplay(), False);
#endif

    if (mFrontSurface && gfxSharedImageSurface::IsSharedImage(mFrontSurface))
        *prevSurface = static_cast<gfxSharedImageSurface*>(mFrontSurface.get())->GetShmem();
    else
        *prevSurface = null_t();

    if (surface) {
        
        
        gfxRect ur(updatedRect.left, updatedRect.top,
                   updatedRect.right - updatedRect.left,
                   updatedRect.bottom - updatedRect.top);
        surface->MarkDirty(ur);
        surface->Flush();
    }

    mFrontSurface = surface;
    RecvNPN_InvalidateRect(updatedRect);

    PLUGIN_LOG_DEBUG(("   (RecvShow invalidated for surface %p)",
                      mFrontSurface.get()));

    return true;
}

nsresult
PluginInstanceParent::AsyncSetWindow(NPWindow* aWindow)
{
    NPRemoteWindow window;
    mWindowType = aWindow->type;
    window.window = reinterpret_cast<uintptr_t>(aWindow->window);
    window.x = aWindow->x;
    window.y = aWindow->y;
    window.width = aWindow->width;
    window.height = aWindow->height;
    window.clipRect = aWindow->clipRect;
    window.type = aWindow->type;
    if (!SendAsyncSetWindow(gfxPlatform::GetPlatform()->ScreenReferenceSurface()->GetType(),
                            window))
        return NS_ERROR_FAILURE;

    return NS_OK;
}

nsresult
PluginInstanceParent::GetImage(ImageContainer* aContainer, Image** aImage)
{
#ifdef XP_MACOSX
    if (!mFrontSurface && !mIOSurface)
#else
    if (!mFrontSurface)
#endif
        return NS_ERROR_NOT_AVAILABLE;

    Image::Format format = Image::CAIRO_SURFACE;
#ifdef XP_MACOSX
    if (mIOSurface) {
        format = Image::MAC_IO_SURFACE;
        if (!aContainer->Manager()) {
            return NS_ERROR_FAILURE;
        }
    }
#endif

    nsRefPtr<Image> image;
    image = aContainer->CreateImage(&format, 1);
    if (!image) {
        return NS_ERROR_FAILURE;
    }

#ifdef XP_MACOSX
    if (mIOSurface) {
        NS_ASSERTION(image->GetFormat() == Image::MAC_IO_SURFACE, "Wrong format?");
        MacIOSurfaceImage* ioImage = static_cast<MacIOSurfaceImage*>(image.get());
        MacIOSurfaceImage::Data ioData;
        ioData.mIOSurface = mIOSurface;
        ioImage->SetData(ioData);
        *aImage = image.forget().get();
        return NS_OK;
    }
#endif

    NS_ASSERTION(image->GetFormat() == Image::CAIRO_SURFACE, "Wrong format?");
    CairoImage* pluginImage = static_cast<CairoImage*>(image.get());
    CairoImage::Data cairoData;
    cairoData.mSurface = mFrontSurface;
    cairoData.mSize = mFrontSurface->GetSize();
    pluginImage->SetData(cairoData);

    *aImage = image.forget().get();
    return NS_OK;
}

nsresult
PluginInstanceParent::GetImageSize(nsIntSize* aSize)
{
    if (mFrontSurface) {
        gfxIntSize size = mFrontSurface->GetSize();
        *aSize = nsIntSize(size.width, size.height);
        return NS_OK;
    }

#ifdef XP_MACOSX
    if (mIOSurface) {
        *aSize = nsIntSize(mIOSurface->GetWidth(), mIOSurface->GetHeight());
        return NS_OK;
    }
#endif

    return NS_ERROR_NOT_AVAILABLE;
}

nsresult
PluginInstanceParent::SetBackgroundUnknown()
{
    PLUGIN_LOG_DEBUG(("[InstanceParent][%p] SetBackgroundUnknown", this));

    if (mBackground) {
        DestroyBackground();
        NS_ABORT_IF_FALSE(!mBackground, "Background not destroyed");
    }

    return NS_OK;
}

nsresult
PluginInstanceParent::BeginUpdateBackground(const nsIntRect& aRect,
                                            gfxContext** aCtx)
{
    PLUGIN_LOG_DEBUG(
        ("[InstanceParent][%p] BeginUpdateBackground for <x=%d,y=%d, w=%d,h=%d>",
         this, aRect.x, aRect.y, aRect.width, aRect.height));

    if (!mBackground) {
        
        
        
        
        NS_ABORT_IF_FALSE(aRect.TopLeft() == nsIntPoint(0, 0),
                          "Expecting rect for whole frame");
        if (!CreateBackground(aRect.Size())) {
            *aCtx = nsnull;
            return NS_OK;
        }
    }

#ifdef DEBUG
    gfxIntSize sz = mBackground->GetSize();
    NS_ABORT_IF_FALSE(nsIntRect(0, 0, sz.width, sz.height).Contains(aRect),
                      "Update outside of background area");
#endif

    nsRefPtr<gfxContext> ctx = new gfxContext(mBackground);
    *aCtx = ctx.forget().get();

    return NS_OK;
}

nsresult
PluginInstanceParent::EndUpdateBackground(gfxContext* aCtx,
                                          const nsIntRect& aRect)
{
    PLUGIN_LOG_DEBUG(
        ("[InstanceParent][%p] EndUpdateBackground for <x=%d,y=%d, w=%d,h=%d>",
         this, aRect.x, aRect.y, aRect.width, aRect.height));

#ifdef MOZ_X11
    
    
    
    
    
    
    
    
    
    XSync(DefaultXDisplay(), False);
#endif

    unused << SendUpdateBackground(BackgroundDescriptor(), aRect);

    return NS_OK;
}

bool
PluginInstanceParent::CreateBackground(const nsIntSize& aSize)
{
    NS_ABORT_IF_FALSE(!mBackground, "Already have a background");

    

#if defined(MOZ_X11)
    Screen* screen = DefaultScreenOfDisplay(DefaultXDisplay());
    Visual* visual = DefaultVisualOfScreen(screen);
    mBackground = gfxXlibSurface::Create(screen, visual,
                                         gfxIntSize(aSize.width, aSize.height));
    return !!mBackground;

#elif defined(XP_WIN)
    
    
    mBackground =
        gfxSharedImageSurface::CreateUnsafe(
            this,
            gfxIntSize(aSize.width, aSize.height),
            gfxASurface::ImageFormatRGB24);
    return !!mBackground;
#else
    return nsnull;
#endif
}

void
PluginInstanceParent::DestroyBackground()
{
    if (!mBackground) {
        return;
    }

    
    PPluginBackgroundDestroyerParent* pbd =
        new PluginBackgroundDestroyerParent(mBackground);
    mBackground = nsnull;

    
    
    unused << SendPPluginBackgroundDestroyerConstructor(pbd);
}

SurfaceDescriptor
PluginInstanceParent::BackgroundDescriptor()
{
    NS_ABORT_IF_FALSE(mBackground, "Need a background here");

    

#ifdef MOZ_X11
    gfxXlibSurface* xsurf = static_cast<gfxXlibSurface*>(mBackground.get());
    return SurfaceDescriptorX11(xsurf->XDrawable(), xsurf->XRenderFormat()->id,
                                xsurf->GetSize());
#endif

#ifdef XP_WIN
    NS_ABORT_IF_FALSE(gfxSharedImageSurface::IsSharedImage(mBackground),
                      "Expected shared image surface");
    gfxSharedImageSurface* shmem =
        static_cast<gfxSharedImageSurface*>(mBackground.get());
    return shmem->GetShmem();
#endif

    
    
    return SurfaceDescriptor();
}

PPluginBackgroundDestroyerParent*
PluginInstanceParent::AllocPPluginBackgroundDestroyer()
{
    NS_RUNTIMEABORT("'Power-user' ctor is used exclusively");
    return nsnull;
}

bool
PluginInstanceParent::DeallocPPluginBackgroundDestroyer(
    PPluginBackgroundDestroyerParent* aActor)
{
    delete aActor;
    return true;
}

NPError
PluginInstanceParent::NPP_SetWindow(const NPWindow* aWindow)
{
    PLUGIN_LOG_DEBUG(("%s (aWindow=%p)", FULLFUNCTION, (void*) aWindow));

    NS_ENSURE_TRUE(aWindow, NPERR_GENERIC_ERROR);

    NPRemoteWindow window;
    mWindowType = aWindow->type;

#if defined(OS_WIN)
    
    if (mWindowType == NPWindowTypeDrawable) {
        
        if (!SharedSurfaceSetWindow(aWindow, window)) {
          return NPERR_OUT_OF_MEMORY_ERROR;
        }
    }
    else {
        SubclassPluginWindow(reinterpret_cast<HWND>(aWindow->window));

        window.window = reinterpret_cast<uintptr_t>(aWindow->window);
        window.x = aWindow->x;
        window.y = aWindow->y;
        window.width = aWindow->width;
        window.height = aWindow->height;
        window.type = aWindow->type;
    }
#else
    window.window = reinterpret_cast<unsigned long>(aWindow->window);
    window.x = aWindow->x;
    window.y = aWindow->y;
    window.width = aWindow->width;
    window.height = aWindow->height;
    window.clipRect = aWindow->clipRect; 
    window.type = aWindow->type;
#endif

#if defined(XP_MACOSX)
    if (mShWidth != window.width || mShHeight != window.height) {
        if (mDrawingModel == NPDrawingModelCoreAnimation || 
            mDrawingModel == NPDrawingModelInvalidatingCoreAnimation) {
            mIOSurface = nsIOSurface::CreateIOSurface(window.width, window.height);
        } else if (mShWidth * mShHeight != window.width * window.height) {
            if (mShWidth != 0 && mShHeight != 0) {
                DeallocShmem(mShSurface);
                mShWidth = 0;
                mShHeight = 0;
            }

            if (window.width != 0 && window.height != 0) {
                if (!AllocShmem(window.width * window.height*4, 
                                SharedMemory::TYPE_BASIC, &mShSurface)) {
                    PLUGIN_LOG_DEBUG(("Shared memory could not be allocated."));
                    return NPERR_GENERIC_ERROR;
                } 
            }
        }
        mShWidth = window.width;
        mShHeight = window.height;
    }
#endif

#if defined(MOZ_X11) && defined(XP_UNIX) && !defined(XP_MACOSX)
    const NPSetWindowCallbackStruct* ws_info =
      static_cast<NPSetWindowCallbackStruct*>(aWindow->ws_info);
    window.visualID = ws_info->visual ? ws_info->visual->visualid : None;
    window.colormap = ws_info->colormap;
#endif

    if (!CallNPP_SetWindow(window))
        return NPERR_GENERIC_ERROR;

    return NPERR_NO_ERROR;
}

NPError
PluginInstanceParent::NPP_GetValue(NPPVariable aVariable,
                                   void* _retval)
{
    switch (aVariable) {

    case NPPVpluginWantsAllNetworkStreams: {
        bool wantsAllStreams;
        NPError rv;

        if (!CallNPP_GetValue_NPPVpluginWantsAllNetworkStreams(&wantsAllStreams, &rv)) {
            return NPERR_GENERIC_ERROR;
        }

        if (NPERR_NO_ERROR != rv) {
            return rv;
        }

        (*(NPBool*)_retval) = wantsAllStreams;
        return NPERR_NO_ERROR;
    }

#ifdef MOZ_X11
    case NPPVpluginNeedsXEmbed: {
        bool needsXEmbed;
        NPError rv;

        if (!CallNPP_GetValue_NPPVpluginNeedsXEmbed(&needsXEmbed, &rv)) {
            return NPERR_GENERIC_ERROR;
        }

        if (NPERR_NO_ERROR != rv) {
            return rv;
        }

        (*(NPBool*)_retval) = needsXEmbed;
        return NPERR_NO_ERROR;
    }
#endif

    case NPPVpluginScriptableNPObject: {
        PPluginScriptableObjectParent* actor;
        NPError rv;
        if (!CallNPP_GetValue_NPPVpluginScriptableNPObject(&actor, &rv)) {
            return NPERR_GENERIC_ERROR;
        }

        if (NPERR_NO_ERROR != rv) {
            return rv;
        }

        if (!actor) {
            NS_ERROR("NPPVpluginScriptableNPObject succeeded but null.");
            return NPERR_GENERIC_ERROR;
        }

        const NPNetscapeFuncs* npn = mParent->GetNetscapeFuncs();
        if (!npn) {
            NS_WARNING("No netscape functions?!");
            return NPERR_GENERIC_ERROR;
        }

        NPObject* object =
            static_cast<PluginScriptableObjectParent*>(actor)->GetObject(true);
        NS_ASSERTION(object, "This shouldn't ever be null!");

        (*(NPObject**)_retval) = npn->retainobject(object);
        return NPERR_NO_ERROR;
    }

#ifdef MOZ_ACCESSIBILITY_ATK
    case NPPVpluginNativeAccessibleAtkPlugId: {
        nsCString plugId;
        NPError rv;
        if (!CallNPP_GetValue_NPPVpluginNativeAccessibleAtkPlugId(&plugId, &rv)) {
            return NPERR_GENERIC_ERROR;
        }

        if (NPERR_NO_ERROR != rv) {
            return rv;
        }

        (*(nsCString*)_retval) = plugId;
        return NPERR_NO_ERROR;
    }
#endif

    default:
        PR_LOG(gPluginLog, PR_LOG_WARNING,
               ("In PluginInstanceParent::NPP_GetValue: Unhandled NPPVariable %i (%s)",
                (int) aVariable, NPPVariableToString(aVariable)));
        return NPERR_GENERIC_ERROR;
    }
}

NPError
PluginInstanceParent::NPP_SetValue(NPNVariable variable, void* value)
{
    switch (variable) {
    case NPNVprivateModeBool:
        NPError result;
        if (!CallNPP_SetValue_NPNVprivateModeBool(*static_cast<NPBool*>(value),
                                                  &result))
            return NPERR_GENERIC_ERROR;

        return result;

    default:
        NS_ERROR("Unhandled NPNVariable in NPP_SetValue");
        PR_LOG(gPluginLog, PR_LOG_WARNING,
               ("In PluginInstanceParent::NPP_SetValue: Unhandled NPNVariable %i (%s)",
                (int) variable, NPNVariableToString(variable)));
        return NPERR_GENERIC_ERROR;
    }
}

void
PluginInstanceParent::NPP_URLRedirectNotify(const char* url, int32_t status,
                                            void* notifyData)
{
  if (!notifyData)
    return;

  PStreamNotifyParent* streamNotify = static_cast<PStreamNotifyParent*>(notifyData);
  unused << streamNotify->SendRedirectNotify(NullableString(url), status);
}

int16_t
PluginInstanceParent::NPP_HandleEvent(void* event)
{
    PLUGIN_LOG_DEBUG_FUNCTION;

#if defined(XP_MACOSX)
    NPCocoaEvent* npevent = reinterpret_cast<NPCocoaEvent*>(event);
#else
    NPEvent* npevent = reinterpret_cast<NPEvent*>(event);
#endif
    NPRemoteEvent npremoteevent;
    npremoteevent.event = *npevent;
    int16_t handled = 0;

#if defined(OS_WIN)
    if (mWindowType == NPWindowTypeDrawable) {
        if (DoublePassRenderingEvent() == npevent->event) {
            CallPaint(npremoteevent, &handled);
            return handled;
        }

        switch (npevent->event) {
            case WM_PAINT:
            {
                RECT rect;
                SharedSurfaceBeforePaint(rect, npremoteevent);
                CallPaint(npremoteevent, &handled);
                SharedSurfaceAfterPaint(npevent);
                return handled;
            }
            break;

            case WM_KILLFOCUS:
            {
              
              
              
              
              
              
              PRUnichar szClass[26];
              HWND hwnd = GetForegroundWindow();
              if (hwnd && hwnd != mPluginHWND &&
                  GetClassNameW(hwnd, szClass,
                                sizeof(szClass)/sizeof(PRUnichar)) &&
                  !wcscmp(szClass, kFlashFullscreenClass)) {
                  return 0;
              }
            }
            break;

            case WM_WINDOWPOSCHANGED:
            {
                
                SendWindowPosChanged(npremoteevent);
                
                
                return 1;
            }
            break;
        }
    }
#endif

#if defined(MOZ_X11)
    switch (npevent->type) {
    case GraphicsExpose:
        PLUGIN_LOG_DEBUG(("  schlepping drawable 0x%lx across the pipe\n",
                          npevent->xgraphicsexpose.drawable));
        
        
        
        
        
        
        
        XSync(DefaultXDisplay(), False);

        return CallPaint(npremoteevent, &handled) ? handled : 0;

    case ButtonPress:
        
        
        Display *dpy = DefaultXDisplay();
#  ifdef MOZ_WIDGET_GTK2
        
        
        gdk_pointer_ungrab(npevent->xbutton.time);
#  else
        XUngrabPointer(dpy, npevent->xbutton.time);
#  endif
        
        XSync(dpy, False);
        break;
    }
#endif

#ifdef XP_MACOSX
    if (npevent->type == NPCocoaEventDrawRect) {
        if (mDrawingModel == NPDrawingModelCoreAnimation ||
            mDrawingModel == NPDrawingModelInvalidatingCoreAnimation) {
            if (!mIOSurface) {
                NS_ERROR("No IOSurface allocated.");
                return false;
            }
            if (!CallNPP_HandleEvent_IOSurface(npremoteevent, 
                                               mIOSurface->GetIOSurfaceID(), 
                                               &handled)) 
                return false; 

            CGContextRef cgContext = npevent->data.draw.context;
            if (!mShColorSpace) {
                mShColorSpace = CreateSystemColorSpace();
            }
            if (!mShColorSpace) {
                PLUGIN_LOG_DEBUG(("Could not allocate ColorSpace."));
                return false;
            }
            if (cgContext) {
                nsCARenderer::DrawSurfaceToCGContext(cgContext, mIOSurface, 
                                                     mShColorSpace,
                                                     npevent->data.draw.x,
                                                     npevent->data.draw.y,
                                                     npevent->data.draw.width,
                                                     npevent->data.draw.height);
            }
            return false;
        } else {
            if (mShWidth == 0 && mShHeight == 0) {
                PLUGIN_LOG_DEBUG(("NPCocoaEventDrawRect on window of size 0."));
                return false;
            }
            if (!mShSurface.IsReadable()) {
                PLUGIN_LOG_DEBUG(("Shmem is not readable."));
                return false;
            }

            if (!CallNPP_HandleEvent_Shmem(npremoteevent, mShSurface, 
                                           &handled, &mShSurface)) 
                return false; 

            if (!mShSurface.IsReadable()) {
                PLUGIN_LOG_DEBUG(("Shmem not returned. Either the plugin crashed "
                                  "or we have a bug."));
                return false;
            }

            char* shContextByte = mShSurface.get<char>();

            if (!mShColorSpace) {
                mShColorSpace = CreateSystemColorSpace();
            }
            if (!mShColorSpace) {
                PLUGIN_LOG_DEBUG(("Could not allocate ColorSpace."));
                return false;
            } 
            CGContextRef shContext = ::CGBitmapContextCreate(shContextByte, 
                                    mShWidth, mShHeight, 8, 
                                    mShWidth*4, mShColorSpace, 
                                    kCGImageAlphaPremultipliedFirst | 
                                    kCGBitmapByteOrder32Host);
            if (!shContext) {
                PLUGIN_LOG_DEBUG(("Could not allocate CGBitmapContext."));
                return false;
            }

            CGImageRef shImage = ::CGBitmapContextCreateImage(shContext);
            if (shImage) {
                CGContextRef cgContext = npevent->data.draw.context;
     
                ::CGContextDrawImage(cgContext, 
                                     CGRectMake(0,0,mShWidth,mShHeight), 
                                     shImage);
                ::CGImageRelease(shImage);
            } else {
                ::CGContextRelease(shContext);
                return false;
            }
            ::CGContextRelease(shContext);
            return true;
        }
    }
#endif

    if (!CallNPP_HandleEvent(npremoteevent, &handled))
        return 0; 

    return handled;
}

NPError
PluginInstanceParent::NPP_NewStream(NPMIMEType type, NPStream* stream,
                                    NPBool seekable, uint16_t* stype)
{
    PLUGIN_LOG_DEBUG(("%s (type=%s, stream=%p, seekable=%i)",
                      FULLFUNCTION, (char*) type, (void*) stream, (int) seekable));

    BrowserStreamParent* bs = new BrowserStreamParent(this, stream);

    NPError err;
    if (!CallPBrowserStreamConstructor(bs,
                                       NullableString(stream->url),
                                       stream->end,
                                       stream->lastmodified,
                                       static_cast<PStreamNotifyParent*>(stream->notifyData),
                                       NullableString(stream->headers),
                                       NullableString(type), seekable,
                                       &err, stype))
        return NPERR_GENERIC_ERROR;

    if (NPERR_NO_ERROR != err)
        unused << PBrowserStreamParent::Send__delete__(bs);

    return err;
}

NPError
PluginInstanceParent::NPP_DestroyStream(NPStream* stream, NPReason reason)
{
    PLUGIN_LOG_DEBUG(("%s (stream=%p, reason=%i)",
                      FULLFUNCTION, (void*) stream, (int) reason));

    AStream* s = static_cast<AStream*>(stream->pdata);
    if (s->IsBrowserStream()) {
        BrowserStreamParent* sp =
            static_cast<BrowserStreamParent*>(s);
        if (sp->mNPP != this)
            NS_RUNTIMEABORT("Mismatched plugin data");

        sp->NPP_DestroyStream(reason);
        return NPERR_NO_ERROR;
    }
    else {
        PluginStreamParent* sp =
            static_cast<PluginStreamParent*>(s);
        if (sp->mInstance != this)
            NS_RUNTIMEABORT("Mismatched plugin data");

        return PPluginStreamParent::Call__delete__(sp, reason, false) ?
            NPERR_NO_ERROR : NPERR_GENERIC_ERROR;
    }
}

void
PluginInstanceParent::NPP_Print(NPPrint* platformPrint)
{
    
    NS_ERROR("Not implemented");
}

PPluginScriptableObjectParent*
PluginInstanceParent::AllocPPluginScriptableObject()
{
    return new PluginScriptableObjectParent(Proxy);
}

#ifdef DEBUG
namespace {

struct ActorSearchData
{
    PluginScriptableObjectParent* actor;
    bool found;
};

PLDHashOperator
ActorSearch(const void* aKey,
            PluginScriptableObjectParent* aData,
            void* aUserData)
{
    ActorSearchData* asd = reinterpret_cast<ActorSearchData*>(aUserData);
    if (asd->actor == aData) {
        asd->found = true;
        return PL_DHASH_STOP;
    }
    return PL_DHASH_NEXT;
}

} 
#endif 

bool
PluginInstanceParent::DeallocPPluginScriptableObject(
                                         PPluginScriptableObjectParent* aObject)
{
    PluginScriptableObjectParent* actor =
        static_cast<PluginScriptableObjectParent*>(aObject);

    NPObject* object = actor->GetObject(false);
    if (object) {
        NS_ASSERTION(mScriptableObjects.Get(object, nsnull),
                     "NPObject not in the hash!");
        mScriptableObjects.Remove(object);
    }
#ifdef DEBUG
    else {
        ActorSearchData asd = { actor, false };
        mScriptableObjects.EnumerateRead(ActorSearch, &asd);
        NS_ASSERTION(!asd.found, "Actor in the hash with a null NPObject!");
    }
#endif

    delete actor;
    return true;
}

bool
PluginInstanceParent::RecvPPluginScriptableObjectConstructor(
                                          PPluginScriptableObjectParent* aActor)
{
    
    
    
    PluginScriptableObjectParent* actor =
        static_cast<PluginScriptableObjectParent*>(aActor);
    NS_ASSERTION(!actor->GetObject(false), "Actor already has an object?!");

    actor->InitializeProxy();
    NS_ASSERTION(actor->GetObject(false), "Actor should have an object!");

    return true;
}

void
PluginInstanceParent::NPP_URLNotify(const char* url, NPReason reason,
                                    void* notifyData)
{
    PLUGIN_LOG_DEBUG(("%s (%s, %i, %p)",
                      FULLFUNCTION, url, (int) reason, notifyData));

    PStreamNotifyParent* streamNotify =
        static_cast<PStreamNotifyParent*>(notifyData);
    unused << PStreamNotifyParent::Send__delete__(streamNotify, reason);
}

bool
PluginInstanceParent::RegisterNPObjectForActor(
                                           NPObject* aObject,
                                           PluginScriptableObjectParent* aActor)
{
    NS_ASSERTION(aObject && aActor, "Null pointers!");
    NS_ASSERTION(mScriptableObjects.IsInitialized(), "Hash not initialized!");
    NS_ASSERTION(!mScriptableObjects.Get(aObject, nsnull), "Duplicate entry!");
    return !!mScriptableObjects.Put(aObject, aActor);
}

void
PluginInstanceParent::UnregisterNPObject(NPObject* aObject)
{
    NS_ASSERTION(aObject, "Null pointer!");
    NS_ASSERTION(mScriptableObjects.IsInitialized(), "Hash not initialized!");
    NS_ASSERTION(mScriptableObjects.Get(aObject, nsnull), "Unknown entry!");
    mScriptableObjects.Remove(aObject);
}

PluginScriptableObjectParent*
PluginInstanceParent::GetActorForNPObject(NPObject* aObject)
{
    NS_ASSERTION(aObject, "Null pointer!");

    if (aObject->_class == PluginScriptableObjectParent::GetClass()) {
        
        ParentNPObject* object = static_cast<ParentNPObject*>(aObject);
        NS_ASSERTION(object->parent, "Null actor!");
        return object->parent;
    }

    PluginScriptableObjectParent* actor;
    if (mScriptableObjects.Get(aObject, &actor)) {
        return actor;
    }

    actor = new PluginScriptableObjectParent(LocalObject);
    if (!actor) {
        NS_ERROR("Out of memory!");
        return nsnull;
    }

    if (!SendPPluginScriptableObjectConstructor(actor)) {
        NS_WARNING("Failed to send constructor message!");
        return nsnull;
    }

    actor->InitializeLocal(aObject);
    return actor;
}

PPluginSurfaceParent*
PluginInstanceParent::AllocPPluginSurface(const WindowsSharedMemoryHandle& handle,
                                          const gfxIntSize& size,
                                          const bool& transparent)
{
#ifdef XP_WIN
    return new PluginSurfaceParent(handle, size, transparent);
#else
    NS_ERROR("This shouldn't be called!");
    return NULL;
#endif
}

bool
PluginInstanceParent::DeallocPPluginSurface(PPluginSurfaceParent* s)
{
#ifdef XP_WIN
    delete s;
    return true;
#else
    return false;
#endif
}

bool
PluginInstanceParent::AnswerNPN_PushPopupsEnabledState(const bool& aState)
{
    mNPNIface->pushpopupsenabledstate(mNPP, aState ? 1 : 0);
    return true;
}

bool
PluginInstanceParent::AnswerNPN_PopPopupsEnabledState()
{
    mNPNIface->poppopupsenabledstate(mNPP);
    return true;
}

bool
PluginInstanceParent::AnswerNPN_GetValueForURL(const NPNURLVariable& variable,
                                               const nsCString& url,
                                               nsCString* value,
                                               NPError* result)
{
    char* v;
    uint32_t len;

    *result = mNPNIface->getvalueforurl(mNPP, (NPNURLVariable) variable,
                                        url.get(), &v, &len);
    if (NPERR_NO_ERROR == *result)
        value->Adopt(v, len);

    return true;
}

bool
PluginInstanceParent::AnswerNPN_SetValueForURL(const NPNURLVariable& variable,
                                               const nsCString& url,
                                               const nsCString& value,
                                               NPError* result)
{
    *result = mNPNIface->setvalueforurl(mNPP, (NPNURLVariable) variable,
                                        url.get(), value.get(),
                                        value.Length());
    return true;
}

bool
PluginInstanceParent::AnswerNPN_GetAuthenticationInfo(const nsCString& protocol,
                                                      const nsCString& host,
                                                      const int32_t& port,
                                                      const nsCString& scheme,
                                                      const nsCString& realm,
                                                      nsCString* username,
                                                      nsCString* password,
                                                      NPError* result)
{
    char* u;
    uint32_t ulen;
    char* p;
    uint32_t plen;

    *result = mNPNIface->getauthenticationinfo(mNPP, protocol.get(),
                                               host.get(), port,
                                               scheme.get(), realm.get(),
                                               &u, &ulen, &p, &plen);
    if (NPERR_NO_ERROR == *result) {
        username->Adopt(u, ulen);
        password->Adopt(p, plen);
    }
    return true;
}

bool
PluginInstanceParent::AnswerNPN_ConvertPoint(const double& sourceX,
                                             const bool&   ignoreDestX,
                                             const double& sourceY,
                                             const bool&   ignoreDestY,
                                             const NPCoordinateSpace& sourceSpace,
                                             const NPCoordinateSpace& destSpace,
                                             double *destX,
                                             double *destY,
                                             bool *result)
{
    *result = mNPNIface->convertpoint(mNPP, sourceX, sourceY, sourceSpace,
                                      ignoreDestX ? nsnull : destX,
                                      ignoreDestY ? nsnull : destY,
                                      destSpace);

    return true;
}

bool
PluginInstanceParent::RecvNegotiatedCarbon()
{
  nsNPAPIPluginInstance *inst = static_cast<nsNPAPIPluginInstance*>(mNPP->ndata);
  if (!inst) {
    return false;
  }
  inst->CarbonNPAPIFailure();
  return true;
}

#if defined(OS_WIN)















static const PRUnichar kPluginInstanceParentProperty[] =
                         L"PluginInstanceParentProperty";


LRESULT CALLBACK
PluginInstanceParent::PluginWindowHookProc(HWND hWnd,
                                           UINT message,
                                           WPARAM wParam,
                                           LPARAM lParam)
{
    PluginInstanceParent* self = reinterpret_cast<PluginInstanceParent*>(
        ::GetPropW(hWnd, kPluginInstanceParentProperty));
    if (!self) {
        NS_NOTREACHED("PluginInstanceParent::PluginWindowHookProc null this ptr!");
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    NS_ASSERTION(self->mPluginHWND == hWnd, "Wrong window!");

    switch (message) {
        case WM_SETFOCUS:
        
        self->CallSetPluginFocus();
        break;

        case WM_CLOSE:
        self->UnsubclassPluginWindow();
        break;
    }

    if (self->mPluginWndProc == PluginWindowHookProc) {
      NS_NOTREACHED(
        "PluginWindowHookProc invoking mPluginWndProc w/"
        "mPluginWndProc == PluginWindowHookProc????");
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return ::CallWindowProc(self->mPluginWndProc, hWnd, message, wParam,
                            lParam);
}

void
PluginInstanceParent::SubclassPluginWindow(HWND aWnd)
{
    NS_ASSERTION(!(mPluginHWND && aWnd != mPluginHWND),
      "PluginInstanceParent::SubclassPluginWindow hwnd is not our window!");

    if (!mPluginHWND) {
        mPluginHWND = aWnd;
        mPluginWndProc = 
            (WNDPROC)::SetWindowLongPtrA(mPluginHWND, GWLP_WNDPROC,
                         reinterpret_cast<LONG_PTR>(PluginWindowHookProc));
        bool bRes = ::SetPropW(mPluginHWND, kPluginInstanceParentProperty, this);
        NS_ASSERTION(mPluginWndProc,
          "PluginInstanceParent::SubclassPluginWindow failed to set subclass!");
        NS_ASSERTION(bRes,
          "PluginInstanceParent::SubclassPluginWindow failed to set prop!");
   }
}

void
PluginInstanceParent::UnsubclassPluginWindow()
{
    if (mPluginHWND && mPluginWndProc) {
        ::SetWindowLongPtrA(mPluginHWND, GWLP_WNDPROC,
                            reinterpret_cast<LONG_PTR>(mPluginWndProc));

        ::RemovePropW(mPluginHWND, kPluginInstanceParentProperty);

        mPluginWndProc = NULL;
        mPluginHWND = NULL;
    }
}























void
PluginInstanceParent::SharedSurfaceRelease()
{
    mSharedSurfaceDib.Close();
}

bool
PluginInstanceParent::SharedSurfaceSetWindow(const NPWindow* aWindow,
                                             NPRemoteWindow& aRemoteWindow)
{
    aRemoteWindow.window = nsnull;
    aRemoteWindow.x      = aWindow->x;
    aRemoteWindow.y      = aWindow->y;
    aRemoteWindow.width  = aWindow->width;
    aRemoteWindow.height = aWindow->height;
    aRemoteWindow.type   = aWindow->type;

    nsIntRect newPort(aWindow->x, aWindow->y, aWindow->width, aWindow->height);

    
    mPluginPort = newPort;

    
    newPort.MoveTo(0,0);

    
    if (mSharedSurfaceDib.IsValid() && mSharedSize.Contains(newPort)) {
      
      aRemoteWindow.surfaceHandle = 0;
      return true;
    }

    
    SharedSurfaceRelease();
    if (NS_FAILED(mSharedSurfaceDib.Create(reinterpret_cast<HDC>(aWindow->window),
                                           newPort.width, newPort.height, false)))
      return false;

    
    mSharedSize = newPort;

    base::SharedMemoryHandle handle;
    if (NS_FAILED(mSharedSurfaceDib.ShareToProcess(mParent->ChildProcessHandle(), &handle)))
      return false;

    aRemoteWindow.surfaceHandle = handle;

    return true;
}

void
PluginInstanceParent::SharedSurfaceBeforePaint(RECT& rect,
                                               NPRemoteEvent& npremoteevent)
{
    RECT* dr = (RECT*)npremoteevent.event.lParam;
    HDC parentHdc = (HDC)npremoteevent.event.wParam;

    nsIntRect dirtyRect(dr->left, dr->top, dr->right-dr->left, dr->bottom-dr->top);
    dirtyRect.MoveBy(-mPluginPort.x, -mPluginPort.y); 

    ::BitBlt(mSharedSurfaceDib.GetHDC(),
             dirtyRect.x,
             dirtyRect.y,
             dirtyRect.width,
             dirtyRect.height,
             parentHdc,
             dr->left,
             dr->top,
             SRCCOPY);

    
    rect.left   = dirtyRect.x;
    rect.top    = dirtyRect.y;
    rect.right  = dirtyRect.x + dirtyRect.width;
    rect.bottom = dirtyRect.y + dirtyRect.height;

    npremoteevent.event.wParam = WPARAM(0);
    npremoteevent.event.lParam = LPARAM(&rect);
}

void
PluginInstanceParent::SharedSurfaceAfterPaint(NPEvent* npevent)
{
    RECT* dr = (RECT*)npevent->lParam;
    HDC parentHdc = (HDC)npevent->wParam;

    nsIntRect dirtyRect(dr->left, dr->top, dr->right-dr->left, dr->bottom-dr->top);
    dirtyRect.MoveBy(-mPluginPort.x, -mPluginPort.y);

    
    ::BitBlt(parentHdc,
             dr->left,
             dr->top,
             dirtyRect.width,
             dirtyRect.height,
             mSharedSurfaceDib.GetHDC(),
             dirtyRect.x,
             dirtyRect.y,
             SRCCOPY);
}

#endif 

bool
PluginInstanceParent::AnswerPluginFocusChange(const bool& gotFocus)
{
    PLUGIN_LOG_DEBUG(("%s", FULLFUNCTION));

    
    
    
    
#if defined(OS_WIN)
    ::SendMessage(mPluginHWND, gOOPPPluginFocusEvent, gotFocus ? 1 : 0, 0);
    return true;
#else
    NS_NOTREACHED("PluginInstanceParent::AnswerPluginFocusChange not implemented!");
    return false;
#endif
}

#ifdef MOZ_WIDGET_COCOA
void
PluginInstanceParent::Invalidate()
{
    NPRect windowRect = {0, 0, mShHeight, mShWidth};
    RecvNPN_InvalidateRect(windowRect);
}
#endif
