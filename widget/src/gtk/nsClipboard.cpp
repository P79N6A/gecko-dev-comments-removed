








































#include "nsClipboard.h"

#include <gdk/gdkx.h>
#include <X11/Xlib.h>

#include "nsCOMPtr.h"
#include "nsCRT.h"
#include "nsISupportsArray.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsReadableUtils.h"

#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsWidgetsCID.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsPrimitiveHelpers.h"

#include "nsTextFormatter.h"

#include "nsIServiceManager.h"

#include "prtime.h"
#include "prthread.h"


#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#ifdef POLL_WITH_XCONNECTIONNUMBER
#include <poll.h>
#endif


#include "nsIPlatformCharset.h"
#include "nsICharsetConverterManager.h"

static void ConvertHTMLtoUCS2(char* data, PRInt32 dataLength,
                              PRUnichar** unicodeData, PRInt32& outUnicodeLen);
static void GetHTMLCharset   (char* data, PRInt32 dataLength, nsACString& str);


GtkWidget* nsClipboard::sWidget = 0;




#undef USE_CUTBUFFERS


static GdkAtom GDK_SELECTION_CLIPBOARD;

NS_IMPL_ISUPPORTS1(nsClipboard, nsIClipboard)






nsClipboard::nsClipboard()
{
#ifdef DEBUG_CLIPBOARD
  g_print("nsClipboard::nsClipboard()\n");
#endif 

  mIgnoreEmptyNotification = PR_FALSE;
  mGlobalTransferable = nsnull;
  mSelectionTransferable = nsnull;
  mGlobalOwner = nsnull;
  mSelectionOwner = nsnull;
  mSelectionData.data = nsnull;
  mSelectionData.length = 0;

  
  Init();
}





typedef struct _GtkSelectionTargetList GtkSelectionTargetList;

struct _GtkSelectionTargetList {
  GdkAtom selection;
  GtkTargetList *list;
};

static const char *gtk_selection_handler_key = "gtk-selection-handlers";

void __gtk_selection_target_list_remove (GtkWidget *widget, GdkAtom selection)
{
  GtkSelectionTargetList *sellist;
  GList *tmp_list, *tmp_list2;
  GList *lists;
  lists = (GList*)gtk_object_get_data (GTK_OBJECT (widget), gtk_selection_handler_key);
  tmp_list = lists;
  while (tmp_list) {
    sellist = (GtkSelectionTargetList*)tmp_list->data;
    if (sellist->selection == selection) {
      gtk_target_list_unref (sellist->list);
      g_free (sellist);
      tmp_list->data = nsnull;
      tmp_list2 = tmp_list->prev;
      lists = g_list_remove_link(lists, tmp_list);
      g_list_free_1(tmp_list);
      tmp_list = tmp_list2;
    }
    if (tmp_list)
      tmp_list = tmp_list->next;
  }
  gtk_object_set_data(GTK_OBJECT(widget), gtk_selection_handler_key, lists);
}






nsClipboard::~nsClipboard()
{
#ifdef DEBUG_CLIPBOARD
  g_print("nsClipboard::~nsClipboard()\n");  
#endif 

  
  if (sWidget) {
    if (gdk_selection_owner_get(GDK_SELECTION_PRIMARY) == sWidget->window)
      gtk_selection_remove_all(sWidget);

    if (gdk_selection_owner_get(GDK_SELECTION_CLIPBOARD) == sWidget->window)
      gtk_selection_remove_all(sWidget);
  }

  
  if (mSelectionData.data != nsnull)
    nsMemory::Free(mSelectionData.data);

  gtk_object_remove_data(GTK_OBJECT(sWidget), "cb");

  if (sWidget) {
    gtk_widget_unref(sWidget);
    sWidget = nsnull;
  }
}



void nsClipboard::Init(void)
{
#ifdef DEBUG_CLIPBOARD
  g_print("nsClipboard::Init\n");
#endif

  GDK_SELECTION_CLIPBOARD = gdk_atom_intern("CLIPBOARD", FALSE);

  
  sWidget = gtk_invisible_new();

  
  gtk_object_set_data(GTK_OBJECT(sWidget), "cb", this);

  
  gtk_signal_connect(GTK_OBJECT(sWidget), "selection_get",
                     GTK_SIGNAL_FUNC(nsClipboard::SelectionGetCB),
                     nsnull);

  
  gtk_signal_connect(GTK_OBJECT(sWidget), "selection_clear_event",
                     GTK_SIGNAL_FUNC(nsClipboard::SelectionClearCB),
                     nsnull);

  
  gtk_signal_connect(GTK_OBJECT(sWidget), "selection_received",
                     GTK_SIGNAL_FUNC(nsClipboard::SelectionReceivedCB),
                     nsnull);
}








NS_IMETHODIMP nsClipboard::SetData(nsITransferable * aTransferable,
                                   nsIClipboardOwner * anOwner,
                                   PRInt32 aWhichClipboard)
{

  if ((aTransferable == mGlobalTransferable.get() &&
       anOwner == mGlobalOwner.get() &&
       aWhichClipboard == kGlobalClipboard ) ||
      (aTransferable == mSelectionTransferable.get() &&
       anOwner == mSelectionOwner.get() &&
       aWhichClipboard == kSelectionClipboard)
      )
  {
    return NS_OK;
  }

  EmptyClipboard(aWhichClipboard);

  switch(aWhichClipboard) {
  case kSelectionClipboard:
    mSelectionOwner = anOwner;
    mSelectionTransferable = aTransferable;
    break;
  case kGlobalClipboard:
    mGlobalOwner = anOwner;
    mGlobalTransferable = aTransferable;
    SetCutBuffer();
    break;
  }

  return SetNativeClipboardData(aWhichClipboard);
}





