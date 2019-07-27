





#include "nsDragService.h"
#include "nsIObserverService.h"
#include "nsWidgetsCID.h"
#include "nsWindow.h"
#include "nsIServiceManager.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsIIOService.h"
#include "nsIFileURL.h"
#include "nsNetUtil.h"
#include "prlog.h"
#include "nsTArray.h"
#include "nsPrimitiveHelpers.h"
#include "prtime.h"
#include "prthread.h"
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include "nsCRT.h"
#include "mozilla/BasicEvents.h"
#include "mozilla/Services.h"

#include "gfxASurface.h"
#include "gfxXlibSurface.h"
#include "gfxContext.h"
#include "nsImageToPixbuf.h"
#include "nsPresContext.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsISelection.h"
#include "nsViewManager.h"
#include "nsIFrame.h"
#include "nsGtkUtils.h"
#include "mozilla/gfx/2D.h"
#include "gfxPlatform.h"

using namespace mozilla;
using namespace mozilla::gfx;


#define DRAG_IMAGE_ALPHA_LEVEL 0.5





enum {
  MOZ_GTK_DRAG_RESULT_SUCCESS,
  MOZ_GTK_DRAG_RESULT_NO_TARGET
};

static PRLogModuleInfo *sDragLm = nullptr;



static guint sMotionEventTimerID;
static GdkEvent *sMotionEvent;
static GtkWidget *sGrabWidget;

static const char gMimeListType[] = "application/x-moz-internal-item-list";
static const char gMozUrlType[] = "_NETSCAPE_URL";
static const char gTextUriListType[] = "text/uri-list";
static const char gTextPlainUTF8Type[] = "text/plain;charset=utf-8";

static void
invisibleSourceDragBegin(GtkWidget        *aWidget,
                         GdkDragContext   *aContext,
                         gpointer          aData);

static void
invisibleSourceDragEnd(GtkWidget        *aWidget,
                       GdkDragContext   *aContext,
                       gpointer          aData);

static gboolean
invisibleSourceDragFailed(GtkWidget        *aWidget,
                          GdkDragContext   *aContext,
                          gint              aResult,
                          gpointer          aData);

static void
invisibleSourceDragDataGet(GtkWidget        *aWidget,
                           GdkDragContext   *aContext,
                           GtkSelectionData *aSelectionData,
                           guint             aInfo,
                           guint32           aTime,
                           gpointer          aData);

nsDragService::nsDragService()
    : mScheduledTask(eDragTaskNone)
    , mTaskSource(0)
{
    
    
    nsCOMPtr<nsIObserverService> obsServ =
        mozilla::services::GetObserverService();
    obsServ->AddObserver(this, "quit-application", false);

    
    mHiddenWidget = gtk_window_new(GTK_WINDOW_POPUP);
    
    
    gtk_widget_realize(mHiddenWidget);
    
    
    g_signal_connect(mHiddenWidget, "drag_begin",
                     G_CALLBACK(invisibleSourceDragBegin), this);
    g_signal_connect(mHiddenWidget, "drag_data_get",
                     G_CALLBACK(invisibleSourceDragDataGet), this);
    g_signal_connect(mHiddenWidget, "drag_end",
                     G_CALLBACK(invisibleSourceDragEnd), this);
    
    guint dragFailedID = g_signal_lookup("drag-failed",
                                         G_TYPE_FROM_INSTANCE(mHiddenWidget));
    if (dragFailedID) {
        g_signal_connect_closure_by_id(mHiddenWidget, dragFailedID, 0,
                                       g_cclosure_new(G_CALLBACK(invisibleSourceDragFailed),
                                                      this, nullptr),
                                       FALSE);
    }

    
    if (!sDragLm)
        sDragLm = PR_NewLogModule("nsDragService");
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("nsDragService::nsDragService"));
    mCanDrop = false;
    mTargetDragDataReceived = false;
    mTargetDragData = 0;
    mTargetDragDataLen = 0;
}

nsDragService::~nsDragService()
{
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("nsDragService::~nsDragService"));
    if (mTaskSource)
        g_source_remove(mTaskSource);

}

NS_IMPL_ISUPPORTS_INHERITED(nsDragService, nsBaseDragService, nsIObserver)

 nsDragService*
nsDragService::GetInstance()
{
    static const nsIID iid = NS_DRAGSERVICE_CID;
    nsCOMPtr<nsIDragService> dragService = do_GetService(iid);
    return static_cast<nsDragService*>(dragService.get());
    
}



NS_IMETHODIMP
nsDragService::Observe(nsISupports *aSubject, const char *aTopic,
                       const char16_t *aData)
{
  if (!nsCRT::strcmp(aTopic, "quit-application")) {
    PR_LOG(sDragLm, PR_LOG_DEBUG,
           ("nsDragService::Observe(\"quit-application\")"));
    if (mHiddenWidget) {
      gtk_widget_destroy(mHiddenWidget);
      mHiddenWidget = 0;
    }
    TargetResetData();
  } else {
    NS_NOTREACHED("unexpected topic");
    return NS_ERROR_UNEXPECTED;
  }

  return NS_OK;
}


















static gboolean
DispatchMotionEventCopy(gpointer aData)
{
    
    
    sMotionEventTimerID = 0;

    GdkEvent *event = sMotionEvent;
    sMotionEvent = nullptr;
    
    
    if (gtk_widget_has_grab(sGrabWidget)) {
        gtk_propagate_event(sGrabWidget, event);
    }
    gdk_event_free(event);

    
    
    return FALSE;
}

static void
OnSourceGrabEventAfter(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
    
    
    if (!gtk_widget_has_grab(sGrabWidget))
        return;

    if (event->type == GDK_MOTION_NOTIFY) {
        if (sMotionEvent) {
            gdk_event_free(sMotionEvent);
        }
        sMotionEvent = gdk_event_copy(event);

        
        
        nsDragService *dragService = static_cast<nsDragService*>(user_data);
        dragService->SetDragEndPoint(nsIntPoint(event->motion.x_root,
                                                event->motion.y_root));
    } else if (sMotionEvent && (event->type == GDK_KEY_PRESS ||
                                event->type == GDK_KEY_RELEASE)) {
        
        sMotionEvent->motion.state = event->key.state;
    } else {
        return;
    }

    if (sMotionEventTimerID) {
        g_source_remove(sMotionEventTimerID);
    }

    
    
    
    
    
    
    sMotionEventTimerID = 
        g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE, 350,
                           DispatchMotionEventCopy, nullptr, nullptr);
}

static GtkWindow*
GetGtkWindow(nsIDOMDocument *aDocument)
{
    nsCOMPtr<nsIDocument> doc = do_QueryInterface(aDocument);
    if (!doc)
        return nullptr;

    nsCOMPtr<nsIPresShell> presShell = doc->GetShell();
    if (!presShell)
        return nullptr;

    nsRefPtr<nsViewManager> vm = presShell->GetViewManager();
    if (!vm)
        return nullptr;

    nsCOMPtr<nsIWidget> widget;
    vm->GetRootWidget(getter_AddRefs(widget));
    if (!widget)
        return nullptr;

    GtkWidget *gtkWidget =
        static_cast<nsWindow*>(widget.get())->GetMozContainerWidget();
    if (!gtkWidget)
        return nullptr;

    GtkWidget *toplevel = nullptr;
    toplevel = gtk_widget_get_toplevel(gtkWidget);
    if (!GTK_IS_WINDOW(toplevel))
        return nullptr;

    return GTK_WINDOW(toplevel);
}   



