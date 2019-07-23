














































#include "nsCOMPtr.h"
#include "nsClipboard.h"

#include "nsIClipboardOwner.h"
#include "nsString.h"
#include "nsIFormatConverter.h"
#include "nsMimeMapper.h"

#include "nsIComponentManager.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsXPIDLString.h"
#include "nsPrimitiveHelpers.h"
#ifndef MOZ_CAIRO_GFX
#include "nsIImageMac.h"
#endif
#include "nsMemory.h"
#include "nsMacNativeUnicodeConverter.h"
#include "nsICharsetConverterManager.h"
#include "nsCRT.h"
#include "nsStylClipboardUtils.h"
#include "nsLinebreakConverter.h"
#include "nsAutoPtr.h"
#include "nsIServiceManager.h"
#include "nsIMacUtils.h"

#include <Scrap.h>
#include <Script.h>
#include <TextEdit.h>

static const PRUint32 kPrivateFlavorMask = 0xffff0000;
static const PRUint32 kPrivateFlavorTag = 'MZ..' & kPrivateFlavorMask;






nsClipboard::nsClipboard() : nsBaseClipboard()
{
}




nsClipboard::~nsClipboard()
{
}












NS_IMETHODIMP
nsClipboard :: SetNativeClipboardData ( PRInt32 aWhichClipboard )
{
  if ( aWhichClipboard != kGlobalClipboard )
    return NS_ERROR_FAILURE;

  nsresult errCode = NS_OK;
  
  mIgnoreEmptyNotification = PR_TRUE;

  
  if ( !mTransferable )
    return NS_ERROR_INVALID_ARG;
  
  nsMimeMapperMac theMapper;

  ::ClearCurrentScrap();

  
  
  nsCOMPtr<nsISupportsArray> flavorList;
  errCode = mTransferable->FlavorsTransferableCanExport ( getter_AddRefs(flavorList) );
  if ( NS_FAILED(errCode) )
    return NS_ERROR_FAILURE;

  
  
  
  
  PRUint32 cnt;
  flavorList->Count(&cnt);
  for ( PRUint32 i = 0; i < cnt; ++i ) {
    nsCOMPtr<nsISupports> genericFlavor;
    flavorList->GetElementAt ( i, getter_AddRefs(genericFlavor) );
    nsCOMPtr<nsISupportsCString> currentFlavor ( do_QueryInterface(genericFlavor) );
    if ( currentFlavor ) {
      nsXPIDLCString flavorStr;
      currentFlavor->ToString( getter_Copies(flavorStr) );
      
      
      ResType macOSFlavor = theMapper.MapMimeTypeToMacOSType(flavorStr);
    
      
      
      void* data = nsnull;
      PRUint32 dataSize = 0;
      if ( strcmp(flavorStr,kUnicodeMime) == 0 ) {
        
        nsCOMPtr<nsISupports> genericDataWrapper;
        errCode = mTransferable->GetTransferData ( flavorStr, getter_AddRefs(genericDataWrapper), &dataSize );
        nsPrimitiveHelpers::CreateDataFromPrimitive ( flavorStr, genericDataWrapper, &data, dataSize );

        

        PRUnichar* castedData = NS_REINTERPRET_CAST(PRUnichar*, data);
        PRUnichar* linebreakConvertedUnicode = castedData;
        nsLinebreakConverter::ConvertUnicharLineBreaksInSitu(&linebreakConvertedUnicode,
                                                             nsLinebreakConverter::eLinebreakUnix,
                                                             nsLinebreakConverter::eLinebreakMac,
                                                             dataSize / sizeof(PRUnichar), nsnull);
          
        errCode = PutOnClipboard ( macOSFlavor, data, dataSize );
        if ( NS_SUCCEEDED(errCode) ) {
          
          char* plainTextData = nsnull;
          PRInt32 plainTextLen = 0;
          errCode = nsPrimitiveHelpers::ConvertUnicodeToPlatformPlainText ( linebreakConvertedUnicode, dataSize / 2, &plainTextData, &plainTextLen );
          
          ScriptCodeRun *scriptCodeRuns = nsnull;
          PRInt32 scriptRunOutLen;
          
          
          
          if (errCode == NS_ERROR_UENC_NOMAPPING) {
            if (plainTextData) {
              nsMemory::Free(plainTextData);
              plainTextData = nsnull;
            }
            errCode = nsMacNativeUnicodeConverter::ConvertUnicodetoScript(linebreakConvertedUnicode, 
                                                                          dataSize / 2,
                                                                          &plainTextData, 
                                                                          &plainTextLen,
                                                                          &scriptCodeRuns,
                                                                          &scriptRunOutLen);
          }
          else if (NS_SUCCEEDED(errCode)) {
            
            scriptCodeRuns = NS_REINTERPRET_CAST(ScriptCodeRun*,
                                                 nsMemory::Alloc(sizeof(ScriptCodeRun)));
            if (scriptCodeRuns) {
              scriptCodeRuns[0].offset = 0;
              scriptCodeRuns[0].script = (ScriptCode) ::GetScriptManagerVariable(smSysScript);
              scriptRunOutLen = 1;
            }
          }
          
          if ( NS_SUCCEEDED(errCode) && plainTextData ) {
            errCode = PutOnClipboard ( 'TEXT', plainTextData, plainTextLen );
            nsMemory::Free ( plainTextData ); 
            
            
            if (NS_SUCCEEDED(errCode) && scriptCodeRuns) {
              char *stylData;
              PRInt32 stylLen;
              errCode = CreateStylFromScriptRuns(scriptCodeRuns,
                                                 scriptRunOutLen,
                                                 &stylData,
                                                 &stylLen);
              if (NS_SUCCEEDED(errCode)) {
                errCode = PutOnClipboard ('styl', stylData, stylLen);
                nsMemory::Free(stylData);
              }
            }
          }
          if (scriptCodeRuns)
            nsMemory::Free(scriptCodeRuns);
        }

        
        
        if (linebreakConvertedUnicode != castedData)
          nsMemory::Free(linebreakConvertedUnicode);
      } 
      else if ( strcmp(flavorStr, kPNGImageMime) == 0 || strcmp(flavorStr, kJPEGImageMime) == 0 ||
                strcmp(flavorStr, kGIFImageMime) == 0 || strcmp(flavorStr, kNativeImageMime) == 0 ) {
        
        
        
        nsCOMPtr<nsISupports> transferSupports;
        errCode = mTransferable->GetTransferData ( flavorStr, getter_AddRefs(transferSupports), &dataSize );
        nsCOMPtr<nsISupportsInterfacePointer> ptrPrimitive(do_QueryInterface(transferSupports));
#ifndef MOZ_CAIRO_GFX
        nsCOMPtr<nsIImageMac> image;
        if (ptrPrimitive) {
          nsCOMPtr<nsISupports> primitiveData;
          ptrPrimitive->GetData(getter_AddRefs(primitiveData));
          image = do_QueryInterface(primitiveData);
        }
        if ( image ) {
          PicHandle picture = nsnull;
          image->ConvertToPICT ( &picture );
          if ( picture ) {
            errCode = PutOnClipboard ( 'PICT', *picture, ::GetHandleSize((Handle)picture) );
            ::KillPicture ( picture );
          }
        }
        else
#endif
          NS_WARNING ( "Image isn't an nsIImageMac in transferable" );
      }
      else if (strcmp(flavorStr.get(), kURLDataMime) == 0 ||
               strcmp(flavorStr.get(), kURLDescriptionMime) == 0) {
        nsCOMPtr<nsISupports> genericDataWrapper;
        errCode = mTransferable->GetTransferData(
                                            flavorStr,
                                            getter_AddRefs(genericDataWrapper),
                                            &dataSize);
        if (NS_SUCCEEDED(errCode)) {
          nsPrimitiveHelpers::CreateDataFromPrimitive(flavorStr,
                                                      genericDataWrapper,
                                                      &data,
                                                      dataSize);

          
          
          PRUnichar* castedData = NS_REINTERPRET_CAST(PRUnichar*, data);
          PRUnichar* linebreakConvertedUnicode = castedData;
          nsLinebreakConverter::ConvertUnicharLineBreaksInSitu(
                                          &linebreakConvertedUnicode,
                                          nsLinebreakConverter::eLinebreakUnix,
                                          nsLinebreakConverter::eLinebreakMac,
                                          dataSize / sizeof(PRUnichar),
                                          nsnull);

          
          
          
          
          
          
          
          
          nsDependentString utf16(linebreakConvertedUnicode);
          NS_ConvertUTF16toUTF8 utf8(utf16);
          PutOnClipboard(macOSFlavor,
                         PromiseFlatCString(utf8).get(), utf8.Length());

          
          
          if (linebreakConvertedUnicode != castedData)
            nsMemory::Free(linebreakConvertedUnicode);
        }
      }
      else {
        
        
        nsCOMPtr<nsISupports> genericDataWrapper;
        errCode = mTransferable->GetTransferData ( flavorStr, getter_AddRefs(genericDataWrapper), &dataSize );
        nsPrimitiveHelpers::CreateDataFromPrimitive ( flavorStr, genericDataWrapper, &data, dataSize );
        errCode = PutOnClipboard ( macOSFlavor, data, dataSize );
      }
              
      nsMemory::Free ( data );
    }
  } 

  
  
  short mappingLen = 0;
  const char* mapping = theMapper.ExportMapping(&mappingLen);
  if ( mapping && mappingLen ) {
    errCode = PutOnClipboard ( nsMimeMapperMac::MappingFlavor(), mapping, mappingLen );
    nsMemory::Free ( NS_CONST_CAST(char*, mapping) );
  }
  
  return errCode;
  
} 








