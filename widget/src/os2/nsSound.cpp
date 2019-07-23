











































#include "nscore.h"
#include "plstr.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_MMIOOS2
#include <os2.h>
#include <mmioos2.h>
#include <mcios2.h>
#define MCI_ERROR_LENGTH 128

#include "nsSound.h"
#include "nsIURL.h"
#include "nsNetUtil.h"
#include "nsString.h"

#include "nsDirectoryServiceDefs.h"

#include "nsNativeCharsetUtils.h"

NS_IMPL_ISUPPORTS2(nsSound, nsISound, nsIStreamLoaderObserver)

static int sInitialized = 0;
static PRBool sMMPMInstalled = PR_FALSE;
static HMODULE sHModMMIO = NULLHANDLE;


HMMIO (*APIENTRY _mmioOpen)(PSZ, PMMIOINFO, ULONG);
USHORT (*APIENTRY _mmioClose)(HMMIO, USHORT);
ULONG (*APIENTRY _mmioGetFormats)(PMMFORMATINFO, LONG, PVOID, PLONG, ULONG, ULONG);
ULONG (*APIENTRY _mciSendCommand)(USHORT, USHORT, ULONG, PVOID, USHORT);
#ifdef DEBUG
ULONG (*APIENTRY _mmioGetLastError)(HMMIO);
ULONG (*APIENTRY _mmioQueryFormatCount)(PMMFORMATINFO, PLONG, ULONG, ULONG);
ULONG (*APIENTRY _mmioGetFormatName)(PMMFORMATINFO, PSZ, PLONG, ULONG, ULONG);
ULONG (*APIENTRY _mciGetErrorString)(ULONG, PSZ, USHORT);
#endif
ULONG (*APIENTRY _mmioIniFileHandler)(PMMINIFILEINFO, ULONG);


typedef struct _ARGBUFFER
{
  HEV hev;
  PRUint32 bufLen;
  const char *buffer;
  PSZ pszFilename;
} ARGBUFFER;



static void InitGlobals(void)
{
  ULONG ulrc = 0;
  char LoadError[CCHMAXPATH];
  HMODULE hModMDM = NULLHANDLE;

  ulrc = DosLoadModule(LoadError, CCHMAXPATH, "MMIO", &sHModMMIO);
  ulrc += DosLoadModule(LoadError, CCHMAXPATH, "MDM", &hModMDM);
  if (ulrc == NO_ERROR) {
#ifdef DEBUG
    printf("InitGlobals: MMOS2 is installed, both DLLs loaded\n");
#endif
    sMMPMInstalled = PR_TRUE;
    
    
    ulrc = DosQueryProcAddr(sHModMMIO, 0L, "mmioOpen", (PFN *)&_mmioOpen);
    ulrc += DosQueryProcAddr(sHModMMIO, 0L, "mmioClose", (PFN *)&_mmioClose);
    ulrc += DosQueryProcAddr(sHModMMIO, 0L, "mmioGetFormats", (PFN *)&_mmioGetFormats);
    
    ulrc += DosQueryProcAddr(hModMDM, 0L, "mciSendCommand", (PFN *)&_mciSendCommand);
#ifdef DEBUG
    ulrc += DosQueryProcAddr(sHModMMIO, 0L, "mmioGetLastError", (PFN *)&_mmioGetLastError);
    ulrc += DosQueryProcAddr(sHModMMIO, 0L, "mmioQueryFormatCount", (PFN *)&_mmioQueryFormatCount);
    ulrc += DosQueryProcAddr(sHModMMIO, 0L, "mmioGetFormatName", (PFN *)&_mmioGetFormatName);
    ulrc += DosQueryProcAddr(hModMDM, 0L, "mciGetErrorString", (PFN *)&_mciGetErrorString);
#endif

    ulrc += DosQueryProcAddr(sHModMMIO, 0L, "mmioIniFileHandler", (PFN *)&_mmioIniFileHandler);

    
    if (ulrc != NO_ERROR) {
      NS_WARNING("MMOS2 is installed, but seems to have corrupt DLLs");
      sMMPMInstalled = PR_FALSE;
    }
  }
}