NS_IMETHODIMP nsClipboard::GetData(nsITransferable * aTransferable, PRInt32 aWhichClipboard)
{
  if (nsnull != aTransferable) {
    return GetNativeClipboardData(aTransferable, aWhichClipboard);
  } else {
#ifdef DEBUG_CLIPBOARD
    printf("  nsClipboard::GetData(), aTransferable is NULL.\n");
#endif
  }

  return NS_ERROR_FAILURE;
}






NS_IMETHODIMP nsClipboard::EmptyClipboard(PRInt32 aWhichClipboard)
{
  if (mIgnoreEmptyNotification) {
    return NS_OK;
  }

  switch(aWhichClipboard) {
  case kSelectionClipboard:
    if (mSelectionOwner) {
      mSelectionOwner->LosingOwnership(mSelectionTransferable);
      mSelectionOwner = nsnull;
    }
    mSelectionTransferable = nsnull;
    break;
  case kGlobalClipboard:
    if (mGlobalOwner) {
      mGlobalOwner->LosingOwnership(mGlobalTransferable);
      mGlobalOwner = nsnull;
    }
    mGlobalTransferable = nsnull;
    break;
  }

  return NS_OK;
}

NS_IMETHODIMP nsClipboard::SupportsSelectionClipboard(PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  *_retval = PR_TRUE; 
  return NS_OK;
}


NS_IMETHODIMP nsClipboard::SetNativeClipboardData(PRInt32 aWhichClipboard)
{
  mIgnoreEmptyNotification = PR_TRUE;

#ifdef DEBUG_CLIPBOARD
  g_print("  nsClipboard::SetNativeClipboardData(%i)\n", aWhichClipboard);
#endif 


  GdkAtom selectionAtom = GetSelectionAtom(aWhichClipboard);
  nsCOMPtr<nsITransferable> transferable(GetTransferable(aWhichClipboard));

  
  if (nsnull == transferable) {
#ifdef DEBUG_CLIPBOARD
    printf("nsClipboard::SetNativeClipboardData(): no transferable!\n");
#endif
    return NS_ERROR_FAILURE;
  }

  
  if (gdk_selection_owner_get(selectionAtom) == sWidget->window)
  {
    
    __gtk_selection_target_list_remove(sWidget, selectionAtom);
    
  }

  
  gint have_selection = gtk_selection_owner_set(sWidget,
                                                selectionAtom,
                                                GDK_CURRENT_TIME);
  if (have_selection == 0)
    return NS_ERROR_FAILURE;

  
  
  nsCOMPtr<nsISupportsArray> flavorList;
  nsresult errCode = transferable->FlavorsTransferableCanExport ( getter_AddRefs(flavorList) );
  if ( NS_FAILED(errCode) )
    return NS_ERROR_FAILURE;

  PRUint32 cnt;
  flavorList->Count(&cnt);
  for ( PRUint32 i=0; i<cnt; ++i )
  {
    nsCOMPtr<nsISupports> genericFlavor;
    flavorList->GetElementAt ( i, getter_AddRefs(genericFlavor) );
    nsCOMPtr<nsISupportsCString> currentFlavor ( do_QueryInterface(genericFlavor) );
    if ( currentFlavor ) {
      nsXPIDLCString flavorStr;
      currentFlavor->ToString(getter_Copies(flavorStr));

      
      RegisterFormat(flavorStr, selectionAtom);
    }
  }

  mIgnoreEmptyNotification = PR_FALSE;

  return NS_OK;
}


PRBool nsClipboard::DoRealConvert(GdkAtom type, GdkAtom aSelectionAtom)
{
#ifdef DEBUG_CLIPBOARD
  g_print("    nsClipboard::DoRealConvert(%li)\n    {\n", type);
#endif

  
  mBlocking = PR_TRUE;

  
  
  
#ifdef DEBUG_CLIPBOARD
  g_print("     Doing real conversion of atom type '%s'\n", gdk_atom_name(type));
#endif
  gtk_selection_convert(sWidget,
                        aSelectionAtom,
                        type,
                        GDK_CURRENT_TIME);


  




  if (mBlocking) {
    
    
#ifdef DEBUG_CLIPBOARD
    g_print("      Waiting for the callback... mBlocking = %d\n", mBlocking);
#endif 

    if (!FindSelectionNotifyEvent())
      return PR_FALSE;

#ifdef DEBUG_CLIPBOARD
    g_print("    }\n");
#endif
  }

  if (mSelectionData.length > 0)
    return PR_TRUE;

  return PR_FALSE;
}