NS_IMETHODIMP
nsDragService::InvokeDragSession(nsIDOMNode *aDOMNode,
                                 nsISupportsArray * aArrayTransferables,
                                 nsIScriptableRegion * aRegion,
                                 uint32_t aActionType)
{
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("nsDragService::InvokeDragSession"));

    
    
    
    
    if (mSourceNode)
        return NS_ERROR_NOT_AVAILABLE;

    nsresult rv = nsBaseDragService::InvokeDragSession(aDOMNode,
                                                       aArrayTransferables,
                                                       aRegion, aActionType);
    NS_ENSURE_SUCCESS(rv, rv);

    
    if (!aArrayTransferables)
        return NS_ERROR_INVALID_ARG;
    
    
    
    mSourceDataItems = aArrayTransferables;
    
    GtkTargetList *sourceList = GetSourceList();

    if (!sourceList)
        return NS_OK;

    
    mSourceRegion = aRegion;

    
    GdkDragAction action = GDK_ACTION_DEFAULT;

    if (aActionType & DRAGDROP_ACTION_COPY)
        action = (GdkDragAction)(action | GDK_ACTION_COPY);
    if (aActionType & DRAGDROP_ACTION_MOVE)
        action = (GdkDragAction)(action | GDK_ACTION_MOVE);
    if (aActionType & DRAGDROP_ACTION_LINK)
        action = (GdkDragAction)(action | GDK_ACTION_LINK);

    
    
    
    
    GdkEvent event;
    memset(&event, 0, sizeof(GdkEvent));
    event.type = GDK_BUTTON_PRESS;
    event.button.window = gtk_widget_get_window(mHiddenWidget);
    event.button.time = nsWindow::GetLastUserInputTime();

    
    
    
    GtkWindowGroup *window_group =
        gtk_window_get_group(GetGtkWindow(mSourceDocument));
    gtk_window_group_add_window(window_group,
                                GTK_WINDOW(mHiddenWidget));

#if (MOZ_WIDGET_GTK == 3)
    
    GdkDisplay *display = gdk_display_get_default();
    GdkDeviceManager *device_manager = gdk_display_get_device_manager(display);
    event.button.device = gdk_device_manager_get_client_pointer(device_manager);
#endif
  
    
    GdkDragContext *context = gtk_drag_begin(mHiddenWidget,
                                             sourceList,
                                             action,
                                             1,
                                             &event);

    mSourceRegion = nullptr;

    if (context) {
        StartDragSession();

        
        sGrabWidget = gtk_window_group_get_current_grab(window_group);
        if (sGrabWidget) {
            g_object_ref(sGrabWidget);
            
            
            g_signal_connect(sGrabWidget, "event-after",
                             G_CALLBACK(OnSourceGrabEventAfter), this);
        }
        
        mEndDragPoint = nsIntPoint(-1, -1);
    }
    else {
        rv = NS_ERROR_FAILURE;
    }

    gtk_target_list_unref(sourceList);

    return rv;
}

bool
nsDragService::SetAlphaPixmap(SourceSurface *aSurface,
                              GdkDragContext *aContext,
                              int32_t aXOffset,
                              int32_t aYOffset,
                              const nsIntRect& dragRect)
{
#if (MOZ_WIDGET_GTK == 2)
    GdkScreen* screen = gtk_widget_get_screen(mHiddenWidget);

    
    
    if (!gdk_screen_is_composited(screen))
      return false;

    GdkColormap* alphaColormap = gdk_screen_get_rgba_colormap(screen);
    if (!alphaColormap)
      return false;

    GdkPixmap* pixmap = gdk_pixmap_new(nullptr, dragRect.width, dragRect.height,
                                       gdk_colormap_get_visual(alphaColormap)->depth);
    if (!pixmap)
      return false;

    gdk_drawable_set_colormap(GDK_DRAWABLE(pixmap), alphaColormap);

    
    nsRefPtr<gfxASurface> xPixmapSurface =
         nsWindow::GetSurfaceForGdkDrawable(GDK_DRAWABLE(pixmap),
                                            dragRect.Size());
    if (!xPixmapSurface)
      return false;

    RefPtr<DrawTarget> dt =
    gfxPlatform::GetPlatform()->
      CreateDrawTargetForSurface(xPixmapSurface, IntSize(dragRect.width, dragRect.height));
    if (!dt)
      return false;

    
    dt->ClearRect(Rect(0, 0, dragRect.width, dragRect.height));

    
    dt->DrawSurface(aSurface,
                    Rect(0, 0, dragRect.width, dragRect.height),
                    Rect(0, 0, dragRect.width, dragRect.height),
                    DrawSurfaceOptions(),
                    DrawOptions(DRAG_IMAGE_ALPHA_LEVEL, CompositionOp::OP_SOURCE));

    
    gtk_drag_set_icon_pixmap(aContext, alphaColormap, pixmap, nullptr,
                             aXOffset, aYOffset);
    g_object_unref(pixmap);
    return true;
#else
    
    return false;
#endif
}

NS_IMETHODIMP
nsDragService::StartDragSession()
{
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("nsDragService::StartDragSession"));
    return nsBaseDragService::StartDragSession();
}
 
NS_IMETHODIMP
nsDragService::EndDragSession(bool aDoneDrag)
{
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("nsDragService::EndDragSession %d",
                                   aDoneDrag));

    if (sGrabWidget) {
        g_signal_handlers_disconnect_by_func(sGrabWidget,
             FuncToGpointer(OnSourceGrabEventAfter), this);
        g_object_unref(sGrabWidget);
        sGrabWidget = nullptr;

        if (sMotionEventTimerID) {
            g_source_remove(sMotionEventTimerID);
            sMotionEventTimerID = 0;
        }
        if (sMotionEvent) {
            gdk_event_free(sMotionEvent);
            sMotionEvent = nullptr;
        }
    }

    
    SetDragAction(DRAGDROP_ACTION_NONE);
    return nsBaseDragService::EndDragSession(aDoneDrag);
}


NS_IMETHODIMP
nsDragService::SetCanDrop(bool aCanDrop)
{
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("nsDragService::SetCanDrop %d",
                                   aCanDrop));
    mCanDrop = aCanDrop;
    return NS_OK;
}

NS_IMETHODIMP
nsDragService::GetCanDrop(bool *aCanDrop)
{
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("nsDragService::GetCanDrop"));
    *aCanDrop = mCanDrop;
    return NS_OK;
}


static uint32_t
CountTextUriListItems(const char *data,
                      uint32_t datalen)
{
    const char *p = data;
    const char *endPtr = p + datalen;
    uint32_t count = 0;

    while (p < endPtr) {
        
        while (p < endPtr && *p != '\0' && isspace(*p))
            p++;
        
        if (p != endPtr && *p != '\0' && *p != '\n' && *p != '\r')
            count++;
        
        while (p < endPtr && *p != '\0' && *p != '\n')
            p++;
        p++; 
    }
    return count;
}



