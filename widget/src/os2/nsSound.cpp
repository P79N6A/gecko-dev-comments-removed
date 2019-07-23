














































#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define INCL_DOS
#define INCL_WINSHELLDATA
#define INCL_MMIOOS2
#include <os2.h>
#include <mmioos2.h>
#include <mcios2.h>

#include "nsSound.h"
#include "nsIURL.h"
#include "nsNetUtil.h"
#include "nsNativeCharsetUtils.h"

NS_IMPL_ISUPPORTS2(nsSound, nsISound, nsIStreamLoaderObserver)




typedef struct _ARGBUFFER
{
  PRUint32  bufLen;
  char      buffer[1];
} ARGBUFFER;

#ifdef DEBUG
  #define DBG_MSG(x)    fprintf(stderr, x "\n")
#else
  #define DBG_MSG(x)
#endif


#define EVENT_CNT       7




static PRBool   sDllError = FALSE;      
static char *   sSoundFiles[EVENT_CNT] = {0}; 


static HMMIO  (*APIENTRY _mmioOpen)(PSZ, PMMIOINFO, ULONG);
static USHORT (*APIENTRY _mmioClose)(HMMIO, USHORT);
static ULONG  (*APIENTRY _mciSendCommand)(USHORT, USHORT, ULONG, PVOID, USHORT);


static void   initSounds(void);
static PRBool initDlls(void);
static void   playSound(void *aArgs);
static FOURCC determineFourCC(PRUint32 aDataLen, const char *aData);









nsSound::nsSound()
{
  initSounds();
}



nsSound::~nsSound()
{
}



NS_IMETHODIMP nsSound::Init()
{
  return NS_OK;
}



NS_IMETHODIMP nsSound::Beep()
{
  WinAlarm(HWND_DESKTOP, WA_WARNING);
  return NS_OK;
}





NS_IMETHODIMP nsSound::Play(nsIURL *aURL)
{
  if (sDllError) {
    DBG_MSG("nsSound::Play:  MMOS2 initialization failed");
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIStreamLoader> loader;
  return NS_NewStreamLoader(getter_AddRefs(loader), aURL, this);
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
            fprintf(stderr, "nsSound::OnStreamComplete:  failed to load %s\n",
                    uriSpec.get());
          }
        }
      }
    }
#endif
    return NS_ERROR_FAILURE;
  }

  
  
  ARGBUFFER *   arg;
  if (DosAllocMem((PPVOID)&arg, sizeof(ARGBUFFER) + dataLen,
                  OBJ_ANY | PAG_READ | PAG_WRITE | PAG_COMMIT)) {
    if (DosAllocMem((PPVOID)&arg, sizeof(ARGBUFFER) + dataLen,
                    PAG_READ | PAG_WRITE | PAG_COMMIT)) {
      DBG_MSG("nsSound::OnStreamComplete:  DosAllocMem failed");
      return NS_ERROR_FAILURE;
    }
  }

  
  arg->bufLen = dataLen;
  memcpy(arg->buffer, data, dataLen);

  
  if (_beginthread(playSound, NULL, 32768, (void*)arg) < 0) {
    DosFreeMem((void*)arg);
    DBG_MSG("nsSound::OnStreamComplete:  _beginthread failed");
  }

  return NS_OK;
}





NS_IMETHODIMP nsSound::PlaySystemSound(const nsAString &aSoundAlias)
{
  if (aSoundAlias.IsEmpty())
    return NS_OK;

  if (NS_IsMozAliasSound(aSoundAlias)) {
    DBG_MSG("nsISound::playSystemSound was called with \"_moz_\" events, "
               "they are obsolete, use nsISound::playEventSound instead");

    PRUint32 eventId;
    if (aSoundAlias.Equals(NS_SYSSOUND_MAIL_BEEP)) {
      eventId = EVENT_NEW_MAIL_RECEIVED;
    } else if (aSoundAlias.Equals(NS_SYSSOUND_ALERT_DIALOG)) {
      eventId = EVENT_ALERT_DIALOG_OPEN;
    } else if (aSoundAlias.Equals(NS_SYSSOUND_CONFIRM_DIALOG)) {
      eventId = EVENT_CONFIRM_DIALOG_OPEN;
    } else if (aSoundAlias.Equals(NS_SYSSOUND_PROMPT_DIALOG)) {
      eventId = EVENT_PROMPT_DIALOG_OPEN;
    } else if (aSoundAlias.Equals(NS_SYSSOUND_SELECT_DIALOG)) {
      eventId = EVENT_SELECT_DIALOG_OPEN;
    } else if (aSoundAlias.Equals(NS_SYSSOUND_MENU_EXECUTE)) {
      eventId = EVENT_MENU_EXECUTE;
    } else if (aSoundAlias.Equals(NS_SYSSOUND_MENU_POPUP)) {
      eventId = EVENT_MENU_POPUP;
    } else {
      return NS_OK;
    }
    return PlayEventSound(eventId);
  }

  
  return PlaySoundFile(aSoundAlias);
}







