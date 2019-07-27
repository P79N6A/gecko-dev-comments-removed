




#include "nsClipboard.h"
#include <ole2.h>
#include <shlobj.h>
#include <intshcut.h>


#include <shellapi.h>

#include "nsCOMPtr.h"
#include "nsDataObj.h"
#include "nsIClipboardOwner.h"
#include "nsString.h"
#include "nsNativeCharsetUtils.h"
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
#include "nsEscape.h"
#include "nsIObserverService.h"

#ifdef PR_LOGGING
PRLogModuleInfo* gWin32ClipboardLog = nullptr;
#endif


UINT nsClipboard::CF_HTML = ::RegisterClipboardFormatW(L"HTML Format");







nsClipboard::nsClipboard() : nsBaseClipboard()
{
#ifdef PR_LOGGING
  if (!gWin32ClipboardLog) {
    gWin32ClipboardLog = PR_NewLogModule("nsClipboard");
  }
#endif

  mIgnoreEmptyNotification = false;
  mWindow         = nullptr;

  
 
  nsCOMPtr<nsIObserverService> observerService =
    do_GetService("@mozilla.org/observer-service;1");
  if (observerService)
    observerService->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, PR_FALSE);
}




nsClipboard::~nsClipboard()
{

}

NS_IMPL_ISUPPORTS_INHERITED(nsClipboard, nsBaseClipboard, nsIObserver)

NS_IMETHODIMP
nsClipboard::Observe(nsISupports *aSubject, const char *aTopic,
                     const char16_t *aData)
{
  
  ::OleFlushClipboard();
  ::CloseClipboard();

  return NS_OK;
}


UINT nsClipboard::GetFormat(const char* aMimeStr)
{
  UINT format;

  if (strcmp(aMimeStr, kTextMime) == 0)
    format = CF_TEXT;
  else if (strcmp(aMimeStr, kUnicodeMime) == 0)
    format = CF_UNICODETEXT;
  else if (strcmp(aMimeStr, kJPEGImageMime) == 0 ||
           strcmp(aMimeStr, kJPGImageMime) == 0 ||
           strcmp(aMimeStr, kPNGImageMime) == 0)
    format = CF_DIBV5;
  else if (strcmp(aMimeStr, kFileMime) == 0 ||
           strcmp(aMimeStr, kFilePromiseMime) == 0)
    format = CF_HDROP;
  else if (strcmp(aMimeStr, kNativeHTMLMime) == 0)
    format = CF_HTML;
  else
    format = ::RegisterClipboardFormatW(NS_ConvertASCIItoUTF16(aMimeStr).get());

  return format;
}


nsresult nsClipboard::CreateNativeDataObject(nsITransferable * aTransferable, IDataObject ** aDataObj, nsIURI * uri)
{
  if (nullptr == aTransferable) {
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
  if (nullptr == aTransferable || nullptr == aDataObj) {
    return NS_ERROR_FAILURE;
  }

  nsDataObj * dObj = static_cast<nsDataObj *>(aDataObj);

  
  
  dObj->SetTransferable(aTransferable);

  
  nsCOMPtr<nsISupportsArray> dfList;
  aTransferable->FlavorsTransferableCanExport(getter_AddRefs(dfList));

  
  
  uint32_t i;
  uint32_t cnt;
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
        SET_FORMATETC(shortcutFE, ::RegisterClipboardFormat(CFSTR_FILECONTENTS), 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL)
        dObj->AddDataFlavor(kURLMime, &shortcutFE);  
        SET_FORMATETC(shortcutFE, ::RegisterClipboardFormat(CFSTR_INETURLA), 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL)
        dObj->AddDataFlavor(kURLMime, &shortcutFE);      
        SET_FORMATETC(shortcutFE, ::RegisterClipboardFormat(CFSTR_INETURLW), 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL)
        dObj->AddDataFlavor(kURLMime, &shortcutFE);      
      }
      else if ( strcmp(flavorStr, kPNGImageMime) == 0 || strcmp(flavorStr, kJPEGImageMime) == 0 ||
                strcmp(flavorStr, kJPGImageMime) == 0 || strcmp(flavorStr, kGIFImageMime) == 0 ||
                strcmp(flavorStr, kNativeImageMime) == 0  ) {
        
        FORMATETC imageFE;
        
        SET_FORMATETC(imageFE, CF_DIBV5, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL)
        dObj->AddDataFlavor(flavorStr, &imageFE);
        
        SET_FORMATETC(imageFE, CF_DIB, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL)
        dObj->AddDataFlavor(flavorStr, &imageFE);
      }
      else if ( strcmp(flavorStr, kFilePromiseMime) == 0 ) {
         
         
         
         
         
         
         
         
         
         
         
         
         
        FORMATETC shortcutFE;
        SET_FORMATETC(shortcutFE, ::RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT), 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL)
        dObj->AddDataFlavor(kFilePromiseMime, &shortcutFE);
      }
    }
  }

  return NS_OK;
}


