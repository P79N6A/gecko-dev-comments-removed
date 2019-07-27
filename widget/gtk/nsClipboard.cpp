






#include "mozilla/ArrayUtils.h"

#include "nsClipboard.h"
#include "nsSupportsPrimitives.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsXPIDLString.h"
#include "nsPrimitiveHelpers.h"
#include "nsIServiceManager.h"
#include "nsImageToPixbuf.h"
#include "nsStringStream.h"
#include "nsIObserverService.h"
#include "mozilla/Services.h"
#include "mozilla/RefPtr.h"
#include "mozilla/TimeStamp.h"

#include "imgIContainer.h"

#include <gtk/gtk.h>


#include <X11/Xlib.h>
#include <gdk/gdkx.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>

#include "mozilla/dom/EncodingUtils.h"
#include "nsIUnicodeDecoder.h"

using mozilla::dom::EncodingUtils;
using namespace mozilla;


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
                            int32_t             dataLength,
                            char16_t         **unicodeData,
                            int32_t            &outUnicodeLen);

static void
GetHTMLCharset             (guchar * data, int32_t dataLength, nsCString& str);







static GtkSelectionData *
wait_for_contents          (GtkClipboard *clipboard, GdkAtom target);

static gchar *
wait_for_text              (GtkClipboard *clipboard);

nsClipboard::nsClipboard()
{
}

nsClipboard::~nsClipboard()
{
    
    
    if (mGlobalTransferable) {
        gtk_clipboard_clear(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD));
    }
    if (mSelectionTransferable) {
        gtk_clipboard_clear(gtk_clipboard_get(GDK_SELECTION_PRIMARY));
    }
}

NS_IMPL_ISUPPORTS(nsClipboard, nsIClipboard)

nsresult
nsClipboard::Init(void)
{
    nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
    if (!os)
      return NS_ERROR_FAILURE;

    os->AddObserver(this, "quit-application", false);

    return NS_OK;
}

