





#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsCOMPtr.h"
#include "nsPrimitiveHelpers.h"
#include "nsXPIDLString.h"
#include "prmem.h"
#include "nsOS2Uni.h"
#include "nsClipboard.h"

#define INCL_DOSERRORS
#define INCL_WIN
#include <os2.h>

inline PRUint32 RegisterClipboardFormat(PCSZ pcszFormat)
{
  ATOM atom = WinFindAtom(WinQuerySystemAtomTable(), pcszFormat);
  if (!atom) {
    atom = WinAddAtom(WinQuerySystemAtomTable(), pcszFormat); 
  }
  return atom;
}

nsClipboard::nsClipboard() : nsBaseClipboard()
{
  RegisterClipboardFormat(kTextMime);
  RegisterClipboardFormat(kUnicodeMime);
  RegisterClipboardFormat(kHTMLMime);
  RegisterClipboardFormat(kAOLMailMime);
  RegisterClipboardFormat(kPNGImageMime);
  RegisterClipboardFormat(kJPEGImageMime);
  RegisterClipboardFormat(kJPGImageMime);
  RegisterClipboardFormat(kGIFImageMime);
  RegisterClipboardFormat(kFileMime);
  RegisterClipboardFormat(kURLMime);
  RegisterClipboardFormat(kNativeImageMime);
  RegisterClipboardFormat(kNativeHTMLMime);
}

nsClipboard::~nsClipboard()
{}

nsresult nsClipboard::SetNativeClipboardData(PRInt32 aWhichClipboard)
{
  if (aWhichClipboard != kGlobalClipboard)
    return NS_ERROR_FAILURE;

  return DoClipboardAction(Write);
}

nsresult nsClipboard::GetNativeClipboardData(nsITransferable *aTransferable, PRInt32 aWhichClipboard)
{
  
  if (!aTransferable || aWhichClipboard != kGlobalClipboard)
    return NS_ERROR_FAILURE;

  nsITransferable *tmp = mTransferable;
  mTransferable = aTransferable;
  nsresult rc = DoClipboardAction(Read);
  mTransferable = tmp;
  return rc;
}


bool nsClipboard::GetClipboardData(const char *aFlavor)
{
  PRUint32 ulFormatID = GetFormatID(aFlavor);
  
  bool found = GetClipboardDataByID( ulFormatID, aFlavor );

  if (!found) 
  {
    if (!strcmp( aFlavor, kUnicodeMime ))
    {
      found = GetClipboardDataByID( CF_TEXT, aFlavor );
    }
    else if (strstr( aFlavor, "image/" ))
    {
      found = GetClipboardDataByID( CF_BITMAP, aFlavor );
    }
  }

  return found;
}

bool nsClipboard::GetClipboardDataByID(PRUint32 aFormatID, const char *aFlavor)
{
  PVOID pDataMem;
  PRUint32 NumOfBytes;
  bool TempBufAllocated = false;

  PVOID pClipboardData = reinterpret_cast<PVOID>(WinQueryClipbrdData(0, aFormatID));

  if (!pClipboardData) 
    return false;

  if (strstr( aFlavor, "text/" ))  
  {
    pDataMem = pClipboardData;

    if (aFormatID == CF_TEXT)     
    {
      PRUint32 NumOfChars = strlen( static_cast<char*>(pDataMem) );
      NumOfBytes = NumOfChars;

      if (!strcmp( aFlavor, kUnicodeMime ))  
      {
        nsAutoChar16Buffer buffer;
        PRInt32 bufLength;
        MultiByteToWideChar(0, static_cast<char*>(pDataMem), NumOfChars,
                            buffer, bufLength);
        pDataMem = ToNewUnicode(nsDependentString(buffer.Elements()));
        TempBufAllocated = true;
        NumOfBytes = bufLength * sizeof(UniChar);
      }

    }
    else                           
    {
      PRUint32 NumOfChars = UniStrlen( static_cast<UniChar*>(pDataMem) );
      NumOfBytes = NumOfChars * sizeof(UniChar);
      PVOID pTempBuf = nsMemory::Alloc(NumOfBytes);
      memcpy(pTempBuf, pDataMem, NumOfBytes);
      pDataMem = pTempBuf;
      TempBufAllocated = true;
    }

    
    nsLinebreakHelpers::ConvertPlatformToDOMLinebreaks( aFlavor, &pDataMem,   
                                                        reinterpret_cast<PRInt32*>(&NumOfBytes) );  

  }
  else                             
  {
    if (aFormatID == CF_BITMAP)
    {
      if (!strcmp( aFlavor, kJPEGImageMime ) || !strcmp( aFlavor, kJPGImageMime ))
      {
        
#ifdef DEBUG
        printf( "nsClipboard:: No JPG found on clipboard; need to convert BMP\n");
#endif
      }
      else if (!strcmp( aFlavor, kGIFImageMime ))
      {
        
#ifdef DEBUG
        printf( "nsClipboard:: No GIF found on clipboard; need to convert BMP\n");
#endif
      }
      else if (!strcmp( aFlavor, kPNGImageMime ))
      {
        
#ifdef DEBUG
        printf( "nsClipboard:: No PNG found on clipboard; need to convert BMP\n");
#endif
      }
    }
    else
    {
      pDataMem = static_cast<PBYTE>(pClipboardData) + sizeof(PRUint32);
      NumOfBytes = *(static_cast<PRUint32*>(pClipboardData));
    }
  }

  nsCOMPtr<nsISupports> genericDataWrapper;
  nsPrimitiveHelpers::CreatePrimitiveForData( aFlavor, pDataMem, NumOfBytes, getter_AddRefs(genericDataWrapper) );
#ifdef DEBUG
  nsresult errCode =
#endif
  mTransferable->SetTransferData( aFlavor, genericDataWrapper, NumOfBytes );
#ifdef DEBUG
  if (errCode != NS_OK)
    printf( "nsClipboard:: Error setting data into transferable\n" );
#endif

  if (TempBufAllocated)
    nsMemory::Free(pDataMem);

  return true;
}