NS_IMETHODIMP nsClipboard::SetNativeClipboardData ( int32_t aWhichClipboard )
{
  if ( aWhichClipboard != kGlobalClipboard )
    return NS_ERROR_FAILURE;

  mIgnoreEmptyNotification = true;

  
  if (nullptr == mTransferable) {
    return NS_ERROR_FAILURE;
  }

  IDataObject * dataObj;
  if ( NS_SUCCEEDED(CreateNativeDataObject(mTransferable, &dataObj, nullptr)) ) { 
    ::OleSetClipboard(dataObj);
    dataObj->Release();
  } else {
    
    ::OleSetClipboard(nullptr);
  }

  mIgnoreEmptyNotification = false;

  return NS_OK;
}



nsresult nsClipboard::GetGlobalData(HGLOBAL aHGBL, void ** aData, uint32_t * aLen)
{
  
  
  
  
  nsresult  result = NS_ERROR_FAILURE;
  if (aHGBL != nullptr) {
    LPSTR lpStr = (LPSTR) GlobalLock(aHGBL);
    DWORD allocSize = GlobalSize(aHGBL);
    char* data = static_cast<char*>(moz_xmalloc(allocSize + sizeof(char16_t)));
    if ( data ) {    
      memcpy ( data, lpStr, allocSize );
      data[allocSize] = data[allocSize + 1] = '\0';     

      GlobalUnlock(aHGBL);
      *aData = data;
      *aLen = allocSize;

      result = NS_OK;
    }
  } else {
    
    
    *aData = nullptr;
    *aLen  = 0;
    LPVOID lpMsgBuf;

    FormatMessageW( 
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        nullptr,
        GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
        (LPWSTR) &lpMsgBuf,
        0,
        nullptr 
    );

    
    MessageBoxW( nullptr, (LPCWSTR) lpMsgBuf, L"GetLastError",
                 MB_OK | MB_ICONINFORMATION );

    
    LocalFree( lpMsgBuf );    
  }

  return result;
}


