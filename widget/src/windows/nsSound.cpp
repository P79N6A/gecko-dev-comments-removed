






































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

class nsSoundPlayer: public nsRunnable {
public:
  nsSoundPlayer(nsISound *aSound, const wchar_t* aSoundName) :
    mSound(aSound), mSoundName(aSoundName)
  {
    Init();
  }

  nsSoundPlayer(nsISound *aSound, const nsAString& aSoundName) :
    mSound(aSound), mSoundName(aSoundName)
  {
    Init();
  }

  NS_DECL_NSIRUNNABLE

protected:
  nsString mSoundName;
  nsISound *mSound; 
  nsCOMPtr<nsIThread> mThread;

  void Init()
  {
    NS_GetCurrentThread(getter_AddRefs(mThread));
    NS_ASSERTION(mThread, "failed to get current thread");
    NS_IF_ADDREF(mSound);
  }

  class SoundReleaser: public nsRunnable {
  public:
    SoundReleaser(nsISound* aSound) :
      mSound(aSound)
    {
    }

    NS_DECL_NSIRUNNABLE

  protected:
    nsISound *mSound;
  };
};

NS_IMETHODIMP
nsSoundPlayer::Run()
{
  NS_PRECONDITION(!mSoundName.IsEmpty(), "Sound name should not be empty");
  ::PlaySoundW(mSoundName.get(), NULL, SND_NODEFAULT | SND_ALIAS | SND_ASYNC);
  nsCOMPtr<nsIRunnable> releaser = new SoundReleaser(mSound);
  
  
  mThread->Dispatch(releaser, NS_DISPATCH_NORMAL);
  return NS_OK;
}

NS_IMETHODIMP
nsSoundPlayer::SoundReleaser::Run()
{
  NS_IF_RELEASE(mSound);
  return NS_OK;
}


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
  if (mPlayerThread) {
    mPlayerThread->Shutdown();
    mPlayerThread = nsnull;
  }
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
    if (aSoundAlias.IsEmpty())
      return NS_OK;
    nsCOMPtr<nsIRunnable> player = new nsSoundPlayer(this, aSoundAlias);
    NS_ENSURE_TRUE(player, NS_ERROR_OUT_OF_MEMORY);
    nsresult rv = NS_NewThread(getter_AddRefs(mPlayerThread), player);
    NS_ENSURE_SUCCESS(rv, rv);
    return NS_OK;
  }

  NS_WARNING("nsISound::playSystemSound is called with \"_moz_\" events, they are obsolete, use nsISound::playEventSound instead");

  PRUint32 eventId;
  if (aSoundAlias.Equals(NS_SYSSOUND_MAIL_BEEP))
    eventId = EVENT_NEW_MAIL_RECEIVED;
  else if (aSoundAlias.Equals(NS_SYSSOUND_CONFIRM_DIALOG))
    eventId = EVENT_CONFIRM_DIALOG_OPEN;
  else if (aSoundAlias.Equals(NS_SYSSOUND_ALERT_DIALOG))
    eventId = EVENT_ALERT_DIALOG_OPEN;
  else if (aSoundAlias.Equals(NS_SYSSOUND_MENU_EXECUTE))
    eventId = EVENT_MENU_EXECUTE;
  else if (aSoundAlias.Equals(NS_SYSSOUND_MENU_POPUP))
    eventId = EVENT_MENU_POPUP;
  else
    return NS_OK;

  return PlayEventSound(eventId);
}

NS_IMETHODIMP nsSound::PlayEventSound(PRUint32 aEventId)
{
  PurgeLastSound();

  const wchar_t *sound = nsnull;
  switch (aEventId) {
    case EVENT_NEW_MAIL_RECEIVED:
      sound = L"MailBeep";
      break;
    case EVENT_ALERT_DIALOG_OPEN:
      sound = L"SystemExclamation";
      break;
    case EVENT_CONFIRM_DIALOG_OPEN:
      sound = L"SystemQuestion";
      break;
    case EVENT_MENU_EXECUTE:
      sound = L"MenuCommand";
      break;
    case EVENT_MENU_POPUP:
      sound = L"MenuPopup";
      break;
    default:
      
      
      return NS_OK;
  }
  NS_ASSERTION(sound, "sound is null");

  nsCOMPtr<nsIRunnable> player = new nsSoundPlayer(this, sound);
  NS_ENSURE_TRUE(player, NS_ERROR_OUT_OF_MEMORY);
  nsresult rv = NS_NewThread(getter_AddRefs(mPlayerThread), player);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}