static void
GetTextUriListItem(const char *data,
                   uint32_t datalen,
                   uint32_t aItemIndex,
                   char16_t **convertedText,
                   int32_t *convertedTextLen)
{
    const char *p = data;
    const char *endPtr = p + datalen;
    unsigned int count = 0;

    *convertedText = nullptr;
    while (p < endPtr) {
        
        while (p < endPtr && *p != '\0' && isspace(*p))
            p++;
        
        if (p != endPtr && *p != '\0' && *p != '\n' && *p != '\r')
            count++;
        
        if (aItemIndex + 1 == count) {
            const char *q = p;
            while (q < endPtr && *q != '\0' && *q != '\n' && *q != '\r')
              q++;
            nsPrimitiveHelpers::ConvertPlatformPlainTextToUnicode(
                                p, q - p, convertedText, convertedTextLen);
            break;
        }
        
        while (p < endPtr && *p != '\0' && *p != '\n')
            p++;
        p++; 
    }

    
    if (!*convertedText) {
        nsPrimitiveHelpers::ConvertPlatformPlainTextToUnicode(
                            data, datalen, convertedText, convertedTextLen);
    }
}

NS_IMETHODIMP
nsDragService::GetNumDropItems(uint32_t * aNumItems)
{
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("nsDragService::GetNumDropItems"));

    if (!mTargetWidget) {
        PR_LOG(sDragLm, PR_LOG_DEBUG,
               ("*** warning: GetNumDropItems \
               called without a valid target widget!\n"));
        *aNumItems = 0;
        return NS_OK;
    }

    bool isList = IsTargetContextList();
    if (isList)
        mSourceDataItems->Count(aNumItems);
    else {
        GdkAtom gdkFlavor = gdk_atom_intern(gTextUriListType, FALSE);
        GetTargetDragData(gdkFlavor);
        if (mTargetDragData) {
            const char *data = reinterpret_cast<char*>(mTargetDragData);
            *aNumItems = CountTextUriListItems(data, mTargetDragDataLen);
        } else
            *aNumItems = 1;
    }
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("%d items", *aNumItems));
    return NS_OK;
}


