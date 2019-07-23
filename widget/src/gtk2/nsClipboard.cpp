





































#include "nsClipboard.h"
#include "nsSupportsPrimitives.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsXPIDLString.h"
#include "nsPrimitiveHelpers.h"
#include "nsICharsetConverterManager.h"
#include "nsIServiceManager.h"
#include "nsImageToPixbuf.h"
#include "nsStringStream.h"
#include "nsIObserverService.h"

#include "imgIContainer.h"

#include <gtk/gtk.h>


#include <X11/Xlib.h>
#include <gdk/gdkx.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef POLL_WITH_XCONNECTIONNUMBER
#include <poll.h>
#endif


void
clipboard_get_cb(GtkClipboard *aGtkClipboard,
                 GtkSelectionData *aSelectionData,
                 guint info,
                 gpointer user_data);


void
clipboard_clear_cb(GtkClipboard *aGtkClipboard,
                   gpointer user_data);
                   
static void
ConvertHTMLtoUCS2          (guchar             *data,
                            PRInt32             dataLength,
                            PRUnichar         **unicodeData,
                            PRInt32            &outUnicodeLen);

static void
GetHTMLCharset             (guchar * data, PRInt32 dataLength, nsCString& str);







static GtkSelectionData *
wait_for_contents          (GtkClipboard *clipboard, GdkAtom target);

static gchar *
wait_for_text              (GtkClipboard *clipboard);

static Bool
checkEventProc(Display *display, XEvent *event, XPointer arg);

struct retrieval_context
{
    PRBool   completed;
    void    *data;

    retrieval_context() : completed(PR_FALSE), data(nsnull) { }
};

static void
wait_for_retrieval(GtkClipboard *clipboard, retrieval_context *transferData);

static void
clipboard_contents_received(GtkClipboard     *clipboard,
                            GtkSelectionData *selection_data,
                            gpointer          data);

static void
clipboard_text_received(GtkClipboard *clipboard,
                        const gchar  *text,
                        gpointer      data);

nsClipboard::nsClipboard()
{    
}

nsClipboard::~nsClipboard()
{
}

NS_IMPL_ISUPPORTS1(nsClipboard, nsIClipboard)

nsresult
nsClipboard::Init(void)
{    
    nsresult rv;
    nsCOMPtr<nsIObserverService> os
      (do_GetService("@mozilla.org/observer-service;1", &rv));
    NS_ENSURE_SUCCESS(rv, rv);

    os->AddObserver(this, "quit-application", PR_FALSE);

    return NS_OK;
}

NS_IMETHODIMP
nsClipboard::Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *aData)
{
    if (strcmp(aTopic, "quit-application") == 0) {
        
        Store();
    }
    return NS_OK;
}

nsresult
nsClipboard::Store(void)
{
    
    if (mGlobalTransferable) {
        GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
        gtk_clipboard_store(clipboard);
    }
    return NS_OK;
}

