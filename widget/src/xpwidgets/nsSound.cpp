






































#include "nsCOMPtr.h"
#include "nsSound.h"
#include "nsString.h"
#include "nsIURL.h"
#include "nsNetUtil.h"
#include "nsServiceManagerUtils.h"
#include "nsITimer.h"






NS_IMPL_ISUPPORTS1(nsSound, nsISound)

nsSound::nsSound()
{
}

nsSound::~nsSound()
{
}

nsresult
nsSound::Stop()
{
  nsSystemSoundServiceBase::StopSoundPlayer();
  return NS_OK;
}

NS_IMETHODIMP
nsSound::Beep()
{
  nsCOMPtr<nsISystemSoundService> sysSound =
    nsSystemSoundServiceBase::GetSystemSoundService();
  NS_ENSURE_TRUE(sysSound, NS_ERROR_FAILURE);
  return sysSound->Beep();
}

NS_IMETHODIMP
nsSound::Init()
{
  
  return NS_OK;
}

NS_IMETHODIMP
nsSound::PlayEventSound(PRUint32 aEventID)
{
  
  nsresult rv = Stop();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISystemSoundService> sysSound =
    nsSystemSoundServiceBase::GetSystemSoundService();
  NS_ENSURE_TRUE(sysSound, NS_ERROR_FAILURE);
  return sysSound->PlayEventSound(aEventID);
}

NS_IMETHODIMP
nsSound::Play(nsIURL *aURL)
{
  NS_ENSURE_ARG_POINTER(aURL);
  nsresult rv = nsSystemSoundServiceBase::Play(aURL);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

NS_IMETHODIMP
nsSound::PlaySystemSound(const nsAString &aSoundAlias)
{
  if (aSoundAlias.IsEmpty())
    return NS_OK;

  
  nsresult rv = Stop();
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ASSERTION(!NS_IsMozAliasSound(aSoundAlias),
    "nsISound::playSystemSound is called with \"_moz_\" events, they are obsolete, use nsISystemSoundService::playEventSound instead");
  if (aSoundAlias.Equals(NS_SYSSOUND_MAIL_BEEP)) {
    rv = PlayEventSound(EVENT_NEW_MAIL_RECEIVED);
  } else if (aSoundAlias.Equals(NS_SYSSOUND_CONFIRM_DIALOG)) {
    rv = PlayEventSound(EVENT_CONFIRM_DIALOG_OPEN);
  } else if (aSoundAlias.Equals(NS_SYSSOUND_ALERT_DIALOG)) {
    rv = PlayEventSound(EVENT_ALERT_DIALOG_OPEN);
  } else if (aSoundAlias.Equals(NS_SYSSOUND_PROMPT_DIALOG)) {
    rv = PlayEventSound(EVENT_PROMPT_DIALOG_OPEN);
  } else if (aSoundAlias.Equals(NS_SYSSOUND_SELECT_DIALOG)) {
    rv = PlayEventSound(EVENT_SELECT_DIALOG_OPEN);
  } else if (aSoundAlias.Equals(NS_SYSSOUND_MENU_EXECUTE)) {
    rv = PlayEventSound(EVENT_MENU_EXECUTE);
  } else if (aSoundAlias.Equals(NS_SYSSOUND_MENU_POPUP)) {
    rv = PlayEventSound(EVENT_MENU_POPUP);
  } else {
    
    nsCOMPtr<nsISystemSoundService> sysSound =
      nsSystemSoundServiceBase::GetSystemSoundService();
    NS_ENSURE_TRUE(sysSound, NS_ERROR_FAILURE);
    rv = sysSound->PlayAlias(aSoundAlias);
    NS_ENSURE_SUCCESS(rv, rv);
    if (rv != NS_SUCCESS_NOT_SUPPORTED) {
      
      
      return NS_OK;
    }

    
    
    nsCOMPtr<nsIFileURL> fileURL =
      nsSystemSoundServiceBase::GetFileURL(aSoundAlias);
    if (!fileURL) {
      return NS_OK;
    }
    rv = Play(fileURL);
  }

  NS_ENSURE_SUCCESS(rv, rv);
  return rv;
}





nsISystemSoundService* nsSystemSoundServiceBase::sInstance = nsnull;
PRBool nsSystemSoundServiceBase::sIsInitialized = PR_FALSE;

NS_IMPL_ISUPPORTS1(nsSystemSoundServiceBase, nsISystemSoundService)

nsSystemSoundServiceBase::nsSystemSoundServiceBase()
{
}

nsSystemSoundServiceBase::~nsSystemSoundServiceBase()
{
  if (sInstance == this) {
    OnShutdown();
    sInstance = nsnull;
    sIsInitialized = PR_FALSE;
  }
}

nsresult
nsSystemSoundServiceBase::Init()
{
  return NS_OK;
}

void
nsSystemSoundServiceBase::OnShutdown()
{
  
}

 void
nsSystemSoundServiceBase::InitService()
{
  nsCOMPtr<nsITimer> timer = do_CreateInstance(NS_TIMER_CONTRACTID);
  if (!timer) {
    return; 
  }
  NS_ADDREF(timer); 
  nsresult rv =
    timer->InitWithFuncCallback(ExecuteInitService, nsnull, 0,
                                nsITimer::TYPE_ONE_SHOT);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "nsITimer::InitWithFuncCallback failed");
}

 void