NS_IMETHODIMP
nsDragService::GetData(nsITransferable * aTransferable,
                       uint32_t aItemIndex)
{
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("nsDragService::GetData %d", aItemIndex));

    
    if (!aTransferable)
        return NS_ERROR_INVALID_ARG;

    if (!mTargetWidget) {
        PR_LOG(sDragLm, PR_LOG_DEBUG,
               ("*** warning: GetData \
               called without a valid target widget!\n"));
        return NS_ERROR_FAILURE;
    }

    
    
    
    nsCOMPtr<nsISupportsArray> flavorList;
    nsresult rv = aTransferable->FlavorsTransferableCanImport(
                        getter_AddRefs(flavorList));
    if (NS_FAILED(rv))
        return rv;

    
    uint32_t cnt;
    flavorList->Count(&cnt);
    unsigned int i;

    
    bool isList = IsTargetContextList();

    if (isList) {
        PR_LOG(sDragLm, PR_LOG_DEBUG, ("it's a list..."));
        
        for (i = 0; i < cnt; ++i) {
            nsCOMPtr<nsISupports> genericWrapper;
            flavorList->GetElementAt(i, getter_AddRefs(genericWrapper));
            nsCOMPtr<nsISupportsCString> currentFlavor;
            currentFlavor = do_QueryInterface(genericWrapper);
            if (!currentFlavor)
                continue;

            nsXPIDLCString flavorStr;
            currentFlavor->ToString(getter_Copies(flavorStr));
            PR_LOG(sDragLm,
                   PR_LOG_DEBUG,
                   ("flavor is %s\n", (const char *)flavorStr));
            
            nsCOMPtr<nsISupports> genericItem;
            mSourceDataItems->GetElementAt(aItemIndex,
                                           getter_AddRefs(genericItem));
            nsCOMPtr<nsITransferable> item(do_QueryInterface(genericItem));
            if (!item)
                continue;

            nsCOMPtr<nsISupports> data;
            uint32_t tmpDataLen = 0;
            PR_LOG(sDragLm, PR_LOG_DEBUG,
                   ("trying to get transfer data for %s\n",
                   (const char *)flavorStr));
            rv = item->GetTransferData(flavorStr,
                                       getter_AddRefs(data),
                                       &tmpDataLen);
            if (NS_FAILED(rv)) {
                PR_LOG(sDragLm, PR_LOG_DEBUG, ("failed.\n"));
                continue;
            }
            PR_LOG(sDragLm, PR_LOG_DEBUG, ("succeeded.\n"));
            rv = aTransferable->SetTransferData(flavorStr,data,tmpDataLen);
            if (NS_FAILED(rv)) {
                PR_LOG(sDragLm,
                       PR_LOG_DEBUG,
                       ("fail to set transfer data into transferable!\n"));
                continue;
            }
            
            return NS_OK;
        }
        
        return NS_ERROR_FAILURE;
    }

    
    
    
    for ( i = 0; i < cnt; ++i ) {
        nsCOMPtr<nsISupports> genericWrapper;
        flavorList->GetElementAt(i,getter_AddRefs(genericWrapper));
        nsCOMPtr<nsISupportsCString> currentFlavor;
        currentFlavor = do_QueryInterface(genericWrapper);
        if (currentFlavor) {
            
            nsXPIDLCString flavorStr;
            currentFlavor->ToString(getter_Copies(flavorStr));
            GdkAtom gdkFlavor = gdk_atom_intern(flavorStr, FALSE);
            PR_LOG(sDragLm, PR_LOG_DEBUG,
                   ("looking for data in type %s, gdk flavor %ld\n",
                   static_cast<const char*>(flavorStr), gdkFlavor));
            bool dataFound = false;
            if (gdkFlavor) {
                GetTargetDragData(gdkFlavor);
            }
            if (mTargetDragData) {
                PR_LOG(sDragLm, PR_LOG_DEBUG, ("dataFound = true\n"));
                dataFound = true;
            }
            else {
                PR_LOG(sDragLm, PR_LOG_DEBUG, ("dataFound = false\n"));

                
                
                if ( strcmp(flavorStr, kFileMime) == 0 ) {
                    gdkFlavor = gdk_atom_intern(kTextMime, FALSE);
                    GetTargetDragData(gdkFlavor);
                    if (!mTargetDragData) {
                        gdkFlavor = gdk_atom_intern(gTextUriListType, FALSE);
                        GetTargetDragData(gdkFlavor);
                    }
                    if (mTargetDragData) {
                        const char* text = static_cast<char*>(mTargetDragData);
                        char16_t* convertedText = nullptr;
                        int32_t convertedTextLen = 0;

                        GetTextUriListItem(text, mTargetDragDataLen, aItemIndex,
                                           &convertedText, &convertedTextLen);

                        if (convertedText) {
                            nsCOMPtr<nsIIOService> ioService = do_GetIOService(&rv);
                            nsCOMPtr<nsIURI> fileURI;
                            nsresult rv = ioService->NewURI(NS_ConvertUTF16toUTF8(convertedText),
                                                            nullptr, nullptr, getter_AddRefs(fileURI));
                            if (NS_SUCCEEDED(rv)) {
                                nsCOMPtr<nsIFileURL> fileURL = do_QueryInterface(fileURI, &rv);
                                if (NS_SUCCEEDED(rv)) {
                                    nsCOMPtr<nsIFile> file;
                                    rv = fileURL->GetFile(getter_AddRefs(file));
                                    if (NS_SUCCEEDED(rv)) {
                                        
                                        
                                        
                                        
                                        
                                        aTransferable->SetTransferData(flavorStr, file,
                                                                       convertedTextLen);
                                        g_free(convertedText);
                                        return NS_OK;
                                    }
                                }
                            }
                            g_free(convertedText);
                        }
                        continue;
                    }
                }

                
                
                
                if ( strcmp(flavorStr, kUnicodeMime) == 0 ) {
                    PR_LOG(sDragLm, PR_LOG_DEBUG,
                           ("we were looking for text/unicode... \
                           trying with text/plain;charset=utf-8\n"));
                    gdkFlavor = gdk_atom_intern(gTextPlainUTF8Type, FALSE);
                    GetTargetDragData(gdkFlavor);
                    if (mTargetDragData) {
                        PR_LOG(sDragLm, PR_LOG_DEBUG, ("Got textplain data\n"));
                        const char* castedText =
                                    reinterpret_cast<char*>(mTargetDragData);
                        char16_t* convertedText = nullptr;
                        NS_ConvertUTF8toUTF16 ucs2string(castedText,
                                                         mTargetDragDataLen);
                        convertedText = ToNewUnicode(ucs2string);
                        if ( convertedText ) {
                            PR_LOG(sDragLm, PR_LOG_DEBUG,
                                   ("successfully converted plain text \
                                   to unicode.\n"));
                            
                            g_free(mTargetDragData);
                            mTargetDragData = convertedText;
                            mTargetDragDataLen = ucs2string.Length() * 2;
                            dataFound = true;
                        } 
                    } else {
                        PR_LOG(sDragLm, PR_LOG_DEBUG,
                               ("we were looking for text/unicode... \
                               trying again with text/plain\n"));
                        gdkFlavor = gdk_atom_intern(kTextMime, FALSE);
                        GetTargetDragData(gdkFlavor);
                        if (mTargetDragData) {
                            PR_LOG(sDragLm, PR_LOG_DEBUG, ("Got textplain data\n"));
                            const char* castedText =
                                        reinterpret_cast<char*>(mTargetDragData);
                            char16_t* convertedText = nullptr;
                            int32_t convertedTextLen = 0;
                            nsPrimitiveHelpers::ConvertPlatformPlainTextToUnicode(
                                                castedText, mTargetDragDataLen,
                                                &convertedText, &convertedTextLen);
                            if ( convertedText ) {
                                PR_LOG(sDragLm, PR_LOG_DEBUG,
                                       ("successfully converted plain text \
                                       to unicode.\n"));
                                
                                g_free(mTargetDragData);
                                mTargetDragData = convertedText;
                                mTargetDragDataLen = convertedTextLen * 2;
                                dataFound = true;
                            } 
                        } 
                    } 
                } 

                
                
                
                if (strcmp(flavorStr, kURLMime) == 0) {
                    PR_LOG(sDragLm, PR_LOG_DEBUG,
                           ("we were looking for text/x-moz-url...\
                           trying again with text/uri-list\n"));
                    gdkFlavor = gdk_atom_intern(gTextUriListType, FALSE);
                    GetTargetDragData(gdkFlavor);
                    if (mTargetDragData) {
                        PR_LOG(sDragLm, PR_LOG_DEBUG,
                               ("Got text/uri-list data\n"));
                        const char *data =
                                   reinterpret_cast<char*>(mTargetDragData);
                        char16_t* convertedText = nullptr;
                        int32_t convertedTextLen = 0;

                        GetTextUriListItem(data, mTargetDragDataLen, aItemIndex,
                                           &convertedText, &convertedTextLen);

                        if ( convertedText ) {
                            PR_LOG(sDragLm, PR_LOG_DEBUG,
                                   ("successfully converted \
                                   _NETSCAPE_URL to unicode.\n"));
                            
                            g_free(mTargetDragData);
                            mTargetDragData = convertedText;
                            mTargetDragDataLen = convertedTextLen * 2;
                            dataFound = true;
                        }
                    }
                    else {
                        PR_LOG(sDragLm, PR_LOG_DEBUG,
                               ("failed to get text/uri-list data\n"));
                    }
                    if (!dataFound) {
                        PR_LOG(sDragLm, PR_LOG_DEBUG,
                               ("we were looking for text/x-moz-url...\
                               trying again with _NETSCAP_URL\n"));
                        gdkFlavor = gdk_atom_intern(gMozUrlType, FALSE);
                        GetTargetDragData(gdkFlavor);
                        if (mTargetDragData) {
                            PR_LOG(sDragLm, PR_LOG_DEBUG,
                                   ("Got _NETSCAPE_URL data\n"));
                            const char* castedText =
                                  reinterpret_cast<char*>(mTargetDragData);
                            char16_t* convertedText = nullptr;
                            int32_t convertedTextLen = 0;
                            nsPrimitiveHelpers::ConvertPlatformPlainTextToUnicode(castedText, mTargetDragDataLen, &convertedText, &convertedTextLen);
                            if ( convertedText ) {
                                PR_LOG(sDragLm,
                                       PR_LOG_DEBUG,
                                       ("successfully converted _NETSCAPE_URL \
                                       to unicode.\n"));
                                
                                g_free(mTargetDragData);
                                mTargetDragData = convertedText;
                                mTargetDragDataLen = convertedTextLen * 2;
                                dataFound = true;
                            }
                        }
                        else {
                            PR_LOG(sDragLm, PR_LOG_DEBUG,
                                   ("failed to get _NETSCAPE_URL data\n"));
                        }
                    }
                }

            } 

            if (dataFound) {
                
                
                nsLinebreakHelpers::ConvertPlatformToDOMLinebreaks(
                             flavorStr,
                             &mTargetDragData,
                             reinterpret_cast<int*>(&mTargetDragDataLen));
        
                
                nsCOMPtr<nsISupports> genericDataWrapper;
                nsPrimitiveHelpers::CreatePrimitiveForData(flavorStr,
                                    mTargetDragData, mTargetDragDataLen,
                                    getter_AddRefs(genericDataWrapper));
                aTransferable->SetTransferData(flavorStr,
                                               genericDataWrapper,
                                               mTargetDragDataLen);
                
                PR_LOG(sDragLm, PR_LOG_DEBUG, ("dataFound and converted!\n"));
                break;
            }
        } 
    } 

    return NS_OK;
  
}

