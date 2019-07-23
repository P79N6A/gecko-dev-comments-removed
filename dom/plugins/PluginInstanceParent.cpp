






































#include "PluginInstanceParent.h"

#include "BrowserStreamParent.h"
#include "PluginModuleParent.h"
#include "PluginStreamParent.h"
#include "StreamNotifyParent.h"

#include "npfunctions.h"
#include "nsAutoPtr.h"

#if defined(OS_WIN)
#define NS_OOPP_DOUBLEPASS_MSGID TEXT("MozDoublePassMsg")
#endif

using namespace mozilla::plugins;

PluginInstanceParent::PluginInstanceParent(PluginModuleParent* parent,
                                           NPP npp,
                                           const NPNetscapeFuncs* npniface)
  : mParent(parent),
    mNPP(npp),
    mNPNIface(npniface),
    mWindowType(NPWindowTypeWindow)
{
#if defined(OS_WIN)
    
    
    
    
    mDoublePassEvent = ::RegisterWindowMessage(NS_OOPP_DOUBLEPASS_MSGID);
    mLocalCopyRender = false;
#endif
}

PluginInstanceParent::~PluginInstanceParent()
{
    if (mNPP)
        mNPP->pdata = NULL;
}

void
PluginInstanceParent::Destroy()
{
    
    nsAutoTArray<PluginScriptableObjectParent*, 10> objects;
    PRUint32 count = mScriptableObjects.Length();
    for (PRUint32 index = 0; index < count; index++) {
        objects.AppendElement(mScriptableObjects[index]);
    }

    count = objects.Length();
    for (PRUint32 index = 0; index < count; index++) {
        NPObject* object = objects[index]->GetObject();
        if (object->_class == PluginScriptableObjectParent::GetClass()) {
          PluginScriptableObjectParent::ScriptableInvalidate(object);
        }
    }

#if defined(OS_WIN)
    SharedSurfaceRelease();
#endif
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

    if (*result != NPERR_NO_ERROR)
        PStreamNotifyParent::Call__delete__(actor, NPERR_GENERIC_ERROR);

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

NPError
PluginInstanceParent::NPP_SetWindow(const NPWindow* aWindow)
{
    _MOZ_LOG(__FUNCTION__);
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
        window.window = reinterpret_cast<unsigned long>(aWindow->window);
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

#if defined(MOZ_X11) && defined(XP_UNIX) && !defined(XP_MACOSX)
    const NPSetWindowCallbackStruct* ws_info =
      static_cast<NPSetWindowCallbackStruct*>(aWindow->ws_info);
    window.visualID = ws_info->visual ? ws_info->visual->visualid : None;
    window.colormap = ws_info->colormap;
#endif

    NPError prv;
    if (!CallNPP_SetWindow(window, &prv))
        return NPERR_GENERIC_ERROR;
    return prv;
}

NPError
PluginInstanceParent::NPP_GetValue(NPPVariable aVariable,
                                   void* _retval)
{
    printf("[PluginInstanceParent] NPP_GetValue(%s)\n",
           NPPVariableToString(aVariable));

    switch (aVariable) {

    case NPPVpluginWindowBool: {
        bool windowed;
        NPError rv;

        if (!CallNPP_GetValue_NPPVpluginWindow(&windowed, &rv)) {
            return NPERR_GENERIC_ERROR;
        }

        if (NPERR_NO_ERROR != rv) {
            return rv;
        }

        (*(NPBool*)_retval) = windowed;
        return NPERR_NO_ERROR;
    }

    case NPPVpluginTransparentBool: {
        bool transparent;
        NPError rv;

        if (!CallNPP_GetValue_NPPVpluginTransparent(&transparent, &rv)) {
            return NPERR_GENERIC_ERROR;
        }

        if (NPERR_NO_ERROR != rv) {
            return rv;
        }

        (*(NPBool*)_retval) = transparent;
        return NPERR_NO_ERROR;
    }

#ifdef OS_LINUX
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
            static_cast<PluginScriptableObjectParent*>(actor)->GetObject();
        NS_ASSERTION(object, "This shouldn't ever be null!");

        (*(NPObject**)_retval) = npn->retainobject(object);
        return NPERR_NO_ERROR;
    }

    default:
        printf("  unhandled var %s\n", NPPVariableToString(aVariable));
        return NPERR_GENERIC_ERROR;
    }
}

int16_t
PluginInstanceParent::NPP_HandleEvent(void* event)
{
    _MOZ_LOG(__FUNCTION__);

    NPEvent* npevent = reinterpret_cast<NPEvent*>(event);
    NPRemoteEvent npremoteevent;
    npremoteevent.event = *npevent;

#if defined(OS_WIN)
    RECT rect;
    if (mWindowType == NPWindowTypeDrawable) {
        if (mDoublePassEvent && mDoublePassEvent == npevent->event) {
            
            mLocalCopyRender = PR_TRUE;
            return true;
        } else if (WM_PAINT == npevent->event) {
            
            if (!SharedSurfaceBeforePaint(rect, npremoteevent))
                return true;
        }
    }
#endif

#if defined(MOZ_X11)
    if (GraphicsExpose == npevent->type) {
        printf("  schlepping drawable 0x%lx across the pipe\n",
               npevent->xgraphicsexpose.drawable);
        
        
        
        
        
        
        
#  ifdef MOZ_WIDGET_GTK2
        XSync(GDK_DISPLAY(), False);
#  endif
    }
#endif

    int16_t handled;
    if (!CallNPP_HandleEvent(npremoteevent, &handled)) {
        return 0;               
    }

#if defined(OS_WIN)
    if (handled && mWindowType == NPWindowTypeDrawable && WM_PAINT == npevent->event)
        SharedSurfaceAfterPaint(npevent);
#endif

    return handled;
}