NS_IMETHODIMP
nsClipboard::SetData(nsITransferable *aTransferable,
                     nsIClipboardOwner *aOwner, PRInt32 aWhichClipboard)
{
    
    if ((aWhichClipboard == kGlobalClipboard &&
         aTransferable == mGlobalTransferable.get() &&
         aOwner == mGlobalOwner.get()) ||
        (aWhichClipboard == kSelectionClipboard &&
         aTransferable == mSelectionTransferable.get() &&
         aOwner == mSelectionOwner.get())) {
        return NS_OK;
    }

    nsresult rv;
    if (!mPrivacyHandler) {
        rv = NS_NewClipboardPrivacyHandler(getter_AddRefs(mPrivacyHandler));
        NS_ENSURE_SUCCESS(rv, rv);
    }
    rv = mPrivacyHandler->PrepareDataForClipboard(aTransferable);
    NS_ENSURE_SUCCESS(rv, rv);

    
    EmptyClipboard(aWhichClipboard);

    
    GtkTargetList *list = gtk_target_list_new(NULL, 0);

    
    nsCOMPtr<nsISupportsArray> flavors;

    rv = aTransferable->FlavorsTransferableCanExport(getter_AddRefs(flavors));
    if (!flavors || NS_FAILED(rv))
        return NS_ERROR_FAILURE;

    
    PRBool imagesAdded = PR_FALSE;
    PRUint32 count;
    flavors->Count(&count);
    for (PRUint32 i=0; i < count; i++) {
        nsCOMPtr<nsISupports> tastesLike;
        flavors->GetElementAt(i, getter_AddRefs(tastesLike));
        nsCOMPtr<nsISupportsCString> flavor = do_QueryInterface(tastesLike);

        if (flavor) {
            nsXPIDLCString flavorStr;
            flavor->ToString(getter_Copies(flavorStr));

            
            
            if (!strcmp(flavorStr, kUnicodeMime)) {
                gtk_target_list_add(list, gdk_atom_intern("UTF8_STRING", FALSE), 0, 0);
                gtk_target_list_add(list, gdk_atom_intern("COMPOUND_TEXT", FALSE), 0, 0);
                gtk_target_list_add(list, gdk_atom_intern("TEXT", FALSE), 0, 0);
                gtk_target_list_add(list, GDK_SELECTION_TYPE_STRING, 0, 0);
                continue;
            }

            if (flavorStr.EqualsLiteral(kNativeImageMime) ||
                flavorStr.EqualsLiteral(kPNGImageMime) ||
                flavorStr.EqualsLiteral(kJPEGImageMime) ||
                flavorStr.EqualsLiteral(kGIFImageMime)) {
                
                if (!imagesAdded) {
                    
                    gtk_target_list_add_image_targets(list, 0, TRUE);
                    imagesAdded = PR_TRUE;
                }
                continue;
            }

            
            GdkAtom atom = gdk_atom_intern(flavorStr, FALSE);
            gtk_target_list_add(list, atom, 0, 0);
        }
    }
    
    
    GtkClipboard *gtkClipboard = gtk_clipboard_get(GetSelectionAtom(aWhichClipboard));
  
    gint numTargets;
    GtkTargetEntry *gtkTargets = gtk_target_table_new_from_list(list, &numTargets);
          
    
    if (gtk_clipboard_set_with_data(gtkClipboard, gtkTargets, numTargets, 
                                    clipboard_get_cb, clipboard_clear_cb, this))
    {
        
        
        
        if (aWhichClipboard == kSelectionClipboard) {
            mSelectionOwner = aOwner;
            mSelectionTransferable = aTransferable;
        }
        else {
            mGlobalOwner = aOwner;
            mGlobalTransferable = aTransferable;
            gtk_clipboard_set_can_store(gtkClipboard, gtkTargets, numTargets);
        }

        rv = NS_OK;
    }
    else {  
        rv = NS_ERROR_FAILURE;
    }

    gtk_target_table_free(gtkTargets, numTargets);
    gtk_target_list_unref(list);
  
    return rv;
}