NS_IMETHODIMP
nsDragService::IsDataFlavorSupported(const char *aDataFlavor,
                                     bool *_retval)
{
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("nsDragService::IsDataFlavorSupported %s",
                                   aDataFlavor));
    if (!_retval)
        return NS_ERROR_INVALID_ARG;

    
    *_retval = false;

    
    if (!mTargetWidget) {
        PR_LOG(sDragLm, PR_LOG_DEBUG,
               ("*** warning: IsDataFlavorSupported \
               called without a valid target widget!\n"));
        return NS_OK;
    }

    
    bool isList = IsTargetContextList();
    
    
    if (isList) {
        PR_LOG(sDragLm, PR_LOG_DEBUG, ("It's a list.."));
        uint32_t numDragItems = 0;
        
        
        if (!mSourceDataItems)
            return NS_OK;
        mSourceDataItems->Count(&numDragItems);
        for (uint32_t itemIndex = 0; itemIndex < numDragItems; ++itemIndex) {
            nsCOMPtr<nsISupports> genericItem;
            mSourceDataItems->GetElementAt(itemIndex,
                                           getter_AddRefs(genericItem));
            nsCOMPtr<nsITransferable> currItem(do_QueryInterface(genericItem));
            if (currItem) {
                nsCOMPtr <nsISupportsArray> flavorList;
                currItem->FlavorsTransferableCanExport(
                          getter_AddRefs(flavorList));
                if (flavorList) {
                    uint32_t numFlavors;
                    flavorList->Count( &numFlavors );
                    for ( uint32_t flavorIndex = 0;
                          flavorIndex < numFlavors ;
                          ++flavorIndex ) {
                        nsCOMPtr<nsISupports> genericWrapper;
                        flavorList->GetElementAt(flavorIndex,
                                                getter_AddRefs(genericWrapper));
                        nsCOMPtr<nsISupportsCString> currentFlavor;
                        currentFlavor = do_QueryInterface(genericWrapper);
                        if (currentFlavor) {
                            nsXPIDLCString flavorStr;
                            currentFlavor->ToString(getter_Copies(flavorStr));
                            PR_LOG(sDragLm, PR_LOG_DEBUG,
                                   ("checking %s against %s\n",
                                   (const char *)flavorStr, aDataFlavor));
                            if (strcmp(flavorStr, aDataFlavor) == 0) {
                                PR_LOG(sDragLm, PR_LOG_DEBUG,
                                       ("boioioioiooioioioing!\n"));
                                *_retval = true;
                            }
                        }
                    }
                }
            }
        }
        return NS_OK;
    }

    
    GList *tmp;
    for (tmp = gdk_drag_context_list_targets(mTargetDragContext); 
         tmp; tmp = tmp->next) {
        
        GdkAtom atom = GDK_POINTER_TO_ATOM(tmp->data);
        gchar *name = nullptr;
        name = gdk_atom_name(atom);
        PR_LOG(sDragLm, PR_LOG_DEBUG,
               ("checking %s against %s\n", name, aDataFlavor));
        if (name && (strcmp(name, aDataFlavor) == 0)) {
            PR_LOG(sDragLm, PR_LOG_DEBUG, ("good!\n"));
            *_retval = true;
        }
        
        if (!*_retval && 
            name &&
            (strcmp(name, gTextUriListType) == 0) &&
            (strcmp(aDataFlavor, kURLMime) == 0 ||
             strcmp(aDataFlavor, kFileMime) == 0)) {
            PR_LOG(sDragLm, PR_LOG_DEBUG,
                   ("good! ( it's text/uri-list and \
                   we're checking against text/x-moz-url )\n"));
            *_retval = true;
        }
        
        if (!*_retval && 
            name &&
            (strcmp(name, gMozUrlType) == 0) &&
            (strcmp(aDataFlavor, kURLMime) == 0)) {
            PR_LOG(sDragLm, PR_LOG_DEBUG,
                   ("good! ( it's _NETSCAPE_URL and \
                   we're checking against text/x-moz-url )\n"));
            *_retval = true;
        }
        
        if (!*_retval && 
            name &&
            (strcmp(name, kTextMime) == 0) &&
            ((strcmp(aDataFlavor, kUnicodeMime) == 0) ||
             (strcmp(aDataFlavor, kFileMime) == 0))) {
            PR_LOG(sDragLm, PR_LOG_DEBUG,
                   ("good! ( it's text plain and we're checking \
                   against text/unicode or application/x-moz-file)\n"));
            *_retval = true;
        }
        g_free(name);
    }
    return NS_OK;
}

void
nsDragService::ReplyToDragMotion()
{
    PR_LOG(sDragLm, PR_LOG_DEBUG,
           ("nsDragService::ReplyToDragMotion %d", mCanDrop));

    GdkDragAction action = (GdkDragAction)0;
    if (mCanDrop) {
        
        switch (mDragAction) {
        case DRAGDROP_ACTION_COPY:
          action = GDK_ACTION_COPY;
          break;
        case DRAGDROP_ACTION_LINK:
          action = GDK_ACTION_LINK;
          break;
        default:
          action = GDK_ACTION_MOVE;
          break;
        }
    }

    gdk_drag_status(mTargetDragContext, action, mTargetTime);
}

void
nsDragService::TargetDataReceived(GtkWidget         *aWidget,
                                  GdkDragContext    *aContext,
                                  gint               aX,
                                  gint               aY,
                                  GtkSelectionData  *aSelectionData,
                                  guint              aInfo,
                                  guint32            aTime)
{
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("nsDragService::TargetDataReceived"));
    TargetResetData();
    mTargetDragDataReceived = true;
    gint len = gtk_selection_data_get_length(aSelectionData);
    const guchar* data = gtk_selection_data_get_data(aSelectionData);
    if (len > 0 && data) {
        mTargetDragDataLen = len;
        mTargetDragData = g_malloc(mTargetDragDataLen);
        memcpy(mTargetDragData, data, mTargetDragDataLen);
    }
    else {
        PR_LOG(sDragLm, PR_LOG_DEBUG,
               ("Failed to get data.  selection data len was %d\n",
                mTargetDragDataLen));
    }
}

bool
nsDragService::IsTargetContextList(void)
{
    bool retval = false;

    
    
    
    
    if (gtk_drag_get_source_widget(mTargetDragContext) == nullptr)
        return retval;

    GList *tmp;

    
    
    for (tmp = gdk_drag_context_list_targets(mTargetDragContext); 
         tmp; tmp = tmp->next) {
        
        GdkAtom atom = GDK_POINTER_TO_ATOM(tmp->data);
        gchar *name = nullptr;
        name = gdk_atom_name(atom);
        if (name && strcmp(name, gMimeListType) == 0)
            retval = true;
        g_free(name);
        if (retval)
            break;
    }
    return retval;
}


#define NS_DND_TIMEOUT 500000

void
nsDragService::GetTargetDragData(GdkAtom aFlavor)
{
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("getting data flavor %d\n", aFlavor));
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("mLastWidget is %p and mLastContext is %p\n",
                                   mTargetWidget.get(),
                                   mTargetDragContext.get()));
    
    TargetResetData();
    gtk_drag_get_data(mTargetWidget, mTargetDragContext, aFlavor, mTargetTime);
    
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("about to start inner iteration."));
    PRTime entryTime = PR_Now();
    while (!mTargetDragDataReceived && mDoingDrag) {
        
        PR_LOG(sDragLm, PR_LOG_DEBUG, ("doing iteration...\n"));
        PR_Sleep(20*PR_TicksPerSecond()/1000);  
        if (PR_Now()-entryTime > NS_DND_TIMEOUT) break;
        gtk_main_iteration();
    }
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("finished inner iteration\n"));
}

void
nsDragService::TargetResetData(void)
{
    mTargetDragDataReceived = false;
    
    g_free(mTargetDragData);
    mTargetDragData = 0;
    mTargetDragDataLen = 0;
}

