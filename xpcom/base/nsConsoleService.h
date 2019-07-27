









#ifndef __nsconsoleservice_h__
#define __nsconsoleservice_h__

#include "mozilla/Attributes.h"
#include "mozilla/Mutex.h"

#include "nsInterfaceHashtable.h"
#include "nsHashKeys.h"

#include "nsIConsoleService.h"

class nsConsoleService final : public nsIConsoleService
{
public:
  nsConsoleService();
  nsresult Init();

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSICONSOLESERVICE

  void SetIsDelivering()
  {
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(!mDeliveringMessage);
    mDeliveringMessage = true;
  }

  void SetDoneDelivering()
  {
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(mDeliveringMessage);
    mDeliveringMessage = false;
  }

  
  
  

  enum OutputMode {
    SuppressLog,
    OutputToLog
  };
  virtual nsresult LogMessageWithMode(nsIConsoleMessage* aMessage,
                                      OutputMode aOutputMode);

  typedef nsInterfaceHashtable<nsISupportsHashKey,
                               nsIConsoleListener> ListenerHash;
  void CollectCurrentListeners(nsCOMArray<nsIConsoleListener>& aListeners);

private:
  ~nsConsoleService();

  
  nsIConsoleMessage** mMessages;

  
  uint32_t mBufferSize;

  
  uint32_t mCurrent;

  
  bool mFull;

  
  
  
  bool mDeliveringMessage;

  
  ListenerHash mListeners;

  
  mozilla::Mutex mLock;
};

#endif 
