





































#include "nsClipboardCE.h"

#include "nsISupportsPrimitives.h"
#include "nsXPIDLString.h"
#include "nsCRT.h"
#include "nsComponentManagerUtils.h"

#include <winuserm.h>

nsClipboard::nsClipboard()
{
}

nsClipboard::~nsClipboard()
{
}

UINT nsClipboard::GetFormat(const char* aMimeStr)
{
  UINT format;

  if (strcmp(aMimeStr, kTextMime) == 0)
    format = CF_TEXT;
  else if (strcmp(aMimeStr, kUnicodeMime) == 0)
    format = CF_UNICODETEXT;
  else if (strcmp(aMimeStr, kJPEGImageMime) == 0 ||
           strcmp(aMimeStr, kPNGImageMime) == 0)
    format = CF_DIB;
  else if (strcmp(aMimeStr, kFileMime) == 0 || 
           strcmp(aMimeStr, kFilePromiseMime) == 0)
    format = CF_HDROP; 
  else
    format = ::RegisterClipboardFormat(NS_ConvertASCIItoUTF16(aMimeStr).get());

  

  return format;
}

NS_IMETHODIMP
nsClipboard::SetNativeClipboardData(PRInt32 aWhichClipboard)
{
  if (aWhichClipboard != kGlobalClipboard || !mTransferable)
    return NS_ERROR_INVALID_ARG;

  if (!::OpenClipboard(NULL))
    return NS_ERROR_FAILURE;

  if (!::EmptyClipboard())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsISupportsArray> flavorList;
  mTransferable->FlavorsTransferableCanExport(getter_AddRefs(flavorList));

  PRUint32 count, i;
  flavorList->Count(&count);
  nsresult rv = NS_OK;

  for (i = 0; i < count; i++) {
    nsCOMPtr<nsISupports> listItem;
    flavorList->GetElementAt(i, getter_AddRefs(listItem));

    nsCOMPtr<nsISupportsCString> flavor(do_QueryInterface(listItem));
    if (!flavor)
      continue;
    nsXPIDLCString flavorStr;
    flavor->ToString(getter_Copies(flavorStr));

    UINT format = GetFormat(flavorStr);

    PRUint32 len;
    nsCOMPtr<nsISupports> wrapper;
    mTransferable->GetTransferData(flavorStr, getter_AddRefs(wrapper), &len);
    if (!wrapper)
      continue;

    char *memory = nsnull;
    nsCOMPtr<nsISupportsString> textItem(do_QueryInterface(wrapper));
    nsCOMPtr<nsISupportsPRBool> boolItem(do_QueryInterface(wrapper));

    if (format == CF_TEXT || format == CF_DIB || format == CF_HDROP) {
      NS_WARNING("Setting this clipboard format not implemented");
      continue;
    } else if (textItem) {
      
      nsAutoString text;
      textItem->GetData(text);
      PRInt32 len = text.Length() * 2;

      memory = reinterpret_cast<char*>(::LocalAlloc(LMEM_FIXED, len + 2));
      if (!memory) {
        rv = NS_ERROR_OUT_OF_MEMORY;
        break;
      }
      memcpy(memory, nsPromiseFlatString(text).get(), len);
      memory[len]   = '\0';
      memory[len+1] = '\0';
    } else if (boolItem) {
      
      PRBool value;
      boolItem->GetData(&value);
      memory = reinterpret_cast<char*>(::LocalAlloc(LMEM_FIXED, 1));
      if (!memory) {
        rv = NS_ERROR_OUT_OF_MEMORY;
        break;
      }
      *memory = value ? 1 : 0;
    } else {
      NS_WARNING("Can't set unknown transferrable primitive");
      continue;
    }

    if (!::SetClipboardData(format, memory)) {
      NS_WARNING("::SetClipboardData failed");
      if (memory)
        ::LocalFree(memory);
    }
  }

  ::CloseClipboard();

  return rv;
}

NS_IMETHODIMP
nsClipboard::GetNativeClipboardData(nsITransferable *aTransferable,
				    PRInt32 aWhichClipboard)
{
  if (aWhichClipboard != kGlobalClipboard || !aTransferable)
    return NS_ERROR_INVALID_ARG;

  if (!::OpenClipboard(NULL))
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsISupportsArray> flavorList;
  aTransferable->FlavorsTransferableCanImport(getter_AddRefs(flavorList));

  PRUint32 count, i;
  flavorList->Count(&count);
  nsresult rv = NS_OK;

  for (i = 0; i < count; i++) {
    nsCOMPtr<nsISupports> listItem;
    flavorList->GetElementAt(i, getter_AddRefs(listItem));

    nsCOMPtr<nsISupportsCString> flavor(do_QueryInterface(listItem));
    if (!flavor)
      continue;
    nsXPIDLCString flavorStr;
    flavor->ToString(getter_Copies(flavorStr));

    UINT format = GetFormat(flavorStr);

    void *data;
    data = ::GetClipboardData(format);
    if (!data)
      continue;

    if (format == CF_UNICODETEXT) {
      PRUnichar *dataStr = reinterpret_cast<PRUnichar*>(data);
      nsString *stringCopy = new nsString(dataStr);

      nsCOMPtr<nsISupportsString> primitive =
        do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID);
      if (!primitive) {
        rv = NS_ERROR_OUT_OF_MEMORY;
        break;
      }

      primitive->SetData(*stringCopy);
      aTransferable->SetTransferData(flavorStr, primitive,
                                     stringCopy->Length() * sizeof(PRUnichar));
    } else {
      NS_WARNING("Getting this clipboard format not implemented");
      continue;
    }

  }

  ::CloseClipboard();

  return rv;
}

NS_IMETHODIMP nsClipboard::HasDataMatchingFlavors(const char** aFlavorList,
                                                  PRUint32 aLength,
                                                  PRInt32 aWhichClipboard,
                                                  PRBool *_retval)
{
  *_retval = PR_FALSE;
  if (aWhichClipboard != kGlobalClipboard || !aFlavorList)
    return NS_OK;
  for (PRUint32 i = 0;i < aLength; ++i) {

    UINT format = GetFormat(aFlavorList[i]);
    if (::IsClipboardFormatAvailable(format)) {
      *_retval = PR_TRUE;
      break;
    }
  }

  return NS_OK;
}