GtkTargetList *
nsDragService::GetSourceList(void)
{
    if (!mSourceDataItems)
        return nullptr;
    nsTArray<GtkTargetEntry*> targetArray;
    GtkTargetEntry *targets;
    GtkTargetList  *targetList = 0;
    uint32_t targetCount = 0;
    unsigned int numDragItems = 0;

    mSourceDataItems->Count(&numDragItems);

    
    if (numDragItems > 1) {
        
        
        

        
        
        GtkTargetEntry *listTarget =
            (GtkTargetEntry *)g_malloc(sizeof(GtkTargetEntry));
        listTarget->target = g_strdup(gMimeListType);
        listTarget->flags = 0;
        PR_LOG(sDragLm, PR_LOG_DEBUG,
               ("automatically adding target %s\n", listTarget->target));
        targetArray.AppendElement(listTarget);

        
        
        nsCOMPtr<nsISupports> genericItem;
        mSourceDataItems->GetElementAt(0, getter_AddRefs(genericItem));
        nsCOMPtr<nsITransferable> currItem(do_QueryInterface(genericItem));

        if (currItem) {
            nsCOMPtr <nsISupportsArray> flavorList;
            currItem->FlavorsTransferableCanExport(getter_AddRefs(flavorList));
            if (flavorList) {
                uint32_t numFlavors;
                flavorList->Count( &numFlavors );
                for (uint32_t flavorIndex = 0;
                     flavorIndex < numFlavors ;
                     ++flavorIndex ) {
                    nsCOMPtr<nsISupports> genericWrapper;
                    flavorList->GetElementAt(flavorIndex,
                                           getter_AddRefs(genericWrapper));
                    nsCOMPtr<nsISupportsCString> currentFlavor;
                    currentFlavor = do_QueryInterface(genericWrapper);
                    if (currentFlavor) {
                        nsXPIDLCString flavorStr;
                        currentFlavor->ToString(getter_Copies(flavorStr));

                        
                        
                        
                        if (strcmp(flavorStr, kURLMime) == 0) {
                            listTarget =
                             (GtkTargetEntry *)g_malloc(sizeof(GtkTargetEntry));
                            listTarget->target = g_strdup(gTextUriListType);
                            listTarget->flags = 0;
                            PR_LOG(sDragLm, PR_LOG_DEBUG,
                                   ("automatically adding target %s\n",
                                    listTarget->target));
                            targetArray.AppendElement(listTarget);
                        }
                    }
                } 
            } 
        } 
    } else if (numDragItems == 1) {
        nsCOMPtr<nsISupports> genericItem;
        mSourceDataItems->GetElementAt(0, getter_AddRefs(genericItem));
        nsCOMPtr<nsITransferable> currItem(do_QueryInterface(genericItem));
        if (currItem) {
            nsCOMPtr <nsISupportsArray> flavorList;
            currItem->FlavorsTransferableCanExport(getter_AddRefs(flavorList));
            if (flavorList) {
                uint32_t numFlavors;
                flavorList->Count( &numFlavors );
                for (uint32_t flavorIndex = 0;
                     flavorIndex < numFlavors ;
                     ++flavorIndex ) {
                    nsCOMPtr<nsISupports> genericWrapper;
                    flavorList->GetElementAt(flavorIndex,
                                             getter_AddRefs(genericWrapper));
                    nsCOMPtr<nsISupportsCString> currentFlavor;
                    currentFlavor = do_QueryInterface(genericWrapper);
                    if (currentFlavor) {
                        nsXPIDLCString flavorStr;
                        currentFlavor->ToString(getter_Copies(flavorStr));
                        GtkTargetEntry *target =
                          (GtkTargetEntry *)g_malloc(sizeof(GtkTargetEntry));
                        target->target = g_strdup(flavorStr);
                        target->flags = 0;
                        PR_LOG(sDragLm, PR_LOG_DEBUG,
                               ("adding target %s\n", target->target));
                        targetArray.AppendElement(target);
                        
                        
                        
                        
                        if (strcmp(flavorStr, kUnicodeMime) == 0) {
                            GtkTargetEntry *plainUTF8Target =
                             (GtkTargetEntry *)g_malloc(sizeof(GtkTargetEntry));
                            plainUTF8Target->target = g_strdup(gTextPlainUTF8Type);
                            plainUTF8Target->flags = 0;
                            PR_LOG(sDragLm, PR_LOG_DEBUG,
                                   ("automatically adding target %s\n",
                                    plainUTF8Target->target));
                            targetArray.AppendElement(plainUTF8Target);

                            GtkTargetEntry *plainTarget =
                             (GtkTargetEntry *)g_malloc(sizeof(GtkTargetEntry));
                            plainTarget->target = g_strdup(kTextMime);
                            plainTarget->flags = 0;
                            PR_LOG(sDragLm, PR_LOG_DEBUG,
                                   ("automatically adding target %s\n",
                                    plainTarget->target));
                            targetArray.AppendElement(plainTarget);
                        }
                        
                        
                        
                        if (strcmp(flavorStr, kURLMime) == 0) {
                            GtkTargetEntry *urlTarget =
                             (GtkTargetEntry *)g_malloc(sizeof(GtkTargetEntry));
                            urlTarget->target = g_strdup(gMozUrlType);
                            urlTarget->flags = 0;
                            PR_LOG(sDragLm, PR_LOG_DEBUG,
                                   ("automatically adding target %s\n",
                                    urlTarget->target));
                            targetArray.AppendElement(urlTarget);
                        }
                    }
                } 
            } 
        } 
    } 

    
    targetCount = targetArray.Length();
    if (targetCount) {
        
        targets =
          (GtkTargetEntry *)g_malloc(sizeof(GtkTargetEntry) * targetCount);
        uint32_t targetIndex;
        for ( targetIndex = 0; targetIndex < targetCount; ++targetIndex) {
            GtkTargetEntry *disEntry = targetArray.ElementAt(targetIndex);
            
            targets[targetIndex].target = disEntry->target;
            targets[targetIndex].flags = disEntry->flags;
            targets[targetIndex].info = 0;
        }
        targetList = gtk_target_list_new(targets, targetCount);
        
        for (uint32_t cleanIndex = 0; cleanIndex < targetCount; ++cleanIndex) {
            GtkTargetEntry *thisTarget = targetArray.ElementAt(cleanIndex);
            g_free(thisTarget->target);
            g_free(thisTarget);
        }
        g_free(targets);
    }
    return targetList;
}

void
nsDragService::SourceEndDragSession(GdkDragContext *aContext,
                                    gint            aResult)
{
    
    mSourceDataItems = nullptr;

    if (!mDoingDrag || mScheduledTask == eDragTaskSourceEnd)
        
        
        return;

    if (mEndDragPoint.x < 0) {
        
        gint x, y;
        GdkDisplay* display = gdk_display_get_default();
        if (display) {
            gdk_display_get_pointer(display, nullptr, &x, &y, nullptr);
            SetDragEndPoint(nsIntPoint(x, y));
        }
    }

    
    
    

    uint32_t dropEffect;

    if (aResult == MOZ_GTK_DRAG_RESULT_SUCCESS) {

        
        
        
        
        GdkDragAction action =
            gdk_drag_context_get_dest_window(aContext) ? 
                gdk_drag_context_get_actions(aContext) : (GdkDragAction)0;

        
        
        
        if (!action)
            dropEffect = DRAGDROP_ACTION_NONE;
        else if (action & GDK_ACTION_COPY)
            dropEffect = DRAGDROP_ACTION_COPY;
        else if (action & GDK_ACTION_LINK)
            dropEffect = DRAGDROP_ACTION_LINK;
        else if (action & GDK_ACTION_MOVE)
            dropEffect = DRAGDROP_ACTION_MOVE;
        else
            dropEffect = DRAGDROP_ACTION_COPY;

    } else {

        dropEffect = DRAGDROP_ACTION_NONE;

        if (aResult != MOZ_GTK_DRAG_RESULT_NO_TARGET) {
            mUserCancelled = true;
        }
    }

    if (mDataTransfer) {
        mDataTransfer->SetDropEffectInt(dropEffect);
    }

    
    Schedule(eDragTaskSourceEnd, nullptr, nullptr, nsIntPoint(), 0);
}

