







































#ifndef nsRepeatService_h__
#define nsRepeatService_h__

#include "nsCOMPtr.h"
#include "nsITimer.h"

class nsITimer;

class nsRepeatService : public nsITimerCallback
{
public:

  NS_DECL_NSITIMERCALLBACK

  void Start(nsITimerCallback* aCallback);
  void Stop();

  static nsRepeatService* GetInstance();
  static void Shutdown();

  NS_DECL_ISUPPORTS
  virtual ~nsRepeatService();

protected:
  nsRepeatService();

private:
  nsCOMPtr<nsITimerCallback> mCallback;
  nsCOMPtr<nsITimer>         mRepeatTimer;
  PRBool                     mFirstCall;
  static nsRepeatService* gInstance;

}; 

#endif