nsresult
nsClipboard :: PutOnClipboard ( ResType inFlavor, const void* inData, PRInt32 inLen )
{
  nsresult errCode = NS_OK;

  void* data = (void*) inData;
  if ((inFlavor & kPrivateFlavorMask) == kPrivateFlavorTag) {
    
    nsCOMPtr<nsIMacUtils> macUtils =
     do_GetService("@mozilla.org/xpcom/mac-utils;1");
    PRBool isTranslated;
    if (macUtils &&
        NS_SUCCEEDED(macUtils->GetIsTranslated(&isTranslated)) &&
        isTranslated) {
      data = nsMemory::Alloc(inLen);
      if (!data)
        return NS_ERROR_OUT_OF_MEMORY;

      swab(inData, data, inLen);
    }
  }
  
  ScrapRef scrap;
  ::GetCurrentScrap(&scrap);
  ::PutScrapFlavor( scrap, inFlavor, kScrapFlavorMaskNone, inLen, data );

  if (data != inData)
    nsMemory::Free(data);

  return errCode;
  
} 







NS_IMETHODIMP
nsClipboard :: GetNativeClipboardData ( nsITransferable * aTransferable, PRInt32 aWhichClipboard )
{
  if ( aWhichClipboard != kGlobalClipboard )
    return NS_ERROR_FAILURE;

  nsresult errCode = NS_OK;

  
  if ( !aTransferable )
    return NS_ERROR_INVALID_ARG;

  
  
  nsCOMPtr<nsISupportsArray> flavorList;
  errCode = aTransferable->FlavorsTransferableCanImport ( getter_AddRefs(flavorList) );
  if ( NS_FAILED(errCode) )
    return NS_ERROR_FAILURE;

  
  
  char* mimeMapperData = nsnull;
  errCode = GetDataOffClipboard ( nsMimeMapperMac::MappingFlavor(), (void**)&mimeMapperData, 0 );
  nsMimeMapperMac theMapper ( mimeMapperData );
  if (mimeMapperData)
    nsMemory::Free ( mimeMapperData );
 
  
  
  
  PRBool dataFound = PR_FALSE;
  PRUint32 cnt;
  flavorList->Count(&cnt);
  for ( PRUint32 i = 0; i < cnt; ++i ) {
    nsCOMPtr<nsISupports> genericFlavor;
    flavorList->GetElementAt ( i, getter_AddRefs(genericFlavor) );
    nsCOMPtr<nsISupportsCString> currentFlavor ( do_QueryInterface(genericFlavor) );
    if ( currentFlavor ) {
      nsXPIDLCString flavorStr;
      currentFlavor->ToString ( getter_Copies(flavorStr) );
      
      
      ResType macOSFlavor = theMapper.MapMimeTypeToMacOSType(flavorStr, PR_FALSE);
    
      void* clipboardData = nsnull;
      PRInt32 dataSize = 0L;
      nsresult loadResult = GetDataOffClipboard ( macOSFlavor, &clipboardData, &dataSize );
      if ( NS_SUCCEEDED(loadResult) && clipboardData )
        dataFound = PR_TRUE;
      else {
        
        
        if ( strcmp(flavorStr, kUnicodeMime) == 0 ) {
        
          
          
          loadResult = GetDataOffClipboard ( 'styl', &clipboardData, &dataSize );
          if (NS_SUCCEEDED(loadResult) && 
              clipboardData &&
              ((PRUint32)dataSize >= (sizeof(ScrpSTElement) + 2))) {
            StScrpRec *scrpRecP = (StScrpRec *) clipboardData;
            ScrpSTElement *styl = scrpRecP->scrpStyleTab;
            ScriptCode script = styl ? ::FontToScript(styl->scrpFont) : smCurrentScript;
            
            
            nsMemory::Free(clipboardData);
            loadResult = GetDataOffClipboard ( 'TEXT', &clipboardData, &dataSize );
            if ( NS_SUCCEEDED(loadResult) && clipboardData ) {
              PRUnichar* convertedText = nsnull;
              PRInt32 convertedTextLen = 0;
              errCode = nsMacNativeUnicodeConverter::ConvertScripttoUnicode(
                                                                    script, 
                                                                    (const char *) clipboardData,
                                                                    dataSize,
                                                                    &convertedText,
                                                                    &convertedTextLen);
              if (NS_SUCCEEDED(errCode) && convertedText) {
                nsMemory::Free(clipboardData);
                clipboardData = convertedText;
                dataSize = convertedTextLen * sizeof(PRUnichar);
                dataFound = PR_TRUE;
              }
            }
          }          
          
          if (!dataFound) {
            loadResult = GetDataOffClipboard ( 'TEXT', &clipboardData, &dataSize );
            if ( NS_SUCCEEDED(loadResult) && clipboardData ) {
              const char* castedText = NS_REINTERPRET_CAST(char*, clipboardData);          
              PRUnichar* convertedText = nsnull;
              PRInt32 convertedTextLen = 0;
              nsPrimitiveHelpers::ConvertPlatformPlainTextToUnicode ( castedText, dataSize, 
                                                                        &convertedText, &convertedTextLen );
              if ( convertedText ) {
                
                nsMemory::Free(clipboardData);
                clipboardData = convertedText;
                dataSize = convertedTextLen * 2;
                dataFound = PR_TRUE;
              }
            } 
          }
        } 
      } 
      
      if ( dataFound ) {
        if ( strcmp(flavorStr, kPNGImageMime) == 0 || strcmp(flavorStr, kJPEGImageMime) == 0 ||
               strcmp(flavorStr, kGIFImageMime) == 0 ) {
          
          
          
          #ifdef DEBUG
          printf ( "----------- IMAGE REQUESTED ----------" );
          #endif
               
        } 
        else {
          if (strcmp(flavorStr.get(), kURLDataMime) == 0 ||
              strcmp(flavorStr.get(), kURLDescriptionMime) == 0) {
            
            
            
            
            
            
            
            nsDependentCString utf8(NS_REINTERPRET_CAST(char*, clipboardData));
            NS_ConvertUTF8toUTF16 utf16(utf8);

            
            nsMemory::Free(clipboardData);
            clipboardData = ToNewUnicode(utf16);
            dataSize = utf16.Length() * 2;
          }

          
          
          nsLinebreakHelpers::ConvertPlatformToDOMLinebreaks ( flavorStr, &clipboardData, &dataSize );
          
          unsigned char *clipboardDataPtr = (unsigned char *) clipboardData;
          
          
          
          
          if ( (macOSFlavor == 'utxt') &&
               (dataSize > 2) &&
               ((clipboardDataPtr[0] == 0xFE && clipboardDataPtr[1] == 0xFF) ||
               (clipboardDataPtr[0] == 0xFF && clipboardDataPtr[1] == 0xFE)) ) {
            dataSize -= sizeof(PRUnichar);
            clipboardDataPtr += sizeof(PRUnichar);
          }

          
          nsCOMPtr<nsISupports> genericDataWrapper;
          nsPrimitiveHelpers::CreatePrimitiveForData ( flavorStr, clipboardDataPtr, dataSize, getter_AddRefs(genericDataWrapper) );        
          errCode = aTransferable->SetTransferData ( flavorStr, genericDataWrapper, dataSize );
        }
        
        nsMemory::Free ( clipboardData );
        
        
        break;        
      } 
    }
  } 
  
  return errCode;
}