NS_IMETHODIMP nsSound::PlayEventSound(PRUint32 aEventId)
{
  if (!sDllError &&
      aEventId < EVENT_CNT &&
      sSoundFiles[aEventId] &&
      NS_SUCCEEDED(PlaySoundFile(
                   nsDependentCString(sSoundFiles[aEventId])))) {
    return NS_OK;
  }

  switch(aEventId) {
    case EVENT_NEW_MAIL_RECEIVED:
      return Beep();                    
    case EVENT_ALERT_DIALOG_OPEN:
      WinAlarm(HWND_DESKTOP, WA_ERROR); 
      break;
    case EVENT_CONFIRM_DIALOG_OPEN:
      WinAlarm(HWND_DESKTOP, WA_NOTE);  
      break;
    case EVENT_PROMPT_DIALOG_OPEN:
    case EVENT_SELECT_DIALOG_OPEN:
    case EVENT_MENU_EXECUTE:
    case EVENT_MENU_POPUP:
      break;
  }

  return NS_OK;
}





nsresult nsSound::PlaySoundFile(const nsAString &aSoundFile)
{
  nsCAutoString buf;
  nsresult rv = NS_CopyUnicodeToNative(aSoundFile, buf);
  NS_ENSURE_SUCCESS(rv,rv);

  return PlaySoundFile(buf);
}





nsresult nsSound::PlaySoundFile(const nsACString &aSoundFile)
{
  nsresult rv;
  nsCOMPtr <nsILocalFile> soundFile;
  rv = NS_NewNativeLocalFile(aSoundFile, PR_FALSE, 
                             getter_AddRefs(soundFile));
  NS_ENSURE_SUCCESS(rv,rv);

  nsCOMPtr <nsIURI> fileURI;
  rv = NS_NewFileURI(getter_AddRefs(fileURI), soundFile);
  NS_ENSURE_SUCCESS(rv,rv);

  nsCOMPtr<nsIFileURL> fileURL = do_QueryInterface(fileURI,&rv);
  NS_ENSURE_SUCCESS(rv,rv);

  return Play(fileURL);
}





















static void initSounds(void)
{
  static PRBool sSoundInit = FALSE;

  if (sSoundInit) {
    return;
  }
  sSoundInit = TRUE;

  
  
  FILESTATUS3 fs3;
  char   buffer[CCHMAXPATH];
  char * ptr;
  ULONG  rc = DosScanEnv("MMBASE", const_cast<const char **>(&ptr));
  if (!rc) {
    strcpy(buffer, ptr);
    ptr = strchr(buffer, ';');
    if (!ptr) {
      ptr = strchr(buffer, 0);
    }
    strcpy(ptr, "\\MMPM.INI");
    rc = DosQueryPathInfo(buffer, FIL_STANDARD, &fs3, sizeof(fs3));
  }
  if (rc) {
    ULONG ulBootDrive = 0;
    strcpy(buffer, "x:\\MMOS2\\MMPM.INI");
    DosQuerySysInfo( QSV_BOOT_DRIVE, QSV_BOOT_DRIVE,
                     &ulBootDrive, sizeof ulBootDrive);
    buffer[0] = 0x40 + ulBootDrive;
    rc = DosQueryPathInfo(buffer, FIL_STANDARD, &fs3, sizeof(fs3));
  }
  if (rc) {
    DBG_MSG("initSounds:  unable to locate mmpm.ini");
    return;
  }

  HINI hini = PrfOpenProfile(0, buffer);
  if (!hini) {
    DBG_MSG("initSounds:  unable to open mmpm.ini");
    return;
  }

  
  
  LONG baseNdx = PrfQueryProfileInt(hini, "MOZILLA_Events",
                                    "BaseIndex", 800);
  if (baseNdx <= 12) {
    baseNdx = 800;
  }

  
  
  
  for (LONG i = 0; i < EVENT_CNT; i++) {
    char  key[16];
    ultoa(i + baseNdx, key, 10);
    if (!PrfQueryProfileString(hini, "MMPM2_AlarmSounds", key,
                               0, buffer, sizeof(buffer))) {
      continue;
    }
    ptr = strchr(buffer, '#');
    if (!ptr || ptr == buffer) {
      continue;
    }
    *ptr = 0;
    if (DosQueryPathInfo(buffer, FIL_STANDARD, &fs3, sizeof(fs3)) ||
        (fs3.attrFile & FILE_DIRECTORY) || !fs3.cbFile) {
      continue;
    }
    sSoundFiles[i] = strdup(buffer);
  }

  PrfCloseProfile(hini);
  return;
}