nsresult nsClipboard::GetNativeDataOffClipboard(nsIWidget * aWidget, UINT , UINT aFormat, void ** aData, uint32_t * aLen)
{
  HGLOBAL   hglb; 
  nsresult  result = NS_ERROR_FAILURE;

  HWND nativeWin = nullptr;
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
    PR_LOG(gWin32ClipboardLog, PR_LOG_ALWAYS, ("E_INVALIDARG\n"));
  } else
  if (hres == E_UNEXPECTED) {
    PR_LOG(gWin32ClipboardLog, PR_LOG_ALWAYS, ("E_UNEXPECTED\n"));
  } else
  if (hres == E_OUTOFMEMORY) {
    PR_LOG(gWin32ClipboardLog, PR_LOG_ALWAYS, ("E_OUTOFMEMORY\n"));
  } else
  if (hres == DV_E_LINDEX ) {
    PR_LOG(gWin32ClipboardLog, PR_LOG_ALWAYS, ("DV_E_LINDEX\n"));
  } else
  if (hres == DV_E_FORMATETC) {
    PR_LOG(gWin32ClipboardLog, PR_LOG_ALWAYS, ("DV_E_FORMATETC\n"));
  }  else
  if (hres == DV_E_TYMED) {
    PR_LOG(gWin32ClipboardLog, PR_LOG_ALWAYS, ("DV_E_TYMED\n"));
  }  else
  if (hres == DV_E_DVASPECT) {
    PR_LOG(gWin32ClipboardLog, PR_LOG_ALWAYS, ("DV_E_DVASPECT\n"));
  }  else
  if (hres == OLE_E_NOTRUNNING) {
    PR_LOG(gWin32ClipboardLog, PR_LOG_ALWAYS, ("OLE_E_NOTRUNNING\n"));
  }  else
  if (hres == STG_E_MEDIUMFULL) {
    PR_LOG(gWin32ClipboardLog, PR_LOG_ALWAYS, ("STG_E_MEDIUMFULL\n"));
  }  else
  if (hres == DV_E_CLIPFORMAT) {
    PR_LOG(gWin32ClipboardLog, PR_LOG_ALWAYS, ("DV_E_CLIPFORMAT\n"));
  }  else
  if (hres == S_OK) {
    PR_LOG(gWin32ClipboardLog, PR_LOG_ALWAYS, ("S_OK\n"));
  } else {
    PR_LOG(gWin32ClipboardLog, PR_LOG_ALWAYS, 
           ("****** DisplayErrCode 0x%X\n", hres));
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






nsresult nsClipboard::GetNativeDataOffClipboard(IDataObject * aDataObject, UINT aIndex, UINT aFormat, const char * aMIMEImageFormat, void ** aData, uint32_t * aLen)
{
  nsresult result = NS_ERROR_FAILURE;
  *aData = nullptr;
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
    static CLIPFORMAT fileFlavor = ::RegisterClipboardFormat( CFSTR_FILECONTENTS ); 
    static CLIPFORMAT preferredDropEffect = ::RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT);

    switch (stm.tymed) {
     case TYMED_HGLOBAL: 
        {
          switch (fe.cfFormat) {
            case CF_TEXT:
              {
                
                
                
                
                
                
                uint32_t allocLen = 0;
                if ( NS_SUCCEEDED(GetGlobalData(stm.hGlobal, aData, &allocLen)) ) {
                  *aLen = strlen ( reinterpret_cast<char*>(*aData) );
                  result = NS_OK;
                }
              } break;

            case CF_UNICODETEXT:
              {
                
                
                
                
                
                
                uint32_t allocLen = 0;
                if ( NS_SUCCEEDED(GetGlobalData(stm.hGlobal, aData, &allocLen)) ) {
                  *aLen = NS_strlen(reinterpret_cast<char16_t*>(*aData)) * 2;
                  result = NS_OK;
                }
              } break;

            case CF_DIBV5:
              if (aMIMEImageFormat)
              {
                uint32_t allocLen = 0;
                unsigned char * clipboardData;
                if (NS_SUCCEEDED(GetGlobalData(stm.hGlobal, (void **)&clipboardData, &allocLen)))
                {
                  nsImageFromClipboard converter;
                  nsIInputStream * inputStream;
                  converter.GetEncodedImageStream(clipboardData, aMIMEImageFormat, &inputStream);   
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

                UINT numFiles = ::DragQueryFileW(dropFiles, 0xFFFFFFFF, nullptr, 0);
                NS_ASSERTION ( numFiles > 0, "File drop flavor, but no files...hmmmm" );
                NS_ASSERTION ( aIndex < numFiles, "Asked for a file index out of range of list" );
                if (numFiles > 0) {
                  UINT fileNameLen = ::DragQueryFileW(dropFiles, aIndex, nullptr, 0);
                  wchar_t* buffer = reinterpret_cast<wchar_t*>(moz_xmalloc((fileNameLen + 1) * sizeof(wchar_t)));
                  if ( buffer ) {
                    ::DragQueryFileW(dropFiles, aIndex, buffer, fileNameLen + 1);
                    *aData = buffer;
                    *aLen = fileNameLen * sizeof(char16_t);
                    result = NS_OK;
                  }
                  else
                    result = NS_ERROR_OUT_OF_MEMORY;
                }
                GlobalUnlock (stm.hGlobal) ;

              } break;

            default: {
              if ( fe.cfFormat == fileDescriptorFlavorA || fe.cfFormat == fileDescriptorFlavorW || fe.cfFormat == fileFlavor ) {
                NS_WARNING ( "Mozilla doesn't yet understand how to read this type of file flavor" );
              } 
              else
              {
                
                
                
                
                
                
                
                
                
                
                uint32_t allocLen = 0;
                if ( NS_SUCCEEDED(GetGlobalData(stm.hGlobal, aData, &allocLen)) ) {
                  if ( fe.cfFormat == CF_HTML ) {
                    
                    
                    
                    
                    *aLen = allocLen;
                  } else if (fe.cfFormat == preferredDropEffect) {
                    
                    
                    
                    NS_ASSERTION(allocLen == sizeof(DWORD),
                      "CFSTR_PREFERREDDROPEFFECT should return a DWORD");
                    *aLen = allocLen;
                  } else {
                    *aLen = NS_strlen(reinterpret_cast<char16_t*>(*aData)) * 
                            sizeof(char16_t);
                  }
                  result = NS_OK;
                }
              }
            } break;
          } 
        } break;

      case TYMED_GDI: 
        {
#ifdef DEBUG
          PR_LOG(gWin32ClipboardLog, PR_LOG_ALWAYS, 
                 ("*********************** TYMED_GDI\n"));
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

  
  uint32_t i;
  uint32_t cnt;
  flavorList->Count(&cnt);
  for (i=0;i<cnt;i++) {
    nsCOMPtr<nsISupports> genericFlavor;
    flavorList->GetElementAt ( i, getter_AddRefs(genericFlavor) );
    nsCOMPtr<nsISupportsCString> currentFlavor ( do_QueryInterface(genericFlavor) );
    if ( currentFlavor ) {
      nsXPIDLCString flavorStr;
      currentFlavor->ToString(getter_Copies(flavorStr));
      UINT format = GetFormat(flavorStr);

      
      
      void* data = nullptr;
      uint32_t dataLen = 0;
      bool dataFound = false;
      if (nullptr != aDataObject) {
        if ( NS_SUCCEEDED(GetNativeDataOffClipboard(aDataObject, anIndex, format, flavorStr, &data, &dataLen)) )
          dataFound = true;
      } 
      else if (nullptr != aWindow) {
        if ( NS_SUCCEEDED(GetNativeDataOffClipboard(aWindow, anIndex, format, &data, &dataLen)) )
          dataFound = true;
      }

      
      
      
      if ( !dataFound ) {
        if ( strcmp(flavorStr, kUnicodeMime) == 0 )
          dataFound = FindUnicodeFromPlainText ( aDataObject, anIndex, &data, &dataLen );
        else if ( strcmp(flavorStr, kURLMime) == 0 ) {
          
          
          dataFound = FindURLFromNativeURL ( aDataObject, anIndex, &data, &dataLen );
          if ( !dataFound )
            dataFound = FindURLFromLocalFile ( aDataObject, anIndex, &data, &dataLen );
        }
      } 

      
      if ( dataFound ) {
        nsCOMPtr<nsISupports> genericDataWrapper;
          if ( strcmp(flavorStr, kFileMime) == 0 ) {
            
            nsDependentString filepath(reinterpret_cast<char16_t*>(data));
            nsCOMPtr<nsIFile> file;
            if ( NS_SUCCEEDED(NS_NewLocalFile(filepath, false, getter_AddRefs(file))) )
              genericDataWrapper = do_QueryInterface(file);
            free(data);
          }
        else if ( strcmp(flavorStr, kNativeHTMLMime) == 0) {
          
          
          
          if ( FindPlatformHTML(aDataObject, anIndex, &data, &dataLen) )
            nsPrimitiveHelpers::CreatePrimitiveForData ( flavorStr, data, dataLen, getter_AddRefs(genericDataWrapper) );
          else
          {
            free(data);
            continue;     
          }
          free(data);
        }
        else if ( strcmp(flavorStr, kJPEGImageMime) == 0 ||
                  strcmp(flavorStr, kJPGImageMime) == 0 ||
                  strcmp(flavorStr, kPNGImageMime) == 0) {
          nsIInputStream * imageStream = reinterpret_cast<nsIInputStream*>(data);
          genericDataWrapper = do_QueryInterface(imageStream);
          NS_IF_RELEASE(imageStream);
        }
        else {
          
          
          int32_t signedLen = static_cast<int32_t>(dataLen);
          nsLinebreakHelpers::ConvertPlatformToDOMLinebreaks ( flavorStr, &data, &signedLen );
          dataLen = signedLen;

          nsPrimitiveHelpers::CreatePrimitiveForData ( flavorStr, data, dataLen, getter_AddRefs(genericDataWrapper) );
          free(data);
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








bool
nsClipboard :: FindPlatformHTML ( IDataObject* inDataObject, UINT inIndex, void** outData, uint32_t* outDataLen )
{
  
  
  
  
  
  
  

  if (!outData || !*outData) {
    return false;
  }

  char version[8] = { 0 };
  int32_t startOfData = 0;
  int32_t endOfData = 0;
  int numFound = sscanf((char*)*outData, "Version:%7s\nStartHTML:%d\nEndHTML:%d", 
                        version, &startOfData, &endOfData);

  if (numFound != 3 || startOfData < -1 || endOfData < -1) {
    return false;
  }

  
  if (startOfData == -1) {
    startOfData = 0;
  }
  if (endOfData == -1) {
    endOfData = *outDataLen;
  }

  
  
  
  if (!endOfData || startOfData >= endOfData || 
      static_cast<uint32_t>(endOfData) > *outDataLen) {
    return false;
  }
  
  
  
  
  *outDataLen = endOfData;
  return true;
}








bool
nsClipboard :: FindUnicodeFromPlainText ( IDataObject* inDataObject, UINT inIndex, void** outData, uint32_t* outDataLen )
{
  bool dataFound = false;

  
  
  nsresult loadResult = GetNativeDataOffClipboard(inDataObject, inIndex, GetFormat(kTextMime), nullptr, outData, outDataLen);
  if ( NS_SUCCEEDED(loadResult) && *outData ) {
    const char* castedText = reinterpret_cast<char*>(*outData);          
    char16_t* convertedText = nullptr;
    int32_t convertedTextLen = 0;
    nsPrimitiveHelpers::ConvertPlatformPlainTextToUnicode ( castedText, *outDataLen, 
                                                              &convertedText, &convertedTextLen );
    if ( convertedText ) {
      
      free(*outData);
      *outData = convertedText;
      *outDataLen = convertedTextLen * sizeof(char16_t);
      dataFound = true;
    }
  } 

  return dataFound;

} 










bool
nsClipboard :: FindURLFromLocalFile ( IDataObject* inDataObject, UINT inIndex, void** outData, uint32_t* outDataLen )
{
  bool dataFound = false;

  nsresult loadResult = GetNativeDataOffClipboard(inDataObject, inIndex, GetFormat(kFileMime), nullptr, outData, outDataLen);
  if ( NS_SUCCEEDED(loadResult) && *outData ) {
    
    const nsDependentString filepath(static_cast<char16_t*>(*outData));
    nsCOMPtr<nsIFile> file;
    nsresult rv = NS_NewLocalFile(filepath, true, getter_AddRefs(file));
    if (NS_FAILED(rv)) {
      free(*outData);
      return dataFound;
    }

    if ( IsInternetShortcut(filepath) ) {
      free(*outData);
      nsAutoCString url;
      ResolveShortcut( file, url );
      if ( !url.IsEmpty() ) {
        
        nsDependentString urlString(UTF8ToNewUnicode(url));
        
        
        nsAutoString title;
        file->GetLeafName(title);
        
        title.SetLength(title.Length() - 4);
        if (title.IsEmpty())
          title = urlString;
        *outData = ToNewUnicode(urlString + NS_LITERAL_STRING("\n") + title);
        *outDataLen = NS_strlen(static_cast<char16_t*>(*outData)) * sizeof(char16_t);

        dataFound = true;
      }
    }
    else {
      
      nsAutoCString urlSpec;
      NS_GetURLSpecFromFile(file, urlSpec);

      
      free(*outData);
      *outData = UTF8ToNewUnicode(urlSpec);
      *outDataLen = NS_strlen(static_cast<char16_t*>(*outData)) * sizeof(char16_t);
      dataFound = true;
    } 
  }

  return dataFound;
} 








bool
nsClipboard :: FindURLFromNativeURL ( IDataObject* inDataObject, UINT inIndex, void** outData, uint32_t* outDataLen )
{
  bool dataFound = false;

  void* tempOutData = nullptr;
  uint32_t tempDataLen = 0;

  nsresult loadResult = GetNativeDataOffClipboard(inDataObject, inIndex, ::RegisterClipboardFormat(CFSTR_INETURLW), nullptr, &tempOutData, &tempDataLen);
  if ( NS_SUCCEEDED(loadResult) && tempOutData ) {
    nsDependentString urlString(static_cast<char16_t*>(tempOutData));
    
    
    
    *outData = ToNewUnicode(urlString + NS_LITERAL_STRING("\n") + urlString);
    *outDataLen = NS_strlen(static_cast<char16_t*>(*outData)) * sizeof(char16_t);
    free(tempOutData);
    dataFound = true;
  }
  else {
    loadResult = GetNativeDataOffClipboard(inDataObject, inIndex, ::RegisterClipboardFormat(CFSTR_INETURLA), nullptr, &tempOutData, &tempDataLen);
    if ( NS_SUCCEEDED(loadResult) && tempOutData ) {
      
      
      nsCString urlUnescapedA;
      bool unescaped = NS_UnescapeURL(static_cast<char*>(tempOutData), tempDataLen, esc_OnlyNonASCII | esc_SkipControl, urlUnescapedA);

      nsString urlString;
      if (unescaped)
        NS_CopyNativeToUnicode(urlUnescapedA, urlString);
      else
        NS_CopyNativeToUnicode(nsDependentCString(static_cast<char*>(tempOutData), tempDataLen), urlString);

      
      
      
      *outData = ToNewUnicode(urlString + NS_LITERAL_STRING("\n") + urlString);
      *outDataLen = NS_strlen(static_cast<char16_t*>(*outData)) * sizeof(char16_t);
      free(tempOutData);
      dataFound = true;
    }
  }

  return dataFound;
} 




void
nsClipboard :: ResolveShortcut ( nsIFile* aFile, nsACString& outURL )
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







bool
nsClipboard :: IsInternetShortcut ( const nsAString& inFileName ) 
{
  return StringEndsWith(inFileName, NS_LITERAL_STRING(".url"), nsCaseInsensitiveStringComparator());
} 



NS_IMETHODIMP 
nsClipboard::GetNativeClipboardData ( nsITransferable * aTransferable, int32_t aWhichClipboard )
{
  
  if ( !aTransferable || aWhichClipboard != kGlobalClipboard )
    return NS_ERROR_FAILURE;

  nsresult res;

  
  IDataObject * dataObj;
  if (S_OK == ::OleGetClipboard(&dataObj)) {
    
    res = GetDataFromDataObject(dataObj, 0, nullptr, aTransferable);
    dataObj->Release();
  } 
  else {
    
    res = GetDataFromDataObject(nullptr, 0, mWindow, aTransferable);
  }
  return res;

}

NS_IMETHODIMP
nsClipboard::EmptyClipboard(int32_t aWhichClipboard)
{
  
  
  
  
  
  if (aWhichClipboard == kGlobalClipboard && !mEmptyingForSetData) {
    OleSetClipboard(nullptr);
  }
  return nsBaseClipboard::EmptyClipboard(aWhichClipboard);
}


NS_IMETHODIMP nsClipboard::HasDataMatchingFlavors(const char** aFlavorList,
                                                  uint32_t aLength,
                                                  int32_t aWhichClipboard,
                                                  bool *_retval)
{
  *_retval = false;
  if (aWhichClipboard != kGlobalClipboard || !aFlavorList)
    return NS_OK;

  for (uint32_t i = 0;i < aLength; ++i) {
#ifdef DEBUG
    if (strcmp(aFlavorList[i], kTextMime) == 0)
      NS_WARNING ( "DO NOT USE THE text/plain DATA FLAVOR ANY MORE. USE text/unicode INSTEAD" );
#endif

    UINT format = GetFormat(aFlavorList[i]);
    if (IsClipboardFormatAvailable(format)) {
      *_retval = true;
      break;
    }
    else {
      
      
      if (strcmp(aFlavorList[i], kUnicodeMime) == 0) {
        
        
        if (IsClipboardFormatAvailable(GetFormat(kTextMime)))
          *_retval = true;
      }
    }
  }

  return NS_OK;
}
