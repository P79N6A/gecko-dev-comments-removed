






































#ifndef __nsSound_h__
#define __nsSound_h__

#include "nsISound.h"
#include "nsISystemSoundService.h"
#include "nsCOMPtr.h"
#include "nsISoundPlayer.h"
#include "nsString.h"
#include "nsIFileURL.h"

class nsITimer;

class nsSound : public nsISound
{
public:
  nsSound();
  virtual ~nsSound();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISOUND

protected:
  nsresult Stop();
};




#define NS_SUCCESS_NOT_SUPPORTED \
  NS_ERROR_GENERATE_SUCCESS(NS_ERROR_MODULE_GENERAL, 1)



#define NS_DECL_ISYSTEMSOUNDSERVICE_GETINSTANCE(_InstanceClass) \
  static _InstanceClass* GetInstance();

#define NS_IMPL_ISYSTEMSOUNDSERVICE_GETINSTANCE(_InstanceClass)                \
_InstanceClass*                                                                \
_InstanceClass::GetInstance()                                                  \
{                                                                              \
  if (sInstance) {                                                             \
    NS_ADDREF(sInstance);                                                      \
    return static_cast<_InstanceClass*>(sInstance);                            \
  }                                                                            \
                                                                               \
  sInstance = new _InstanceClass();                                            \
  NS_ENSURE_TRUE(sInstance, nsnull);                                           \
  NS_ADDREF(sInstance);                                                        \
  return static_cast<_InstanceClass*>(sInstance);                              \
}


class nsSystemSoundServiceBase : public nsISystemSoundService
{
public:
  nsSystemSoundServiceBase();
  virtual ~nsSystemSoundServiceBase();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISYSTEMSOUNDSERVICE

  static void InitService();
  static already_AddRefed<nsIFileURL> GetFileURL(const nsAString &aFilePath);
  static already_AddRefed<nsISystemSoundService> GetSystemSoundService();
  static already_AddRefed<nsISoundPlayer> GetSoundPlayer();
  static nsresult PlayFile(const nsAString &aFilePath);
  static nsresult Play(nsIURL *aURL);
  
  
  
  static void StopSoundPlayer();

protected:
  static nsISystemSoundService* sInstance;
  static PRBool sIsInitialized;

  virtual nsresult Init();
  virtual void OnShutdown();

private:
  static void ExecuteInitService(nsITimer* aTimer, void* aClosure);
};

#endif 