void nsClipboard::SetClipboardData(const char *aFlavor)
{
  void *pMozData = nullptr;
  PRUint32 NumOfBytes = 0;

  
  nsCOMPtr<nsISupports> genericDataWrapper;
#ifdef DEBUG
  nsresult errCode =
#endif
  mTransferable->GetTransferData( aFlavor, getter_AddRefs(genericDataWrapper), &NumOfBytes );
#ifdef DEBUG
  if (NS_FAILED(errCode)) printf( "nsClipboard:: Error getting data from transferable\n" );
#endif
  if (NumOfBytes == 0) return;
  nsPrimitiveHelpers::CreateDataFromPrimitive( aFlavor, genericDataWrapper, &pMozData, NumOfBytes );

  
  if (!pMozData) {
    return;
  }

  PRUint32 ulFormatID = GetFormatID(aFlavor);

  if (strstr( aFlavor, "text/" ))  
  {
    if (ulFormatID == CF_TEXT)     
    {
      char* pByteMem = nullptr;

      if (DosAllocSharedMem( reinterpret_cast<PPVOID>(&pByteMem), nullptr, NumOfBytes + sizeof(char), 
                             PAG_WRITE | PAG_COMMIT | OBJ_GIVEABLE ) == NO_ERROR)
      {
        memcpy( pByteMem, pMozData, NumOfBytes );       
        pByteMem[NumOfBytes] = '\0';                    

        
        
        
        if (strlen(pByteMem) > 0xFFFF) {
          WinAlarm(HWND_DESKTOP, WA_ERROR);
        }
        WinSetClipbrdData(0, reinterpret_cast<ULONG>(pByteMem), ulFormatID, CFI_POINTER);
      }
    }
    else                           
    {
      UniChar* pUnicodeMem = nullptr;
      PRUint32 NumOfChars = NumOfBytes / sizeof(UniChar);
   
      if (DosAllocSharedMem( reinterpret_cast<PPVOID>(&pUnicodeMem), nullptr, NumOfBytes + sizeof(UniChar), 
                             PAG_WRITE | PAG_COMMIT | OBJ_GIVEABLE ) == NO_ERROR) 
      {
        memcpy( pUnicodeMem, pMozData, NumOfBytes );    
        pUnicodeMem[NumOfChars] = L'\0';                

        WinSetClipbrdData( 0, reinterpret_cast<ULONG>(pUnicodeMem), ulFormatID, CFI_POINTER );
      }

      
      

      if (!strcmp( aFlavor, kUnicodeMime ))
      {
        char* pByteMem = nullptr;

        if (DosAllocSharedMem(reinterpret_cast<PPVOID>(&pByteMem), nullptr,
                              NumOfBytes + 1, 
                              PAG_WRITE | PAG_COMMIT | OBJ_GIVEABLE ) == NO_ERROR) 
        {
          PRUnichar* uchtemp = (PRUnichar*)pMozData;
          for (PRUint32 i=0;i<NumOfChars;i++) {
            switch (uchtemp[i]) {
              case 0x2018:
              case 0x2019:
                uchtemp[i] = 0x0027;
                break;
              case 0x201C:
              case 0x201D:
                uchtemp[i] = 0x0022;
                break;
              case 0x2014:
                uchtemp[i] = 0x002D;
                break;
            }
          }

          nsAutoCharBuffer buffer;
          PRInt32 bufLength;
          WideCharToMultiByte(0, static_cast<PRUnichar*>(pMozData),
                              NumOfBytes, buffer, bufLength);
          memcpy(pByteMem, buffer.Elements(), NumOfBytes);
          
          
          
          if (strlen(pByteMem) > 0xFFFF) {
            WinAlarm(HWND_DESKTOP, WA_ERROR);
          }
          WinSetClipbrdData(0, reinterpret_cast<ULONG>(pByteMem), CF_TEXT, CFI_POINTER);
        }
      }
    }
  }
  else                             
  {
    PBYTE pBinaryMem = nullptr;

    if (DosAllocSharedMem( reinterpret_cast<PPVOID>(&pBinaryMem), nullptr, NumOfBytes + sizeof(PRUint32), 
                           PAG_WRITE | PAG_COMMIT | OBJ_GIVEABLE ) == NO_ERROR) 
    {
      *(reinterpret_cast<PRUint32*>(pBinaryMem)) = NumOfBytes;          
      memcpy( pBinaryMem + sizeof(PRUint32), pMozData, NumOfBytes );  

      WinSetClipbrdData( 0, reinterpret_cast<ULONG>(pBinaryMem), ulFormatID, CFI_POINTER );
    }

    
    

    if (strstr (aFlavor, "image/"))
    {
      
#ifdef DEBUG
      printf( "nsClipboard:: Putting image on clipboard; should also convert to BMP\n" );
#endif
    }
  }
  nsMemory::Free(pMozData);
}


