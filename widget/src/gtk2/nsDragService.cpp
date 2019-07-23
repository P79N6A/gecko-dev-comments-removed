











































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

#include "gfxASurface.h"
#include "gfxXlibSurface.h"
#include "gfxContext.h"
#include "nsImageToPixbuf.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsIDocument.h"
#include "nsISelection.h"


#define DRAG_IMAGE_ALPHA_LEVEL 0.5





enum {
  MOZ_GTK_DRAG_RESULT_SUCCESS,
  MOZ_GTK_DRAG_RESULT_NO_TARGET
};

static PRLogModuleInfo *sDragLm = NULL;

static const char gMimeListType[] = "application/x-moz-internal-item-list";
static const char gMozUrlType[] = "_NETSCAPE_URL";
static const char gTextUriListType[] = "text/uri-list";
static const char gTextPlainUTF8Type[] = "text/plain;charset=utf-8";

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
{
    
    
    nsCOMPtr<nsIObserverService> obsServ =
        do_GetService("@mozilla.org/observer-service;1");
    obsServ->AddObserver(this, "quit-application", PR_FALSE);

    
    mHiddenWidget = gtk_invisible_new();
    
    
    gtk_widget_realize(mHiddenWidget);
    
    
    g_signal_connect(GTK_OBJECT(mHiddenWidget), "drag_data_get",
                     G_CALLBACK(invisibleSourceDragDataGet), this);
    g_signal_connect(GTK_OBJECT(mHiddenWidget), "drag_end",
                     G_CALLBACK(invisibleSourceDragEnd), this);
    
    guint dragFailedID = g_signal_lookup("drag-failed",
                                         G_TYPE_FROM_INSTANCE(mHiddenWidget));
    if (dragFailedID) {
        g_signal_connect_closure_by_id(mHiddenWidget, dragFailedID, 0,
                                       g_cclosure_new(G_CALLBACK(invisibleSourceDragFailed),
                                                      this, NULL),
                                       FALSE);
    }

    
    if (!sDragLm)
        sDragLm = PR_NewLogModule("nsDragService");
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("nsDragService::nsDragService"));
    mTargetWidget = 0;
    mTargetDragContext = 0;
    mTargetTime = 0;
    mCanDrop = PR_FALSE;
    mTargetDragDataReceived = PR_FALSE;
    mTargetDragData = 0;
    mTargetDragDataLen = 0;
}

nsDragService::~nsDragService()
{
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("nsDragService::~nsDragService"));
}

NS_IMPL_ISUPPORTS_INHERITED2(nsDragService, nsBaseDragService,
                             nsIDragSessionGTK, nsIObserver)



NS_IMETHODIMP
nsDragService::Observe(nsISupports *aSubject, const char *aTopic,
                       const PRUnichar *aData)
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



NS_IMETHODIMP
nsDragService::InvokeDragSession(nsIDOMNode *aDOMNode,
                                 nsISupportsArray * aArrayTransferables,
                                 nsIScriptableRegion * aRegion,
                                 PRUint32 aActionType)
{
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("nsDragService::InvokeDragSession"));
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
    event.button.window = mHiddenWidget->window;
    event.button.time = nsWindow::mLastButtonPressTime;

    
    GdkDragContext *context = gtk_drag_begin(mHiddenWidget,
                                             sourceList,
                                             action,
                                             1,
                                             &event);

    if (!context) {
        rv = NS_ERROR_FAILURE;
    } else {
        PRBool needsFallbackIcon = PR_FALSE;
        nsIntRect dragRect;
        nsPresContext* pc;
        nsRefPtr<gfxASurface> surface;
        if (mHasImage || mSelection) {
          DrawDrag(aDOMNode, aRegion, mScreenX, mScreenY,
                   &dragRect, getter_AddRefs(surface), &pc);
        }

        if (surface) {
          PRInt32 sx = mScreenX, sy = mScreenY;
          ConvertToUnscaledDevPixels(pc, &sx, &sy);

          PRInt32 offsetX = sx - dragRect.x;
          PRInt32 offsetY = sy - dragRect.y;
          if (!SetAlphaPixmap(surface, context, offsetX, offsetY, dragRect)) {
            GdkPixbuf* dragPixbuf =
              nsImageToPixbuf::SurfaceToPixbuf(surface, dragRect.width, dragRect.height);
            if (dragPixbuf)
              gtk_drag_set_icon_pixbuf(context, dragPixbuf, offsetX, offsetY);
            else
              needsFallbackIcon = PR_TRUE;
          }
        } else {
          needsFallbackIcon = PR_TRUE;
        }

        if (needsFallbackIcon)
          gtk_drag_set_icon_default(context);
    }

    gtk_target_list_unref(sourceList);

    StartDragSession();

    return rv;
}