NS_IMETHODIMP
nsClipboard::GetNativeClipboardData(nsITransferable * aTransferable, 
                                    PRInt32 aWhichClipboard)
{
  GdkAtom selectionAtom = GetSelectionAtom(aWhichClipboard);

#ifdef DEBUG_CLIPBOARD
  g_print("nsClipboard::GetNativeClipboardData(%i)\n", aWhichClipboard);
#endif 

  
  if (nsnull == aTransferable) {
#ifdef DEBUG_CLIPBOARD
    printf("  GetNativeClipboardData: Transferable is null!\n");
#endif
    return NS_ERROR_FAILURE;
  }

  
  
  nsCOMPtr<nsISupportsArray> flavorList;
  nsresult errCode = aTransferable->FlavorsTransferableCanImport ( getter_AddRefs(flavorList) );
  if ( NS_FAILED(errCode) )
    return NS_ERROR_FAILURE;

  
  PRUint32 cnt;
  flavorList->Count(&cnt);
  nsCAutoString foundFlavor;
  PRBool foundData = PR_FALSE;
  for ( PRUint32 i = 0; i < cnt; ++i ) {
    nsCOMPtr<nsISupports> genericFlavor;
    flavorList->GetElementAt ( i, getter_AddRefs(genericFlavor) );
    nsCOMPtr<nsISupportsCString> currentFlavor ( do_QueryInterface(genericFlavor) );
    if ( currentFlavor ) {
      nsXPIDLCString flavorStr;
      currentFlavor->ToString ( getter_Copies(flavorStr) );
      if (DoConvert(flavorStr, selectionAtom)) {
        foundFlavor = flavorStr;
        foundData = PR_TRUE;
        break;
      }
    }
  }
  
#ifdef DEBUG_CLIPBOARD
  g_print("  Got the callback: '%s', %d\n",
         mSelectionData.data, mSelectionData.length);
#endif 

  
  mBlocking = PR_FALSE;

  
  
  
  

  if ( foundData ) {
    nsCOMPtr<nsISupports> genericDataWrapper;
    nsPrimitiveHelpers::CreatePrimitiveForData(foundFlavor.get(), mSelectionData.data,
                                               mSelectionData.length, getter_AddRefs(genericDataWrapper));
    aTransferable->SetTransferData(foundFlavor.get(),
                                   genericDataWrapper,
                                   mSelectionData.length);
  }
    
  
  nsMemory::Free(mSelectionData.data);
  mSelectionData.data = nsnull;
  mSelectionData.length = 0;

  return NS_OK;
}








void
nsClipboard::SelectionReceivedCB (GtkWidget        *aWidget,
                                  GtkSelectionData *aSelectionData,
                                  guint             aTime)
{
#ifdef DEBUG_CLIPBOARD
  g_print("      nsClipboard::SelectionReceivedCB\n      {\n");
#endif 
  nsClipboard *cb =(nsClipboard *)gtk_object_get_data(GTK_OBJECT(aWidget),
                                                      "cb");
  if (!cb)
  {
#ifdef DEBUG_CLIPBOARD
    g_print("no clipboard found.. this is bad.\n");
#endif
    return;
  }
  cb->SelectionReceiver(aWidget, aSelectionData);
#ifdef DEBUG_CLIPBOARD
  g_print("      }\n");
#endif
}