nsresult
nsClipboard :: GetDataOffClipboard ( ResType inMacFlavor, void** outData, PRInt32* outDataSize )
{
  if ( !outData || !inMacFlavor )
    return NS_ERROR_FAILURE;

  
  *outData = nsnull;
  if ( outDataSize )
      *outDataSize = 0;


  ScrapRef scrap;
  long dataSize;
  OSStatus err;

  err = ::GetCurrentScrap(&scrap);
  if (err != noErr) return NS_ERROR_FAILURE;
  err = ::GetScrapFlavorSize(scrap, inMacFlavor, &dataSize);
  if (err != noErr) return NS_ERROR_FAILURE;
  if (dataSize > 0) {
    char* dataBuff = (char*) nsMemory::Alloc(dataSize);
    if ( !dataBuff )
      return NS_ERROR_OUT_OF_MEMORY;
      
    
    
    
    
    err = ::GetScrapFlavorData(scrap, inMacFlavor, &dataSize, dataBuff);
    NS_ASSERTION(err == noErr, "nsClipboard:: Error getting data off clipboard");
    if ( err ) {
      nsMemory::Free(dataBuff);
      return NS_ERROR_FAILURE;
    }

    if ((inMacFlavor & kPrivateFlavorMask) == kPrivateFlavorTag) {
      
      nsCOMPtr<nsIMacUtils> macUtils =
       do_GetService("@mozilla.org/xpcom/mac-utils;1");
      PRBool isTranslated;
      if (macUtils &&
          NS_SUCCEEDED(macUtils->GetIsTranslated(&isTranslated)) &&
          isTranslated) {
        char* swappedData = (char*) nsMemory::Alloc(dataSize);
        if (!swappedData) {
          nsMemory::Free(dataBuff);
          return NS_ERROR_OUT_OF_MEMORY;
        }

        swab(dataBuff, swappedData, dataSize);
        nsMemory::Free(dataBuff);
        dataBuff = swappedData;
      }
    }

    
    if ( outDataSize )
      *outDataSize = dataSize;
    *outData = dataBuff;
  }
  return NS_OK;
  
} 












