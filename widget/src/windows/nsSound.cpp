






































#include "nscore.h"
#include "plstr.h"
#include <stdio.h>
#include "nsString.h"
#include <windows.h>


#include <mmsystem.h>

#include "nsSound.h"
#include "nsIURL.h"
#include "nsNetUtil.h"
#include "nsCRT.h"

#include "nsNativeCharsetUtils.h"

#ifndef SND_PURGE


#define SND_PURGE 0
#endif

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
    ::PlaySoundW(reinterpret_cast<LPCWSTR>(data), 0, flags);
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

  if (!NS_IsMozAliasSound(aSoundAlias)) {
    ::PlaySoundW(PromiseFlatString(aSoundAlias).get(), nsnull,
                 SND_NODEFAULT | SND_ALIAS | SND_ASYNC);
    return NS_OK;
  }

  
  
  const wchar_t *sound = nsnull;
  if (aSoundAlias.Equals(NS_SYSSOUND_MAIL_BEEP))
    sound = L"MailBeep";
  else if (aSoundAlias.Equals(NS_SYSSOUND_CONFIRM_DIALOG))
    sound = L"SystemQuestion";
  else if (aSoundAlias.Equals(NS_SYSSOUND_ALERT_DIALOG))
    sound = L"SystemExclamation";
  else if (aSoundAlias.Equals(NS_SYSSOUND_MENU_EXECUTE))
    sound = L"MenuCommand";
  else if (aSoundAlias.Equals(NS_SYSSOUND_MENU_POPUP))
    sound = L"MenuPopup";

  if (sound)
    ::PlaySoundW(sound, nsnull, SND_NODEFAULT | SND_ALIAS | SND_ASYNC);

  return NS_OK;
}

