







































#include "nsClipboard.h"
#include <ole2.h>
#include <shlobj.h>
#include <intshcut.h>


#include <shellapi.h>

#include "nsCOMPtr.h"
#include "nsDataObj.h"
#include "nsIClipboardOwner.h"
#include "nsString.h"
#include "nsIFormatConverter.h"
#include "nsITransferable.h"
#include "nsCOMPtr.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsPrimitiveHelpers.h"
#include "nsImageClipboard.h"
#include "nsIWidget.h"
#include "nsIComponentManager.h"
#include "nsWidgetsCID.h"
#include "nsCRT.h"

#include "nsNetUtil.h"

#include "nsIImage.h"
#include "nsIObserverService.h"



UINT nsClipboard::CF_HTML = ::RegisterClipboardFormat("HTML Format");







nsClipboard::nsClipboard() : nsBaseClipboard()
{
  mIgnoreEmptyNotification = PR_FALSE;
  mWindow         = nsnull;

  
  
  nsCOMPtr<nsIObserverService> observerService =
    do_GetService("@mozilla.org/observer-service;1");
  if (observerService)
    observerService->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, PR_FALSE);
}




nsClipboard::~nsClipboard()
{

}

NS_IMPL_ISUPPORTS_INHERITED1(nsClipboard, nsBaseClipboard, nsIObserver)


UINT nsClipboard::GetFormat(const char* aMimeStr)
{
  UINT format;

  if (strcmp(aMimeStr, kTextMime) == 0)
    format = CF_TEXT;
  else if (strcmp(aMimeStr, kUnicodeMime) == 0)
    format = CF_UNICODETEXT;
#ifndef WINCE
  else if (strcmp(aMimeStr, kJPEGImageMime) == 0)
    format = CF_DIB;
  else if (strcmp(aMimeStr, kFileMime) == 0 || 
           strcmp(aMimeStr, kFilePromiseMime) == 0)
    format = CF_HDROP;
#endif
  else if (strcmp(aMimeStr, kNativeHTMLMime) == 0)
    format = CF_HTML;
  else
    format = ::RegisterClipboardFormat(aMimeStr);

  return format;
}


nsresult nsClipboard::CreateNativeDataObject(nsITransferable * aTransferable, IDataObject ** aDataObj, nsIURI * uri)
{
  if (nsnull == aTransferable) {
    return NS_ERROR_FAILURE;
  }

  
  
  nsDataObj * dataObj = new nsDataObj(uri);

  if (!dataObj) 
    return NS_ERROR_OUT_OF_MEMORY;

  dataObj->AddRef();

  
  nsresult res = SetupNativeDataObject(aTransferable, dataObj);
  if (NS_OK == res) {
    *aDataObj = dataObj; 
  } else {
    delete dataObj;
  }
  return res;
}