NS_IMETHODIMP
nsClipboard::GetData(nsITransferable *aTransferable, PRInt32 aWhichClipboard)
{
    if (!aTransferable)
        return NS_ERROR_FAILURE;

    GtkClipboard *clipboard;
    clipboard = gtk_clipboard_get(GetSelectionAtom(aWhichClipboard));

    guchar        *data = NULL;
    gint           length = 0;
    PRBool         foundData = PR_FALSE;
    nsCAutoString  foundFlavor;

    
    nsCOMPtr<nsISupportsArray> flavors;
    nsresult rv;
    rv = aTransferable->FlavorsTransferableCanImport(getter_AddRefs(flavors));
    if (!flavors || NS_FAILED(rv))
        return NS_ERROR_FAILURE;

    PRUint32 count;
    flavors->Count(&count);
    for (PRUint32 i=0; i < count; i++) {
        nsCOMPtr<nsISupports> genericFlavor;
        flavors->GetElementAt(i, getter_AddRefs(genericFlavor));

        nsCOMPtr<nsISupportsCString> currentFlavor;
        currentFlavor = do_QueryInterface(genericFlavor);

        if (currentFlavor) {
            nsXPIDLCString flavorStr;
            currentFlavor->ToString(getter_Copies(flavorStr));

            
            
            if (!strcmp(flavorStr, kUnicodeMime)) {
                gchar* new_text = wait_for_text(clipboard);
                if (new_text) {
                    
                    NS_ConvertUTF8toUTF16 ucs2string(new_text);
                    data = (guchar *)ToNewUnicode(ucs2string);
                    length = ucs2string.Length() * 2;
                    g_free(new_text);
                    foundData = PR_TRUE;
                    foundFlavor = kUnicodeMime;
                    break;
                }
                
                
                
                continue;
            }

            
            
            if (!strcmp(flavorStr, kJPEGImageMime) || !strcmp(flavorStr, kPNGImageMime) || !strcmp(flavorStr, kGIFImageMime)) {
                GdkAtom atom;
                if (!strcmp(flavorStr, kJPEGImageMime)) 
                    atom = gdk_atom_intern("image/jpeg", FALSE);
                else
                    atom = gdk_atom_intern(flavorStr, FALSE);

                GtkSelectionData *selectionData = wait_for_contents(clipboard, atom);
                if (!selectionData)
                    continue;

                nsCOMPtr<nsIInputStream> byteStream;
                NS_NewByteInputStream(getter_AddRefs(byteStream), (const char*)selectionData->data,
                                      selectionData->length, NS_ASSIGNMENT_COPY);
                aTransferable->SetTransferData(flavorStr, byteStream, sizeof(nsIInputStream*));
                gtk_selection_data_free(selectionData);
                return NS_OK;
            }

            
            
            GdkAtom atom = gdk_atom_intern(flavorStr, FALSE);
            GtkSelectionData *selectionData;
            selectionData = wait_for_contents(clipboard, atom);
            if (selectionData) {
                length = selectionData->length;
                
                if (!strcmp(flavorStr, kHTMLMime)) {
                    PRUnichar* htmlBody= nsnull;
                    PRInt32 htmlBodyLen = 0;
                    
                    ConvertHTMLtoUCS2((guchar *)selectionData->data, length,
                                      &htmlBody, htmlBodyLen);
                    if (!htmlBodyLen)
                        break;
                    data = (guchar *)htmlBody;
                    length = htmlBodyLen * 2;
                } else {
                    data = (guchar *)nsMemory::Alloc(length);
                    if (!data)
                        break;
                    memcpy(data, selectionData->data, length);
                }
                foundData = PR_TRUE;
                foundFlavor = flavorStr;
                break;
            }
        }
    }

    if (foundData) {
        nsCOMPtr<nsISupports> wrapper;
        nsPrimitiveHelpers::CreatePrimitiveForData(foundFlavor.get(),
                                                   data, length,
                                                   getter_AddRefs(wrapper));
        aTransferable->SetTransferData(foundFlavor.get(),
                                       wrapper, length);
    }

    if (data)
        nsMemory::Free(data);

    return NS_OK;
}

NS_IMETHODIMP
nsClipboard::EmptyClipboard(PRInt32 aWhichClipboard)
{
    if (aWhichClipboard == kSelectionClipboard) {
        if (mSelectionOwner) {
            mSelectionOwner->LosingOwnership(mSelectionTransferable);
            mSelectionOwner = nsnull;
        }
        mSelectionTransferable = nsnull;
    }
    else {
        if (mGlobalOwner) {
            mGlobalOwner->LosingOwnership(mGlobalTransferable);
            mGlobalOwner = nsnull;
        }
        mGlobalTransferable = nsnull;
    }

    return NS_OK;
}

