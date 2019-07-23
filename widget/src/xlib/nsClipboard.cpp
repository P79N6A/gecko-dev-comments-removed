











































#include "nsAppShell.h"
#include "nsClipboard.h"

#include "nsCOMPtr.h"
#include "nsCRT.h"
#include "nsISupportsArray.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"

#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsWidgetsCID.h"
#include "nsXPIDLString.h"
#include "nsPrimitiveHelpers.h"

#include "nsTextFormatter.h"

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "xlibrgb.h"


#include "nsIServiceManager.h"
#include "nsICharsetConverterManager.h"

#  include "nsIPlatformCharset.h"



nsITransferable            *nsClipboard::mTransferable = nsnull;
Window                      nsClipboard::sWindow;
Display                    *nsClipboard::sDisplay;

#if defined(DEBUG_mcafee) || defined(DEBUG_pavlov)
#define DEBUG_CLIPBOARD
#endif

NS_IMPL_ISUPPORTS1(nsClipboard, nsIClipboard)

nsClipboard::nsClipboard() {
  sDisplay = xxlib_rgb_get_display(nsAppShell::GetXlibRgbHandle());

  Init();
}

nsClipboard::~nsClipboard() {
  NS_IF_RELEASE(sWidget);
}

 void nsClipboard::Shutdown() {
  NS_IF_RELEASE(mTransferable);
}



void nsClipboard::Init() {
  NS_ASSERTION(!sWidget, "already initialized");

  sWidget = new nsWidget();
  if (!sWidget) return;
  NS_ADDREF(sWidget);

  const nsRect rect(0,0,100,100);

  sWidget->Create((nsIWidget *)nsnull, rect, Callback,
                  (nsIDeviceContext *)nsnull, (nsIAppShell *)nsnull,
                  (nsIToolkit *)nsnull, (nsWidgetInitData *)nsnull);
  sWindow = (Window)sWidget->GetNativeData(NS_NATIVE_WINDOW);

  XSelectInput(sDisplay, sWindow, 0x0fffff);
}





nsEventStatus PR_CALLBACK nsClipboard::Callback(nsGUIEvent *event) {
  XEvent *ev = (XEvent *)event->nativeMsg;
  
  
  if(ev == nsnull)
    return nsEventStatus_eIgnore;

  
  if (ev->type == SelectionRequest) {
    if (mTransferable == nsnull) {
      fprintf(stderr, "nsClipboard::Callback: null transferable\n");
      return nsEventStatus_eIgnore;
    }

    

    const char *dataFlavor = kUnicodeMime;
    nsCOMPtr<nsISupports> genDataWrapper;
    nsresult rv;
    PRUint32 dataLength;
    void *data;
    data = malloc(16384);
    rv = mTransferable->GetTransferData(dataFlavor,
                                        getter_AddRefs(genDataWrapper),
                                        &dataLength);
    nsPrimitiveHelpers::CreateDataFromPrimitive(dataFlavor, genDataWrapper,
                                                &data, dataLength);
    if (NS_SUCCEEDED(rv) && data && dataLength) {
      char *plainText = nsnull;
      PRUnichar* unicodeData = NS_REINTERPRET_CAST(PRUnichar*, data);
      PRInt32 plainLen = 0;
      nsPrimitiveHelpers::ConvertUnicodeToPlatformPlainText(unicodeData,
                                                            dataLength/2,
                                                            &plainText,
                                                            &plainLen);
      if (data) {
        free(data);
        data = plainText;
        dataLength = plainLen;
      }

      
      XChangeProperty(sDisplay,
                      ev->xselectionrequest.requestor,
                      ev->xselectionrequest.property, 
                      ev->xselectionrequest.target,
                      8, PropModeReplace,
                      (unsigned char *)data, dataLength);

      
      XEvent aEvent;
      aEvent.type = SelectionNotify;
      aEvent.xselection.serial = ev->xselectionrequest.serial;
      aEvent.xselection.display = ev->xselectionrequest.display;
      aEvent.xselection.requestor = ev->xselectionrequest.requestor;
      aEvent.xselection.selection = ev->xselectionrequest.selection;
      aEvent.xselection.target = ev->xselectionrequest.target;
      aEvent.xselection.property = ev->xselectionrequest.property;
      aEvent.xselection.time = CurrentTime;
      XSendEvent(sDisplay, ev->xselectionrequest.requestor, 1, 0, &aEvent);
   }
  }
  return nsEventStatus_eIgnore;
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




NS_IMETHODIMP nsClipboard::SetNativeClipboardData(PRInt32 aWhichClipboard)
{
  
  if (XSetSelectionOwner(sDisplay, XA_PRIMARY, sWindow, CurrentTime))
    if (XGetSelectionOwner(sDisplay, XA_PRIMARY) != sWindow) {
      fprintf(stderr, "nsClipboard::SetData: Cannot get ownership\n");
      return NS_ERROR_FAILURE;
    }
  
  
  
  nsCOMPtr<nsISupportsArray> flavorList;
  nsCOMPtr<nsITransferable> transferable(GetTransferable(aWhichClipboard));

  
  
  
  if (aWhichClipboard == kSelectionClipboard) {
    NS_IF_RELEASE(mTransferable);
    mTransferable = transferable; 
    NS_IF_ADDREF(mTransferable);
  }
  
  
  if (nsnull == transferable) {
#ifdef DEBUG_faulkner
    fprintf(stderr, "nsClipboard::SetNativeClipboardData(): no transferable!\n");
#endif 
    return NS_ERROR_FAILURE;
  }

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

      
      
      
    }
  }

  mIgnoreEmptyNotification = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP nsClipboard::SetData(nsITransferable *aTransferable,
                                   nsIClipboardOwner *anOwner,
                                   PRInt32 aWhichClipboard) 
{
  if (XSetSelectionOwner(sDisplay, XA_PRIMARY, sWindow, CurrentTime))
    if (XGetSelectionOwner(sDisplay, XA_PRIMARY) != sWindow) {
      fprintf(stderr, "nsClipboard::SetData: Cannot get ownership\n");
      return NS_ERROR_FAILURE;
    }

  
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
    break;
  }
  
  return SetNativeClipboardData(aWhichClipboard);
}