nsresult nsClipboard::SetupNativeDataObject(nsITransferable * aTransferable, IDataObject * aDataObj)
{
  if (nsnull == aTransferable || nsnull == aDataObj) {
    return NS_ERROR_FAILURE;
  }

  nsDataObj * dObj = NS_STATIC_CAST(nsDataObj *, aDataObj);

  
  
  dObj->SetTransferable(aTransferable);

  
  nsCOMPtr<nsISupportsArray> dfList;
  aTransferable->FlavorsTransferableCanExport(getter_AddRefs(dfList));

  
  
  PRUint32 i;
  PRUint32 cnt;
  dfList->Count(&cnt);
  for (i=0;i<cnt;i++) {
    nsCOMPtr<nsISupports> genericFlavor;
    dfList->GetElementAt ( i, getter_AddRefs(genericFlavor) );
    nsCOMPtr<nsISupportsCString> currentFlavor ( do_QueryInterface(genericFlavor) );
    if ( currentFlavor ) {
      nsXPIDLCString flavorStr;
      currentFlavor->ToString(getter_Copies(flavorStr));
      UINT format = GetFormat(flavorStr);

      
      
      FORMATETC fe;
      SET_FORMATETC(fe, format, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL);
      dObj->AddDataFlavor(flavorStr, &fe);
      
      
      
      
      if ( strcmp(flavorStr, kUnicodeMime) == 0 ) {
        
        
        FORMATETC textFE;
        SET_FORMATETC(textFE, CF_TEXT, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL);
        dObj->AddDataFlavor(kTextMime, &textFE);
      }
      else if ( strcmp(flavorStr, kHTMLMime) == 0 ) {      
        
        
        FORMATETC htmlFE;
        SET_FORMATETC(htmlFE, CF_HTML, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL);
        dObj->AddDataFlavor(kHTMLMime, &htmlFE);     
      }
      else if ( strcmp(flavorStr, kURLMime) == 0 ) {
        
        
        
        FORMATETC shortcutFE;
        SET_FORMATETC(shortcutFE, ::RegisterClipboardFormat(CFSTR_FILEDESCRIPTORA), 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL)
        dObj->AddDataFlavor(kURLMime, &shortcutFE);      
        SET_FORMATETC(shortcutFE, ::RegisterClipboardFormat(CFSTR_FILEDESCRIPTORW), 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL)
        dObj->AddDataFlavor(kURLMime, &shortcutFE);      
#ifndef WINCE
        SET_FORMATETC(shortcutFE, ::RegisterClipboardFormat(CFSTR_FILECONTENTS), 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL)
        dObj->AddDataFlavor(kURLMime, &shortcutFE);  
#endif
        SET_FORMATETC(shortcutFE, ::RegisterClipboardFormat(CFSTR_INETURLA), 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL)
        dObj->AddDataFlavor(kURLMime, &shortcutFE);      
        SET_FORMATETC(shortcutFE, ::RegisterClipboardFormat(CFSTR_INETURLW), 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL)
        dObj->AddDataFlavor(kURLMime, &shortcutFE);      
      }
      else if ( strcmp(flavorStr, kPNGImageMime) == 0 || strcmp(flavorStr, kJPEGImageMime) == 0 ||
                  strcmp(flavorStr, kGIFImageMime) == 0 || strcmp(flavorStr, kNativeImageMime) == 0  ) {
        
        FORMATETC imageFE;
        SET_FORMATETC(imageFE, CF_DIB, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL)
        dObj->AddDataFlavor(flavorStr, &imageFE);      
      }
#ifndef WINCE
      else if ( strcmp(flavorStr, kFilePromiseMime) == 0 ) {
         
         
         
         
         
         
         
         
         
         
         
         
         
        FORMATETC shortcutFE;
        SET_FORMATETC(shortcutFE, ::RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT), 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL)
        dObj->AddDataFlavor(kFilePromiseMime, &shortcutFE);
      }
#endif
    }
  }

  return NS_OK;
}


NS_IMETHODIMP nsClipboard::SetNativeClipboardData ( PRInt32 aWhichClipboard )
{
  if ( aWhichClipboard != kGlobalClipboard )
    return NS_ERROR_FAILURE;

  mIgnoreEmptyNotification = PR_TRUE;

  
  if (nsnull == mTransferable) {
    return NS_ERROR_FAILURE;
  }

  IDataObject * dataObj;
  if ( NS_SUCCEEDED(CreateNativeDataObject(mTransferable, &dataObj, NULL)) ) { 
    ::OleSetClipboard(dataObj);
    dataObj->Release();
  } else {
    
    ::OleSetClipboard(NULL);
  }

  mIgnoreEmptyNotification = PR_FALSE;

  return NS_OK;
}