void
nsClipboard::SelectionReceiver (GtkWidget *aWidget,
                                GtkSelectionData *aSD)
{
  mBlocking = PR_FALSE;

  if (aSD->length <= 0)
  {
#ifdef DEBUG_CLIPBOARD
    g_print("        Error retrieving selection: length was %d\n", aSD->length);
#endif
    mSelectionData.length = aSD->length;
    return;
  }

  char *str = gdk_atom_name(aSD->type);
  nsCAutoString type(str);
  g_free(str);

#ifdef DEBUG_CLIPBOARD
  g_print("        Type is %s\n", type.mBuffer);

  if (type.Equals("ATOM")) {
    g_print("        Asked for TARGETS\n");
  }
#endif

  if (type.Equals("COMPOUND_TEXT")) {
#ifdef DEBUG_CLIPBOARD
    g_print("        Copying mSelectionData pointer -- \n");
#endif
    mSelectionData = *aSD;

    char *data = (char*)aSD->data;
    PRInt32 len = (PRInt32)aSD->length;

    int status = 0;
    XTextProperty prop;

#ifdef DEBUG_CLIPBOARD
    g_print("        Converted text from COMPOUND_TEXT to platform locale\n");
    g_print("        data is %s\n", data);
    g_print("        len is %d\n", len);
#endif

    prop.value = (unsigned char *)data;
    prop.nitems = len;
    prop.encoding = XInternAtom(GDK_DISPLAY(), "COMPOUND_TEXT", FALSE);
    prop.format = 8;

    char **tmpData;
    int foo;
    status = XmbTextPropertyToTextList(GDK_DISPLAY(), &prop, &tmpData, &foo);

#ifdef DEBUG_CLIPBOARD
    if (foo > 1)
      printf("Got multiple strings from XmbTextPropertyToTextList.. don't know how to handle this yet\n");
#endif

    PRInt32 numberOfBytes = 0;

    if (status == XNoMemory || status == XLocaleNotSupported ||
        status == XConverterNotFound) {
#ifdef DEBUG_CLIPBOARD
      g_print("\n         XmbTextListToTextProperty failed.  returned %d\n", status);
      g_print("          text is \"%s\"\n", tmpData[0]);
#endif
      numberOfBytes = strlen(NS_REINTERPRET_CAST(const char *, data));
    } else {
      if (foo > 0 && tmpData[0] != 0 && (*tmpData[0]) != 0) {
        data = tmpData[0];
      }
      numberOfBytes = strlen(NS_REINTERPRET_CAST(const char *, data));
#ifdef DEBUG_CLIPBOARD
      g_print("\n        XmbTextListToTextProperty succeeded\n");
      g_print("          text is \"%s\"\n", data);
      g_print("          numberOfBytes is %d\n", numberOfBytes);
#endif
    }

    nsresult rv;
    PRInt32 outUnicodeLen;
    PRUnichar *unicodeData = nsnull;

#ifdef DEBUG_CLIPBOARD
    g_print("        Converting from current locale to unicode\n");
#endif

    nsCOMPtr<nsIUnicodeDecoder> decoder;
    
    nsCAutoString platformCharset;
    nsCOMPtr <nsIPlatformCharset> platformCharsetService = do_GetService(NS_PLATFORMCHARSET_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv))
      rv = platformCharsetService->GetCharset(kPlatformCharsetSel_Menu, platformCharset);
    if (NS_FAILED(rv))
      platformCharset.AssignLiteral("ISO-8859-1");
      
    
    nsCOMPtr<nsICharsetConverterManager> ccm = do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);
    rv = ccm->GetUnicodeDecoderRaw(platformCharset.get(),
                                   getter_AddRefs(decoder));
    NS_ASSERTION(NS_SUCCEEDED(rv), "GetUnicodeEncoderRaw failed");
    if (NS_FAILED(rv)) {
      if (tmpData) XFreeStringList(tmpData);
      return;
    }
      
    
    
    decoder->GetMaxLength(data, numberOfBytes, &outUnicodeLen);   
    if (outUnicodeLen) {
      unicodeData = NS_REINTERPRET_CAST(PRUnichar*, nsMemory::Alloc((outUnicodeLen + 1) * sizeof(PRUnichar)));
      if ( unicodeData ) {
        PRInt32 numberTmp = numberOfBytes;
        rv = decoder->Convert(data, &numberTmp, unicodeData, &outUnicodeLen);
#ifdef DEBUG_CLIPBOARD
        if (numberTmp != numberOfBytes)
          printf("didn't consume all the bytes\n");
#endif

        (unicodeData)[outUnicodeLen] = '\0';    
      }
    } 


    mSelectionData.data = NS_REINTERPRET_CAST(guchar*,unicodeData);
    mSelectionData.length = outUnicodeLen * 2;
    if (tmpData) XFreeStringList(tmpData);
  }
  else if (type.Equals("UTF8_STRING")) {
    mSelectionData = *aSD;

    nsresult rv;
    PRInt32 outUnicodeLen;
    PRUnichar *unicodeData = nsnull;

    char *data = (char*)aSD->data;
    PRInt32 numberOfBytes = (PRInt32)aSD->length;

#ifdef DEBUG_CLIPBOARD
    printf("UTF8_STRING is %s\nlength is %i\n", aSD->data, aSD->length);
#endif

    
    nsCOMPtr<nsIUnicodeDecoder> decoder;
    nsCOMPtr<nsICharsetConverterManager> ccm = do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);
    rv = ccm->GetUnicodeDecoderRaw("UTF-8", getter_AddRefs(decoder));

    g_return_if_fail(NS_SUCCEEDED(rv));

    decoder->GetMaxLength(data, numberOfBytes, &outUnicodeLen);   
    if (outUnicodeLen) {
      unicodeData = NS_REINTERPRET_CAST(PRUnichar*, nsMemory::Alloc((outUnicodeLen + 1) * sizeof(PRUnichar)));
      if ( unicodeData ) {
        PRInt32 numberTmp = numberOfBytes;
        rv = decoder->Convert(data, &numberTmp, unicodeData, &outUnicodeLen);
#ifdef DEBUG_CLIPBOARD
        if (numberTmp != numberOfBytes)
          printf("didn't consume all the bytes\n");
#endif

        (unicodeData)[outUnicodeLen] = '\0';    
      }
    } 


    mSelectionData.data = NS_REINTERPRET_CAST(guchar*,unicodeData);
    mSelectionData.length = outUnicodeLen * 2;
    mSelectionData.type = gdk_atom_intern(kUnicodeMime, FALSE);

  } else if (type.Equals("STRING")) {
#ifdef DEBUG_CLIPBOARD
    g_print("        Copying mSelectionData pointer -- \n");
    g_print("         Data = %s\n         Length = %i\n", aSD->data, aSD->length);
#endif
    mSelectionData = *aSD;

    
    const char* castedText = NS_REINTERPRET_CAST(char*, mSelectionData.data);          
    PRUnichar* convertedText = nsnull;
    PRInt32 convertedTextLen = 0;
    nsPrimitiveHelpers::ConvertPlatformPlainTextToUnicode (castedText, mSelectionData.length, 
                                                           &convertedText, &convertedTextLen);
    if (convertedText) {
      
      mSelectionData.data = NS_REINTERPRET_CAST(guchar*, convertedText);
      mSelectionData.length = convertedTextLen * 2;
    }
  } else if (type.Equals(kHTMLMime)) {
    mSelectionData = *aSD;
    PRUnichar* htmlBody= nsnull;
    PRInt32 htmlBodyLen = 0;
    ConvertHTMLtoUCS2((char*)aSD->data, aSD->length, &htmlBody, htmlBodyLen);
    if (htmlBodyLen) {
      mSelectionData.data = NS_REINTERPRET_CAST(guchar*, htmlBody);
      mSelectionData.length = htmlBodyLen * 2;
    }
  } else {
    mSelectionData = *aSD;
    mSelectionData.data = g_new(guchar, aSD->length + 1);
    memcpy(mSelectionData.data,
           aSD->data,
           aSD->length);
    mSelectionData.length = aSD->length;
  }
}

