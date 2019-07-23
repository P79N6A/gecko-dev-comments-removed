





































#include <string.h>

#include "nscore.h"
#include "plstr.h"
#include "prlink.h"

#include "nsSound.h"
#include "nsString.h"

#include "nsIURL.h"
#include "nsIFileURL.h"
#include "nsNetUtil.h"
#include "nsCOMPtr.h"
#include "nsNativeCharsetUtils.h"
#include "nsAutoPtr.h"

#include <QApplication>
#include <QSound>


static int esdref = -1;
static PRLibrary *elib = nsnull;



#define ESD_BITS8  (0x0000)
#define ESD_BITS16 (0x0001) 
#define ESD_MONO (0x0010)
#define ESD_STEREO (0x0020) 
#define ESD_STREAM (0x0000)
#define ESD_PLAY (0x1000)

#define WAV_MIN_LENGTH 44

typedef int (*EsdOpenSoundType)(const char *host);
typedef int (*EsdCloseType)(int);


typedef int  (*EsdPlayStreamType) (int, int, const char *, const char *);
typedef int  (*EsdAudioOpenType)  (void);
typedef int  (*EsdAudioWriteType) (const void *, int);
typedef void (*EsdAudioCloseType) (void);

NS_IMPL_ISUPPORTS2(nsSound, nsISound, nsIStreamLoaderObserver)

nsSound::nsSound()
 : mInited( PR_FALSE )
{
}

nsSound::~nsSound()
{
    
    if (esdref != -1) {
        EsdCloseType EsdClose = (EsdCloseType) PR_FindFunctionSymbol(elib, "esd_close");
        if (EsdClose)
            (*EsdClose)(esdref);
        esdref = -1;
    }
}





void
nsSound::Shutdown()
{
    if (elib) {
        PR_UnloadLibrary(elib);
        elib = nsnull;
    }
}

NS_IMETHODIMP
nsSound::Init()
{
    


    if (mInited) 
        return NS_OK;
    if (elib) 
        return NS_OK;

    EsdOpenSoundType EsdOpenSound;

    elib = PR_LoadLibrary("libesd.so.0");
    if (!elib) return NS_ERROR_FAILURE;

    EsdOpenSound = (EsdOpenSoundType) PR_FindFunctionSymbol(elib, "esd_open_sound");

    if (!EsdOpenSound)
        return NS_ERROR_FAILURE;

    esdref = (*EsdOpenSound)("localhost");

    if (!esdref)
        return NS_ERROR_FAILURE;

    mInited = PR_TRUE;

    return NS_OK;
}

NS_METHOD nsSound::Beep()
{
    QApplication::beep();
    return NS_OK;
}