NS_IMETHODIMP nsClipboard::GetData(nsITransferable *aTransferable,
                                   PRInt32 aWhichClipboard)
{
  unsigned char *data = 0;
  unsigned long bytes = 0;
  Bool only_if_exists;
  Atom data_atom;
  int i;

  if (aTransferable == nsnull) {
    fprintf(stderr, "nsClipboard::GetData: NULL transferable\n");
    return NS_ERROR_FAILURE;
  }


  
  NS_IF_RELEASE(mTransferable);
  mTransferable = GetTransferable(aWhichClipboard);
  NS_ASSERTION(!mTransferable,"mTransferable is null!! see bug 80181");
  if (!mTransferable) return NS_ERROR_FAILURE;
  NS_ADDREF(mTransferable);
  
  
  

  if (XGetSelectionOwner(sDisplay, XA_PRIMARY) == sWindow) {
    const char *dataFlavor = kUnicodeMime;
    nsCOMPtr<nsISupports> genDataWrapper;
    nsresult rv;
    PRUint32 dataLength;
    nsCOMPtr<nsITransferable> trans = do_QueryInterface(aTransferable);
    if (!trans)
      return NS_ERROR_FAILURE;

    rv = mTransferable->GetTransferData(dataFlavor,
                                        getter_AddRefs(genDataWrapper),
                                        &dataLength);
    if (NS_SUCCEEDED(rv)) {
      rv = trans->SetTransferData(dataFlavor,
                                  genDataWrapper,
                                  dataLength);
    }
  } else {
    data_atom = XInternAtom(sDisplay, "DATA_ATOM", only_if_exists = False);
    XConvertSelection(sDisplay, XA_PRIMARY, XA_STRING, data_atom,
                      sWindow, CurrentTime);

    
    mBlocking = PR_TRUE;
    XEvent event;
    for (i=0; (mBlocking == PR_TRUE) && i<10000; i++) {
      if (XPending(sDisplay)) {
        XNextEvent(sDisplay, &event);
        if (event.type == SelectionNotify) {
          mBlocking = PR_FALSE;
        }
      }
    }
    
    
    if (mBlocking == PR_FALSE) {
      Atom type;
      int format;
      unsigned long items;
      if (event.xselection.property != None) {
        XGetWindowProperty(sDisplay, event.xselection.requestor, 
                           event.xselection.property, 0, 16384/4,
                           0, AnyPropertyType,
                           &type, &format, &items, &bytes, &data);
        
        bytes = strlen((char *)data);
      }
    }
    mBlocking = PR_FALSE;

    
    PRInt32 length = 0;
    PRUnichar *testing = nsnull;
    const char *constData = "";
    if (data)
       constData = (char*) data;
    nsPrimitiveHelpers::ConvertPlatformPlainTextToUnicode(constData,
                                                          (PRInt32)bytes,
                                                          &testing,
                                                          &length);

    nsCOMPtr<nsISupports> genDataWrapper;
    nsPrimitiveHelpers::CreatePrimitiveForData("text/unicode",
                                               testing, length,
                                               getter_AddRefs(genDataWrapper));

    aTransferable->SetTransferData("text/unicode",
                                   genDataWrapper,
                                   length);
    if (data)
      XFree(data);
    free(testing);
  }
  return NS_OK;
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

NS_IMETHODIMP nsClipboard::HasDataMatchingFlavors(nsISupportsArray *aFlavorList,
                                                  PRInt32 aWhichClipboard,
                                                  PRBool *_retval) {
  *_retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP nsClipboard::SupportsSelectionClipboard(PRBool *_retval) {
  NS_ENSURE_ARG_POINTER(_retval);

  *_retval = PR_TRUE; 
  return NS_OK;
}
