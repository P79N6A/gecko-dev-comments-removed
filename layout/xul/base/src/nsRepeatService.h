







































#ifndef nsRepeatService_h__
#define nsRepeatService_h__

#include "nsCOMPtr.h"
#include "nsITimer.h"

class nsITimer;

class nsRepeatService : public nsITimerCallback
{
public:

  typedef void (* Callback)(void* aData);
    
  NS_DECL_NSITIMERCALLBACK

  
  
  
  void Start(Callback aCallback, void* aData);
  
  
  
  void Stop(Callback aCallback, void* aData);

  static nsRepeatService* GetInstance();
  static void Shutdown();

  NS_DECL_ISUPPORTS
  virtual ~nsRepeatService();

protected:
  nsRepeatService();

private:
  Callback           mCallback;
  void*              mCallbackData;
  nsCOMPtr<nsITimer> mRepeatTimer;
  static nsRepeatService* gInstance;

}; 

#endif
