






































#include <string.h>

#include "nscore.h"
#include "plstr.h"
#include "prlink.h"

#include "nsSound.h"

#include "nsIURL.h"
#include "nsIFileURL.h"
#include "nsNetUtil.h"
#include "nsCOMPtr.h"

#include <stdio.h>
#include <unistd.h>

#include <gtk/gtk.h>

static int esdref = -1;
static PRLibrary *elib = nsnull;



#define ESD_BITS8  (0x0000)
#define ESD_BITS16 (0x0001) 
#define ESD_MONO (0x0010)
#define ESD_STEREO (0x0020) 
#define ESD_STREAM (0x0000)
#define ESD_PLAY (0x1000)

#define WAV_MIN_LENGTH 44

typedef int (PR_CALLBACK *EsdOpenSoundType)(const char *host);
typedef int (PR_CALLBACK *EsdCloseType)(int);


typedef int (PR_CALLBACK *EsdPlayStreamFallbackType)(int, int, const char *, const char *);

NS_IMPL_ISUPPORTS2(nsSound, nsISound, nsIStreamLoaderObserver)


nsSound::nsSound()
{
  mInited = PR_FALSE;
}

nsSound::~nsSound()
{
  

  if (esdref != -1) {
    EsdCloseType EsdClose = (EsdCloseType) PR_FindSymbol(elib, "esd_close");
    (*EsdClose)(esdref);
    esdref = -1;
  }
}

NS_IMETHODIMP
nsSound::Init()
{
  


  if (mInited) return NS_OK;
  if (elib) return NS_OK;
  

  EsdOpenSoundType EsdOpenSound;

  elib = PR_LoadLibrary("libesd.so.0");
  if (!elib) return NS_ERROR_FAILURE;

  EsdOpenSound = (EsdOpenSoundType) PR_FindSymbol(elib, "esd_open_sound");

  if (!EsdOpenSound)
    return NS_ERROR_FAILURE;

  esdref = (*EsdOpenSound)("localhost");

  if (!esdref)
    return NS_ERROR_FAILURE;

  mInited = PR_TRUE;

  return NS_OK;
}

 void
nsSound::Shutdown()
{
  if (elib) {
    PR_UnloadLibrary(elib)
    elib = nsnull;
  }
}

#define GET_WORD(s, i) (s[i+1] << 8) | s[i]
#define GET_DWORD(s, i) (s[i+3] << 24) | (s[i+2] << 16) | (s[i+1] << 8) | s[i]

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
    return aStatus;
  }

  int fd, mask = 0;

  unsigned long samples_per_sec=0, avg_bytes_per_sec=0;
  unsigned long rate=0;
  unsigned short format, channels = 1, block_align, bits_per_sample=0;


  if (memcmp(data, "RIFF", 4)) {
#ifdef DEBUG
    printf("We only support WAV files currently.\n");
#endif
    return NS_ERROR_FAILURE;
  }

  if (dataLen <= WAV_MIN_LENGTH) {
      NS_WARNING("WAV files should be longer than 44 bytes.");
      return NS_ERROR_FAILURE;
  }

  PRUint32 i;
  for (i= 0; i < dataLen; i++) {

    if (i+3 <= dataLen) 
      if ((data[i] == 'f') &&
          (data[i+1] == 'm') &&
          (data[i+2] == 't') &&
          (data[i+3] == ' ')) {
        i += 4;

        
        
        i+=4;

        format = GET_WORD(data, i);
        i+=2;

        channels = GET_WORD(data, i);
        i+=2;

        samples_per_sec = GET_DWORD(data, i);
        i+=4;

        avg_bytes_per_sec = GET_DWORD(data, i);
        i+=4;

        block_align = GET_WORD(data, i);
        i+=2;

        bits_per_sample = GET_WORD(data, i);
        i+=2;

        rate = samples_per_sec;

        break;
      }
  }

#ifdef DEBUG
  printf("f: %d | c: %d | sps: %li | abps: %li | ba: %d | bps: %d | rate: %li\n",
         format, channels, samples_per_sec, avg_bytes_per_sec, block_align, bits_per_sample, rate);
#endif

    
  EsdPlayStreamFallbackType EsdPlayStreamFallback = (EsdPlayStreamFallbackType) PR_FindSymbol(elib, "esd_play_stream_fallback");
  
  mask = ESD_PLAY | ESD_STREAM;

  if (bits_per_sample == 8)
    mask |= ESD_BITS8;
  else 
    mask |= ESD_BITS16;

  if (channels == 1)
    mask |= ESD_MONO;
  else 
    mask |= ESD_STEREO;

  fd = (*EsdPlayStreamFallback)(mask, rate, NULL, "mozillaSound"); 
  
  if (fd < 0) {
    return NS_ERROR_FAILURE;
  }

  
  
  
#ifdef IS_BIG_ENDIAN
  if (bits_per_sample == 8)
    write(fd, data, dataLen);
  else {
    PRUint8 *buf = new PRUint8[dataLen - WAV_MIN_LENGTH];
    
    
    if (!buf)
      return NS_ERROR_OUT_OF_MEMORY;
    for (PRUint32 j = 0; j < dataLen - WAV_MIN_LENGTH - 1; j += 2) {
      buf[j] = data[j + WAV_MIN_LENGTH + 1];
      buf[j + 1] = data[j + WAV_MIN_LENGTH];
    }
    write(fd, buf, (dataLen - WAV_MIN_LENGTH));
    delete [] buf;
  }
#else
  write(fd, data, dataLen);
#endif

  close(fd);

  return NS_OK;
}

NS_METHOD nsSound::Beep()
{
  ::gdk_beep();

  return NS_OK;
}

NS_METHOD nsSound::Play(nsIURL *aURL)
{
  nsresult rv;

  if (!mInited)
    Init();

  if (!elib) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIStreamLoader> loader;
  rv = NS_NewStreamLoader(getter_AddRefs(loader), aURL, this);

  return rv;
}

NS_IMETHODIMP nsSound::PlaySystemSound(const nsAString &aSoundAlias)
{
  if (aSoundAlias.EqualsLiteral("_moz_mailbeep"))
  {
    return Beep();
  }
  nsresult rv;
  nsCOMPtr <nsIURI> fileURI;

  
  nsCOMPtr <nsILocalFile> soundFile;
  rv = NS_NewLocalFile(aSoundAlias, PR_TRUE, 
                       getter_AddRefs(soundFile));
  NS_ENSURE_SUCCESS(rv,rv);

  rv = NS_NewFileURI(getter_AddRefs(fileURI), soundFile);
  NS_ENSURE_SUCCESS(rv,rv);

  nsCOMPtr<nsIFileURL> fileURL = do_QueryInterface(fileURI,&rv);
  NS_ENSURE_SUCCESS(rv,rv);
  rv = Play(fileURL);
  return rv;
}