nsresult nsClipboard::GetGlobalData(HGLOBAL aHGBL, void ** aData, PRUint32 * aLen)
{
  
  
  
  
  nsresult  result = NS_ERROR_FAILURE;
  if (aHGBL != NULL) {
    LPSTR lpStr = (LPSTR) GlobalLock(aHGBL);
    DWORD allocSize = GlobalSize(aHGBL);
    char* data = NS_STATIC_CAST(char*, nsMemory::Alloc(allocSize + sizeof(PRUnichar)));
    if ( data ) {    
      memcpy ( data, lpStr, allocSize );
      data[allocSize] = data[allocSize + 1] = '\0';     

      GlobalUnlock(aHGBL);
      *aData = data;
      *aLen = allocSize;

      result = NS_OK;
    }
  } 
  else {
    
    
    *aData = nsnull;
    *aLen  = 0;
    LPVOID lpMsgBuf;

    FormatMessage( 
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
        (LPTSTR) &lpMsgBuf,
        0,
        NULL 
    );

    
    MessageBox( NULL, (const char *)lpMsgBuf, "GetLastError", MB_OK|MB_ICONINFORMATION );

    
    LocalFree( lpMsgBuf );    
  }

  return result;
}


nsresult nsClipboard::GetNativeDataOffClipboard(nsIWidget * aWindow, UINT , UINT aFormat, void ** aData, PRUint32 * aLen)
{
  HGLOBAL   hglb; 
  nsresult  result = NS_ERROR_FAILURE;

  HWND nativeWin = nsnull;
  if (::OpenClipboard(nativeWin)) { 
    hglb = ::GetClipboardData(aFormat); 
    result = GetGlobalData(hglb, aData, aLen);
    ::CloseClipboard();
  }
  return result;
}

static void DisplayErrCode(HRESULT hres) 
{
#if defined(DEBUG_rods) || defined(DEBUG_pinkerton)
  if (hres == E_INVALIDARG) {
    printf("E_INVALIDARG\n");
  } else
  if (hres == E_UNEXPECTED) {
    printf("E_UNEXPECTED\n");
  } else
  if (hres == E_OUTOFMEMORY) {
    printf("E_OUTOFMEMORY\n");
  } else
  if (hres == DV_E_LINDEX ) {
    printf("DV_E_LINDEX\n");
  } else
  if (hres == DV_E_FORMATETC) {
    printf("DV_E_FORMATETC\n");
  }  else
  if (hres == DV_E_TYMED) {
    printf("DV_E_TYMED\n");
  }  else
  if (hres == DV_E_DVASPECT) {
    printf("DV_E_DVASPECT\n");
  }  else
  if (hres == OLE_E_NOTRUNNING) {
    printf("OLE_E_NOTRUNNING\n");
  }  else
  if (hres == STG_E_MEDIUMFULL) {
    printf("STG_E_MEDIUMFULL\n");
  }  else
  if (hres == DV_E_CLIPFORMAT) {
    printf("DV_E_CLIPFORMAT\n");
  }  else
  if (hres == S_OK) {
    printf("S_OK\n");
  } else {
    printf("****** DisplayErrCode 0x%X\n", hres);
  }
#endif
}


static HRESULT FillSTGMedium(IDataObject * aDataObject, UINT aFormat, LPFORMATETC pFE, LPSTGMEDIUM pSTM, DWORD aTymed)
{
  SET_FORMATETC(*pFE, aFormat, 0, DVASPECT_CONTENT, -1, aTymed);

  
  HRESULT hres = S_FALSE;
  hres = aDataObject->QueryGetData(pFE);
  DisplayErrCode(hres);
  if (S_OK == hres) {
    hres = aDataObject->GetData(pFE, pSTM);
    DisplayErrCode(hres);
  }
  return hres;
}