NS_IMETHODIMP
nsClipboard :: HasDataMatchingFlavors ( nsISupportsArray* aFlavorList, PRInt32 aWhichClipboard, PRBool * outResult ) 
{
  nsresult rv = NS_OK;
  *outResult = PR_FALSE;  
  if ( aWhichClipboard != kGlobalClipboard )
    return NS_OK;
  
  
  
  char* mimeMapperData = nsnull;
  rv = GetDataOffClipboard ( nsMimeMapperMac::MappingFlavor(), (void**)&mimeMapperData, 0 );
  nsMimeMapperMac theMapper ( mimeMapperData );
  nsMemory::Free ( mimeMapperData );
  
  PRUint32 length;
  aFlavorList->Count(&length);
  for ( PRUint32 i = 0; i < length; ++i ) {
    nsCOMPtr<nsISupports> genericFlavor;
    aFlavorList->GetElementAt ( i, getter_AddRefs(genericFlavor) );
    nsCOMPtr<nsISupportsCString> flavorWrapper ( do_QueryInterface(genericFlavor) );
    if ( flavorWrapper ) {
      nsXPIDLCString flavor;
      flavorWrapper->ToString ( getter_Copies(flavor) );
      
#ifdef NS_DEBUG
      if ( strcmp(flavor, kTextMime) == 0 )
        NS_WARNING ( "DO NOT USE THE text/plain DATA FLAVOR ANY MORE. USE text/unicode INSTEAD" );
#endif

      
      
      ResType macFlavor = theMapper.MapMimeTypeToMacOSType ( flavor, PR_FALSE );
      if ( macFlavor ) {
        if ( CheckIfFlavorPresent(macFlavor) ) {
          *outResult = PR_TRUE;   
          break;
        }
        else {
          
          
          if ( strcmp(flavor, kUnicodeMime) == 0 ) {
            if ( CheckIfFlavorPresent('TEXT') ) {
              *outResult = PR_TRUE;
              break;
            }
          }
        }
      }
    }  
  } 
  
  return NS_OK;
}







PRBool
nsClipboard :: CheckIfFlavorPresent ( ResType inMacFlavor )
{
  PRBool retval = PR_FALSE;

  ScrapRef scrap = nsnull;
  OSStatus err = ::GetCurrentScrap(&scrap);
  if ( scrap ) {
  
    
    
    
    
    UInt32 flavorCount = 0;
    ::GetScrapFlavorCount ( scrap, &flavorCount );
    nsAutoArrayPtr<ScrapFlavorInfo> flavorList(new ScrapFlavorInfo[flavorCount]);
    if ( flavorList ) {
      err = ::GetScrapFlavorInfoList ( scrap, &flavorCount, flavorList );
      if ( !err && flavorList ) {
        for ( unsigned int i = 0; i < flavorCount; ++i ) {
          if ( flavorList[i].flavorType == inMacFlavor )
            retval = PR_TRUE;
        }
      }
    }

  }
  return retval;
} 