NS_IMETHODIMP
nsClipboard::HasDataMatchingFlavors(nsISupportsArray* aFlavorList, 
                                    PRInt32 aWhichClipboard, 
                                    PRBool * outResult)
{
  
  
  
  
  
  
  
  
#ifdef DEBUG_CLIPBOARD
  g_print("  nsClipboard::HasDataMatchingFlavors()\n  {\n");
#endif


  GetTargets(GetSelectionAtom(aWhichClipboard));

  guchar *data = mSelectionData.data;
  PRInt32 dataLength = mSelectionData.length;
  int position = 0;
  gchar *str;


  *outResult = PR_FALSE;
  PRUint32 length;
  aFlavorList->Count(&length);
  for ( PRUint32 i = 0; i < length; ++i ) {
    nsCOMPtr<nsISupports> genericFlavor;
    aFlavorList->GetElementAt ( i, getter_AddRefs(genericFlavor) );
    nsCOMPtr<nsISupportsCString> flavorWrapper ( do_QueryInterface(genericFlavor) );
    if ( flavorWrapper ) {
      nsCAutoString flavorStr;
      nsXPIDLCString myStr;
      flavorWrapper->ToString(getter_Copies(myStr));
      flavorStr = nsCAutoString(myStr);

      position = 0;
      while (position < dataLength) {
        str = gdk_atom_name(*(GdkAtom*)(data+position));
        position += sizeof(GdkAtom);
        nsCAutoString atomName(str);
        g_free(str);
        
        if (flavorStr.Equals(kUnicodeMime)) {
          if (atomName.Equals("COMPOUND_TEXT") ||
              atomName.Equals("UTF8_STRING") ||
              atomName.Equals("STRING")) {
#ifdef DEBUG_CLIPBOARD
            g_print("    Selection owner has matching type: %s\n", atomName.mBuffer);
#endif
            *outResult = PR_TRUE;
            break;
          }
        }
        if (flavorStr.Equals(atomName)) {
#ifdef DEBUG_CLIPBOARD
          g_print("    Selection owner has matching type: %s\n", atomName.mBuffer);
#endif
          *outResult = PR_TRUE;
          break;
        }
      }
    }
  }
#ifdef DEBUG_CLIPBOARD
  g_print("    returning %i\n  }\n", *outResult);
#endif

  nsMemory::Free(mSelectionData.data);
  mSelectionData.data = nsnull;
  mSelectionData.length = 0;

  return NS_OK;

}










void nsClipboard::SelectionGetCB(GtkWidget        *widget,
                                 GtkSelectionData *aSelectionData,
                                 guint            aInfo,
                                 guint            aTime)
{
#ifdef DEBUG_CLIPBOARD
  g_print("nsClipboard::SelectionGetCB\n"); 
#endif
  nsClipboard *cb = (nsClipboard *)gtk_object_get_data(GTK_OBJECT(widget),
                                                       "cb");

  void     *clipboardData;
  PRUint32 dataLength;
  nsresult rv;

  PRInt32 whichClipboard = -1;

  if (aSelectionData->selection == GDK_SELECTION_PRIMARY)
    whichClipboard = kSelectionClipboard;
  else if (aSelectionData->selection == GDK_SELECTION_CLIPBOARD)
    whichClipboard = kGlobalClipboard;

#ifdef DEBUG_CLIPBOARD
  g_print("  whichClipboard = %d\n", whichClipboard);
#endif
  nsCOMPtr<nsITransferable> transferable(cb->GetTransferable(whichClipboard));

  
  if (!transferable) {
#ifdef DEBUG_CLIPBOARD
    g_print("Clipboard has no transferable!\n");
#endif
    return;
  }
#ifdef DEBUG_CLIPBOARD
  g_print("  aInfo == %s\n  transferable == %p\n", gdk_atom_name(aInfo), transferable.get());
  g_print("  aSD->type == %s\n  aSD->target == %s\n", gdk_atom_name(aSelectionData->type),
          gdk_atom_name(aSelectionData->target));
#endif
  const char *dataFlavor = nsnull;
  char *tstr = gdk_atom_name(aInfo);
  nsCAutoString type(tstr);
  g_free(tstr);

  if (type.Equals("STRING") ||
      type.Equals("UTF8_STRING") ||
      type.Equals("COMPOUND_TEXT") ||
      type.Equals("TEXT"))
  {
    dataFlavor = kUnicodeMime;
  } else {
    dataFlavor = type.get();
  }

  
  nsCOMPtr<nsISupports> genericDataWrapper;
  rv = transferable->GetTransferData(dataFlavor,
                                     getter_AddRefs(genericDataWrapper),
                                     &dataLength);
  nsPrimitiveHelpers::CreateDataFromPrimitive ( dataFlavor, genericDataWrapper, &clipboardData, dataLength );
  if (NS_SUCCEEDED(rv) && clipboardData && dataLength > 0) {
    size_t size = 1;
    
    
    
    

#ifdef DEBUG_CLIPBOARD
    printf("got data from transferable\n");
    printf("clipboardData is %s\n", clipboardData);
    printf("length is %d\n", dataLength);
#endif

    if (type.Equals("STRING")) {
      
      char* plainTextData = nsnull;
      PRUnichar* castedUnicode = NS_REINTERPRET_CAST(PRUnichar*, clipboardData);
      PRInt32 plainTextLen = 0;
      nsPrimitiveHelpers::ConvertUnicodeToPlatformPlainText (castedUnicode, dataLength / 2,
                                                             &plainTextData, &plainTextLen);
      if (clipboardData) {
        nsMemory::Free(NS_REINTERPRET_CAST(char*, clipboardData));
        clipboardData = plainTextData;
        dataLength = plainTextLen;
      }
    } else if (type.Equals("UTF8_STRING")) {
      if (clipboardData) {
        PRUnichar* castedUnicode = NS_REINTERPRET_CAST(PRUnichar*, clipboardData);
        char *utf8String =
            ToNewUTF8String(nsDependentString(castedUnicode, dataLength/2));
        nsMemory::Free(NS_REINTERPRET_CAST(char*, clipboardData));
        clipboardData = utf8String;
        dataLength = strlen(utf8String);
      }
    } else if (type.Equals(kHTMLMime)) {
      if (clipboardData) {
        







        char *buffer = NS_STATIC_CAST(char *,
                         nsMemory::Alloc((dataLength + 2) * sizeof(char)));
        if (buffer) {
          PRUnichar prefix = 0xFEFF;
          memcpy(buffer, &prefix, 2);
          memcpy(buffer + 2, clipboardData, dataLength);
          nsMemory::Free(NS_REINTERPRET_CAST(char*, clipboardData));
          clipboardData = NS_REINTERPRET_CAST(char*, buffer);
          dataLength = dataLength + 2;
        }
      }
    } else if (type.Equals("COMPOUND_TEXT") || type.Equals("TEXT")) {
      if (type.Equals("TEXT")) {
        
        aInfo = gdk_atom_intern("COMPOUND_TEXT", FALSE);
      }
      char *platformText;
      PRInt32 platformLen;
      
      

      nsCOMPtr<nsIUnicodeEncoder> encoder;
      
      nsCAutoString platformCharset;
      nsCOMPtr <nsIPlatformCharset> platformCharsetService = do_GetService(NS_PLATFORMCHARSET_CONTRACTID, &rv);
      if (NS_SUCCEEDED(rv))
        rv = platformCharsetService->GetCharset(kPlatformCharsetSel_Menu, platformCharset);
      if (NS_FAILED(rv))
        platformCharset.AssignLiteral("ISO-8859-1");
      
      
      nsCOMPtr<nsICharsetConverterManager> ccm = do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);
      rv = ccm->GetUnicodeEncoderRaw(platformCharset.get(), getter_AddRefs(encoder));
      NS_ASSERTION(NS_SUCCEEDED(rv), "GetUnicodeEncoderRaw failed");
      if (NS_FAILED(rv)) {
        nsMemory::Free(NS_REINTERPRET_CAST(char*, clipboardData));
        return;
      }

      encoder->SetOutputErrorBehavior(nsIUnicodeEncoder::kOnError_Replace, nsnull, '?');

      
      
      PRUnichar *castedData = NS_REINTERPRET_CAST(PRUnichar*, clipboardData);
      encoder->GetMaxLength(castedData, dataLength/2, &platformLen);
      if ( platformLen ) {
        platformText = NS_REINTERPRET_CAST(char*, nsMemory::Alloc(platformLen + sizeof(char)));
        if ( platformText ) {
          PRInt32 len = (PRInt32)dataLength/2;
          rv = encoder->Convert(castedData, &len, platformText, &platformLen);
          (platformText)[platformLen] = '\0';  
        }
 } 

      if (platformLen > 0) {
        int status = 0;
        XTextProperty prop;

#ifdef DEBUG_CLIPBOARD
        g_print("\nConverted text from unicode to platform locale\n");
        g_print("platformText is %s\n", platformText);
        g_print("platformLen is %d\n", platformLen);
#endif

        status = XmbTextListToTextProperty(GDK_DISPLAY(), &platformText, 1, XCompoundTextStyle,
                                           &prop);

        if (status == Success) {
#ifdef DEBUG_CLIPBOARD
          g_print("\nXmbTextListToTextProperty succeeded\n  text is %s\n  length is %d\n", prop.value,
                  prop.nitems);
#endif
          nsMemory::Free(platformText);
          platformText = (char *)prop.value;
          platformLen = prop.nitems;
        }
      }

#ifdef DEBUG_CLIPBOARD
      g_print("\nFinished trying to convert to platform charset\n");
#endif

      if (clipboardData) {
        nsMemory::Free(NS_REINTERPRET_CAST(char*, clipboardData));
        clipboardData = platformText;
        dataLength = platformLen;
      }
    }
#ifdef DEBUG_CLIPBOARD
    g_print("\nPutting data on clipboard:\n");
    g_print("  clipboardData is %s\n", clipboardData);
    g_print("  length is %d\n", dataLength);
#endif
    if (clipboardData && dataLength > 0)
      gtk_selection_data_set(aSelectionData,
                             aInfo, size*8,
                             NS_REINTERPRET_CAST(unsigned char *, clipboardData),
                             dataLength);
    else
      gtk_selection_data_set(aSelectionData,
                             gdk_atom_intern("NULL", FALSE), 8,
                             nsnull, 0);
    nsMemory::Free ( NS_REINTERPRET_CAST(char*, clipboardData) );
  }