static PRBool initDlls(void)
{
  static PRBool sDllInit = FALSE;

  if (sDllInit) {
    return TRUE;
  }
  sDllInit = TRUE;

  HMODULE   hmodMMIO = 0;
  HMODULE   hmodMDM = 0;
  char      szError[32];
  if (DosLoadModule(szError, sizeof(szError), "MMIO", &hmodMMIO) ||
      DosLoadModule(szError, sizeof(szError), "MDM", &hmodMDM)) {
    DBG_MSG("initDlls:  DosLoadModule failed");
    sDllError = TRUE;
    return FALSE;
  }

  if (DosQueryProcAddr(hmodMMIO, 0L, "mmioOpen", (PFN *)&_mmioOpen) ||
      DosQueryProcAddr(hmodMMIO, 0L, "mmioClose", (PFN *)&_mmioClose) ||
      DosQueryProcAddr(hmodMDM,  0L, "mciSendCommand", (PFN *)&_mciSendCommand)) {
    DBG_MSG("initDlls:  DosQueryProcAddr failed");
    sDllError = TRUE;
    return FALSE;
  }

  return TRUE;
}







static void playSound(void * aArgs)
{
  BOOL        fOK = FALSE;
  HMMIO       hmmio = 0;
  MMIOINFO    mi;

  do {
    if (!initDlls())
      break;

    memset(&mi, 0, sizeof(MMIOINFO));
    mi.cchBuffer = ((ARGBUFFER*)aArgs)->bufLen;
    mi.pchBuffer = ((ARGBUFFER*)aArgs)->buffer;
    mi.ulTranslate = MMIO_TRANSLATEDATA | MMIO_TRANSLATEHEADER;
    mi.fccChildIOProc = FOURCC_MEM;
    mi.fccIOProc = determineFourCC(mi.cchBuffer, mi.pchBuffer);
    if (!mi.fccIOProc) {
      DBG_MSG("playSound:  unknown sound format");
      break;
    }

    hmmio = _mmioOpen(NULL, &mi, MMIO_READ | MMIO_DENYWRITE);
    if (!hmmio) {
      DBG_MSG("playSound:  _mmioOpen failed");
      break;
    }

    
    MCI_OPEN_PARMS mop;
    memset(&mop, 0, sizeof(mop));
    mop.pszElementName = (PSZ)hmmio;
    mop.pszDeviceType = (PSZ)MAKEULONG(MCI_DEVTYPE_WAVEFORM_AUDIO, 0);
    if (_mciSendCommand(0, MCI_OPEN,
                        MCI_OPEN_MMIO | MCI_OPEN_TYPE_ID |
                        MCI_OPEN_SHAREABLE | MCI_WAIT,
                        (PVOID)&mop, 0)) {
      DBG_MSG("playSound:  MCI_OPEN failed");
      break;
    }
    fOK = TRUE;

    
    MCI_PLAY_PARMS mpp;
    memset(&mpp, 0, sizeof(mpp));
    if (_mciSendCommand(mop.usDeviceID, MCI_PLAY, MCI_WAIT, &mpp, 0)) {
      DBG_MSG("playSound:  MCI_PLAY failed");
    }

    
    _mciSendCommand(mop.usDeviceID, MCI_STOP,  MCI_WAIT, &mpp, 0);
    if (_mciSendCommand(mop.usDeviceID, MCI_CLOSE, MCI_WAIT, &mpp, 0)) {
      DBG_MSG("playSound:  MCI_CLOSE failed");
    }

  } while (0);

  if (!fOK)
    WinAlarm(HWND_DESKTOP, WA_WARNING);

  if (hmmio)
    _mmioClose(hmmio, 0);
  DosFreeMem(aArgs);

  _endthread();
}







static FOURCC determineFourCC(PRUint32 aDataLen, const char *aData)
{
  FOURCC fcc = 0;

  
  
  
  if (memcmp(aData, "RIFF", 4) == 0)        
    fcc = mmioFOURCC('W', 'A', 'V', 'E');
  else
  if (memcmp(aData, "ID3", 3) == 0)         
    fcc = mmioFOURCC('M','P','3',' ');
  else
  if ((aData[0] & 0xFF) == 0xFF &&          
      ((aData[1] & 0xFE) == 0xFA ||         
       (aData[1] & 0xFE) == 0xF2 ||         
       (aData[1] & 0xFE) == 0xE2))          
    fcc = mmioFOURCC('M','P','3',' ');
  else
  if (memcmp(aData, "OggS", 4) == 0)        
    fcc = mmioFOURCC('O','G','G','S');
  else
  if (memcmp(aData, "fLaC", 4) == 0)        
    fcc = mmioFOURCC('f','L','a','C');

  return fcc;
}