FOURCC determineFourCC(PRUint32 aDataLen, const char *aData)
{
  FOURCC fcc = 0;

  
  
  if (memcmp(aData, "RIFF", 4) == 0) {                                    
    fcc = mmioFOURCC('W', 'A', 'V', 'E');
  } else if (memcmp(aData, "ID3", 3) == 0 ||       
             ((aData[0] & 0xFF) == 0xFF &&   
              ((aData[1] & 0xFE) == 0xFA ||                                
               (aData[1] & 0xFE) == 0xF2 ||                                
               (aData[1] & 0xFE) == 0xE2)))                              
  {
    fcc = mmioFOURCC('M','P','3',' ');
  } else if (memcmp(aData, "OggS", 4) == 0) {                             
    fcc = mmioFOURCC('O','G','G','S');
  } else if (memcmp(aData, "fLaC", 4) == 0) {                            
    fcc = mmioFOURCC('f','L','a','C');
  }

  
  
#if 0
  if (fcc) 
    return fcc;

  
  
  MMFORMATINFO mmfi;
  LONG lNum;
  memset(&mmfi, '\0', sizeof(mmfi));
  mmfi.ulStructLen = sizeof(mmfi);
  mmfi.ulMediaType |= MMIO_MEDIATYPE_AUDIO;
  ULONG ulrc = _mmioQueryFormatCount(&mmfi, &lNum, 0L, 0L);

  PMMFORMATINFO mmflist = (PMMFORMATINFO)calloc(lNum, sizeof(MMFORMATINFO));
  LONG lFormats;
  ulrc = _mmioGetFormats(&mmfi, lNum, mmflist, &lFormats, 0L, 0L);

  MMIOINFO mi;
  memset(&mi, '\0', sizeof(mi));
  mi.fccChildIOProc = FOURCC_MEM;
  unsigned char szBuffer[sizeof(FOURCC) + CCHMAXPATH + 4];
  for (int i = lFormats-1; i >= 0; i--) {
    
    
    MMFORMATINFO mmfi = mmflist[i];
#ifdef DEBUG
    LONG lBytesRead;
    _mmioGetFormatName(&mmfi, (char *)szBuffer, &lBytesRead, 0L, 0L);
    printf("determineFour Codec %d: name=%s media=0x%lx ext=%s fcc=%c%c%c%c/%ld/%p\n",
           i, szBuffer, mmfi.ulMediaType, mmfi.szDefaultFormatExt,
           (char)(mmfi.fccIOProc), (char)(mmfi.fccIOProc >> 8),
           (char)(mmfi.fccIOProc >> 16), (char)(mmfi.fccIOProc >> 24),
           mmfi.fccIOProc, (void *)mmfi.fccIOProc);
#endif

    
    
    if (mmfi.fccIOProc == mmioFOURCC('A','V','C','A')) {
      continue;
    }

    mi.fccIOProc = mmfi.fccIOProc;
    HMMIO hmmio= _mmioOpen(NULL, &mi, MMIO_READ);
    if (hmmio) {
      fcc = mmfi.fccIOProc;
      _mmioClose(hmmio, 0);
      break;
    }
  }
  free(mmflist);
#endif

#ifdef DEBUG
  printf("determineFourCC: Codec fcc is 0x%lx or --%c%c%c%c--\n", fcc,
         (char)(fcc), (char)(fcc >> 8), (char)(fcc >> 16), (char)(fcc >> 24));
#endif

  return fcc;
}