#ifdef DEBUG_pavlov
  else
    printf("Transferable didn't support data flavor %s (type = %d)\n",
           dataFlavor ? dataFlavor : "None", aInfo);
#endif
}









void nsClipboard::SelectionClearCB(GtkWidget *aWidget,
                                   GdkEventSelection *aEvent,
                                   gpointer aData)
{
#ifdef DEBUG_CLIPBOARD
  g_print("  nsClipboard::SelectionClearCB\n");
#endif 

  if (!aWidget) {
    NS_ASSERTION(PR_FALSE, "Null widget passed to SelectionClearCB)\n");
    return;
  }

  if (!aEvent) {
    NS_ASSERTION(PR_FALSE, "Null event passed to SelectionClearCB)\n");
    return;
  }

  nsClipboard *cb = (nsClipboard *)gtk_object_get_data(GTK_OBJECT(aWidget),
                                                       "cb");

  if (aEvent->selection == GDK_SELECTION_PRIMARY) {
#ifdef DEBUG_CLIPBOARD
    g_print("clearing PRIMARY clipboard\n");
#endif
    cb->EmptyClipboard(kSelectionClipboard);
  } else if (aEvent->selection == GDK_SELECTION_CLIPBOARD) {
#ifdef DEBUG_CLIPBOARD
    g_print("clearing CLIPBOARD clipboard\n");
#endif
    cb->EmptyClipboard(kGlobalClipboard);
  }
}









void
nsClipboard::SelectionRequestCB (GtkWidget *aWidget,
                                 GtkSelectionData *aSelectionData,
                                 gpointer aData)
{
#ifdef DEBUG_CLIPBOARD
  g_print("  nsClipboard::SelectionRequestCB\n");
#endif 
}