nsresult nsClipboard::GetNativeDataOffClipboard(IDataObject * aDataObject, UINT aIndex, UINT aFormat, void ** aData, PRUint32 * aLen)
{
  nsresult result = NS_ERROR_FAILURE;
  *aData = nsnull;
  *aLen = 0;

  if ( !aDataObject )
    return result;

  UINT    format = aFormat;
  HRESULT hres   = S_FALSE;

  
  
  
  FORMATETC fe;
  STGMEDIUM stm;
  hres = FillSTGMedium(aDataObject, format, &fe, &stm, TYMED_HGLOBAL);

  
  
  if (S_OK == hres) {
    static CLIPFORMAT fileDescriptorFlavorA = ::RegisterClipboardFormat( CFSTR_FILEDESCRIPTORA ); 
    static CLIPFORMAT fileDescriptorFlavorW = ::RegisterClipboardFormat( CFSTR_FILEDESCRIPTORW ); 
#ifndef WINCE
    static CLIPFORMAT fileFlavor = ::RegisterClipboardFormat( CFSTR_FILECONTENTS ); 
#endif
    switch (stm.tymed) {
     case TYMED_HGLOBAL: 
        {
          switch (fe.cfFormat) {
            case CF_TEXT:
              {
                
                
                
                
                
                
                PRUint32 allocLen = 0;
                if ( NS_SUCCEEDED(GetGlobalData(stm.hGlobal, aData, &allocLen)) ) {
                  *aLen = strlen ( NS_REINTERPRET_CAST(char*, *aData) );
                  result = NS_OK;
                }
              } break;

            case CF_UNICODETEXT:
              {
                
                
                
                
                
                
                PRUint32 allocLen = 0;
                if ( NS_SUCCEEDED(GetGlobalData(stm.hGlobal, aData, &allocLen)) ) {
                  *aLen = nsCRT::strlen(NS_REINTERPRET_CAST(PRUnichar*, *aData)) * 2;
                  result = NS_OK;
                }
              } break;

#ifndef WINCE
            case CF_DIB :
              {
                PRUint32 allocLen = 0;
                unsigned char * clipboardData;
                nsresult rv = GetGlobalData(stm.hGlobal, (void **) &clipboardData, &allocLen);
                if (NS_SUCCEEDED(rv))
                {
                  nsImageFromClipboard converter;
                  nsIInputStream * inputStream;
                  converter.GetEncodedImageStream (clipboardData,  &inputStream );   
                  if ( inputStream ) {
                    *aData = inputStream;
                    *aLen = sizeof(nsIInputStream*);
                    result = NS_OK;
                  }
                }
              } break;

            case CF_HDROP : 
              {
                
                
                
                
                HDROP dropFiles = (HDROP) GlobalLock(stm.hGlobal);

                UINT numFiles = ::DragQueryFileW(dropFiles, 0xFFFFFFFF, NULL, 0);
                NS_ASSERTION ( numFiles > 0, "File drop flavor, but no files...hmmmm" );
                NS_ASSERTION ( aIndex < numFiles, "Asked for a file index out of range of list" );
                if (numFiles > 0) {
                  UINT fileNameLen = ::DragQueryFileW(dropFiles, aIndex, nsnull, 0);
                  PRUnichar* buffer = NS_REINTERPRET_CAST(PRUnichar*, nsMemory::Alloc((fileNameLen + 1) * sizeof(PRUnichar)));
                  if ( buffer ) {
                    ::DragQueryFileW(dropFiles, aIndex, buffer, fileNameLen + 1);
                    *aData = buffer;
                    *aLen = fileNameLen * sizeof(PRUnichar);
                    result = NS_OK;
                  }
                  else
                    result = NS_ERROR_OUT_OF_MEMORY;
                }
                GlobalUnlock (stm.hGlobal) ;

              } break;

#endif
            default: {
#ifndef WINCE
              if ( fe.cfFormat == fileDescriptorFlavorA || fe.cfFormat == fileDescriptorFlavorW || fe.cfFormat == fileFlavor ) {
                NS_WARNING ( "Mozilla doesn't yet understand how to read this type of file flavor" );
              } 
              else
#endif
              {
                
                
                
                
                
                
                
                
                
                
                PRUint32 allocLen = 0;
                if ( NS_SUCCEEDED(GetGlobalData(stm.hGlobal, aData, &allocLen)) ) {
                  if ( fe.cfFormat == CF_HTML ) {
                    
                    
                    
                    
                    *aLen = allocLen;
                  }
                  else
                    *aLen = nsCRT::strlen(NS_REINTERPRET_CAST(PRUnichar*, *aData)) * sizeof(PRUnichar);
                  result = NS_OK;
                }
              }
            } break;
          } 
        } break;

      case TYMED_GDI: 
        {
#ifdef DEBUG
          printf("*********************** TYMED_GDI\n");
#endif
        } break;

      default:
        break;
    } 
    
    ReleaseStgMedium(&stm);
  }

  return result;
}



