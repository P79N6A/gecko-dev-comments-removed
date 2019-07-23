







































#include "nsSystemSoundService.h"
#include "nsServiceManagerUtils.h"
#include <windows.h>
#include <mmsystem.h>

#ifndef SND_PURGE


#define SND_PURGE 0
#endif

class nsStopSoundPlayer : public nsRunnable {
public:
  nsStopSoundPlayer()
  {
  }

  NS_DECL_NSIRUNNABLE
};

NS_IMETHODIMP
nsStopSoundPlayer::Run()
{
  nsSystemSoundServiceBase::StopSoundPlayer();
  return NS_OK;
}

class nsSystemSoundPlayer : public nsRunnable {
public:
  nsSystemSoundPlayer(PRUint32 aEventID) :
    mEventID(aEventID)
  {
  }

  nsSystemSoundPlayer(const nsAString &aName) :
    mEventID(PR_UINT32_MAX), mName(aName)
  {
  }

  NS_DECL_NSIRUNNABLE

protected:
  PRUint32 mEventID;
  nsString mName;
};

NS_IMETHODIMP
nsSystemSoundPlayer::Run()
{
  const wchar_t *sound = nsnull;
  if (!mName.IsEmpty()) {
    sound = static_cast<const wchar_t*>(mName.get());
  } else if (mEventID != PR_UINT32_MAX) {
    switch (mEventID) {
      case nsISystemSoundService::EVENT_NEW_MAIL_RECEIVED:
        sound = L"MailBeep";
        break;
      case nsISystemSoundService::EVENT_ALERT_DIALOG_OPEN:
        sound = L"SystemExclamation";
        break;
      case nsISystemSoundService::EVENT_CONFIRM_DIALOG_OPEN:
        sound = L"SystemQuestion";
        break;
      case nsISystemSoundService::EVENT_MENU_EXECUTE:
        sound = L"MenuCommand";
        break;
      case nsISystemSoundService::EVENT_MENU_POPUP:
        sound = L"MenuPopup";
        break;
      case nsISystemSoundService::EVENT_MENU_NOT_FOUND:
        
        ::MessageBeep(0);
        return NS_OK;
      default:
        
        
        return NS_OK;
    }
  }
  if (sound) {
    nsCOMPtr<nsStopSoundPlayer> stopper = new nsStopSoundPlayer();
    NS_DispatchToMainThread(stopper, nsIEventTarget::DISPATCH_SYNC);
    ::PlaySoundW(sound, NULL, SND_NODEFAULT | SND_ALIAS | SND_ASYNC);
  }
  return NS_OK;
}





NS_IMPL_ISUPPORTS1(nsSystemSoundService, nsISystemSoundService)

NS_IMPL_ISYSTEMSOUNDSERVICE_GETINSTANCE(nsSystemSoundService)

nsSystemSoundService::nsSystemSoundService() :
  nsSystemSoundServiceBase()
{
}

nsSystemSoundService::~nsSystemSoundService()
{
  if (mPlayerThread) {
    mPlayerThread->Shutdown();
    mPlayerThread = nsnull;
  }
}

nsresult
nsSystemSoundService::Init()
{
  
  
  
  
  
  ::PlaySound(nsnull, nsnull, SND_PURGE);

  return NS_OK;
}

NS_IMETHODIMP
nsSystemSoundService::Beep()
{
  nsresult rv = nsSystemSoundServiceBase::Beep();
  NS_ENSURE_SUCCESS(rv, rv);

  ::MessageBeep(0);
  return NS_OK;
}

NS_IMETHODIMP
nsSystemSoundService::PlayAlias(const nsAString &aSoundAlias)
{
  nsresult rv = nsSystemSoundServiceBase::PlayAlias(aSoundAlias);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsSystemSoundPlayer> player = new nsSystemSoundPlayer(aSoundAlias);
  NS_ENSURE_TRUE(player, NS_ERROR_OUT_OF_MEMORY);
  return PostPlayer(player);
}

NS_IMETHODIMP
nsSystemSoundService::PlayEventSound(PRUint32 aEventID)
{
  nsresult rv = nsSystemSoundServiceBase::PlayEventSound(aEventID);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsSystemSoundPlayer> player = new nsSystemSoundPlayer(aEventID);
  NS_ENSURE_TRUE(player, NS_ERROR_OUT_OF_MEMORY);
  return PostPlayer(player);
}

nsresult
nsSystemSoundService::PostPlayer(nsSystemSoundPlayer *aPlayer)
{
  nsresult rv;
  if (mPlayerThread) {
    rv = mPlayerThread->Dispatch(aPlayer, NS_DISPATCH_NORMAL);
    NS_ENSURE_SUCCESS(rv, rv);
    return NS_OK;
  }

  rv = NS_NewThread(getter_AddRefs(mPlayerThread), aPlayer);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(mPlayerThread, NS_ERROR_OUT_OF_MEMORY);
  return NS_OK;
}