PRBool
nsDragService::SetAlphaPixmap(gfxASurface *aSurface,
                                 GdkDragContext *aContext,
                                 PRInt32 aXOffset,
                                 PRInt32 aYOffset,
                                 const nsIntRect& dragRect)
{
    GdkScreen* screen = gtk_widget_get_screen(mHiddenWidget);

    
    
    if (!gdk_screen_is_composited(screen))
      return PR_FALSE;

    GdkColormap* alphaColormap = gdk_screen_get_rgba_colormap(screen);
    if (!alphaColormap)
      return PR_FALSE;

    GdkPixmap* pixmap = gdk_pixmap_new(NULL, dragRect.width, dragRect.height,
                                       gdk_colormap_get_visual(alphaColormap)->depth);
    if (!pixmap)
      return PR_FALSE;

    gdk_drawable_set_colormap(GDK_DRAWABLE(pixmap), alphaColormap);

    
    nsRefPtr<gfxASurface> xPixmapSurface =
         nsWindow::GetSurfaceForGdkDrawable(GDK_DRAWABLE(pixmap),
                                            dragRect.Size());
    if (!xPixmapSurface)
      return PR_FALSE;

    nsRefPtr<gfxContext> xPixmapCtx = new gfxContext(xPixmapSurface);

    
    xPixmapCtx->SetOperator(gfxContext::OPERATOR_CLEAR);
    xPixmapCtx->Paint();

    
    xPixmapCtx->SetOperator(gfxContext::OPERATOR_SOURCE);
    xPixmapCtx->SetSource(aSurface);
    xPixmapCtx->Paint(DRAG_IMAGE_ALPHA_LEVEL);

    
    gtk_drag_set_icon_pixmap(aContext, alphaColormap, pixmap, NULL,
                             aXOffset, aYOffset);
    g_object_unref(pixmap);
    return PR_TRUE;
}

NS_IMETHODIMP
nsDragService::StartDragSession()
{
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("nsDragService::StartDragSession"));
    return nsBaseDragService::StartDragSession();
}
 
NS_IMETHODIMP
nsDragService::EndDragSession(PRBool aDoneDrag)
{
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("nsDragService::EndDragSession %d",
                                   aDoneDrag));
    
    SetDragAction(DRAGDROP_ACTION_NONE);
    return nsBaseDragService::EndDragSession(aDoneDrag);
}


NS_IMETHODIMP
nsDragService::SetCanDrop(PRBool aCanDrop)
{
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("nsDragService::SetCanDrop %d",
                                   aCanDrop));
    mCanDrop = aCanDrop;
    return NS_OK;
}

NS_IMETHODIMP
nsDragService::GetCanDrop(PRBool *aCanDrop)
{
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("nsDragService::GetCanDrop"));
    *aCanDrop = mCanDrop;
    return NS_OK;
}