NS_IMETHODIMP
nsClipboard::Observe(nsISupports *aSubject, const char *aTopic, const char16_t *aData)
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
                     nsIClipboardOwner *aOwner, int32_t aWhichClipboard)
{
    
    if ((aWhichClipboard == kGlobalClipboard &&
         aTransferable == mGlobalTransferable.get() &&
         aOwner == mGlobalOwner.get()) ||
        (aWhichClipboard == kSelectionClipboard &&
         aTransferable == mSelectionTransferable.get() &&
         aOwner == mSelectionOwner.get())) {
        return NS_OK;
    }

    
    EmptyClipboard(aWhichClipboard);

    
    GtkTargetList *list = gtk_target_list_new(nullptr, 0);

    
    nsCOMPtr<nsISupportsArray> flavors;

    nsresult rv =
        aTransferable->FlavorsTransferableCanExport(getter_AddRefs(flavors));
    if (!flavors || NS_FAILED(rv))
        return NS_ERROR_FAILURE;

    
    bool imagesAdded = false;
    uint32_t count;
    flavors->Count(&count);
    for (uint32_t i=0; i < count; i++) {
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
                flavorStr.EqualsLiteral(kJPGImageMime) ||
                flavorStr.EqualsLiteral(kGIFImageMime)) {
                
                if (!imagesAdded) {
                    
                    gtk_target_list_add_image_targets(list, 0, TRUE);
                    imagesAdded = true;
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
nsClipboard::GetData(nsITransferable *aTransferable, int32_t aWhichClipboard)
{
    if (!aTransferable)
        return NS_ERROR_FAILURE;

    GtkClipboard *clipboard;
    clipboard = gtk_clipboard_get(GetSelectionAtom(aWhichClipboard));

    guchar        *data = nullptr;
    gint           length = 0;
    bool           foundData = false;
    nsAutoCString  foundFlavor;

    
    nsCOMPtr<nsISupportsArray> flavors;
    nsresult rv;
    rv = aTransferable->FlavorsTransferableCanImport(getter_AddRefs(flavors));
    if (!flavors || NS_FAILED(rv))
        return NS_ERROR_FAILURE;

    uint32_t count;
    flavors->Count(&count);
    for (uint32_t i=0; i < count; i++) {
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
                    foundData = true;
                    foundFlavor = kUnicodeMime;
                    break;
                }
                
                
                
                continue;
            }

            
            
            if (!strcmp(flavorStr, kJPEGImageMime) ||
                !strcmp(flavorStr, kJPGImageMime) ||
                !strcmp(flavorStr, kPNGImageMime) ||
                !strcmp(flavorStr, kGIFImageMime)) {
                
                if (!strcmp(flavorStr, kJPGImageMime)) {
                    flavorStr.Assign(kJPEGImageMime);
                }

                GdkAtom atom = gdk_atom_intern(flavorStr, FALSE);

                GtkSelectionData *selectionData = wait_for_contents(clipboard, atom);
                if (!selectionData)
                    continue;

                nsCOMPtr<nsIInputStream> byteStream;
                NS_NewByteInputStream(getter_AddRefs(byteStream), 
                                      (const char*)gtk_selection_data_get_data(selectionData),
                                      gtk_selection_data_get_length(selectionData), 
                                      NS_ASSIGNMENT_COPY);
                aTransferable->SetTransferData(flavorStr, byteStream, sizeof(nsIInputStream*));
                gtk_selection_data_free(selectionData);
                return NS_OK;
            }

            
            
            GdkAtom atom = gdk_atom_intern(flavorStr, FALSE);
            GtkSelectionData *selectionData;
            selectionData = wait_for_contents(clipboard, atom);
            if (selectionData) {
                const guchar *clipboardData = gtk_selection_data_get_data(selectionData);
                length = gtk_selection_data_get_length(selectionData);
                
                if (!strcmp(flavorStr, kHTMLMime)) {
                    char16_t* htmlBody= nullptr;
                    int32_t htmlBodyLen = 0;
                    
                    ConvertHTMLtoUCS2(const_cast<guchar*>(clipboardData), length,
                                      &htmlBody, htmlBodyLen);
                    
                    if (!htmlBodyLen)
                        continue;
                    data = (guchar *)htmlBody;
                    length = htmlBodyLen * 2;
                } else {
                    data = (guchar *)moz_xmalloc(length);
                    if (!data)
                        break;
                    memcpy(data, clipboardData, length);
                }
                gtk_selection_data_free(selectionData);
                foundData = true;
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
        free(data);

    return NS_OK;
}

NS_IMETHODIMP
nsClipboard::EmptyClipboard(int32_t aWhichClipboard)
{
    if (aWhichClipboard == kSelectionClipboard) {
        if (mSelectionOwner) {
            mSelectionOwner->LosingOwnership(mSelectionTransferable);
            mSelectionOwner = nullptr;
        }
        mSelectionTransferable = nullptr;
    }
    else {
        if (mGlobalOwner) {
            mGlobalOwner->LosingOwnership(mGlobalTransferable);
            mGlobalOwner = nullptr;
        }
        mGlobalTransferable = nullptr;
    }

    return NS_OK;
}

NS_IMETHODIMP
nsClipboard::HasDataMatchingFlavors(const char** aFlavorList, uint32_t aLength,
                                    int32_t aWhichClipboard, bool *_retval)
{
    if (!aFlavorList || !_retval)
        return NS_ERROR_NULL_POINTER;

    *_retval = false;

    GtkSelectionData *selection_data =
        GetTargets(GetSelectionAtom(aWhichClipboard));
    if (!selection_data)
        return NS_OK;

    gint n_targets = 0;
    GdkAtom *targets = nullptr;

    if (!gtk_selection_data_get_targets(selection_data, 
                                        &targets, &n_targets) ||
        !n_targets)
        return NS_OK;

    
    
    for (uint32_t i = 0; i < aLength && !*_retval; i++) {
        
        if (!strcmp(aFlavorList[i], kUnicodeMime) && 
            gtk_selection_data_targets_include_text(selection_data)) {
            *_retval = true;
            break;
        }

        for (int32_t j = 0; j < n_targets; j++) {
            gchar *atom_name = gdk_atom_name(targets[j]);
            if (!atom_name)
                continue;

            if (!strcmp(atom_name, aFlavorList[i]))
                *_retval = true;

            
            
            if (!strcmp(aFlavorList[i], kJPGImageMime) && !strcmp(atom_name, kJPEGImageMime))
                *_retval = true;

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
nsClipboard::SupportsSelectionClipboard(bool *_retval)
{
    *_retval = true; 
    return NS_OK;
}

NS_IMETHODIMP
nsClipboard::SupportsFindClipboard(bool* _retval)
{
  *_retval = false;
  return NS_OK;
}


GdkAtom
nsClipboard::GetSelectionAtom(int32_t aWhichClipboard)
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
nsClipboard::GetTransferable(int32_t aWhichClipboard)
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
    
    
    
    

    int32_t whichClipboard;

    
    GdkAtom selection = gtk_selection_data_get_selection(aSelectionData);
    if (selection == GDK_SELECTION_PRIMARY)
        whichClipboard = kSelectionClipboard;
    else if (selection == GDK_SELECTION_CLIPBOARD)
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
    uint32_t len;

  
    GdkAtom selectionTarget = gtk_selection_data_get_target(aSelectionData);
  
    
    
    if (selectionTarget == gdk_atom_intern ("STRING", FALSE) ||
        selectionTarget == gdk_atom_intern ("TEXT", FALSE) ||
        selectionTarget == gdk_atom_intern ("COMPOUND_TEXT", FALSE) ||
        selectionTarget == gdk_atom_intern ("UTF8_STRING", FALSE)) {
        
        
        
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

        free(utf8string);
        return;
    }

    
    if (gtk_targets_include_image(&selectionTarget, 1, TRUE)) {
        
        static const char* const imageMimeTypes[] = {
            kNativeImageMime, kPNGImageMime, kJPEGImageMime, kJPGImageMime, kGIFImageMime };
        nsCOMPtr<nsISupports> item;
        uint32_t len;
        nsCOMPtr<nsISupportsInterfacePointer> ptrPrimitive;
        for (uint32_t i = 0; !ptrPrimitive && i < ArrayLength(imageMimeTypes); i++) {
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

    
    
    gchar *target_name = gdk_atom_name(selectionTarget);
    if (!target_name)
        return;

    rv = trans->GetTransferData(target_name, getter_AddRefs(item), &len);
    
    if (!item || NS_FAILED(rv)) {
        g_free(target_name);
        return;
    }

    void *primitive_data = nullptr;
    nsPrimitiveHelpers::CreateDataFromPrimitive(target_name, item,
                                                &primitive_data, len);

    if (primitive_data) {
        
        if (selectionTarget == gdk_atom_intern (kHTMLMime, FALSE)) {
            






            guchar *buffer = (guchar *)
                    moz_xmalloc((len * sizeof(guchar)) + sizeof(char16_t));
            if (!buffer)
                return;
            char16_t prefix = 0xFEFF;
            memcpy(buffer, &prefix, sizeof(prefix));
            memcpy(buffer + sizeof(prefix), primitive_data, len);
            free((guchar *)primitive_data);
            primitive_data = (guchar *)buffer;
            len += sizeof(prefix);
        }
  
        gtk_selection_data_set(aSelectionData, selectionTarget,
                               8, 
                               (const guchar *)primitive_data, len);
        free(primitive_data);
    }

    g_free(target_name);
                           
}

void
nsClipboard::SelectionClearEvent(GtkClipboard *aGtkClipboard)
{
    int32_t whichClipboard;

    
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




















void ConvertHTMLtoUCS2(guchar * data, int32_t dataLength,
                       char16_t** unicodeData, int32_t& outUnicodeLen)
{
    nsAutoCString charset;
    GetHTMLCharset(data, dataLength, charset);
    if (charset.EqualsLiteral("UTF-16")) {
        outUnicodeLen = (dataLength / 2) - 1;
        *unicodeData = reinterpret_cast<char16_t*>
                                       (moz_xmalloc((outUnicodeLen + sizeof('\0')) *
                       sizeof(char16_t)));
        if (*unicodeData) {
            memcpy(*unicodeData, data + sizeof(char16_t),
                   outUnicodeLen * sizeof(char16_t));
            (*unicodeData)[outUnicodeLen] = '\0';
        }
    } else if (charset.EqualsLiteral("UNKNOWN")) {
        outUnicodeLen = 0;
        return;
    } else {
        
        nsCOMPtr<nsIUnicodeDecoder> decoder;
        
        nsAutoCString encoding;
        if (!EncodingUtils::FindEncodingForLabelNoReplacement(charset,
                                                              encoding)) {
#ifdef DEBUG_CLIPBOARD
            g_print("        get unicode decoder error\n");
#endif
            outUnicodeLen = 0;
            return;
        }
        decoder = EncodingUtils::DecoderForEncoding(encoding);
        
        decoder->GetMaxLength((const char *)data, dataLength, &outUnicodeLen);
        
        if (outUnicodeLen) {
            *unicodeData = reinterpret_cast<char16_t*>
                                           (moz_xmalloc((outUnicodeLen + sizeof('\0')) *
                           sizeof(char16_t)));
            if (*unicodeData) {
                int32_t numberTmp = dataLength;
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








void GetHTMLCharset(guchar * data, int32_t dataLength, nsCString& str)
{
    
    char16_t* beginChar =  (char16_t*)data;
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
    event.selection.window = gtk_widget_get_window(widget);
    event.selection.selection = gdk_x11_xatom_to_atom(xevent->xselection.selection);
    event.selection.target = gdk_x11_xatom_to_atom(xevent->xselection.target);
    event.selection.property = gdk_x11_xatom_to_atom(xevent->xselection.property);
    event.selection.time = xevent->xselection.time;

    gtk_widget_event(widget, &event);
}

static void
DispatchPropertyNotifyEvent(GtkWidget *widget, XEvent *xevent)
{
    GdkWindow *window = gtk_widget_get_window(widget);
    if ((gdk_window_get_events(window)) & GDK_PROPERTY_CHANGE_MASK) {
        GdkEvent event;
        event.property.type = GDK_PROPERTY_NOTIFY;
        event.property.window = window;
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

        GdkWindow *cbWindow = 
            gdk_x11_window_lookup_for_display(gdk_x11_lookup_xdisplay(display),
                                              event->xany.window);
        if (cbWindow) {
            GtkWidget *cbWidget = nullptr;
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

static gchar* CopyRetrievedData(const gchar *aData)
{
    return g_strdup(aData);
}

static GtkSelectionData* CopyRetrievedData(GtkSelectionData *aData)
{
    
    return gtk_selection_data_get_length(aData) >= 0 ?
        gtk_selection_data_copy(aData) : nullptr;
}

class RetrievalContext {
    ~RetrievalContext()
    {
        MOZ_ASSERT(!mData, "Wait() wasn't called");
    }

public:
    NS_INLINE_DECL_REFCOUNTING(RetrievalContext)
    enum State { INITIAL, COMPLETED, TIMED_OUT };

    RetrievalContext() : mState(INITIAL), mData(nullptr) {}

    


    template <class T> void Complete(T *aData)
    {
        if (mState == INITIAL) {
            mState = COMPLETED;
            mData = CopyRetrievedData(aData);
        } else {
            
            MOZ_ASSERT(mState == TIMED_OUT);
        }
    }

    




    void *Wait();

protected:
    State mState;
    void* mData;
};

void *
RetrievalContext::Wait()
{
    if (mState == COMPLETED) { 
        void *data = mData;
        mData = nullptr;
        return data;
    }

    GdkDisplay *gdkDisplay = gdk_display_get_default();
    if (GDK_IS_X11_DISPLAY(gdkDisplay)) {
        Display *xDisplay = GDK_DISPLAY_XDISPLAY(gdkDisplay);
        checkEventContext context;
        context.cbWidget = nullptr;
        context.selAtom = gdk_x11_atom_to_xatom(gdk_atom_intern("GDK_SELECTION",
                                                                FALSE));

        
        
        

        int select_result;

        int cnumber = ConnectionNumber(xDisplay);
        fd_set select_set;
        FD_ZERO(&select_set);
        FD_SET(cnumber, &select_set);
        ++cnumber;
        TimeStamp start = TimeStamp::Now();

        do {
            XEvent xevent;

            while (XCheckIfEvent(xDisplay, &xevent, checkEventProc,
                                 (XPointer) &context)) {

                if (xevent.xany.type == SelectionNotify)
                    DispatchSelectionNotifyEvent(context.cbWidget, &xevent);
                else
                    DispatchPropertyNotifyEvent(context.cbWidget, &xevent);

                if (mState == COMPLETED) {
                    void *data = mData;
                    mData = nullptr;
                    return data;
                }
            }

            TimeStamp now = TimeStamp::Now();
            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = std::max<int32_t>(0,
                kClipboardTimeout - (now - start).ToMicroseconds());
            select_result = select(cnumber, &select_set, nullptr, nullptr, &tv);
        } while (select_result == 1 ||
                 (select_result == -1 && errno == EINTR));
    }
#ifdef DEBUG_CLIPBOARD
    printf("exceeded clipboard timeout\n");
#endif
    mState = TIMED_OUT;
    return nullptr;
}

static void
clipboard_contents_received(GtkClipboard     *clipboard,
                            GtkSelectionData *selection_data,
                            gpointer          data)
{
    RetrievalContext *context = static_cast<RetrievalContext*>(data);
    context->Complete(selection_data);
    context->Release();
}

static GtkSelectionData *
wait_for_contents(GtkClipboard *clipboard, GdkAtom target)
{
    RefPtr<RetrievalContext> context = new RetrievalContext();
    
    context.get()->AddRef();
    gtk_clipboard_request_contents(clipboard, target,
                                   clipboard_contents_received,
                                   context.get());
    return static_cast<GtkSelectionData*>(context->Wait());
}

static void
clipboard_text_received(GtkClipboard *clipboard,
                        const gchar  *text,
                        gpointer      data)
{
    RetrievalContext *context = static_cast<RetrievalContext*>(data);
    context->Complete(text);
    context->Release();
}

static gchar *
wait_for_text(GtkClipboard *clipboard)
{
    RefPtr<RetrievalContext> context = new RetrievalContext();
    
    context.get()->AddRef();
    gtk_clipboard_request_text(clipboard, clipboard_text_received, context.get());
    return static_cast<gchar*>(context->Wait());
}
