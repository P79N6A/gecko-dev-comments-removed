






































#include "nsClipboard.h"
#include "nsCOMPtr.h"
#include "nsITransferable.h"
#include "nsWidgetsCID.h"
#include "nsXPIDLString.h"
#include "nsPrimitiveHelpers.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsString.h"

#include <View.h>
#include <Clipboard.h>
#include <Message.h>


BView *nsClipboard::sView = 0;

#if defined(DEBUG_akkana) || defined(DEBUG_mcafee) || defined(DEBUG_toyoshim)
#  define DEBUG_CLIPBOARD
#endif
 






nsClipboard::nsClipboard() : nsBaseClipboard()
{
#ifdef DEBUG_CLIPBOARD
  printf("  nsClipboard::nsClipboard()\n");
#endif 

  mIgnoreEmptyNotification = PR_FALSE;
  mClipboardOwner = nsnull;
  mTransferable   = nsnull;
}






nsClipboard::~nsClipboard()
{
#ifdef DEBUG_CLIPBOARD
  printf("  nsClipboard::~nsClipboard()\n");  
#endif 
}

void nsClipboard::SetTopLevelView(BView *v)
{
  
  
  if (sView == v)
    return;

  if (sView != 0 && sView->Window() != 0)
    return;

  if(v == 0 || v->Window() == 0)
  {
#ifdef DEBUG_CLIPBOARD
    printf("  nsClipboard::SetTopLevelView: widget passed in is null or has no window!\n");
#endif 
    return;
  }

#ifdef DEBUG_CLIPBOARD
  printf("  nsClipboard::SetTopLevelView\n");
#endif 
}




NS_IMETHODIMP nsClipboard::SetNativeClipboardData(PRInt32 aWhichClipboard)
{
  mIgnoreEmptyNotification = PR_TRUE;

#ifdef DEBUG_CLIPBOARD
  printf("  nsClipboard::SetNativeClipboardData()\n");
#endif 

  
  if (nsnull == mTransferable) {
#ifdef DEBUG_CLIPBOARD
    printf("  SetNativeClipboardData: no transferable!\n");
#endif 
    return NS_ERROR_FAILURE;
  }

  
  if (!be_clipboard->Lock())
    return NS_ERROR_FAILURE;

  
  nsresult rv = NS_OK;
  if (B_OK == be_clipboard->Clear()) {
    
    if (BMessage *msg = be_clipboard->Data()) {
      
      nsCOMPtr<nsISupportsArray> dfList;
      mTransferable->FlavorsTransferableCanExport(getter_AddRefs(dfList));

      
      
      PRUint32 i;
      PRUint32 cnt;
      dfList->Count(&cnt);
      for (i = 0; i < cnt && rv == NS_OK; i++) {
        nsCOMPtr<nsISupports> genericFlavor;
        dfList->GetElementAt(i, getter_AddRefs(genericFlavor));
        nsCOMPtr<nsISupportsCString> currentFlavor (do_QueryInterface(genericFlavor));
        if (currentFlavor) {
          nsXPIDLCString flavorStr;
          currentFlavor->ToString(getter_Copies(flavorStr));

#ifdef DEBUG_CLIPBOARD
          printf("nsClipboard: %d/%d = %s\n", i, cnt, (const char *)flavorStr);
#endif 
          if (0 == strncmp(flavorStr, "text/", 5)) {
            
            void *data = nsnull;
            PRUint32 dataSize = 0;
            nsCOMPtr<nsISupports> genericDataWrapper;
            rv = mTransferable->GetTransferData(flavorStr, getter_AddRefs(genericDataWrapper), &dataSize);
            nsPrimitiveHelpers::CreateDataFromPrimitive(flavorStr, genericDataWrapper, &data, dataSize);
#ifdef DEBUG_CLIPBOARD
            if (NS_FAILED(rv))
              printf("nsClipboard: Error getting data from transferable\n");
#endif 
            if (dataSize && data != nsnull) {
              NS_ConvertUTF16toUTF8 cv((const PRUnichar *)data, (PRUint32)dataSize / 2);
              const char *utf8Str = cv.get();
              uint32 utf8Len = strlen(utf8Str);
#ifdef DEBUG_CLIPBOARD
              if (0 == strcmp(flavorStr, kUnicodeMime))
                printf(" => [%s]%s\n", kTextMime, utf8Str);
              else
                printf(" => [%s]%s\n", (const char *)flavorStr, utf8Str);
#endif 
              status_t rc;
              if (0 == strcmp(flavorStr, kUnicodeMime)) {
                
                rc = msg->AddData(kTextMime, B_MIME_TYPE, (void *)utf8Str, utf8Len);
              } else {
                
                rc = msg->AddData((const char *)flavorStr, B_MIME_TYPE, (void *)utf8Str, utf8Len);
              }
              if (rc != B_OK)
                rv = NS_ERROR_FAILURE;
            } else {
#ifdef DEBUG_CLIPBOARD
              printf("nsClipboard: Error null data from transferable\n");
#endif 
                
                rv = NS_OK;
            }
          } else {
            
            void *data = nsnull;
            PRUint32 dataSize = 0;
            nsCOMPtr<nsISupports> genericDataWrapper;
            rv = mTransferable->GetTransferData(flavorStr, getter_AddRefs(genericDataWrapper), &dataSize);
            nsPrimitiveHelpers::CreateDataFromPrimitive(flavorStr, genericDataWrapper, &data, dataSize);
#ifdef DEBUG_CLIPBOARD
            if (NS_FAILED(rv))
              printf("nsClipboard: Error getting data from transferable\n");
#endif 
            if (dataSize && data != nsnull) {
#ifdef DEBUG_CLIPBOARD
              printf("[%s](binary)\n", (const char *)flavorStr);
#endif 
              if (B_OK != msg->AddData((const char *)flavorStr, B_MIME_TYPE, data, dataSize))
                rv = NS_ERROR_FAILURE;
            }
          }
        } else {
#ifdef DEBUG_CLIPBOARD
          printf("nsClipboard: Error getting flavor\n");
#endif 
          rv = NS_ERROR_FAILURE;
        }
      } 
    } else {
      rv = NS_ERROR_FAILURE;
    }
  } else {
    rv = NS_ERROR_FAILURE;
  }
  if (B_OK != be_clipboard->Commit())
    rv = NS_ERROR_FAILURE;
  be_clipboard->Unlock();

  mIgnoreEmptyNotification = PR_FALSE;

  return rv;
}