static PRUint32
CountTextUriListItems(const char *data,
                      PRUint32 datalen)
{
    const char *p = data;
    const char *endPtr = p + datalen;
    PRUint32 count = 0;

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
                   PRUint32 datalen,
                   PRUint32 aItemIndex,
                   PRUnichar **convertedText,
                   PRInt32 *convertedTextLen)
{
    const char *p = data;
    const char *endPtr = p + datalen;
    unsigned int count = 0;

    *convertedText = nsnull;
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
nsDragService::GetNumDropItems(PRUint32 * aNumItems)
{
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("nsDragService::GetNumDropItems"));
    PRBool isList = IsTargetContextList();
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
                       PRUint32 aItemIndex)
{
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("nsDragService::GetData %d", aItemIndex));

    
    if (!aTransferable)
        return NS_ERROR_INVALID_ARG;

    
    
    
    nsresult rv = NS_ERROR_FAILURE;
    nsCOMPtr<nsISupportsArray> flavorList;
    rv = aTransferable->FlavorsTransferableCanImport(
                        getter_AddRefs(flavorList));
    if (NS_FAILED(rv))
        return rv;

    
    PRUint32 cnt;
    flavorList->Count(&cnt);
    unsigned int i;

    
    PRBool isList = IsTargetContextList();

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
            PRUint32 tmpDataLen = 0;
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
            PRBool dataFound = PR_FALSE;
            if (gdkFlavor) {
                GetTargetDragData(gdkFlavor);
            }
            if (mTargetDragData) {
                PR_LOG(sDragLm, PR_LOG_DEBUG, ("dataFound = PR_TRUE\n"));
                dataFound = PR_TRUE;
            }
            else {
                PR_LOG(sDragLm, PR_LOG_DEBUG, ("dataFound = PR_FALSE\n"));

                
                
                if ( strcmp(flavorStr, kFileMime) == 0 ) {
                    gdkFlavor = gdk_atom_intern(kTextMime, FALSE);
                    GetTargetDragData(gdkFlavor);
                    if (mTargetDragData) {
                        const char* text = static_cast<char*>(mTargetDragData);
                        PRUnichar* convertedText = nsnull;
                        PRInt32 convertedTextLen = 0;

                        GetTextUriListItem(text, mTargetDragDataLen, aItemIndex,
                                           &convertedText, &convertedTextLen);

                        if (convertedText) {
                            nsCOMPtr<nsIIOService> ioService = do_GetIOService(&rv);
                            nsCOMPtr<nsIURI> fileURI;
                            nsresult rv = ioService->NewURI(NS_ConvertUTF16toUTF8(convertedText),
                                                            nsnull, nsnull, getter_AddRefs(fileURI));
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
                        PRUnichar* convertedText = nsnull;
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
                            dataFound = PR_TRUE;
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
                            PRUnichar* convertedText = nsnull;
                            PRInt32 convertedTextLen = 0;
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
                                dataFound = PR_TRUE;
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
                        PRUnichar* convertedText = nsnull;
                        PRInt32 convertedTextLen = 0;

                        GetTextUriListItem(data, mTargetDragDataLen, aItemIndex,
                                           &convertedText, &convertedTextLen);

                        if ( convertedText ) {
                            PR_LOG(sDragLm, PR_LOG_DEBUG,
                                   ("successfully converted \
                                   _NETSCAPE_URL to unicode.\n"));
                            
                            g_free(mTargetDragData);
                            mTargetDragData = convertedText;
                            mTargetDragDataLen = convertedTextLen * 2;
                            dataFound = PR_TRUE;
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
                            PRUnichar* convertedText = nsnull;
                            PRInt32 convertedTextLen = 0;
                            nsPrimitiveHelpers::ConvertPlatformPlainTextToUnicode(castedText, mTargetDragDataLen, &convertedText, &convertedTextLen);
                            if ( convertedText ) {
                                PR_LOG(sDragLm,
                                       PR_LOG_DEBUG,
                                       ("successfully converted _NETSCAPE_URL \
                                       to unicode.\n"));
                                
                                g_free(mTargetDragData);
                                mTargetDragData = convertedText;
                                mTargetDragDataLen = convertedTextLen * 2;
                                dataFound = PR_TRUE;
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
                                     PRBool *_retval)
{
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("nsDragService::IsDataFlavorSupported %s",
                                   aDataFlavor));
    if (!_retval)
        return NS_ERROR_INVALID_ARG;

    
    *_retval = PR_FALSE;

    
    if (!mTargetDragContext) {
        PR_LOG(sDragLm, PR_LOG_DEBUG,
               ("*** warning: IsDataFlavorSupported \
               called without a valid drag context!\n"));
        return NS_OK;
    }

    
    PRBool isList = IsTargetContextList();
    
    
    if (isList) {
        PR_LOG(sDragLm, PR_LOG_DEBUG, ("It's a list.."));
        PRUint32 numDragItems = 0;
        
        
        if (!mSourceDataItems)
            return NS_OK;
        mSourceDataItems->Count(&numDragItems);
        for (PRUint32 itemIndex = 0; itemIndex < numDragItems; ++itemIndex) {
            nsCOMPtr<nsISupports> genericItem;
            mSourceDataItems->GetElementAt(itemIndex,
                                           getter_AddRefs(genericItem));
            nsCOMPtr<nsITransferable> currItem(do_QueryInterface(genericItem));
            if (currItem) {
                nsCOMPtr <nsISupportsArray> flavorList;
                currItem->FlavorsTransferableCanExport(
                          getter_AddRefs(flavorList));
                if (flavorList) {
                    PRUint32 numFlavors;
                    flavorList->Count( &numFlavors );
                    for ( PRUint32 flavorIndex = 0;
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
                                *_retval = PR_TRUE;
                            }
                        }
                    }
                }
            }
        }
        return NS_OK;
    }

    
    GList *tmp;
    for (tmp = mTargetDragContext->targets; tmp; tmp = tmp->next) {
        
        GdkAtom atom = GDK_POINTER_TO_ATOM(tmp->data);
        gchar *name = NULL;
        name = gdk_atom_name(atom);
        PR_LOG(sDragLm, PR_LOG_DEBUG,
               ("checking %s against %s\n", name, aDataFlavor));
        if (name && (strcmp(name, aDataFlavor) == 0)) {
            PR_LOG(sDragLm, PR_LOG_DEBUG, ("good!\n"));
            *_retval = PR_TRUE;
        }
        
        if (*_retval == PR_FALSE && 
            name &&
            (strcmp(name, gTextUriListType) == 0) &&
            (strcmp(aDataFlavor, kURLMime) == 0)) {
            PR_LOG(sDragLm, PR_LOG_DEBUG,
                   ("good! ( it's text/uri-list and \
                   we're checking against text/x-moz-url )\n"));
            *_retval = PR_TRUE;
        }
        
        if (*_retval == PR_FALSE && 
            name &&
            (strcmp(name, gMozUrlType) == 0) &&
            (strcmp(aDataFlavor, kURLMime) == 0)) {
            PR_LOG(sDragLm, PR_LOG_DEBUG,
                   ("good! ( it's _NETSCAPE_URL and \
                   we're checking against text/x-moz-url )\n"));
            *_retval = PR_TRUE;
        }
        
        if (*_retval == PR_FALSE && 
            name &&
            (strcmp(name, kTextMime) == 0) &&
            ((strcmp(aDataFlavor, kUnicodeMime) == 0) ||
             (strcmp(aDataFlavor, kFileMime) == 0))) {
            PR_LOG(sDragLm, PR_LOG_DEBUG,
                   ("good! ( it's text plain and we're checking \
                   against text/unicode or application/x-moz-file)\n"));
            *_retval = PR_TRUE;
        }
        g_free(name);
    }
    return NS_OK;
}



NS_IMETHODIMP
nsDragService::TargetSetLastContext(GtkWidget      *aWidget,
                                    GdkDragContext *aContext,
                                    guint           aTime)
{
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("nsDragService::TargetSetLastContext"));
    mTargetWidget = aWidget;
    mTargetDragContext = aContext;
    mTargetTime = aTime;
    return NS_OK;
}

NS_IMETHODIMP
nsDragService::TargetStartDragMotion(void)
{
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("nsDragService::TargetStartDragMotion"));
    mCanDrop = PR_FALSE;
    return NS_OK;
}