static void playSound(void *aArgs)
{
  ULONG ulrc = NO_ERROR;
  ARGBUFFER args;
  memcpy(&args, aArgs, sizeof(args));

  MMIOINFO mi;
  memset(&mi, '\0', sizeof(mi));
  HMMIO hmmio = NULLHANDLE;

  do { 
    if (args.pszFilename) {
      
      FILESTATUS3 fs3;
      memset(&fs3, '\0', sizeof(fs3));
      ulrc = DosQueryPathInfo(args.pszFilename, FIL_STANDARD, &fs3, sizeof(fs3));
      mi.cchBuffer = fs3.cbFile;
    } else {
      
      mi.cchBuffer = args.bufLen;
    }

    
    
    
    ulrc = DosAllocMem((PPVOID)&mi.pchBuffer, mi.cchBuffer,
#ifdef OS2_HIGH_MEMORY             
                       OBJ_ANY | 
#endif
                       PAG_READ | PAG_WRITE | PAG_COMMIT);
    if (ulrc != NO_ERROR) {
#ifdef DEBUG
      printf("playSound: Could not allocate the sound buffer, ulrc=%ld\n", ulrc);
#endif
      break;
    }

    if (args.pszFilename) {
      
      HFILE hf = NULLHANDLE;
      ULONG ulAction = 0;
      ulrc = DosOpen(args.pszFilename, &hf, &ulAction, 0, FILE_NORMAL,
                     OPEN_ACTION_OPEN_IF_EXISTS | OPEN_ACTION_FAIL_IF_NEW,
                     OPEN_ACCESS_READONLY | OPEN_SHARE_DENYNONE,
                     NULL);
      if (ulrc != NO_ERROR) {
#ifdef DEBUG
        printf("playSound: could not open the sound file \"%s\" (%ld)\n",
               args.pszFilename, ulrc);
#endif
        break;
      }
      ULONG ulRead = 0;
      ulrc = DosRead(hf, mi.pchBuffer, mi.cchBuffer, &ulRead);
      DosClose(hf);
      if (ulrc != NO_ERROR) {
#ifdef DEBUG
        printf("playSound: read %ld of %ld bytes from the sound file \"%s\" (%ld)\n",
               ulRead, mi.cchBuffer, args.pszFilename, ulrc);
#endif
        break;
      }
    } else {
      
      memcpy(mi.pchBuffer, args.buffer, args.bufLen);
    }

    DosPostEventSem(args.hev); 

    
    mi.fccChildIOProc = FOURCC_MEM;
    mi.fccIOProc = determineFourCC(mi.cchBuffer, mi.pchBuffer);
    if (!mi.fccIOProc) {
      NS_WARNING("playSound: unknown sound format in memory buffer");
      break;
    }
    mi.ulTranslate = MMIO_TRANSLATEDATA | MMIO_TRANSLATEHEADER;
    hmmio = _mmioOpen(NULL, &mi, MMIO_READ | MMIO_DENYWRITE);

    if (!hmmio) {
#ifdef DEBUG
      ULONG ulrc = _mmioGetLastError(hmmio);
      if (args.pszFilename) {
        printf("playSound: mmioOpen failed, cannot play sound from \"%s\" (%ld)\n",
               args.pszFilename, ulrc);
      } else {
        printf("playSound: mmioOpen failed, cannot play sound buffer (%ld)\n",
               ulrc);
      }
#endif
      break;
    }

    
    MCI_OPEN_PARMS mop;
    memset(&mop, '\0', sizeof(mop));
    mop.pszElementName = (PSZ)hmmio;
    mop.pszDeviceType = (PSZ)MAKEULONG(MCI_DEVTYPE_WAVEFORM_AUDIO, 0);
    ulrc = _mciSendCommand(0, MCI_OPEN,
                           MCI_OPEN_MMIO | MCI_OPEN_TYPE_ID | MCI_OPEN_SHAREABLE | MCI_WAIT,
                           (PVOID)&mop, 0);
    if (ulrc != MCIERR_SUCCESS) {
#ifdef DEBUG
      CHAR errorBuffer[MCI_ERROR_LENGTH];
      _mciGetErrorString(ulrc, errorBuffer, MCI_ERROR_LENGTH);
      printf("playSound: mciSendCommand with MCI_OPEN_MMIO returned %ld: %s\n",
             ulrc, errorBuffer);
#endif
      break;
    }

    
    MCI_PLAY_PARMS mpp;
    memset(&mpp, '\0', sizeof(mpp));
    ulrc = _mciSendCommand(mop.usDeviceID, MCI_PLAY, MCI_WAIT, &mpp, 0);
#ifdef DEBUG
    
    if (ulrc != MCIERR_SUCCESS) {
      CHAR errorBuffer[MCI_ERROR_LENGTH];
      _mciGetErrorString(ulrc, errorBuffer, MCI_ERROR_LENGTH);
      printf("playSound: mciSendCommand with MCI_PLAY returned %ld: %s\n",
             ulrc, errorBuffer);
    }
#endif

    
    ulrc = _mciSendCommand(mop.usDeviceID, MCI_STOP, MCI_WAIT, &mpp, 0); 
    ulrc = _mciSendCommand(mop.usDeviceID, MCI_CLOSE, MCI_WAIT, &mpp, 0);
#ifdef DEBUG
    if (ulrc != MCIERR_SUCCESS) {
      CHAR errorBuffer[MCI_ERROR_LENGTH];
      _mciGetErrorString(ulrc, errorBuffer, MCI_ERROR_LENGTH);
      printf("playSound: mciSendCommand with MCI_CLOSE returned %ld: %s\n",
             ulrc, errorBuffer);
    }
#endif
    _mmioClose(hmmio, 0);
    DosFreeMem(mi.pchBuffer);
    _endthread();
  } while(0); 

  
  WinAlarm(HWND_DESKTOP, WA_WARNING); 
  if (hmmio)
    _mmioClose(hmmio, 0);
  if (mi.pchBuffer)
    DosFreeMem(mi.pchBuffer);
}



nsSound::nsSound()
{
  if (!sInitialized) {
    InitGlobals();
  }
  sInitialized++;
#ifdef DEBUG
  printf("nsSound::nsSound: sInitialized=%d\n", sInitialized);
#endif
}