NS_IMETHODIMP
nsClipboard::HasDataMatchingFlavors(const char** aFlavorList, PRUint32 aLength,
                                    PRInt32 aWhichClipboard, PRBool *_retval)
{
    if (!aFlavorList || !_retval)
        return NS_ERROR_NULL_POINTER;

    *_retval = PR_FALSE;

    GtkSelectionData *selection_data =
        GetTargets(GetSelectionAtom(aWhichClipboard));
    if (!selection_data)
        return NS_OK;

    gint n_targets = 0;
    GdkAtom *targets = NULL;

    if (!gtk_selection_data_get_targets(selection_data, 
                                        &targets, &n_targets) ||
        !n_targets)
        return NS_OK;

    
    
    for (PRUint32 i = 0; i < aLength && !*_retval; i++) {
        
        if (!strcmp(aFlavorList[i], kUnicodeMime) && 
            gtk_selection_data_targets_include_text(selection_data)) {
            *_retval = PR_TRUE;
            break;
        }

        for (PRInt32 j = 0; j < n_targets; j++) {
            gchar *atom_name = gdk_atom_name(targets[j]);
            if (!strcmp(atom_name, aFlavorList[i]))
                *_retval = PR_TRUE;

            
            if (!strcmp(aFlavorList[i], kJPEGImageMime) && !strcmp(atom_name, "image/jpeg"))
                *_retval = PR_TRUE;

            g_free(atom_name);

            if (*_retval)
                break;
        }
    }
    gtk_selection_data_free(selection_data);
    g_free(targets);

    return NS_OK;
}

NS_IMETHODIMP
nsClipboard::SupportsSelectionClipboard(PRBool *_retval)
{
    *_retval = PR_TRUE; 
    return NS_OK;
}


GdkAtom
nsClipboard::GetSelectionAtom(PRInt32 aWhichClipboard)
{
    if (aWhichClipboard == kGlobalClipboard)
        return GDK_SELECTION_CLIPBOARD;

    return GDK_SELECTION_PRIMARY;
}


GtkSelectionData *
nsClipboard::GetTargets(GdkAtom aWhichClipboard)
{
    GtkClipboard *clipboard = gtk_clipboard_get(aWhichClipboard);
    return wait_for_contents(clipboard, gdk_atom_intern("TARGETS", FALSE));
}

nsITransferable *
nsClipboard::GetTransferable(PRInt32 aWhichClipboard)
{
    nsITransferable *retval;

    if (aWhichClipboard == kSelectionClipboard)
        retval = mSelectionTransferable.get();
    else
        retval = mGlobalTransferable.get();
        
    return retval;
}