static void
CreateUriList(nsISupportsArray *items, gchar **text, gint *length)
{
    uint32_t i, count;
    GString *uriList = g_string_new(nullptr);

    items->Count(&count);
    for (i = 0; i < count; i++) {
        nsCOMPtr<nsISupports> genericItem;
        items->GetElementAt(i, getter_AddRefs(genericItem));
        nsCOMPtr<nsITransferable> item;
        item = do_QueryInterface(genericItem);

        if (item) {
            uint32_t tmpDataLen = 0;
            void    *tmpData = nullptr;
            nsresult rv = NS_OK;
            nsCOMPtr<nsISupports> data;
            rv = item->GetTransferData(kURLMime,
                                       getter_AddRefs(data),
                                       &tmpDataLen);

            if (NS_SUCCEEDED(rv)) {
                nsPrimitiveHelpers::CreateDataFromPrimitive(kURLMime,
                                                            data,
                                                            &tmpData,
                                                            tmpDataLen);
                char* plainTextData = nullptr;
                char16_t* castedUnicode = reinterpret_cast<char16_t*>
                                                           (tmpData);
                int32_t plainTextLen = 0;
                nsPrimitiveHelpers::ConvertUnicodeToPlatformPlainText(
                                    castedUnicode,
                                    tmpDataLen / 2,
                                    &plainTextData,
                                    &plainTextLen);
                if (plainTextData) {
                    int32_t j;

                    
                    
                    for (j = 0; j < plainTextLen; j++)
                        if (plainTextData[j] == '\n' ||
                            plainTextData[j] == '\r') {
                            plainTextData[j] = '\0';
                            break;
                        }
                    g_string_append(uriList, plainTextData);
                    g_string_append(uriList, "\r\n");
                    
                    free(plainTextData);
                }
                if (tmpData) {
                    
                    free(tmpData);
                }
            }
        }
    }
    *text = uriList->str;
    *length = uriList->len + 1;
    g_string_free(uriList, FALSE); 
}


void
nsDragService::SourceDataGet(GtkWidget        *aWidget,
                             GdkDragContext   *aContext,
                             GtkSelectionData *aSelectionData,
                             guint32           aTime)
{
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("nsDragService::SourceDataGet"));
    GdkAtom target = gtk_selection_data_get_target(aSelectionData);
    nsXPIDLCString mimeFlavor;
    gchar *typeName = 0;
    typeName = gdk_atom_name(target);
    if (!typeName) {
        PR_LOG(sDragLm, PR_LOG_DEBUG, ("failed to get atom name.\n"));
        return;
    }

    PR_LOG(sDragLm, PR_LOG_DEBUG, ("Type is %s\n", typeName));
    
    mimeFlavor.Adopt(strdup(typeName));
    g_free(typeName);
    
    if (!mSourceDataItems) {
        PR_LOG(sDragLm, PR_LOG_DEBUG, ("Failed to get our data items\n"));
        return;
    }

    nsCOMPtr<nsISupports> genericItem;
    mSourceDataItems->GetElementAt(0, getter_AddRefs(genericItem));
    nsCOMPtr<nsITransferable> item;
    item = do_QueryInterface(genericItem);
    if (item) {
        
        
        bool needToDoConversionToPlainText = false;
        const char* actualFlavor = mimeFlavor;
        if (strcmp(mimeFlavor, kTextMime) == 0 ||
            strcmp(mimeFlavor, gTextPlainUTF8Type) == 0) {
            actualFlavor = kUnicodeMime;
            needToDoConversionToPlainText = true;
        }
        
        
        else if (strcmp(mimeFlavor, gMozUrlType) == 0) {
            actualFlavor = kURLMime;
            needToDoConversionToPlainText = true;
        }
        
        
        else if (strcmp(mimeFlavor, gTextUriListType) == 0) {
            actualFlavor = gTextUriListType;
            needToDoConversionToPlainText = true;
        }
        else
            actualFlavor = mimeFlavor;

        uint32_t tmpDataLen = 0;
        void    *tmpData = nullptr;
        nsresult rv;
        nsCOMPtr<nsISupports> data;
        rv = item->GetTransferData(actualFlavor,
                                   getter_AddRefs(data),
                                   &tmpDataLen);
        if (NS_SUCCEEDED(rv)) {
            nsPrimitiveHelpers::CreateDataFromPrimitive (actualFlavor, data,
                                                         &tmpData, tmpDataLen);
            
            
            if (needToDoConversionToPlainText) {
                char* plainTextData = nullptr;
                char16_t* castedUnicode = reinterpret_cast<char16_t*>
                                                           (tmpData);
                int32_t plainTextLen = 0;
                if (strcmp(mimeFlavor, gTextPlainUTF8Type) == 0) {
                    plainTextData =
                        ToNewUTF8String(
                            nsDependentString(castedUnicode, tmpDataLen / 2),
                            (uint32_t*)&plainTextLen);
                } else {
                    nsPrimitiveHelpers::ConvertUnicodeToPlatformPlainText(
                                        castedUnicode,
                                        tmpDataLen / 2,
                                        &plainTextData,
                                        &plainTextLen);
                }
                if (tmpData) {
                    
                    free(tmpData);
                    tmpData = plainTextData;
                    tmpDataLen = plainTextLen;
                }
            }
            if (tmpData) {
                
                gtk_selection_data_set(aSelectionData, target,
                                       8,
                                       (guchar *)tmpData, tmpDataLen);
                
                free(tmpData);
            }
        } else {
            if (strcmp(mimeFlavor, gTextUriListType) == 0) {
                
                gchar *uriList;
                gint length;
                CreateUriList(mSourceDataItems, &uriList, &length);
                gtk_selection_data_set(aSelectionData, target,
                                       8, (guchar *)uriList, length);
                g_free(uriList);
                return;
            }
        }
    }
}

void nsDragService::SetDragIcon(GdkDragContext* aContext)
{
    if (!mHasImage && !mSelection)
        return;

    nsIntRect dragRect;
    nsPresContext* pc;
    RefPtr<SourceSurface> surface;
    DrawDrag(mSourceNode, mSourceRegion, mScreenX, mScreenY,
             &dragRect, &surface, &pc);
    if (!pc)
        return;

    int32_t sx = mScreenX, sy = mScreenY;
    ConvertToUnscaledDevPixels(pc, &sx, &sy);

    int32_t offsetX = sx - dragRect.x;
    int32_t offsetY = sy - dragRect.y;

    
    
    if (mDragPopup) {
        GtkWidget* gtkWidget = nullptr;
        nsIFrame* frame = mDragPopup->GetPrimaryFrame();
        if (frame) {
            
            nsCOMPtr<nsIWidget> widget = frame->GetNearestWidget();
            if (widget) {
                gtkWidget = (GtkWidget *)widget->GetNativeData(NS_NATIVE_SHELLWIDGET);
                if (gtkWidget) {
                    OpenDragPopup();
                    gtk_drag_set_icon_widget(aContext, gtkWidget, offsetX, offsetY);
                }
            }
        }
    }
    else if (surface) {
        if (!SetAlphaPixmap(surface, aContext, offsetX, offsetY, dragRect)) {
            GdkPixbuf* dragPixbuf =
              nsImageToPixbuf::SourceSurfaceToPixbuf(surface, dragRect.width, dragRect.height);
            if (dragPixbuf) {
                gtk_drag_set_icon_pixbuf(aContext, dragPixbuf, offsetX, offsetY);
                g_object_unref(dragPixbuf);
            }
        }
    }
}