nsSound::~nsSound()
{
  sInitialized--;
#ifdef DEBUG
  printf("nsSound::~nsSound: sInitialized=%d\n", sInitialized);
#endif
  
  if (!sInitialized) {
#ifdef DEBUG
    printf("nsSound::~nsSound: Trying to free modules...\n");
#endif
    ULONG ulrc;
    ulrc = DosFreeModule(sHModMMIO);
    
    if (ulrc != NO_ERROR) {
      NS_WARNING("DosFreeModule did not work");
    }
  }
}

NS_IMETHODIMP nsSound::OnStreamComplete(nsIStreamLoader *aLoader,
                                        nsISupports *context,
                                        nsresult aStatus,
                                        PRUint32 dataLen,
                                        const PRUint8 *data)
{
  if (NS_FAILED(aStatus)) {
#ifdef DEBUG
    if (aLoader) {
      nsCOMPtr<nsIRequest> request;
      aLoader->GetRequest(getter_AddRefs(request));
      if (request) {
        nsCOMPtr<nsIURI> uri;
        nsCOMPtr<nsIChannel> channel = do_QueryInterface(request);
        if (channel) {
            channel->GetURI(getter_AddRefs(uri));
            if (uri) {
                nsCAutoString uriSpec;
                uri->GetSpec(uriSpec);
                printf("Failed to load %s\n", uriSpec.get());
            }
        }
      }
    }
#endif
    return NS_ERROR_FAILURE;
  }

  if (!sMMPMInstalled) {
    NS_WARNING("Sound output only works with MMOS2 installed");
    Beep();
    return NS_OK;
  }

  ARGBUFFER arg;
  memset(&arg, '\0', sizeof(arg));
  APIRET rc = DosCreateEventSem(NULL, &(arg.hev), 0UL, 0UL);

  
  
  arg.bufLen = dataLen;
  arg.buffer = (char *)data;
  _beginthread(playSound, NULL, 32768, (void *)&arg);

  
  
  rc = DosWaitEventSem(arg.hev, 100);
  rc = DosCloseEventSem(arg.hev);

  return NS_OK;
}

NS_IMETHODIMP nsSound::Beep()
{
  WinAlarm(HWND_DESKTOP, WA_WARNING);

  return NS_OK;
}

NS_IMETHODIMP nsSound::Play(nsIURL *aURL)
{
  nsresult rv;

  nsCOMPtr<nsIStreamLoader> loader;
  rv = NS_NewStreamLoader(getter_AddRefs(loader), aURL, this);

  return rv;
}

NS_IMETHODIMP nsSound::Init()
{
  return NS_OK;
}

NS_IMETHODIMP nsSound::PlaySystemSound(const nsAString &aSoundAlias)
{
  if (!sMMPMInstalled) {
    return Beep();
  }

  if (NS_IsMozAliasSound(aSoundAlias)) {
    NS_WARNING("nsISound::playSystemSound is called with \"_moz_\" events, they are obsolete, use nsISound::playEventSound instead");
    PRUint32 eventId;
    if (aSoundAlias.Equals(NS_SYSSOUND_ALERT_DIALOG))
        eventId = EVENT_ALERT_DIALOG_OPEN;
    else if (aSoundAlias.Equals(NS_SYSSOUND_CONFIRM_DIALOG))
        eventId = EVENT_CONFIRM_DIALOG_OPEN;
    else if (aSoundAlias.Equals(NS_SYSSOUND_MAIL_BEEP))
        eventId = EVENT_NEW_MAIL_RECEIVED;
    else
        return NS_OK;
    return PlayEventSound(eventId);
  }
  nsCAutoString nativeSoundAlias;
  NS_CopyUnicodeToNative(aSoundAlias, nativeSoundAlias);

  ARGBUFFER arg;
  memset(&arg, '\0', sizeof(arg));
  APIRET rc = DosCreateEventSem(NULL, &(arg.hev), 0UL, 0UL);

  
  
  arg.pszFilename = (PSZ)nativeSoundAlias.get();
  _beginthread(playSound, NULL, 32768, (void *)&arg);

  
  rc = DosWaitEventSem(arg.hev, 100);
  rc = DosCloseEventSem(arg.hev);

  return NS_OK;
}

NS_IMETHODIMP nsSound::PlayEventSound(PRUint32 aEventId)
{
  
  
  
  
  switch(aEventId) {
  case EVENT_NEW_MAIL_RECEIVED:
    
    return Beep(); 
  case EVENT_ALERT_DIALOG_OPEN:
    WinAlarm(HWND_DESKTOP, WA_ERROR); 
    break;
  case EVENT_CONFIRM_DIALOG_OPEN:
    WinAlarm(HWND_DESKTOP, WA_NOTE); 
    break;
  }

  return NS_OK;
}