void
nsClipboard::SelectionGetEvent(GtkClipboard     *aClipboard,
                               GtkSelectionData *aSelectionData)
{
    
    
    
    

    PRInt32 whichClipboard;

    
    if (aSelectionData->selection == GDK_SELECTION_PRIMARY)
        whichClipboard = kSelectionClipboard;
    else if (aSelectionData->selection == GDK_SELECTION_CLIPBOARD)
        whichClipboard = kGlobalClipboard;
    else
        return; 

    nsCOMPtr<nsITransferable> trans = GetTransferable(whichClipboard);
    if (!trans) {
      
#ifdef DEBUG_CLIPBOARD
      printf("nsClipboard::SelectionGetEvent() - %s clipboard is empty!\n",
             whichClipboard == kSelectionClipboard ? "Selection" : "Global");
#endif
      return;
    }

    nsresult rv;
    nsCOMPtr<nsISupports> item;
    PRUint32 len;

    
    
    if (aSelectionData->target == gdk_atom_intern ("STRING", FALSE) ||
        aSelectionData->target == gdk_atom_intern ("TEXT", FALSE) ||
        aSelectionData->target == gdk_atom_intern ("COMPOUND_TEXT", FALSE) ||
        aSelectionData->target == gdk_atom_intern ("UTF8_STRING", FALSE)) {
        
        
        
        rv = trans->GetTransferData("text/unicode", getter_AddRefs(item),
                                    &len);
        if (!item || NS_FAILED(rv))
            return;
        
        nsCOMPtr<nsISupportsString> wideString;
        wideString = do_QueryInterface(item);
        if (!wideString)
            return;

        nsAutoString ucs2string;
        wideString->GetData(ucs2string);
        char *utf8string = ToNewUTF8String(ucs2string);
        if (!utf8string)
            return;
        
        gtk_selection_data_set_text (aSelectionData, utf8string,
                                     strlen(utf8string));

        nsMemory::Free(utf8string);
        return;
    }

    
    if (gtk_targets_include_image(&aSelectionData->target, 1, TRUE)) {
        
        static const char* const imageMimeTypes[] = {
            kNativeImageMime, kPNGImageMime, kJPEGImageMime, kGIFImageMime };
        nsCOMPtr<nsISupports> item;
        PRUint32 len;
        nsCOMPtr<nsISupportsInterfacePointer> ptrPrimitive;
        for (PRUint32 i = 0; !ptrPrimitive && i < NS_ARRAY_LENGTH(imageMimeTypes); i++) {
            rv = trans->GetTransferData(imageMimeTypes[i], getter_AddRefs(item), &len);
            ptrPrimitive = do_QueryInterface(item);
        }
        if (!ptrPrimitive)
            return;

        nsCOMPtr<nsISupports> primitiveData;
        ptrPrimitive->GetData(getter_AddRefs(primitiveData));
        nsCOMPtr<imgIContainer> image(do_QueryInterface(primitiveData));
        if (!image) 
            return;

        GdkPixbuf* pixbuf = nsImageToPixbuf::ImageToPixbuf(image);
        if (!pixbuf)
            return;

        gtk_selection_data_set_pixbuf(aSelectionData, pixbuf);
        g_object_unref(pixbuf);
        return;
    }

    
    
    gchar *target_name = gdk_atom_name(aSelectionData->target);
    if (!target_name)
        return;

    rv = trans->GetTransferData(target_name, getter_AddRefs(item), &len);
    
    if (!item || NS_FAILED(rv)) {
        g_free(target_name);
        return;
    }

    void *primitive_data = nsnull;
    nsPrimitiveHelpers::CreateDataFromPrimitive(target_name, item,
                                                &primitive_data, len);

    if (primitive_data) {
        
        if (aSelectionData->target == gdk_atom_intern (kHTMLMime, FALSE)) {
            






            guchar *buffer = (guchar *)
                    nsMemory::Alloc((len * sizeof(guchar)) + sizeof(PRUnichar));
            if (!buffer)
                return;
            PRUnichar prefix = 0xFEFF;
            memcpy(buffer, &prefix, sizeof(prefix));
            memcpy(buffer + sizeof(prefix), primitive_data, len);
            nsMemory::Free((guchar *)primitive_data);
            primitive_data = (guchar *)buffer;
            len += sizeof(prefix);
        }
  
        gtk_selection_data_set(aSelectionData, aSelectionData->target,
                               8, 
                               (const guchar *)primitive_data, len);
        nsMemory::Free(primitive_data);
    }

    g_free(target_name);
                           
}

void
nsClipboard::SelectionClearEvent(GtkClipboard *aGtkClipboard)
{
    PRInt32 whichClipboard;

    
    if (aGtkClipboard == gtk_clipboard_get(GDK_SELECTION_PRIMARY))
        whichClipboard = kSelectionClipboard;
    else if (aGtkClipboard == gtk_clipboard_get(GDK_SELECTION_CLIPBOARD))
        whichClipboard = kGlobalClipboard;
    else
        return; 

    EmptyClipboard(whichClipboard);
}

void
clipboard_get_cb(GtkClipboard *aGtkClipboard,
                 GtkSelectionData *aSelectionData,
                 guint info,
                 gpointer user_data)
{
    nsClipboard *aClipboard = static_cast<nsClipboard *>(user_data);
    aClipboard->SelectionGetEvent(aGtkClipboard, aSelectionData);
}

void
clipboard_clear_cb(GtkClipboard *aGtkClipboard,
                   gpointer user_data)
{
    nsClipboard *aClipboard = static_cast<nsClipboard *>(user_data);
    aClipboard->SelectionClearEvent(aGtkClipboard);
}




