NPError
PluginInstanceParent::NPP_NewStream(NPMIMEType type, NPStream* stream,
                                    NPBool seekable, uint16_t* stype)
{
    _MOZ_LOG(__FUNCTION__);

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
        PBrowserStreamParent::Call__delete__(bs, NPERR_GENERIC_ERROR, true);

    return err;
}

NPError
PluginInstanceParent::NPP_DestroyStream(NPStream* stream, NPReason reason)
{
    _MOZ_LOG(__FUNCTION__);

    AStream* s = static_cast<AStream*>(stream->pdata);
    if (s->IsBrowserStream()) {
        BrowserStreamParent* sp =
            static_cast<BrowserStreamParent*>(s);
        if (sp->mNPP != this)
            NS_RUNTIMEABORT("Mismatched plugin data");

        PBrowserStreamParent::Call__delete__(sp, reason, false);
        return NPERR_NO_ERROR;
    }
    else {
        PluginStreamParent* sp =
            static_cast<PluginStreamParent*>(s);
        if (sp->mInstance != this)
            NS_RUNTIMEABORT("Mismatched plugin data");

        PPluginStreamParent::Call__delete__(sp, reason, false);
        return NPERR_NO_ERROR;
    }
}

PPluginScriptableObjectParent*
PluginInstanceParent::AllocPPluginScriptableObject()
{
    nsAutoPtr<PluginScriptableObjectParent>* object =
        mScriptableObjects.AppendElement();
    NS_ENSURE_TRUE(object, nsnull);

    *object = new PluginScriptableObjectParent();
    NS_ENSURE_TRUE(*object, nsnull);

    return object->get();
}

bool
PluginInstanceParent::DeallocPPluginScriptableObject(
                                         PPluginScriptableObjectParent* aObject)
{
    PluginScriptableObjectParent* object =
        reinterpret_cast<PluginScriptableObjectParent*>(aObject);

    PRUint32 count = mScriptableObjects.Length();
    for (PRUint32 index = 0; index < count; index++) {
        if (mScriptableObjects[index] == object) {
            mScriptableObjects.RemoveElementAt(index);
            return true;
        }
    }
    NS_NOTREACHED("An actor we don't know about?!");
    return false;
}

bool
PluginInstanceParent::AnswerPPluginScriptableObjectConstructor(
                                          PPluginScriptableObjectParent* aActor)
{
    
    
    
    const NPNetscapeFuncs* npn = mParent->GetNetscapeFuncs();
    if (!npn) {
        NS_WARNING("No netscape function pointers?!");
        return false;
    }

    NPClass* npclass =
        const_cast<NPClass*>(PluginScriptableObjectParent::GetClass());

    ParentNPObject* object = reinterpret_cast<ParentNPObject*>(
        npn->createobject(mNPP, npclass));
    if (!object) {
        NS_WARNING("Failed to create NPObject!");
        return false;
    }

    static_cast<PluginScriptableObjectParent*>(aActor)->Initialize(
        const_cast<PluginInstanceParent*>(this), object);
    return true;
}

void
PluginInstanceParent::NPP_URLNotify(const char* url, NPReason reason,
                                    void* notifyData)
{
    _MOZ_LOG(__FUNCTION__);

    PStreamNotifyParent* streamNotify =
        static_cast<PStreamNotifyParent*>(notifyData);
    PStreamNotifyParent::Call__delete__(streamNotify, reason);
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

    PRUint32 count = mScriptableObjects.Length();
    for (PRUint32 index = 0; index < count; index++) {
        nsAutoPtr<PluginScriptableObjectParent>& actor =
            mScriptableObjects[index];
        if (actor->GetObject() == aObject) {
            return actor;
        }
    }

    PluginScriptableObjectParent* actor =
        static_cast<PluginScriptableObjectParent*>(
            CallPPluginScriptableObjectConstructor());
    NS_ENSURE_TRUE(actor, nsnull);

    actor->Initialize(const_cast<PluginInstanceParent*>(this), aObject);
    return actor;
}

bool
PluginInstanceParent::AnswerNPN_PushPopupsEnabledState(const bool& aState,
                                                       bool* aSuccess)
{
    *aSuccess = mNPNIface->pushpopupsenabledstate(mNPP, aState ? 1 : 0);
    return true;
}

bool
PluginInstanceParent::AnswerNPN_PopPopupsEnabledState(bool* aSuccess)
{
    *aSuccess = mNPNIface->poppopupsenabledstate(mNPP);
    return true;
}

#if defined(OS_WIN)



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
    aRemoteWindow.x      = 0;
    aRemoteWindow.y      = 0;
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
                                           newPort.width, newPort.height, 32)))
      return false;

    
    mSharedSize = newPort;
    
    base::SharedMemoryHandle handle;
    if (NS_FAILED(mSharedSurfaceDib.ShareToProcess(mParent->ChildProcessHandle(), &handle)))
      return false;

    aRemoteWindow.surfaceHandle = handle;
    
    return true;
}

bool
PluginInstanceParent::SharedSurfaceBeforePaint(RECT& rect,
                                               NPRemoteEvent& npremoteevent)
{
    RECT* dr = (RECT*)npremoteevent.event.lParam;
    HDC parentHdc = (HDC)npremoteevent.event.wParam;

    
    
    
    
    
    
    if (mLocalCopyRender) {
      mLocalCopyRender = false;
      
      SharedSurfaceAfterPaint(&npremoteevent.event);
      return false;
    }

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
    rect.right  = dirtyRect.width;
    rect.bottom = dirtyRect.height;

    npremoteevent.event.wParam = WPARAM(0);
    npremoteevent.event.lParam = LPARAM(&rect);

    
    return true;
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
