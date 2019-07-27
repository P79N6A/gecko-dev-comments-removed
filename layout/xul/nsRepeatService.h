







#ifndef nsRepeatService_h__
#define nsRepeatService_h__

#include "nsCOMPtr.h"
#include "nsITimer.h"

#define INITAL_REPEAT_DELAY 250

#ifdef XP_MACOSX
#define REPEAT_DELAY        25
#else
#define REPEAT_DELAY        50
#endif

class nsITimer;

class nsRepeatService final : public nsITimerCallback
{
public:

  typedef void (* Callback)(void* aData);
    
  NS_DECL_NSITIMERCALLBACK

  
  
  
  void Start(Callback aCallback, void* aData,
             uint32_t aInitialDelay = INITAL_REPEAT_DELAY);
  
  
  
  void Stop(Callback aCallback, void* aData);

  static nsRepeatService* GetInstance();
  static void Shutdown();

  NS_DECL_ISUPPORTS

protected:
  nsRepeatService();
  virtual ~nsRepeatService();

private:
  Callback           mCallback;
  void*              mCallbackData;
  nsCOMPtr<nsITimer> mRepeatTimer;
  static nsRepeatService* gInstance;

}; 

#endif