nsSystemSoundServiceBase::ExecuteInitService(nsITimer* aTimer, void* aClosure)
{
  NS_IF_RELEASE(aTimer);

  
  nsCOMPtr<nsISystemSoundService> sysSound =
    do_GetService("@mozilla.org/systemsoundservice;1");
  if (!sysSound) {
    return;
  }
  nsresult rv = static_cast<nsSystemSoundServiceBase*>(sysSound.get())->Init();
  sIsInitialized = NS_SUCCEEDED(rv);
}

 already_AddRefed<nsIFileURL>
nsSystemSoundServiceBase::GetFileURL(const nsAString &aFilePath)
{
  if (aFilePath.IsEmpty()) {
    return nsnull;
  }

  nsresult rv;
  nsCOMPtr<nsILocalFile> file;
  rv = NS_NewLocalFile(aFilePath, PR_TRUE, getter_AddRefs(file));
  if (rv == NS_ERROR_FILE_UNRECOGNIZED_PATH) {
    return nsnull;
  }
  NS_ENSURE_SUCCESS(rv, nsnull);
  NS_ENSURE_TRUE(file, nsnull);

  PRBool isExists;
  PRBool isFile;
  rv = file->Exists(&isExists);
  NS_ENSURE_SUCCESS(rv, nsnull);
  rv = file->IsFile(&isFile);
  NS_ENSURE_SUCCESS(rv, nsnull);
  if (!isExists || !isFile) {
    return nsnull;
  }

  nsCOMPtr<nsIURI> fileURI;
  rv = NS_NewFileURI(getter_AddRefs(fileURI), file);
  NS_ENSURE_SUCCESS(rv, nsnull);

  nsCOMPtr<nsIFileURL> fileURL = do_QueryInterface(fileURI, &rv);
  NS_ENSURE_SUCCESS(rv, nsnull);

  
  
  NS_ENSURE_TRUE(fileURI, nsnull);

  return fileURL.forget();
}

 already_AddRefed<nsISystemSoundService>
nsSystemSoundServiceBase::GetSystemSoundService()
{
  NS_ENSURE_TRUE(sInstance, nsnull);
  NS_ADDREF(sInstance);
  return sInstance;
}

 already_AddRefed<nsISoundPlayer>
nsSystemSoundServiceBase::GetSoundPlayer()
{
  nsCOMPtr<nsISoundPlayer> player =
    do_GetService("@mozilla.org/content/media/soundplayer;1");
  NS_ENSURE_TRUE(player, nsnull);
  return player.forget();
}

 nsresult
nsSystemSoundServiceBase::PlayFile(const nsAString &aFilePath)
{
  nsCOMPtr<nsIFileURL> fileURL = GetFileURL(aFilePath);
  if (!fileURL) {
    return NS_OK;
  }

  nsCOMPtr<nsISoundPlayer> player = GetSoundPlayer();
  NS_ENSURE_TRUE(player, NS_ERROR_FAILURE);

  return player->Play(fileURL);
}

 nsresult
nsSystemSoundServiceBase::Play(nsIURL *aURL)
{
  nsCOMPtr<nsISoundPlayer> player = GetSoundPlayer();
  NS_ENSURE_TRUE(player, NS_ERROR_FAILURE);

  
  return player->Play(aURL);
}

 void
nsSystemSoundServiceBase::StopSoundPlayer()
{
  nsCOMPtr<nsISoundPlayer> player = GetSoundPlayer();
  if (player) {
    player->Stop();
  }
}

NS_IMETHODIMP
nsSystemSoundServiceBase::Beep()
{
  NS_ENSURE_TRUE(sIsInitialized, NS_ERROR_NOT_INITIALIZED);
  return NS_SUCCESS_NOT_SUPPORTED;
}

NS_IMETHODIMP
nsSystemSoundServiceBase::PlayAlias(const nsAString &aSoundAlias)
{
  NS_ENSURE_TRUE(sIsInitialized, NS_ERROR_NOT_INITIALIZED);
  return NS_SUCCESS_NOT_SUPPORTED;
}

NS_IMETHODIMP
nsSystemSoundServiceBase::PlayEventSound(PRUint32 aEventID)
{
  NS_ENSURE_TRUE(sIsInitialized, NS_ERROR_NOT_INITIALIZED);
  return NS_SUCCESS_NOT_SUPPORTED;
}