void
nsClipboard::SelectionNotifyCB (GtkWidget *aWidget,
                                GtkSelectionData *aSelectionData,
                                gpointer aData)
{
#ifdef DEBUG_CLIPBOARD
  g_print("  nsClipboard::SelectionNotifyCB\n");
#endif 
}




GdkAtom nsClipboard::GetSelectionAtom(PRInt32 aWhichClipboard)
{
  switch (aWhichClipboard)
  {
  case kGlobalClipboard:
    return GDK_SELECTION_CLIPBOARD;
  case kSelectionClipboard:
  default:
    return GDK_SELECTION_PRIMARY;
  }
}


nsITransferable *nsClipboard::GetTransferable(PRInt32 aWhichClipboard)
{
  nsITransferable *transferable = nsnull;
  switch (aWhichClipboard)
  {
  case kGlobalClipboard:
    transferable = mGlobalTransferable;
    break;
  case kSelectionClipboard:
    transferable = mSelectionTransferable;
    break;
  }
  return transferable;
}


void nsClipboard::AddTarget(GdkAtom aAtom, GdkAtom aSelectionAtom)
{
  gtk_selection_add_target(sWidget, aSelectionAtom, aAtom, aAtom);
}

void nsClipboard::RegisterFormat(const char *aMimeStr, GdkAtom aSelectionAtom)
{
#ifdef DEBUG_CLIPBOARD
  g_print("  nsClipboard::RegisterFormat(%s)\n", aMimeStr);
#endif
  nsCAutoString mimeStr(aMimeStr);

  GdkAtom atom = gdk_atom_intern(aMimeStr, FALSE);

  
  if (mimeStr.Equals(kUnicodeMime)) {
    
    
    AddTarget(gdk_atom_intern("TEXT", FALSE), aSelectionAtom); 
    AddTarget(gdk_atom_intern("COMPOUND_TEXT", FALSE), aSelectionAtom);
    AddTarget(gdk_atom_intern("UTF8_STRING", FALSE), aSelectionAtom);
    AddTarget(GDK_SELECTION_TYPE_STRING, aSelectionAtom);
  }

  AddTarget(atom, aSelectionAtom);
}


PRBool nsClipboard::DoConvert(const char *aMimeStr, GdkAtom aSelectionAtom)
{
#ifdef DEBUG_CLIPBOARD
  g_print("  nsClipboard::DoConvert(%s, %s)\n", aMimeStr, gdk_atom_name(aSelectionAtom));
#endif
  

  PRBool r = PR_FALSE;

  nsCAutoString mimeStr(aMimeStr);

  if (mimeStr.Equals(kUnicodeMime)) {
    r = DoRealConvert(gdk_atom_intern("UTF8_STRING", FALSE), aSelectionAtom);
    if (r) return r;
    r = DoRealConvert(gdk_atom_intern("COMPOUND_TEXT", FALSE), aSelectionAtom);
    if (r) return r;
    r = DoRealConvert(GDK_SELECTION_TYPE_STRING, aSelectionAtom);
    if (r) return r;
  }

  GdkAtom atom = gdk_atom_intern(aMimeStr, FALSE);
  r = DoRealConvert(atom, aSelectionAtom);
  if (r) return r;

  return r;
}

PRBool nsClipboard::GetTargets(GdkAtom aSelectionAtom)
{
#ifdef DEBUG_CLIPBOARD
  g_print("    nsClipboard::GetTargets(%d)\n    {\n", aSelectionAtom);
#endif

  
  mBlocking = PR_TRUE;

  
  
  
  static GdkAtom targetsAtom = gdk_atom_intern("TARGETS", PR_FALSE);

  gtk_selection_convert(sWidget,
                        aSelectionAtom,
                        targetsAtom,
                        GDK_CURRENT_TIME);

  
  if (mBlocking) {
    
#ifdef DEBUG_CLIPBOARD
    g_print("      Waiting for the callback... mBlocking = %d\n", mBlocking);
#endif 

    if (!FindSelectionNotifyEvent())
      return PR_FALSE;

  }

#ifdef DEBUG_CLIPBOARD
  g_print("    }\n");
#endif

  if (mSelectionData.length <= 0)
    return PR_FALSE;

  return PR_TRUE;
}

void nsClipboard::SetCutBuffer()
{
#ifdef USE_CUTBUFFERS
  void *clipboardData;
  PRUint32 dataLength;
  nsresult rv;

  nsCOMPtr<nsITransferable> transferable(GetTransferable(kGlobalClipboard));

  
  if (!transferable) {
#ifdef DEBUG_CLIPBOARD
    g_print("Clipboard has no transferable!\n");
#endif
    return;
  }

  nsCOMPtr<nsISupports> genericDataWrapper;
  rv = transferable->GetTransferData(kUnicodeMime, getter_AddRefs(genericDataWrapper), &dataLength);
  nsPrimitiveHelpers::CreateDataFromPrimitive(kUnicodeMime, genericDataWrapper, &clipboardData, dataLength);
  if (NS_SUCCEEDED(rv) && clipboardData && dataLength > 0) {
    char* plainTextData = nsnull;
    PRUnichar* castedUnicode = NS_REINTERPRET_CAST(PRUnichar*, clipboardData);
    PRInt32 plainTextLen = 0;
    nsPrimitiveHelpers::ConvertUnicodeToPlatformPlainText (castedUnicode, dataLength / 2,
                                                           &plainTextData, &plainTextLen);
    if (clipboardData) {
      nsMemory::Free(NS_REINTERPRET_CAST(char*, clipboardData));
      clipboardData = plainTextData;
      dataLength = plainTextLen;
    }
  }

  XRotateBuffers(GDK_DISPLAY(), 1);
  XStoreBytes(GDK_DISPLAY(), (const char *) clipboardData, nsCRT::strlen((const char *)clipboardData));
#endif
}

