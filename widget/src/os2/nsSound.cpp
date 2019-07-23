








































#include "nscore.h"
#include "plstr.h"
#include <stdio.h>
#include <string.h>

#define INCL_DOS
#define INCL_DOSERRORS
#include <os2.h>

#include "nsSound.h"
#include "nsIURL.h"
#include "nsNetUtil.h"

#include "nsDirectoryServiceDefs.h"

#include "nsNativeCharsetUtils.h"

NS_IMPL_ISUPPORTS2(nsSound, nsISound, nsIStreamLoaderObserver)

static int                gInitialized = 0;
static PRBool             gMMPMInstalled = PR_FALSE;

static void
InitGlobals(void)
{
  APIRET ulrc;
  HMODULE hmod;
  char LoadError[CCHMAXPATH];
  ulrc = DosLoadModule(LoadError, CCHMAXPATH, "MMPM", &hmod);
  if (ulrc == NO_ERROR) {
    gMMPMInstalled = PR_TRUE;
  }
  DosFreeModule(hmod);
  gInitialized = 1;
}



nsSound::nsSound()
{
  if (!gInitialized) {
    InitGlobals();
  }
}

nsSound::~nsSound()
{
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

  if (memcmp(data, "RIFF", 4) || (!gMMPMInstalled)) {
    NS_WARNING("We only support WAV files currently.\n");
    Beep();
    return NS_OK;
  }

  nsresult rv;
    
  static const char *kSoundTmpFileName = "mozsound.wav";

  nsCOMPtr<nsIFile> soundTmp;
  rv = NS_GetSpecialDirectory(NS_OS_TEMP_DIR, getter_AddRefs(soundTmp));
  if (NS_FAILED(rv)) return rv;
  rv = soundTmp->AppendNative(nsDependentCString(kSoundTmpFileName));
  nsCAutoString soundFilename;
  (void) soundTmp->GetNativePath(soundFilename);
  FILE *fp = fopen(soundFilename.get(), "wb+");
  if (fp) {
    fwrite(data, dataLen, 1, fp);
    fclose(fp);
    HOBJECT hobject = WinQueryObject(soundFilename.get());
    WinSetObjectData(hobject, "OPEN=DEFAULT");
  } else {
    NS_WARNING("Could not open WAV file for binary writing.\n");
    Beep();
  }

  return NS_OK;

#ifdef OLDCODE 
  ULONG ulRC;
  CHAR errorBuffer[128];

  HMMIO hmmio;
  MMIOINFO mmioinfo;

  memset(&mmioinfo, 0, sizeof(MMIOINFO));
  mmioinfo.fccIOProc = FOURCC_MEM;
  mmioinfo.cchBuffer = dataLen;
  mmioinfo.pchBuffer = (char*)data;
  USHORT usDeviceID;

  hmmio = mmioOpen(NULL, &mmioinfo, MMIO_READWRITE);

  MCI_OPEN_PARMS mop;
  memset(&mop, 0, sizeof(MCI_OPEN_PARMS));

  mop.pszElementName = (char*)hmmio;
  CHAR DeviceType[] = "waveaudio";
  mop.pszDeviceType = (PSZ)&DeviceType;

  ulRC = mciSendCommand(0, MCI_OPEN, MCI_OPEN_MMIO | MCI_WAIT, &mop, 0);

  if (ulRC != MCIERR_SUCCESS) {
     ulRC = mciGetErrorString(ulRC, errorBuffer, 128);
  }

  usDeviceID = mop.usDeviceID;

  MCI_OPEN_PARMS mpp;

  memset(&mpp, 0, sizeof(MCI_OPEN_PARMS));
  ulRC = mciSendCommand(usDeviceID, MCI_PLAY, MCI_WAIT, &mpp, 0);

  if (ulRC != MCIERR_SUCCESS) {
     ulRC = mciGetErrorString(ulRC, errorBuffer, 128);
  }

  MCI_GENERIC_PARMS mgp;
  memset(&mgp, 0, sizeof(MCI_GENERIC_PARMS));
  ulRC = mciSendCommand(usDeviceID, MCI_CLOSE, MCI_WAIT, &mgp, 0);

  if (ulRC != MCIERR_SUCCESS) {
     ulRC = mciGetErrorString(ulRC, errorBuffer, 128);
  }

  mmioClose(hmmio, 0);
#endif
}

NS_METHOD nsSound::Beep()
{
  WinAlarm(HWND_DESKTOP, WA_WARNING);

  return NS_OK;
}

NS_METHOD nsSound::Play(nsIURL *aURL)
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
  
  
  if (aSoundAlias.EqualsLiteral("_moz_mailbeep") || (!gMMPMInstalled)) {
    Beep();
  }
  else {
    nsCAutoString nativeSoundAlias;
    NS_CopyUnicodeToNative(aSoundAlias, nativeSoundAlias);
    HOBJECT hobject = WinQueryObject(nativeSoundAlias.get());
    if (hobject)
      WinSetObjectData(hobject, "OPEN=DEFAULT");
    else 
      Beep();
  }
  return NS_OK;
}