NS_IMETHODIMP
nsClipboard::GetNativeClipboardData(nsITransferable * aTransferable, PRInt32 aWhichClipboard )
{
#ifdef DEBUG_CLIPBOARD
  printf("  nsClipboard::GetNativeClipboardData()\n");
#endif 

  
  if (nsnull == aTransferable) {
    printf("  GetNativeClipboardData: Transferable is null!\n");
    return NS_ERROR_FAILURE;
  }

  
  nsresult rv;
  nsCOMPtr<nsISupportsArray> flavorList;
  rv = aTransferable->FlavorsTransferableCanImport(getter_AddRefs(flavorList));
  if (NS_FAILED(rv))
    return NS_ERROR_FAILURE;

  
  if (!be_clipboard->Lock())
    return NS_ERROR_FAILURE;

  BMessage *msg = be_clipboard->Data();
  if (!msg)
    return NS_ERROR_FAILURE;  
#ifdef DEBUG_CLIPBOARD
  msg->PrintToStream();
#endif 

  PRUint32 cnt;
  flavorList->Count(&cnt);
  for (PRUint32 i = 0; i < cnt; i++) {
    nsCOMPtr<nsISupports> genericFlavor;
    flavorList->GetElementAt(i, getter_AddRefs(genericFlavor));
    nsCOMPtr<nsISupportsCString> currentFlavor(do_QueryInterface(genericFlavor));
    if (currentFlavor) {
      nsXPIDLCString flavorStr;
      currentFlavor->ToString(getter_Copies(flavorStr));

#ifdef DEBUG_CLIPBOARD
      printf("nsClipboard: %d/%d = %s\n", i, cnt, (const char *)flavorStr);
#endif 
      const void *data;
      ssize_t size;
      if (0 == strncmp(flavorStr, "text/", 5)) {
        
        status_t rc;
        if (0 == strcmp(flavorStr, kUnicodeMime))
          rc = msg->FindData(kTextMime, B_MIME_TYPE, &data, &size);
        else
          rc = msg->FindData(flavorStr, B_MIME_TYPE, &data, &size);
        if (rc != B_OK || !data || !size) {
#ifdef DEBUG_CLIPBOARD
          printf("nsClipboard: not found in BMessage\n");
#endif 
        } else {
          NS_ConvertUTF8toUTF16 ucs2Str((const char *)data, (PRUint32)size);
          nsCOMPtr<nsISupports> genericDataWrapper;
          nsPrimitiveHelpers::CreatePrimitiveForData(flavorStr, (void *)ucs2Str.get(), ucs2Str.Length() * 2, getter_AddRefs(genericDataWrapper));
          rv = aTransferable->SetTransferData(flavorStr, genericDataWrapper, ucs2Str.Length() * 2);
        }
      } else {
        
        if (B_OK != msg->FindData(flavorStr, B_MIME_TYPE, &data, &size)) {
#ifdef DEBUG_CLIPBOARD
          printf("nsClipboard: not found in BMessage\n");
#endif 
        } else {
          nsCOMPtr<nsISupports> genericDataWrapper;
          nsPrimitiveHelpers::CreatePrimitiveForData(flavorStr, (void *)data, (PRUint32)size, getter_AddRefs(genericDataWrapper));
          rv = aTransferable->SetTransferData(flavorStr, genericDataWrapper, size);
        }
      }
#ifdef DEBUG_CLIPBOARD
      if (NS_FAILED(rv))
        printf("nsClipboard: Error SetTransferData\n");
#endif 
    } else {
      rv = NS_ERROR_FAILURE;
#ifdef DEBUG_CLIPBOARD
      printf("nsClipboard: Error gerring flavor");
#endif 
    }
    if (rv != NS_OK)
      break;
  } 

  be_clipboard->Unlock();

  return rv;
}
