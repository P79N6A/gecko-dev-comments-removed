






































#include "nscore.h"
#include "plstr.h"
#include "prlink.h"

#include "nsSound.h"
#include "nsString.h"

#include "nsIURL.h"
#include "nsNetUtil.h"
#include "nsCOMPtr.h"

#include <Pt.h>

NS_IMPL_ISUPPORTS2(nsSound, nsISound, nsIStreamLoaderObserver)


nsSound::nsSound()
{
  mInited = PR_FALSE;
}

nsSound::~nsSound()
{
}

nsresult nsSound::Init()
{
  if (mInited) return NS_OK;

  mInited = PR_TRUE;
  return NS_OK;
}

NS_METHOD nsSound::Beep()
{
  ::PtBeep();
  return NS_OK;
}

NS_METHOD nsSound::Play(nsIURL *aURL)
{
  NS_NOTYETIMPLEMENTED("nsSound::Play");

#ifdef DEBUG
printf( "\n\n\nnsSound::Play\n\n" );
#endif

  return NS_OK;
}

NS_IMETHODIMP nsSound::OnStreamComplete(nsIStreamLoader *aLoader,
                                        nsISupports *context,
                                        nsresult aStatus,
                                        PRUint32 stringLen,
                                        const PRUint8 *stringData)
{
  nsresult rv = NS_ERROR_FAILURE;

#ifdef DEBUG
printf( "\n\n\nnsSound::OnStreamComplete stringData=%s\n\n", stringData );
#endif

  if (NS_FAILED(aStatus))
    return NS_ERROR_FAILURE;

  return rv;
}

static void child_exit( void *data, int status ) { }

NS_IMETHODIMP nsSound::PlaySystemSound(const nsAString &aSoundAlias)
{
  NS_ConvertUTF16toUTF8 utf8SoundAlias(aSoundAlias);

#ifdef DEBUG
printf( "\n\n\nnsSound::PlaySystemSound aSoundAlias=%s\n\n",
        utf8SoundAlias.get() );
#endif

  const char *soundfile;

  if( NS_IsMozAliasSound(aSoundAlias) ) {
    NS_WARNING("nsISound::playSystemSound is called with \"_moz_\" events, they are obsolete, use nsISound::playEventSound instead");
    if ( aSoundAlias.Equals(NS_SYSSOUND_MAIL_BEEP) )
      soundfile = "/usr/share/mozilla/gotmail.wav";
    else
      return NS_OK;
  } else {
    
    if( !access( utf8SoundAlias.get(), F_OK ) )
      soundfile = utf8SoundAlias.get();
    else
      soundfile = "/usr/share/mozilla/rest.wav";
  }

  const char* argv[] = { "/opt/Mozilla/mozilla/wave", soundfile, NULL };
  PtSpawn( "/opt/Mozilla/mozilla/wave", ( const char ** ) argv,
           NULL, NULL, child_exit, NULL, NULL );

  return NS_OK;
}

NS_IMETHODIMP nsSound::PlayEventSound(PRUint32 aEventId)
{
  if (aEventId != EVENT_NEW_MAIL_RECEIVED) {
    return NS_OK;
  }

  soundfile = "/usr/share/mozilla/gotmail.wav";
  const char* argv[] = { "/opt/Mozilla/mozilla/wave",
                         "/usr/share/mozilla/gotmail.wav", NULL };
  PtSpawn( "/opt/Mozilla/mozilla/wave", ( const char ** ) argv,
           NULL, NULL, child_exit, NULL, NULL );

  return NS_OK;
}