static void
DispatchSelectionNotifyEvent(GtkWidget *widget, XEvent *xevent)
{
  GdkEvent event;
  event.selection.type = GDK_SELECTION_NOTIFY;
  event.selection.window = widget->window;
  event.selection.selection = xevent->xselection.selection;
  event.selection.target = xevent->xselection.target;
  event.selection.property = xevent->xselection.property;
  event.selection.time = xevent->xselection.time;

  gtk_widget_event(widget, &event);
}

static void
DispatchPropertyNotifyEvent(GtkWidget *widget, XEvent *xevent)
{
  if (gdk_window_get_events(widget->window) & GDK_PROPERTY_CHANGE_MASK) {
    GdkEvent event;
    event.property.type = GDK_PROPERTY_NOTIFY;
    event.property.window = widget->window;
    event.property.atom = xevent->xproperty.atom;
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
      GtkWidget *cbWidget = nsnull;
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

PRBool nsClipboard::FindSelectionNotifyEvent()
{
  Display *xDisplay = GDK_DISPLAY();
  checkEventContext context;
  context.cbWidget = nsnull;
  context.selAtom = gdk_atom_intern("GDK_SELECTION", FALSE);

  
  
  

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

      if (!mBlocking)
        return PR_TRUE;
    }

#ifdef POLL_WITH_XCONNECTIONNUMBER
    select_result = poll(fds, 1, kClipboardTimeout / 1000);
#else
    tv.tv_sec = 0;
    tv.tv_usec = kClipboardTimeout;
    select_result = select(cnumber, &select_set, nsnull, nsnull, &tv);
#endif
  } while (select_result == 1);

#ifdef DEBUG_CLIPBOARD
  printf("exceeded clipboard timeout\n");
#endif
  return PR_FALSE;
}




















void ConvertHTMLtoUCS2(char* data, PRInt32 dataLength,
                       PRUnichar** unicodeData, PRInt32& outUnicodeLen)
{
  nsCAutoString charset;
  GetHTMLCharset(data, dataLength, charset);
  if (charset.EqualsLiteral("UTF-16")) {
    outUnicodeLen = dataLength / 2 - 1;
    *unicodeData = NS_REINTERPRET_CAST(PRUnichar*,
                   nsMemory::Alloc((outUnicodeLen + 1) * sizeof(PRUnichar)));
    if ( unicodeData ) {
      memcpy(*unicodeData, data + sizeof(PRUnichar),
             outUnicodeLen * sizeof(PRUnichar));
      (*unicodeData)[outUnicodeLen] = '\0';
    }
  }
  else if (charset.EqualsLiteral("OLD-MOZILLA")) {
    outUnicodeLen = dataLength / 2;
    *unicodeData = NS_REINTERPRET_CAST(PRUnichar*,
                   nsMemory::Alloc((outUnicodeLen + 1) * sizeof(PRUnichar)));
    if ( unicodeData ) {
      memcpy(*unicodeData, data, outUnicodeLen * sizeof(PRUnichar));
      (*unicodeData)[outUnicodeLen] = '\0';
    }
  }
  else {
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
    
    decoder->GetMaxLength(data, dataLength, &outUnicodeLen);
    
    if (outUnicodeLen) {
      *unicodeData = NS_REINTERPRET_CAST(PRUnichar*,
                     nsMemory::Alloc((outUnicodeLen + 1) * sizeof(PRUnichar)));
      if ( unicodeData ) {
        PRInt32 numberTmp = dataLength;
        decoder->Convert(data, &numberTmp, *unicodeData, &outUnicodeLen);
#ifdef DEBUG_CLIPBOARD
        if (numberTmp != dataLength)
          printf("didn't consume all the bytes\n");
#endif
        
        (*unicodeData)[outUnicodeLen] = '\0';
      }
    } 
  }
}








void GetHTMLCharset(char* data, PRInt32 dataLength, nsACString& str)
{
  
  PRUnichar* beginChar =  (PRUnichar*)data;
  if ((beginChar[0] == 0xFFFE) || (beginChar[0] == 0xFEFF)) {
    str.AssignLiteral("UTF-16");
    return;
  }
  
  nsDependentCString htmlStr = nsDependentCString(data, dataLength);
  nsACString::const_iterator start, end, valueStart, valueEnd;

  htmlStr.BeginReading(start);
  htmlStr.EndReading(end);
  htmlStr.BeginReading(valueStart);
  htmlStr.BeginReading(valueEnd);
  
  if (CaseInsensitiveFindInReadable(NS_LITERAL_CSTRING("CONTENT=\"text/html;"),
                                    start, end)) {
    start = end;
    htmlStr.EndReading(end);

    if (CaseInsensitiveFindInReadable(NS_LITERAL_CSTRING("charset="),
                                      start, end)) {
      valueStart = end;
      start = end;
      htmlStr.EndReading(end);
          
      if (CaseInsensitiveFindInReadable(NS_LITERAL_CSTRING("\""), start, end))
        valueEnd = start;
    }
  }
  
  if (valueStart != valueEnd) {
    const nsACString & charsetStr = Substring(valueStart, valueEnd);
    if ( !charsetStr.IsEmpty() ) {
      nsCString charsetUpperStr;
      ToUpperCase(charsetStr, charsetUpperStr);
#ifdef DEBUG_CLIPBOARD
      printf("Charset of HTML = %s\n", charsetUpperStr.get());
#endif
      str.Assign(charsetUpperStr);
      return;
    }
  }
  
  
  
  
  
  
  str.AssignLiteral("OLD-MOZILLA");
}