NS_IMETHODIMP
nsDragService::TargetEndDragMotion(GtkWidget      *aWidget,
                                   GdkDragContext *aContext,
                                   guint           aTime)
{
    PR_LOG(sDragLm, PR_LOG_DEBUG,
           ("nsDragService::TargetEndDragMotion %d", mCanDrop));

    if (mCanDrop) {
        GdkDragAction action;
        
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
        gdk_drag_status(aContext, action, aTime);
    }
    else {
        gdk_drag_status(aContext, (GdkDragAction)0, aTime);
    }

    return NS_OK;
}

NS_IMETHODIMP
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
    mTargetDragDataReceived = PR_TRUE;
    if (aSelectionData->length > 0) {
        mTargetDragDataLen = aSelectionData->length;
        mTargetDragData = g_malloc(mTargetDragDataLen);
        memcpy(mTargetDragData, aSelectionData->data, mTargetDragDataLen);
    }
    else {
        PR_LOG(sDragLm, PR_LOG_DEBUG,
               ("Failed to get data.  selection data len was %d\n",
                aSelectionData->length));
    }
    return NS_OK;
}


NS_IMETHODIMP
nsDragService::TargetSetTimeCallback(nsIDragSessionGTKTimeCB aCallback)
{
    return NS_OK;
}


