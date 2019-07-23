






































#include "nscore.h"
#include "plstr.h"
#include <stdio.h>

#include <windows.h>


#include <mmsystem.h>

#include "nsSound.h"
#include "nsIURL.h"
#include "nsNetUtil.h"
#include "nsCRT.h"

#include "nsNativeCharsetUtils.h"

NS_IMPL_ISUPPORTS2(nsSound, nsISound, nsIStreamLoaderObserver)


nsSound::nsSound()
{
  mLastSound = nsnull;
}

nsSound::~nsSound()
{
  PurgeLastSound();
}

void nsSound::PurgeLastSound() {
  if (mLastSound) {
    
    ::PlaySound(nsnull, nsnull, SND_PURGE); 

    
    free(mLastSound);
    mLastSound = nsnull;
  }
}

NS_IMETHODIMP nsSound::Beep()
{
  ::MessageBeep(0);

  return NS_OK;
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
      nsCOMPtr<nsIChannel> channel;
      aLoader->GetRequest(getter_AddRefs(request));
      if (request)
          channel = do_QueryInterface(request);
      if (channel) {
        nsCOMPtr<nsIURI> uri;
        channel->GetURI(getter_AddRefs(uri));
        if (uri) {
          nsCAutoString uriSpec;
          uri->GetSpec(uriSpec);
          printf("Failed to load %s\n", uriSpec.get());
        }
      }
    }
#endif
    return aStatus;
  }

  PurgeLastSound();

  if (data && dataLen > 0) {
    DWORD flags = SND_MEMORY | SND_NODEFAULT;
    
    mLastSound = (PRUint8 *) malloc(dataLen);
    if (mLastSound) {
      memcpy(mLastSound, data, dataLen);
      data = mLastSound;
      flags |= SND_ASYNC;
    }

    ::PlaySound(NS_REINTERPRET_CAST(const char*, data), 0, flags);
  }

  return NS_OK;
}

NS_IMETHODIMP nsSound::Play(nsIURL *aURL)
{
  nsresult rv;

#ifdef DEBUG_SOUND
  char *url;
  aURL->GetSpec(&url);
  printf("%s\n", url);
#endif

  nsCOMPtr<nsIStreamLoader> loader;
  rv = NS_NewStreamLoader(getter_AddRefs(loader), aURL, this);

  return rv;
}


NS_IMETHODIMP nsSound::Init()
{
  
  
  
  
  
  ::PlaySound(nsnull, nsnull, SND_PURGE); 

  return NS_OK;
}


NS_IMETHODIMP nsSound::PlaySystemSound(const nsAString &aSoundAlias)
{
  PurgeLastSound();

  if (aSoundAlias.EqualsLiteral("_moz_mailbeep")) {
    ::PlaySound("MailBeep", nsnull, SND_ALIAS | SND_ASYNC);
  }
  else {
    nsCAutoString nativeSoundAlias;
    NS_CopyUnicodeToNative(aSoundAlias, nativeSoundAlias);
    ::PlaySound(nativeSoundAlias.get(), nsnull, SND_ALIAS | SND_ASYNC);
  }

  return NS_OK;
}