NS_IMETHODIMP nsSound::OnStreamComplete(nsIStreamLoader *aLoader,
                                        nsISupports *context,
                                        nsresult aStatus,
                                        PRUint32 dataLen,
                                        const PRUint8 *data)
{

#define GET_WORD(s, i) (s[i+1] << 8) | s[i]
#define GET_DWORD(s, i) (s[i+3] << 24) | (s[i+2] << 16) | (s[i+1] << 8) | s[i]

    
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
    PRUint32 samples_per_sec = 0, avg_bytes_per_sec = 0, chunk_len = 0;
    PRUint16 format, channels = 1, bits_per_sample = 0;
    const PRUint8 *audio = nsnull;
    size_t audio_len = 0;

    if (dataLen < 4) {
        NS_WARNING("Sound stream too short to determine its type");
        return NS_ERROR_FAILURE;
    }

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

    PRUint32 i = 12;
    while (i + 7 < dataLen) {
        if (!memcmp(data + i, "fmt ", 4) && !chunk_len) {
            i += 4;

            
            chunk_len = GET_DWORD(data, i);
            i += 4;

            if (chunk_len < 16 || i + chunk_len >= dataLen) {
                NS_WARNING("Invalid WAV file: bad fmt chunk.");
                return NS_ERROR_FAILURE;
            }

            format = GET_WORD(data, i);
            i += 2;

            channels = GET_WORD(data, i);
            i += 2;

            samples_per_sec = GET_DWORD(data, i);
            i += 4;

            avg_bytes_per_sec = GET_DWORD(data, i);
            i += 4;

            
            i += 2;

            bits_per_sample = GET_WORD(data, i);
            i += 2;

            
            if (chunk_len != 16)
                NS_WARNING("Extra format bits found in WAV. Ignoring");

            i += chunk_len - 16;
        } else if (!memcmp(data + i, "data", 4)) {
            i += 4;
            if (!chunk_len) {
                NS_WARNING("Invalid WAV file: no fmt chunk found");
                return NS_ERROR_FAILURE;
            }

            audio_len = GET_DWORD(data, i);
            i += 4;

            
            if (i + audio_len > dataLen)
                audio_len = dataLen - i;

            audio = data + i;
            break;
        } else {
            i += 4;
            i += GET_DWORD(data, i);
            i += 4;
        }
    }

    if (!audio) {
        NS_WARNING("Invalid WAV file: no data chunk found");
        return NS_ERROR_FAILURE;
    }

    
    if (!audio_len)
        return NS_OK;

#if 0
    printf("f: %d | c: %d | sps: %li | abps: %li | ba: %d | bps: %d | rate: %li\n",
         format, channels, samples_per_sec, avg_bytes_per_sec, block_align, bits_per_sample, rate);
#endif

      
    EsdPlayStreamType EsdPlayStream = 
        (EsdPlayStreamType) PR_FindFunctionSymbol(elib, 
                                                  "esd_play_stream");
    if (!EsdPlayStream)
        return NS_ERROR_FAILURE;

    mask = ESD_PLAY | ESD_STREAM;

    if (bits_per_sample == 8)
        mask |= ESD_BITS8;
    else 
        mask |= ESD_BITS16;

    if (channels == 1)
        mask |= ESD_MONO;
    else 
        mask |= ESD_STEREO;

    nsAutoArrayPtr<PRUint8> buf;

    
    
#ifdef IS_BIG_ENDIAN
    if (bits_per_sample != 8) {
        buf = new PRUint8[audio_len];
        if (!buf)
            return NS_ERROR_OUT_OF_MEMORY;
        for (PRUint32 j = 0; j + 2 < audio_len; j += 2) {
            buf[j]     = audio[j + 1];
            buf[j + 1] = audio[j];
        }

    audio = buf;
    }
#endif

    fd = (*EsdPlayStream)(mask, samples_per_sec, NULL, "mozillaSound"); 
  
    if (fd < 0) {
      int *esd_audio_format = (int *) PR_FindSymbol(elib, "esd_audio_format");
      int *esd_audio_rate = (int *) PR_FindSymbol(elib, "esd_audio_rate");
      EsdAudioOpenType EsdAudioOpen = (EsdAudioOpenType) PR_FindFunctionSymbol(elib, "esd_audio_open");
      EsdAudioWriteType EsdAudioWrite = (EsdAudioWriteType) PR_FindFunctionSymbol(elib, "esd_audio_write");
      EsdAudioCloseType EsdAudioClose = (EsdAudioCloseType) PR_FindFunctionSymbol(elib, "esd_audio_close");

      if (!esd_audio_format || !esd_audio_rate ||
          !EsdAudioOpen || !EsdAudioWrite || !EsdAudioClose)
          return NS_ERROR_FAILURE;

      *esd_audio_format = mask;
      *esd_audio_rate = samples_per_sec;
      fd = (*EsdAudioOpen)();

      if (fd < 0)
        return NS_ERROR_FAILURE;

      (*EsdAudioWrite)(audio, audio_len);
      (*EsdAudioClose)();
    } else {
      while (audio_len > 0) {
        size_t written = write(fd, audio, audio_len);
        if (written <= 0)
          break;
        audio += written;
        audio_len -= written;
      }
      close(fd);
    }

    return NS_OK;
}

NS_METHOD nsSound::Play(nsIURL *aURL)
{
    nsresult rv;

    if (!mInited)
        Init();

    if (!elib) 
        return NS_ERROR_FAILURE;

    nsCOMPtr<nsIStreamLoader> loader;
    rv = NS_NewStreamLoader(getter_AddRefs(loader), aURL, this);

    return rv;
}

NS_IMETHODIMP nsSound::PlaySystemSound(const nsAString &aSoundAlias)
{
    if (NS_IsMozAliasSound(aSoundAlias)) {
      NS_WARNING("nsISound::playSystemSound is called with \"_moz_\" events, they are obsolete, use nsISound::playEventSound instead");
      if (aSoundAlias.Equals(NS_SYSSOUND_MAIL_BEEP))
        return Beep();
      return NS_OK;
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

NS_IMETHODIMP nsSound::PlayEventSound(PRUint32 aEventId)
{
    return aEventId == EVENT_NEW_MAIL_RECEIVED ? Beep() : NS_OK;
}