nsresult nsClipboard::GetDataFromDataObject(IDataObject     * aDataObject,
                                            UINT              anIndex,
                                            nsIWidget       * aWindow,
                                            nsITransferable * aTransferable)
{
  
  if ( !aTransferable )
    return NS_ERROR_INVALID_ARG;

  nsresult res = NS_ERROR_FAILURE;

  
  
  nsCOMPtr<nsISupportsArray> flavorList;
  res = aTransferable->FlavorsTransferableCanImport ( getter_AddRefs(flavorList) );
  if ( NS_FAILED(res) )
    return NS_ERROR_FAILURE;

  
  PRUint32 i;
  PRUint32 cnt;
  flavorList->Count(&cnt);
  for (i=0;i<cnt;i++) {
    nsCOMPtr<nsISupports> genericFlavor;
    flavorList->GetElementAt ( i, getter_AddRefs(genericFlavor) );
    nsCOMPtr<nsISupportsCString> currentFlavor ( do_QueryInterface(genericFlavor) );
    if ( currentFlavor ) {
      nsXPIDLCString flavorStr;
      currentFlavor->ToString(getter_Copies(flavorStr));
      UINT format = GetFormat(flavorStr);

      
      
      void* data = nsnull;
      PRUint32 dataLen = 0;
      PRBool dataFound = PR_FALSE;
      if (nsnull != aDataObject) {
        if ( NS_SUCCEEDED(GetNativeDataOffClipboard(aDataObject, anIndex, format, &data, &dataLen)) )
          dataFound = PR_TRUE;
      } 
      else if (nsnull != aWindow) {
        if ( NS_SUCCEEDED(GetNativeDataOffClipboard(aWindow, anIndex, format, &data, &dataLen)) )
          dataFound = PR_TRUE;
      }

      
      
      
      if ( !dataFound ) {
        if ( strcmp(flavorStr, kUnicodeMime) == 0 )
          dataFound = FindUnicodeFromPlainText ( aDataObject, anIndex, &data, &dataLen );
        else if ( strcmp(flavorStr, kURLMime) == 0 )
          dataFound = FindURLFromLocalFile ( aDataObject, anIndex, &data, &dataLen );
      } 

      
      if ( dataFound ) {
        nsCOMPtr<nsISupports> genericDataWrapper;
          if ( strcmp(flavorStr, kFileMime) == 0 ) {
            
            nsDependentString filepath(NS_REINTERPRET_CAST(PRUnichar*, data));
            nsCOMPtr<nsILocalFile> file;
            if ( NS_SUCCEEDED(NS_NewLocalFile(filepath, PR_FALSE, getter_AddRefs(file))) )
              genericDataWrapper = do_QueryInterface(file);
            nsMemory::Free(data);
          }
        else if ( strcmp(flavorStr, kNativeHTMLMime) == 0) {
          
          
          
          if ( FindPlatformHTML(aDataObject, anIndex, &data, &dataLen) )
            nsPrimitiveHelpers::CreatePrimitiveForData ( flavorStr, data, dataLen, getter_AddRefs(genericDataWrapper) );
          else
          {
            nsMemory::Free(data);
            continue;     
          }
          nsMemory::Free(data);
        }
        else if ( strcmp(flavorStr, kJPEGImageMime) == 0) {
          nsIInputStream * imageStream = NS_REINTERPRET_CAST(nsIInputStream*, data);
          genericDataWrapper = do_QueryInterface(imageStream);
          NS_IF_RELEASE(imageStream);
        }
        else {
          
          
          PRInt32 signedLen = NS_STATIC_CAST(PRInt32, dataLen);
          nsLinebreakHelpers::ConvertPlatformToDOMLinebreaks ( flavorStr, &data, &signedLen );
          dataLen = signedLen;

          nsPrimitiveHelpers::CreatePrimitiveForData ( flavorStr, data, dataLen, getter_AddRefs(genericDataWrapper) );
          nsMemory::Free(data);
        }
        
        NS_ASSERTION ( genericDataWrapper, "About to put null data into the transferable" );
        aTransferable->SetTransferData(flavorStr, genericDataWrapper, dataLen);
        res = NS_OK;

        
        break;
      }

    }
  } 

  return res;

}