nsresult nsClipboard::DoClipboardAction(ClipboardAction aAction)
{
  nsresult rc = NS_ERROR_FAILURE;

  if (WinOpenClipbrd(0)) {

    if (aAction == Write)
      WinEmptyClipbrd(0);

    
    nsCOMPtr<nsISupportsArray> pFormats;
    if(aAction == Read)
      rc = mTransferable->FlavorsTransferableCanImport(getter_AddRefs(pFormats));
    else
      rc = mTransferable->FlavorsTransferableCanExport(getter_AddRefs(pFormats));

    if (NS_FAILED(rc))
      return NS_ERROR_FAILURE;

    PRUint32 cFormats = 0;
    pFormats->Count(&cFormats);

    for (PRUint32 i = 0; i < cFormats; i++) {

      nsCOMPtr<nsISupports> genericFlavor;
      pFormats->GetElementAt(i, getter_AddRefs(genericFlavor));
      nsCOMPtr<nsISupportsCString> currentFlavor(do_QueryInterface(genericFlavor));

      if (currentFlavor) {
        nsXPIDLCString flavorStr;
        currentFlavor->ToString(getter_Copies(flavorStr));

        if (aAction == Read) {
          if (GetClipboardData(flavorStr))
            break;
        }
        else
          SetClipboardData(flavorStr);
      }
    }
    WinCloseClipbrd(0);
    rc = NS_OK;
  }
  return rc;
}


PRUint32 nsClipboard::GetFormatID(const char *aMimeStr)
{
  if (strcmp(aMimeStr, kTextMime) == 0)
    return CF_TEXT;

  return RegisterClipboardFormat(aMimeStr);
}

NS_IMETHODIMP nsClipboard::HasDataMatchingFlavors(const char** aFlavorList,
                                                  PRUint32 aLength,
                                                  PRInt32 aWhichClipboard,
                                                  bool *_retval)
{
  *_retval = false;
  if (aWhichClipboard != kGlobalClipboard || !aFlavorList)
    return NS_OK;

  for (PRUint32 i = 0; i < aLength; ++i) {
    ULONG fmtInfo = 0;
    PRUint32 format = GetFormatID(aFlavorList[i]);

    if (WinQueryClipbrdFmtInfo(0, format, &fmtInfo)) {
      *_retval = true;
      break;
    }

    
    if (!strcmp(aFlavorList[i], kUnicodeMime)) {
      if (WinQueryClipbrdFmtInfo(0, CF_TEXT, &fmtInfo)) {
        *_retval = true;
        break;
      }
    }


    
    if (strstr(aFlavorList[i], "image/")) {
      if (WinQueryClipbrdFmtInfo (0, CF_BITMAP, &fmtInfo)) {
#ifdef DEBUG
        printf("nsClipboard:: Image present on clipboard; need to add BMP conversion!\n");
#endif


      }
    }
  }
  return NS_OK;
}
