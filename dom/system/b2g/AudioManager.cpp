




































#include "AudioManager.h"
#include <media/AudioSystem.h>

using namespace mozilla::dom::telephony;
using namespace android;

NS_IMPL_ISUPPORTS1(AudioManager, nsIAudioManager)

NS_IMETHODIMP
AudioManager::GetMicrophoneMuted(bool* aMicrophoneMuted)
{
  if (AudioSystem::isMicrophoneMuted(aMicrophoneMuted)) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

NS_IMETHODIMP
AudioManager::SetMicrophoneMuted(bool aMicrophoneMuted)
{
  if (AudioSystem::muteMicrophone(aMicrophoneMuted)) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

NS_IMETHODIMP
AudioManager::GetMasterVolume(float* aMasterVolume)
{
  if (AudioSystem::getMasterVolume(aMasterVolume)) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

NS_IMETHODIMP
AudioManager::SetMasterVolume(float aMasterVolume)
{
  if (AudioSystem::setMasterVolume(aMasterVolume)) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

NS_IMETHODIMP
AudioManager::GetMasterMuted(bool* aMasterMuted)
{
  if (AudioSystem::getMasterMute(aMasterMuted)) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

NS_IMETHODIMP
AudioManager::SetMasterMuted(bool aMasterMuted)
{
  if (AudioSystem::setMasterMute(aMasterMuted)) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

NS_IMETHODIMP
AudioManager::GetPhoneState(PRInt32* aState)
{
  *aState = mPhoneState;
  return NS_OK;
}

NS_IMETHODIMP
AudioManager::SetPhoneState(PRInt32 aState)
{
  if (AudioSystem::setPhoneState(aState)) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

NS_IMETHODIMP
AudioManager::SetForceForUse(PRInt32 aUsage, PRInt32 aForce)
{
  if (AudioSystem::setForceUse((AudioSystem::force_use)aUsage,
                               (AudioSystem::forced_config)aForce)) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

NS_IMETHODIMP
AudioManager::GetForceForUse(PRInt32 aUsage, PRInt32* aForce) {
  *aForce = AudioSystem::getForceUse((AudioSystem::force_use)aUsage);
  return NS_OK;
}