PRBool
nsClipboard :: FindPlatformHTML ( IDataObject* inDataObject, UINT inIndex, void** outData, PRUint32* outDataLen )
{
  PRBool dataFound = PR_FALSE;

  if ( outData && *outData ) {
    
    
    
    
    float vers = 0.0;
    PRUint32 startOfData = 0;
    sscanf((char*)*outData, "Version:%f\nStartHTML:%d\nEndHTML:%d", &vers, &startOfData, outDataLen);
    NS_ASSERTION(startOfData && *outDataLen, "Couldn't parse CF_HTML description header");
 
    if ( *outDataLen )
      dataFound = PR_TRUE;
  }

  return dataFound;
}








PRBool
nsClipboard :: FindUnicodeFromPlainText ( IDataObject* inDataObject, UINT inIndex, void** outData, PRUint32* outDataLen )
{
  PRBool dataFound = PR_FALSE;

  
  
  nsresult loadResult = GetNativeDataOffClipboard(inDataObject, inIndex, GetFormat(kTextMime), outData, outDataLen);
  if ( NS_SUCCEEDED(loadResult) && *outData ) {
    const char* castedText = NS_REINTERPRET_CAST(char*, *outData);          
    PRUnichar* convertedText = nsnull;
    PRInt32 convertedTextLen = 0;
    nsPrimitiveHelpers::ConvertPlatformPlainTextToUnicode ( castedText, *outDataLen, 
                                                              &convertedText, &convertedTextLen );
    if ( convertedText ) {
      
      nsMemory::Free(*outData);
      *outData = convertedText;
      *outDataLen = convertedTextLen * sizeof(PRUnichar);
      dataFound = PR_TRUE;
    }
  } 

  return dataFound;

} 










PRBool
nsClipboard :: FindURLFromLocalFile ( IDataObject* inDataObject, UINT inIndex, void** outData, PRUint32* outDataLen )
{
  PRBool dataFound = PR_FALSE;

  nsresult loadResult = GetNativeDataOffClipboard(inDataObject, inIndex, GetFormat(kFileMime), outData, outDataLen);
  if ( NS_SUCCEEDED(loadResult) && *outData ) {
    
    const nsDependentString filepath(NS_STATIC_CAST(PRUnichar*, *outData));
    nsCOMPtr<nsILocalFile> file;
    nsresult rv = NS_NewLocalFile(filepath, PR_TRUE, getter_AddRefs(file));
    if (NS_FAILED(rv))
      return dataFound;

    if ( IsInternetShortcut(filepath) ) {
      nsCAutoString url;
      ResolveShortcut( file, url );
      if ( !url.IsEmpty() ) {
        
        nsMemory::Free(*outData);
        *outData = UTF8ToNewUnicode(url);
        *outDataLen = nsCRT::strlen(NS_STATIC_CAST(PRUnichar*, *outData)) * sizeof(PRUnichar);

        dataFound = PR_TRUE;
      }
    }
    else {
      
      nsCAutoString urlSpec;
      NS_GetURLSpecFromFile(file, urlSpec);

      
      nsMemory::Free(*outData);
      *outData = UTF8ToNewUnicode(urlSpec);
      *outDataLen = nsCRT::strlen(NS_STATIC_CAST(PRUnichar*, *outData)) * sizeof(PRUnichar);
      dataFound = PR_TRUE;
    } 
  }

  return dataFound;
} 