PRBool
nsDragService::IsTargetContextList(void)
{
    PRBool retval = PR_FALSE;

    if (!mTargetDragContext)
        return retval;

    
    
    
    
    if (gtk_drag_get_source_widget(mTargetDragContext) == NULL)
        return retval;

    GList *tmp;

    
    
    for (tmp = mTargetDragContext->targets; tmp; tmp = tmp->next) {
        
        GdkAtom atom = GDK_POINTER_TO_ATOM(tmp->data);
        gchar *name = NULL;
        name = gdk_atom_name(atom);
        if (strcmp(name, gMimeListType) == 0)
            retval = PR_TRUE;
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
                                   mTargetWidget, mTargetDragContext));
    
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
    mTargetDragDataReceived = PR_FALSE;
    
    if (mTargetDragData)
      g_free(mTargetDragData);
    mTargetDragData = 0;
    mTargetDragDataLen = 0;
}

GtkTargetList *
nsDragService::GetSourceList(void)
{
    if (!mSourceDataItems)
        return NULL;
    nsTArray<GtkTargetEntry*> targetArray;
    GtkTargetEntry *targets;
    GtkTargetList  *targetList = 0;
    PRUint32 targetCount = 0;
    unsigned int numDragItems = 0;

    mSourceDataItems->Count(&numDragItems);

    
    if (numDragItems > 1) {
        
        
        

        
        
        GdkAtom listAtom = gdk_atom_intern(gMimeListType, FALSE);
        GtkTargetEntry *listTarget =
            (GtkTargetEntry *)g_malloc(sizeof(GtkTargetEntry));
        listTarget->target = g_strdup(gMimeListType);
        listTarget->flags = 0;
        
        listTarget->info = NS_PTR_TO_UINT32(listAtom);
        PR_LOG(sDragLm, PR_LOG_DEBUG,
               ("automatically adding target %s with id %ld\n",
               listTarget->target, listAtom));
        targetArray.AppendElement(listTarget);

        
        
        nsCOMPtr<nsISupports> genericItem;
        mSourceDataItems->GetElementAt(0, getter_AddRefs(genericItem));
        nsCOMPtr<nsITransferable> currItem(do_QueryInterface(genericItem));

        if (currItem) {
            nsCOMPtr <nsISupportsArray> flavorList;
            currItem->FlavorsTransferableCanExport(getter_AddRefs(flavorList));
            if (flavorList) {
                PRUint32 numFlavors;
                flavorList->Count( &numFlavors );
                for (PRUint32 flavorIndex = 0;
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
                            listAtom = gdk_atom_intern(gTextUriListType, FALSE);
                            listTarget =
                             (GtkTargetEntry *)g_malloc(sizeof(GtkTargetEntry));
                            listTarget->target = g_strdup(gTextUriListType);
                            listTarget->flags = 0;
                            
                            listTarget->info = NS_PTR_TO_UINT32(listAtom);
                            PR_LOG(sDragLm, PR_LOG_DEBUG,
                                   ("automatically adding target %s with \
                                   id %ld\n", listTarget->target, listAtom));
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
                PRUint32 numFlavors;
                flavorList->Count( &numFlavors );
                for (PRUint32 flavorIndex = 0;
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
                        
                        GdkAtom atom = gdk_atom_intern(flavorStr, FALSE);
                        GtkTargetEntry *target =
                          (GtkTargetEntry *)g_malloc(sizeof(GtkTargetEntry));
                        target->target = g_strdup(flavorStr);
                        target->flags = 0;
                        
                        target->info = NS_PTR_TO_UINT32(atom);
                        PR_LOG(sDragLm, PR_LOG_DEBUG,
                               ("adding target %s with id %ld\n",
                               target->target, atom));
                        targetArray.AppendElement(target);
                        
                        
                        
                        
                        if (strcmp(flavorStr, kUnicodeMime) == 0) {
                            
                            GdkAtom plainUTF8Atom =
                              gdk_atom_intern(gTextPlainUTF8Type, FALSE);
                            GtkTargetEntry *plainUTF8Target =
                             (GtkTargetEntry *)g_malloc(sizeof(GtkTargetEntry));
                            plainUTF8Target->target = g_strdup(gTextPlainUTF8Type);
                            plainUTF8Target->flags = 0;
                            
                            plainUTF8Target->info = NS_PTR_TO_UINT32(plainUTF8Atom);
                            PR_LOG(sDragLm, PR_LOG_DEBUG,
                                   ("automatically adding target %s with \
                                   id %ld\n", plainUTF8Target->target, plainUTF8Atom));
                            targetArray.AppendElement(plainUTF8Target);

                            
                            GdkAtom plainAtom =
                              gdk_atom_intern(kTextMime, FALSE);
                            GtkTargetEntry *plainTarget =
                             (GtkTargetEntry *)g_malloc(sizeof(GtkTargetEntry));
                            plainTarget->target = g_strdup(kTextMime);
                            plainTarget->flags = 0;
                            
                            plainTarget->info = NS_PTR_TO_UINT32(plainAtom);
                            PR_LOG(sDragLm, PR_LOG_DEBUG,
                                   ("automatically adding target %s with \
                                   id %ld\n", plainTarget->target, plainAtom));
                            targetArray.AppendElement(plainTarget);
                        }
                        
                        
                        
                        if (strcmp(flavorStr, kURLMime) == 0) {
                            
                            GdkAtom urlAtom =
                             gdk_atom_intern(gMozUrlType, FALSE);
                            GtkTargetEntry *urlTarget =
                             (GtkTargetEntry *)g_malloc(sizeof(GtkTargetEntry));
                            urlTarget->target = g_strdup(gMozUrlType);
                            urlTarget->flags = 0;
                            
                            urlTarget->info = NS_PTR_TO_UINT32(urlAtom);
                            PR_LOG(sDragLm, PR_LOG_DEBUG,
                                   ("automatically adding target %s with \
                                   id %ld\n", urlTarget->target, urlAtom));
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
        PRUint32 targetIndex;
        for ( targetIndex = 0; targetIndex < targetCount; ++targetIndex) {
            GtkTargetEntry *disEntry = targetArray.ElementAt(targetIndex);
            
            targets[targetIndex].target = disEntry->target;
            targets[targetIndex].flags = disEntry->flags;
            targets[targetIndex].info = disEntry->info;
        }
        targetList = gtk_target_list_new(targets, targetCount);
        
        for (PRUint32 cleanIndex = 0; cleanIndex < targetCount; ++cleanIndex) {
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
    
    mSourceDataItems = nsnull;

    if (!mDoingDrag)
        return; 

    gint x, y;
    GdkDisplay* display = gdk_display_get_default();
    if (display) {
      gdk_display_get_pointer(display, NULL, &x, &y, NULL);
      SetDragEndPoint(nsIntPoint(x, y));
    }

    
    
    

    PRUint32 dropEffect;

    if (aResult == MOZ_GTK_DRAG_RESULT_SUCCESS) {

        
        
        
        GdkDragAction action =
            aContext->dest_window ? aContext->action : (GdkDragAction)0;

        
        
        
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
            mUserCancelled = PR_TRUE;
        }
    }

    nsCOMPtr<nsIDOMNSDataTransfer> dataTransfer =
        do_QueryInterface(mDataTransfer);

    if (dataTransfer) {
        dataTransfer->SetDropEffectInt(dropEffect);
    }

    
    EndDragSession(PR_TRUE);
}

static void
CreateUriList(nsISupportsArray *items, gchar **text, gint *length)
{
    PRUint32 i, count;
    GString *uriList = g_string_new(NULL);

    items->Count(&count);
    for (i = 0; i < count; i++) {
        nsCOMPtr<nsISupports> genericItem;
        items->GetElementAt(i, getter_AddRefs(genericItem));
        nsCOMPtr<nsITransferable> item;
        item = do_QueryInterface(genericItem);

        if (item) {
            PRUint32 tmpDataLen = 0;
            void    *tmpData = NULL;
            nsresult rv = 0;
            nsCOMPtr<nsISupports> data;
            rv = item->GetTransferData(kURLMime,
                                       getter_AddRefs(data),
                                       &tmpDataLen);

            if (NS_SUCCEEDED(rv)) {
                nsPrimitiveHelpers::CreateDataFromPrimitive(kURLMime,
                                                            data,
                                                            &tmpData,
                                                            tmpDataLen);
                char* plainTextData = nsnull;
                PRUnichar* castedUnicode = reinterpret_cast<PRUnichar*>
                                                           (tmpData);
                PRInt32 plainTextLen = 0;
                nsPrimitiveHelpers::ConvertUnicodeToPlatformPlainText(
                                    castedUnicode,
                                    tmpDataLen / 2,
                                    &plainTextData,
                                    &plainTextLen);
                if (plainTextData) {
                    PRInt32 j;

                    
                    
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
                             guint             aInfo,
                             guint32           aTime)
{
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("nsDragService::SourceDataGet"));
    GdkAtom atom = (GdkAtom)aInfo;
    nsXPIDLCString mimeFlavor;
    gchar *typeName = 0;
    typeName = gdk_atom_name(atom);
    if (!typeName) {
        PR_LOG(sDragLm, PR_LOG_DEBUG, ("failed to get atom name.\n"));
        return;
    }

    PR_LOG(sDragLm, PR_LOG_DEBUG, ("Type is %s\n", typeName));
    
    mimeFlavor.Adopt(nsCRT::strdup(typeName));
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
        
        
        PRBool needToDoConversionToPlainText = PR_FALSE;
        const char* actualFlavor = mimeFlavor;
        if (strcmp(mimeFlavor, kTextMime) == 0 ||
            strcmp(mimeFlavor, gTextPlainUTF8Type) == 0) {
            actualFlavor = kUnicodeMime;
            needToDoConversionToPlainText = PR_TRUE;
        }
        
        
        else if (strcmp(mimeFlavor, gMozUrlType) == 0) {
            actualFlavor = kURLMime;
            needToDoConversionToPlainText = PR_TRUE;
        }
        
        
        else if (strcmp(mimeFlavor, gTextUriListType) == 0) {
            actualFlavor = gTextUriListType;
            needToDoConversionToPlainText = PR_TRUE;
        }
        else
            actualFlavor = mimeFlavor;

        PRUint32 tmpDataLen = 0;
        void    *tmpData = NULL;
        nsresult rv;
        nsCOMPtr<nsISupports> data;
        rv = item->GetTransferData(actualFlavor,
                                   getter_AddRefs(data),
                                   &tmpDataLen);
        if (NS_SUCCEEDED(rv)) {
            nsPrimitiveHelpers::CreateDataFromPrimitive (actualFlavor, data,
                                                         &tmpData, tmpDataLen);
            
            
            if (needToDoConversionToPlainText) {
                char* plainTextData = nsnull;
                PRUnichar* castedUnicode = reinterpret_cast<PRUnichar*>
                                                           (tmpData);
                PRInt32 plainTextLen = 0;
                if (strcmp(mimeFlavor, gTextPlainUTF8Type) == 0) {
                    plainTextData =
                        ToNewUTF8String(
                            nsDependentString(castedUnicode, tmpDataLen / 2),
                            (PRUint32*)&plainTextLen);
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
                
                gtk_selection_data_set(aSelectionData,
                                       aSelectionData->target,
                                       8,
                                       (guchar *)tmpData, tmpDataLen);
                
                free(tmpData);
            }
        } else {
            if (strcmp(mimeFlavor, gTextUriListType) == 0) {
                
                gchar *uriList;
                gint length;
                CreateUriList(mSourceDataItems, &uriList, &length);
                gtk_selection_data_set(aSelectionData,
                                       aSelectionData->target,
                                       8, (guchar *)uriList, length);
                g_free(uriList);
                return;
            }
        }
    }
}


void
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
                               aSelectionData, aInfo, aTime);
}


gboolean
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


void
invisibleSourceDragEnd(GtkWidget        *aWidget,
                       GdkDragContext   *aContext,
                       gpointer          aData)
{
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("invisibleSourceDragEnd"));
    nsDragService *dragService = (nsDragService *)aData;

    
    dragService->SourceEndDragSession(aContext, MOZ_GTK_DRAG_RESULT_SUCCESS);
}

