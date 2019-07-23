










































#include "nsDragService.h"
#include "nsIObserverService.h"
#include "nsWidgetsCID.h"
#include "nsWindow.h"
#include "nsIServiceManager.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "prlog.h"
#include "nsVoidArray.h"
#include "nsXPIDLString.h"
#include "nsPrimitiveHelpers.h"
#include "prtime.h"
#include "prthread.h"
#include <gtk/gtkinvisible.h>
#include <gdk/gdkx.h>
#include "nsCRT.h"

#include "gfxASurface.h"
#include "nsImageToPixbuf.h"

static PRLogModuleInfo *sDragLm = NULL;

static const char gMimeListType[] = "application/x-moz-internal-item-list";
static const char gMozUrlType[] = "_NETSCAPE_URL";
static const char gTextUriListType[] = "text/uri-list";

NS_IMPL_ADDREF_INHERITED(nsDragService, nsBaseDragService)
NS_IMPL_RELEASE_INHERITED(nsDragService, nsBaseDragService)
NS_IMPL_QUERY_INTERFACE4(nsDragService,
                         nsIDragService,
                         nsIDragSession,
                         nsIDragSessionGTK,
                         nsIObserver)

static void
invisibleSourceDragEnd(GtkWidget        *aWidget,
                       GdkDragContext   *aContext,
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
    
    
    gtk_signal_connect(GTK_OBJECT(mHiddenWidget), "drag_data_get",
                       GTK_SIGNAL_FUNC(invisibleSourceDragDataGet), this);
    gtk_signal_connect(GTK_OBJECT(mHiddenWidget), "drag_end",
                       GTK_SIGNAL_FUNC(invisibleSourceDragEnd), this);

    
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
    nsBaseDragService::InvokeDragSession(aDOMNode, aArrayTransferables,
                                         aRegion, aActionType);
    
    if (!aArrayTransferables)
        return NS_ERROR_INVALID_ARG;
    
    
    
    mSourceDataItems = aArrayTransferables;
    
    GtkTargetList *sourceList = 0;

    sourceList = GetSourceList();

    if (sourceList) {
        
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

        GdkPixbuf* dragPixbuf = nsnull;
        nsRect dragRect;
        if (mHasImage || mSelection) {
          nsRefPtr<gfxASurface> surface;
          DrawDrag(aDOMNode, aRegion, mScreenX, mScreenY,
                   &dragRect, getter_AddRefs(surface));
          if (surface) {
            dragPixbuf =
              nsImageToPixbuf::SurfaceToPixbuf(surface, dragRect.width, dragRect.height);
          }
        }

        if (dragPixbuf)
          gtk_drag_set_icon_pixbuf(context, dragPixbuf,
                                   mScreenX - NSToIntRound(dragRect.x),
                                   mScreenY - NSToIntRound(dragRect.y));
        else
          gtk_drag_set_icon_default(context);

        gtk_target_list_unref(sourceList);
    }

    return NS_OK;
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
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("nsDragService::EndDragSession"));
    
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
            const char *data = NS_REINTERPRET_CAST(char*, mTargetDragData);
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
        nsCOMPtr<nsISupports> genericWrapper;
        
        flavorList->GetElementAt(0, getter_AddRefs(genericWrapper));
        nsCOMPtr<nsISupportsCString> currentFlavor;
        currentFlavor = do_QueryInterface(genericWrapper);
        if (currentFlavor) {
            nsXPIDLCString flavorStr;
            currentFlavor->ToString(getter_Copies(flavorStr));
            PR_LOG(sDragLm,
                   PR_LOG_DEBUG,
                   ("flavor is %s\n", (const char *)flavorStr));
            
            nsCOMPtr<nsISupports> genericItem;
            mSourceDataItems->GetElementAt(aItemIndex,
                                           getter_AddRefs(genericItem));
            nsCOMPtr<nsITransferable> item(do_QueryInterface(genericItem));
            if (item) {
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
                    return NS_ERROR_FAILURE;
                }
                PR_LOG(sDragLm, PR_LOG_DEBUG, ("succeeded.\n"));
                rv = aTransferable->SetTransferData(flavorStr,data,tmpDataLen);
                if (NS_FAILED(rv)) {
                    PR_LOG(sDragLm,
                           PR_LOG_DEBUG,
                           ("fail to set transfer data into transferable!\n"));
                    return NS_ERROR_FAILURE;
                }
                
                return NS_OK;
            }
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
                   NS_STATIC_CAST(const char*,flavorStr), gdkFlavor));
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
                
                
                
                if ( strcmp(flavorStr, kUnicodeMime) == 0 ) {
                    PR_LOG(sDragLm, PR_LOG_DEBUG,
                           ("we were looking for text/unicode... \
                           trying again with text/plain\n"));
                    gdkFlavor = gdk_atom_intern(kTextMime, FALSE);
                    GetTargetDragData(gdkFlavor);
                    if (mTargetDragData) {
                        PR_LOG(sDragLm, PR_LOG_DEBUG, ("Got textplain data\n"));
                        const char* castedText =
                                    NS_REINTERPRET_CAST(char*, mTargetDragData);
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
                                   NS_REINTERPRET_CAST(char*, mTargetDragData);
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
                                  NS_REINTERPRET_CAST(char*, mTargetDragData);
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
                             NS_REINTERPRET_CAST(int*, &mTargetDragDataLen));
        
                
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
        GdkAtom atom = (GdkAtom)GPOINTER_TO_INT(tmp->data);
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
            (strcmp(aDataFlavor, kUnicodeMime) == 0)) {
            PR_LOG(sDragLm, PR_LOG_DEBUG,
                   ("good! ( it's text plain and we're checking \
                   against text/unicode )\n"));
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
        GdkAtom atom = (GdkAtom)GPOINTER_TO_INT(tmp->data);
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
    nsVoidArray targetArray;
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
        listTarget->info = GPOINTER_TO_UINT(listAtom);
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
                            listTarget->info = GPOINTER_TO_UINT(listAtom);
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
                        target->info = GPOINTER_TO_UINT(atom);
                        PR_LOG(sDragLm, PR_LOG_DEBUG,
                               ("adding target %s with id %ld\n",
                               target->target, atom));
                        targetArray.AppendElement(target);
                        
                        
                        
                        
                        if (strcmp(flavorStr, kUnicodeMime) == 0) {
                            
                            GdkAtom plainAtom =
                              gdk_atom_intern(kTextMime, FALSE);
                            GtkTargetEntry *plainTarget =
                             (GtkTargetEntry *)g_malloc(sizeof(GtkTargetEntry));
                            plainTarget->target = g_strdup(kTextMime);
                            plainTarget->flags = 0;
                            plainTarget->info = GPOINTER_TO_UINT(plainAtom);
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
                            urlTarget->info = GPOINTER_TO_UINT(urlAtom);
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

    
    targetCount = targetArray.Count();
    if (targetCount) {
        
        targets =
          (GtkTargetEntry *)g_malloc(sizeof(GtkTargetEntry) * targetCount);
        PRUint32 targetIndex;
        for ( targetIndex = 0; targetIndex < targetCount; ++targetIndex) {
            GtkTargetEntry *disEntry =
              (GtkTargetEntry *)targetArray.ElementAt(targetIndex);
            
            targets[targetIndex].target = disEntry->target;
            targets[targetIndex].flags = disEntry->flags;
            targets[targetIndex].info = disEntry->info;
        }
        targetList = gtk_target_list_new(targets, targetCount);
        
        for (PRUint32 cleanIndex = 0; cleanIndex < targetCount; ++cleanIndex) {
            GtkTargetEntry *thisTarget =
              (GtkTargetEntry *)targetArray.ElementAt(cleanIndex);
            g_free(thisTarget->target);
            g_free(thisTarget);
        }
        g_free(targets);
    }
    return targetList;
}

void
nsDragService::SourceEndDrag(void)
{
    
    mSourceDataItems = 0;

    
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
                PRUnichar* castedUnicode = NS_REINTERPRET_CAST(PRUnichar*,
                                                               tmpData);
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
        if (strcmp(mimeFlavor,kTextMime) == 0) {
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
                PRUnichar* castedUnicode = NS_REINTERPRET_CAST(PRUnichar*,
                                                               tmpData);
                PRInt32 plainTextLen = 0;
                nsPrimitiveHelpers::ConvertUnicodeToPlatformPlainText(
                                    castedUnicode,
                                    tmpDataLen / 2,
                                    &plainTextData,
                                    &plainTextLen);
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
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("invisibleDragDataGet"));
    nsDragService *dragService = (nsDragService *)aData;
    dragService->SourceDataGet(aWidget, aContext,
                               aSelectionData, aInfo, aTime);
}


void
invisibleSourceDragEnd(GtkWidget        *aWidget,
                       GdkDragContext   *aContext,
                       gpointer          aData)
{
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("invisibleDragEnd"));
    nsDragService *dragService = (nsDragService *)aData;
    
    dragService->SourceEndDrag();
}