void ConvertHTMLtoUCS2(guchar * data, PRInt32 dataLength,
                       PRUnichar** unicodeData, PRInt32& outUnicodeLen)
{
    nsCAutoString charset;
    GetHTMLCharset(data, dataLength, charset);
    if (charset.EqualsLiteral("UTF-16")) {
        outUnicodeLen = (dataLength / 2) - 1;
        *unicodeData = reinterpret_cast<PRUnichar*>
                                       (nsMemory::Alloc((outUnicodeLen + sizeof('\0')) *
                       sizeof(PRUnichar)));
        if (*unicodeData) {
            memcpy(*unicodeData, data + sizeof(PRUnichar),
                   outUnicodeLen * sizeof(PRUnichar));
            (*unicodeData)[outUnicodeLen] = '\0';
        }
    } else if (charset.EqualsLiteral("UNKNOWN")) {
        outUnicodeLen = 0;
        return;
    } else {
        
        nsCOMPtr<nsIUnicodeDecoder> decoder;
        nsresult rv;
        
        nsCOMPtr<nsICharsetConverterManager> ccm =
            do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);
        if (NS_FAILED(rv)) {
#ifdef DEBUG_CLIPBOARD
            g_print("        can't get CHARSET CONVERTER MANAGER service\n");
#endif
            outUnicodeLen = 0;
            return;
        }
        rv = ccm->GetUnicodeDecoder(charset.get(), getter_AddRefs(decoder));
        if (NS_FAILED(rv)) {
#ifdef DEBUG_CLIPBOARD
            g_print("        get unicode decoder error\n");
#endif
            outUnicodeLen = 0;
            return;
        }
        
        decoder->GetMaxLength((const char *)data, dataLength, &outUnicodeLen);
        
        if (outUnicodeLen) {
            *unicodeData = reinterpret_cast<PRUnichar*>
                                           (nsMemory::Alloc((outUnicodeLen + sizeof('\0')) *
                           sizeof(PRUnichar)));
            if (*unicodeData) {
                PRInt32 numberTmp = dataLength;
                decoder->Convert((const char *)data, &numberTmp,
                                 *unicodeData, &outUnicodeLen);
#ifdef DEBUG_CLIPBOARD
                if (numberTmp != dataLength)
                    printf("didn't consume all the bytes\n");
#endif
                
                (*unicodeData)[outUnicodeLen] = '\0';
            }
        } 
    }
}








void GetHTMLCharset(guchar * data, PRInt32 dataLength, nsCString& str)
{
    
    PRUnichar* beginChar =  (PRUnichar*)data;
    if ((beginChar[0] == 0xFFFE) || (beginChar[0] == 0xFEFF)) {
        str.AssignLiteral("UTF-16");
        return;
    }
    
    const nsDependentCString htmlStr((const char *)data, dataLength);
    nsACString::const_iterator start, end;
    htmlStr.BeginReading(start);
    htmlStr.EndReading(end);
    nsACString::const_iterator valueStart(start), valueEnd(start);

    if (CaseInsensitiveFindInReadable(
        NS_LITERAL_CSTRING("CONTENT=\"text/html;"),
        start, end)) {
        start = end;
        htmlStr.EndReading(end);

        if (CaseInsensitiveFindInReadable(
            NS_LITERAL_CSTRING("charset="),
            start, end)) {
            valueStart = end;
            start = end;
            htmlStr.EndReading(end);
          
            if (FindCharInReadable('"', start, end))
                valueEnd = start;
        }
    }
    
    if (valueStart != valueEnd) {
        str = Substring(valueStart, valueEnd);
        ToUpperCase(str);
#ifdef DEBUG_CLIPBOARD
        printf("Charset of HTML = %s\n", charsetUpperStr.get());
#endif
        return;
    }
    str.AssignLiteral("UNKNOWN");
}

static void
DispatchSelectionNotifyEvent(GtkWidget *widget, XEvent *xevent)
{
    GdkEvent event;
    event.selection.type = GDK_SELECTION_NOTIFY;
    event.selection.window = widget->window;
    event.selection.selection = gdk_x11_xatom_to_atom(xevent->xselection.selection);
    event.selection.target = gdk_x11_xatom_to_atom(xevent->xselection.target);
    event.selection.property = gdk_x11_xatom_to_atom(xevent->xselection.property);
    event.selection.time = xevent->xselection.time;

    gtk_widget_event(widget, &event);
}

static void
DispatchPropertyNotifyEvent(GtkWidget *widget, XEvent *xevent)
{
    if (((GdkWindowObject *) widget->window)->event_mask & GDK_PROPERTY_CHANGE_MASK) {
        GdkEvent event;
        event.property.type = GDK_PROPERTY_NOTIFY;
        event.property.window = widget->window;
        event.property.atom = gdk_x11_xatom_to_atom(xevent->xproperty.atom);
        event.property.time = xevent->xproperty.time;
        event.property.state = xevent->xproperty.state;

        gtk_widget_event(widget, &event);
    }
}