void
nsClipboard :: ResolveShortcut ( nsILocalFile* aFile, nsACString& outURL )
{
  nsCOMPtr<nsIFileProtocolHandler> fph;
  nsresult rv = NS_GetFileProtocolHandler(getter_AddRefs(fph));
  if (NS_FAILED(rv))
    return;

  nsCOMPtr<nsIURI> uri;
  rv = fph->ReadURLFile(aFile, getter_AddRefs(uri));
  if (NS_FAILED(rv))
    return;

  uri->GetSpec(outURL);
} 







PRBool
nsClipboard :: IsInternetShortcut ( const nsAString& inFileName ) 
{
  return StringEndsWith(inFileName, NS_LITERAL_STRING(".url"), nsCaseInsensitiveStringComparator());
} 



NS_IMETHODIMP 
nsClipboard::GetNativeClipboardData ( nsITransferable * aTransferable, PRInt32 aWhichClipboard )
{
  
  if ( !aTransferable || aWhichClipboard != kGlobalClipboard )
    return NS_ERROR_FAILURE;

  nsresult res;

  
  IDataObject * dataObj;
  if (S_OK == ::OleGetClipboard(&dataObj)) {
    
    res = GetDataFromDataObject(dataObj, 0, nsnull, aTransferable);
    dataObj->Release();
  } 
  else {
    
    res = GetDataFromDataObject(nsnull, 0, mWindow, aTransferable);
  }
  return res;

}



NS_IMETHODIMP
nsClipboard::Observe(nsISupports *aSubject, const char *aTopic,
                     const PRUnichar *aData)
{
  
  ::OleFlushClipboard();
  ::CloseClipboard();

  return NS_OK;
}


NS_IMETHODIMP nsClipboard::HasDataMatchingFlavors(nsISupportsArray *aFlavorList, PRInt32 aWhichClipboard,
                                                  PRBool           *_retval)
{
  *_retval = PR_FALSE;
  if ( aWhichClipboard != kGlobalClipboard )
    return NS_OK;

  PRUint32 cnt;
  aFlavorList->Count(&cnt);
  for ( PRUint32 i = 0;i < cnt; ++i ) {
    nsCOMPtr<nsISupports> genericFlavor;
    aFlavorList->GetElementAt (i, getter_AddRefs(genericFlavor));
    nsCOMPtr<nsISupportsCString> currentFlavor (do_QueryInterface(genericFlavor));
    if (currentFlavor) {
      nsXPIDLCString flavorStr;
      currentFlavor->ToString(getter_Copies(flavorStr));

#ifdef NS_DEBUG
      if ( strcmp(flavorStr, kTextMime) == 0 )
        NS_WARNING ( "DO NOT USE THE text/plain DATA FLAVOR ANY MORE. USE text/unicode INSTEAD" );
#endif

      UINT format = GetFormat(flavorStr);
      if (IsClipboardFormatAvailable(format)) {
        *_retval = PR_TRUE;
        break;
      }
      else {
        
        
        if ( strcmp(flavorStr, kUnicodeMime) == 0 ) {
          
          
          if (IsClipboardFormatAvailable(GetFormat(kTextMime)) )
            *_retval = PR_TRUE;
        }
      }
    }
  }

  return NS_OK;
}