static void
invisibleSourceDragBegin(GtkWidget        *aWidget,
                         GdkDragContext   *aContext,
                         gpointer          aData)
{
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("invisibleSourceDragBegin"));
    nsDragService *dragService = (nsDragService *)aData;

    dragService->SetDragIcon(aContext);
}

static void
invisibleSourceDragDataGet(GtkWidget        *aWidget,
                           GdkDragContext   *aContext,
                           GtkSelectionData *aSelectionData,
                           guint             aInfo,
                           guint32           aTime,
                           gpointer          aData)
{
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("invisibleSourceDragDataGet"));
    nsDragService *dragService = (nsDragService *)aData;
    dragService->SourceDataGet(aWidget, aContext,
                               aSelectionData, aTime);
}

static gboolean
invisibleSourceDragFailed(GtkWidget        *aWidget,
                          GdkDragContext   *aContext,
                          gint              aResult,
                          gpointer          aData)
{
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("invisibleSourceDragFailed %i", aResult));
    nsDragService *dragService = (nsDragService *)aData;
    
    
    
    dragService->SourceEndDragSession(aContext, aResult);

    
    
    
    return FALSE;
}

static void
invisibleSourceDragEnd(GtkWidget        *aWidget,
                       GdkDragContext   *aContext,
                       gpointer          aData)
{
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("invisibleSourceDragEnd"));
    nsDragService *dragService = (nsDragService *)aData;

    
    dragService->SourceEndDragSession(aContext, MOZ_GTK_DRAG_RESULT_SUCCESS);
}










































gboolean
nsDragService::ScheduleMotionEvent(nsWindow *aWindow,
                                   GdkDragContext *aDragContext,
                                   nsIntPoint aWindowPoint, guint aTime)
{
    if (mScheduledTask == eDragTaskMotion) {
        
        
        
        
        
        NS_WARNING("Drag Motion message received before previous reply was sent");
    }

    
    
    return Schedule(eDragTaskMotion, aWindow, aDragContext,
                    aWindowPoint, aTime);
}

void
nsDragService::ScheduleLeaveEvent()
{
    
    
    
    if (!Schedule(eDragTaskLeave, nullptr, nullptr, nsIntPoint(), 0)) {
        NS_WARNING("Drag leave after drop");
    }        
}

gboolean
nsDragService::ScheduleDropEvent(nsWindow *aWindow,
                                 GdkDragContext *aDragContext,
                                 nsIntPoint aWindowPoint, guint aTime)
{
    if (!Schedule(eDragTaskDrop, aWindow,
                  aDragContext, aWindowPoint, aTime)) {
        NS_WARNING("Additional drag drop ignored");
        return FALSE;        
    }

    SetDragEndPoint(aWindowPoint + aWindow->WidgetToScreenOffsetUntyped());

    
    return TRUE;
}

gboolean
nsDragService::Schedule(DragTask aTask, nsWindow *aWindow,
                        GdkDragContext *aDragContext,
                        nsIntPoint aWindowPoint, guint aTime)
{
    
    
    

    
    
    
    
    
    if (mScheduledTask == eDragTaskSourceEnd ||
        (mScheduledTask == eDragTaskDrop && aTask != eDragTaskSourceEnd))
        return FALSE;

    mScheduledTask = aTask;
    mPendingWindow = aWindow;
    mPendingDragContext = aDragContext;
    mPendingWindowPoint = aWindowPoint;
    mPendingTime = aTime;

    if (!mTaskSource) {
        
        
        
        
        
        
        mTaskSource = g_idle_add_full(G_PRIORITY_HIGH, TaskDispatchCallback,
                                      this, nullptr);
    }
    return TRUE;
}

gboolean
nsDragService::TaskDispatchCallback(gpointer data)
{
    nsRefPtr<nsDragService> dragService = static_cast<nsDragService*>(data);
    return dragService->RunScheduledTask();
}

gboolean
nsDragService::RunScheduledTask()
{
    if (mTargetWindow && mTargetWindow != mPendingWindow) {
        PR_LOG(sDragLm, PR_LOG_DEBUG,
               ("nsDragService: dispatch drag leave (%p)\n",
                mTargetWindow.get()));
        mTargetWindow->
            DispatchDragEvent(NS_DRAGDROP_EXIT, mTargetWindowPoint, 0);

        if (!mSourceNode) {
            
            
            
            EndDragSession(false);
        }
    }

    
    

    
    
    
    
    bool positionHasChanged =
        mPendingWindow != mTargetWindow ||
        mPendingWindowPoint != mTargetWindowPoint;
    DragTask task = mScheduledTask;
    mScheduledTask = eDragTaskNone;
    mTargetWindow = mPendingWindow.forget();
    mTargetWindowPoint = mPendingWindowPoint;

    if (task == eDragTaskLeave || task == eDragTaskSourceEnd) {
        if (task == eDragTaskSourceEnd) {
            
            EndDragSession(true);
        }

        
        
        mTaskSource = 0;
        return FALSE;
    }

    
    StartDragSession();

    
    
    
    
    mTargetWidget = mTargetWindow->GetMozContainerWidget();
    mTargetDragContext.steal(mPendingDragContext);
    mTargetTime = mPendingTime;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (task == eDragTaskMotion || positionHasChanged) {
        UpdateDragAction();
        DispatchMotionEvents();

        if (task == eDragTaskMotion) {
            
            
            ReplyToDragMotion();
        }
    }

    if (task == eDragTaskDrop) {
        gboolean success = DispatchDropEvent();

        
        
        
        gtk_drag_finish(mTargetDragContext, success,
                         FALSE, mTargetTime);

        
        
        mTargetWindow = nullptr;
        
        
        EndDragSession(true);
    }

    
    mTargetWidget = nullptr;
    mTargetDragContext = nullptr;

    
    
    
    if (mScheduledTask != eDragTaskNone)
        return TRUE;

    
    
    mTaskSource = 0;
    return FALSE;
}





void
nsDragService::UpdateDragAction()
{
    
    
    
    
    
    

    
    int action = nsIDragService::DRAGDROP_ACTION_NONE;
    GdkDragAction gdkAction = gdk_drag_context_get_actions(mTargetDragContext);

    
    if (gdkAction & GDK_ACTION_DEFAULT)
        action = nsIDragService::DRAGDROP_ACTION_MOVE;

    
    if (gdkAction & GDK_ACTION_MOVE)
        action = nsIDragService::DRAGDROP_ACTION_MOVE;

    
    else if (gdkAction & GDK_ACTION_LINK)
        action = nsIDragService::DRAGDROP_ACTION_LINK;

    
    else if (gdkAction & GDK_ACTION_COPY)
        action = nsIDragService::DRAGDROP_ACTION_COPY;

    
    SetDragAction(action);
}

void
nsDragService::DispatchMotionEvents()
{
    mCanDrop = false;

    FireDragEventAtSource(NS_DRAGDROP_DRAG);

    mTargetWindow->
        DispatchDragEvent(NS_DRAGDROP_OVER, mTargetWindowPoint, mTargetTime);
}


gboolean
nsDragService::DispatchDropEvent()
{
    
    
    
    if (mTargetWindow->IsDestroyed())
        return FALSE;

    uint32_t msg = mCanDrop ? NS_DRAGDROP_DROP : NS_DRAGDROP_EXIT;

    mTargetWindow->DispatchDragEvent(msg, mTargetWindowPoint, mTargetTime);

    return mCanDrop;
}
