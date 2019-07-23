







































#include <string.h>

#include "nscore.h"
#include "plstr.h"
#include "prlink.h"

#include "nsSound.h"

#include "nsIURL.h"
#include "nsIFileURL.h"
#include "nsNetUtil.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsString.h"

#include <stdio.h>
#include <unistd.h>

#include <gtk/gtk.h>

static int esdref = -1;
static PRLibrary *elib = nsnull;
static PRLibrary *libcanberra = nsnull;



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




typedef struct _ca_context ca_context;

typedef int (*ca_context_create_fn) (ca_context **);
typedef int (*ca_context_destroy_fn) (ca_context *);
typedef int (*ca_context_play_fn) (ca_context *c,
                                   uint32_t id,
                                   ...);
typedef int (*ca_context_change_props_fn) (ca_context *c,
                                           ...);

static ca_context_create_fn ca_context_create;
static ca_context_destroy_fn ca_context_destroy;
static ca_context_play_fn ca_context_play;
static ca_context_change_props_fn ca_context_change_props;

NS_IMPL_ISUPPORTS2(nsSound, nsISound, nsIStreamLoaderObserver)


nsSound::nsSound()
{
    mInited = PR_FALSE;
}

nsSound::~nsSound()
{
    if (esdref > 0) {
        EsdCloseType EsdClose = (EsdCloseType) PR_FindFunctionSymbol(elib, "esd_close");
        if (EsdClose)
            (*EsdClose)(esdref);
        esdref = -1;
    }
}

NS_IMETHODIMP
nsSound::Init()
{
    
    
    if (mInited) 
        return NS_OK;

    mInited = PR_TRUE;

    if (!elib) {
        elib = PR_LoadLibrary("libesd.so.0");
        if (elib) {
            
            EsdOpenSoundType EsdOpenSound = (EsdOpenSoundType) PR_FindFunctionSymbol(elib, "esd_open_sound");
            if (!EsdOpenSound) {
                PR_UnloadLibrary(elib);
                elib = nsnull;
            } else {
                esdref = (*EsdOpenSound)("localhost");
                if (esdref < 0) {
                    PR_UnloadLibrary(elib);
                    elib = nsnull;
                }
            }
        }
    }

    if (!libcanberra) {
        libcanberra = PR_LoadLibrary("libcanberra.so.0");
        if (libcanberra) {
            ca_context_create = (ca_context_create_fn) PR_FindFunctionSymbol(libcanberra, "ca_context_create");
            if (!ca_context_create) {
                PR_UnloadLibrary(libcanberra);
                libcanberra = nsnull;
            } else {
                
                ca_context_destroy = (ca_context_destroy_fn) PR_FindFunctionSymbol(libcanberra, "ca_context_destroy");
                ca_context_play = (ca_context_play_fn) PR_FindFunctionSymbol(libcanberra, "ca_context_play");
                ca_context_change_props = (ca_context_change_props_fn) PR_FindFunctionSymbol(libcanberra, "ca_context_change_props");
            }
        }
    }

    return NS_OK;
}

 void
nsSound::Shutdown()
{
    if (elib) {
        PR_UnloadLibrary(elib);
        elib = nsnull;
    }
    if (libcanberra) {
        PR_UnloadLibrary(libcanberra);
        libcanberra = nsnull;
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

    if (!elib) 
	    return NS_ERROR_FAILURE;

    nsCOMPtr<nsIStreamLoader> loader;
    rv = NS_NewStreamLoader(getter_AddRefs(loader), aURL, this);

    return rv;
}

nsresult nsSound::PlaySystemEventSound(const nsAString &aSoundAlias)
{
    if (!libcanberra)
        return NS_OK;

    
    
    GtkSettings* settings = gtk_settings_get_default();
    gchar* sound_theme_name = nsnull;

    if (g_object_class_find_property(G_OBJECT_GET_CLASS(settings), "gtk-sound-theme-name") &&
        g_object_class_find_property(G_OBJECT_GET_CLASS(settings), "gtk-enable-event-sounds")) {
        gboolean enable_sounds = TRUE;
        g_object_get(settings, "gtk-enable-event-sounds", &enable_sounds,
                               "gtk-sound-theme-name", &sound_theme_name,
                               NULL);

        if (!enable_sounds) {
            g_free(sound_theme_name);
            return NS_OK;
        }
    }

    
    
    ca_context* ctx = nsnull;
    static GStaticPrivate ctx_static_private = G_STATIC_PRIVATE_INIT;
    ctx = (ca_context*) g_static_private_get(&ctx_static_private);
    if (!ctx) {
        ca_context_create(&ctx);
        if (!ctx) {
            g_free(sound_theme_name);
            return NS_ERROR_OUT_OF_MEMORY;
        }

        g_static_private_set(&ctx_static_private, ctx, (GDestroyNotify) ca_context_destroy);
    }

    if (sound_theme_name) {
        ca_context_change_props(ctx, "canberra.xdg-theme.name", sound_theme_name, NULL);
        g_free(sound_theme_name);
    }

    if (aSoundAlias.Equals(NS_SYSSOUND_ALERT_DIALOG))
        ca_context_play(ctx, 0, "event.id", "dialog-warning", NULL);
    else if (aSoundAlias.Equals(NS_SYSSOUND_CONFIRM_DIALOG))
        ca_context_play(ctx, 0, "event.id", "dialog-question", NULL);
    else if (aSoundAlias.Equals(NS_SYSSOUND_MAIL_BEEP))
        ca_context_play(ctx, 0, "event.id", "message-new-email", NULL);

    return NS_OK;
}

NS_IMETHODIMP nsSound::PlaySystemSound(const nsAString &aSoundAlias)
{
    if (!mInited)
        Init();

    if (NS_IsMozAliasSound(aSoundAlias))
        return PlaySystemEventSound(aSoundAlias);

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