struct checkEventContext
{
    GtkWidget *cbWidget;
    Atom       selAtom;
};

static Bool
checkEventProc(Display *display, XEvent *event, XPointer arg)
{
    checkEventContext *context = (checkEventContext *) arg;

    if (event->xany.type == SelectionNotify ||
        (event->xany.type == PropertyNotify &&
         event->xproperty.atom == context->selAtom)) {

        GdkWindow *cbWindow = gdk_window_lookup(event->xany.window);
        if (cbWindow) {
            GtkWidget *cbWidget = NULL;
            gdk_window_get_user_data(cbWindow, (gpointer *)&cbWidget);
            if (cbWidget && GTK_IS_WIDGET(cbWidget)) {
                context->cbWidget = cbWidget;
                return True;
            }
        }
    }

    return False;
}


static const int kClipboardTimeout = 500000;

static void
wait_for_retrieval(GtkClipboard *clipboard, retrieval_context *r_context)
{
    if (r_context->completed)  
        return;

    Display *xDisplay = GDK_DISPLAY();
    checkEventContext context;
    context.cbWidget = NULL;
    context.selAtom = gdk_x11_atom_to_xatom(gdk_atom_intern("GDK_SELECTION",
                                                            FALSE));

    
    
    

    int select_result;

#ifdef POLL_WITH_XCONNECTIONNUMBER
    struct pollfd fds[1];
    fds[0].fd = XConnectionNumber(xDisplay);
    fds[0].events = POLLIN;
#else
    int cnumber = ConnectionNumber(xDisplay);
    fd_set select_set;
    FD_ZERO(&select_set);
    FD_SET(cnumber, &select_set);
    ++cnumber;
    struct timeval tv;
#endif

    do {
        XEvent xevent;

        while (XCheckIfEvent(xDisplay, &xevent, checkEventProc,
                             (XPointer) &context)) {

            if (xevent.xany.type == SelectionNotify)
                DispatchSelectionNotifyEvent(context.cbWidget, &xevent);
            else
                DispatchPropertyNotifyEvent(context.cbWidget, &xevent);

            if (r_context->completed)
                return;
        }

#ifdef POLL_WITH_XCONNECTIONNUMBER
        select_result = poll(fds, 1, kClipboardTimeout / 1000);
#else
        tv.tv_sec = 0;
        tv.tv_usec = kClipboardTimeout;
        select_result = select(cnumber, &select_set, NULL, NULL, &tv);
#endif
    } while (select_result == 1);

#ifdef DEBUG_CLIPBOARD
    printf("exceeded clipboard timeout\n");
#endif
}

static void
clipboard_contents_received(GtkClipboard     *clipboard,
                            GtkSelectionData *selection_data,
                            gpointer          data)
{
    retrieval_context *context = static_cast<retrieval_context *>(data);
    context->completed = PR_TRUE;

    if (selection_data->length >= 0)
        context->data = gtk_selection_data_copy(selection_data);
}


static GtkSelectionData *
wait_for_contents(GtkClipboard *clipboard, GdkAtom target)
{
    retrieval_context context;
    gtk_clipboard_request_contents(clipboard, target,
                                   clipboard_contents_received,
                                   &context);

    wait_for_retrieval(clipboard, &context);
    return static_cast<GtkSelectionData *>(context.data);
}

static void
clipboard_text_received(GtkClipboard *clipboard,
                        const gchar  *text,
                        gpointer      data)
{
    retrieval_context *context = static_cast<retrieval_context *>(data);
    context->completed = PR_TRUE;
    context->data = g_strdup(text);
}

static gchar *
wait_for_text(GtkClipboard *clipboard)
{
    retrieval_context context;
    gtk_clipboard_request_text(clipboard, clipboard_text_received, &context);

    wait_for_retrieval(clipboard, &context);
    return static_cast<gchar *>(context.data);
}
